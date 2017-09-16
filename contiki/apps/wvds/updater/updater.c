#include "contiki.h"
#include "node-id.h"
#include "lib/crc16.h"
#include "net/netstack.h"
#include "net/rime.h"
#include "net/rime/trickle.h"
#include "net/neighbor.h"
#include "net/netcmd.h"
#include "cfs/cfs.h"
#include "cc11xx.h"
#include "dev/battery-sensor.h"
#include "dev/solarbat-sensor.h"
#include "dev/watchdog.h"
#include "dev/leds.h"
#include "dev/uart.h"
#include "sys/ctimer.h"
#include "sys/logger.h"
#include "sys/unixtime.h"
#include "sys/taskmon.h"
#include "simple-aes.h"
#include "base64.h"
#include "ds3231.h"
#include "m26.h"
#include "at-cmd.h"
#include "deluge.h"
#include "app.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifndef APP_VER_MAJOR
#define APP_VER_MAJOR 1
#endif
#ifndef APP_VER_MINOR
#define APP_VER_MINOR 0
#endif
#ifndef APP_VER_PATCH
#define APP_VER_PATCH 2
#endif

/*------------------------------------------------------------------*/
// 是否保存升级命令和进度到存储器
#define OTA_EXEC_NV  1
// 是否保存升级节点列表到存储器
#define OTA_NODES_NV 0

// 每隔该时间向VD发送指令使其保持射频开启
#define OTA_KEEP_RADIO_PERIOD (APP_FAECMD_WAIT - (CLOCK_SECOND << 3))
// 如果累计该时间内没有收到deluge request则发送指令让其重启
#define OTA_MAX_NOREQ_PERIOD  (CLOCK_SECOND << 4)
// 向没有发送request的节点发送重启指令的最大次数
#define OTA_MAX_RESET_TIMES   4

// 向VD发送连接应答到发送OTA执行命令的延时
#define OTA_START_DELAY  (CLOCK_SECOND<<1)
// VD未能启动更新时再次向其发送OTA执行命令的间隔
#define OTA_START_RETRY  (CLOCK_SECOND<<3)

/*------------------------------------------------------------------*/
#define DEBUG 0
#if DEBUG
#define PRINTF(...)  printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

// 如果要调试数据包具体内容，定义DEBUG_PKT为1
#define DEBUG_PKT 0
#if DEBUG_PKT
#define PRINTP(...) printf(__VA_ARGS__)
#define PRINTD(data,ofs,len) do { \
  uint8_t i; \
  for (i = 0; i < (len); i++) \
    printf(" %02X", (data)[(ofs) + i]); \
} while(0)
#else
#define PRINTP(...)
#define PRINTD(...)
#endif

// 定义为1时使能发送消息诊断重要消息是否存储和再发送，定义为0时禁用
#define DEBUG_MSG_SAVE 0
// 定义为1时使能发送运行时调试消息，定义为0时禁用
#define RDEBUG 0
// 定义为1时使能发送运行时警告消息，定义为0时禁用
#define RWARN  1

#define MIN(a,b) (((a) < (b)) ? (a) : (b))
/*------------------------------------------------------------------*/
static uint8_t app_s = 0; // 主线程的自定义状态
#define enterS(s) do { \
  PRINTF("app s%d->s%d at %d\n", app_s, s, __LINE__); \
  app_s = s; \
} while(0)

static uint8_t gprs_s = 0; // GPRS接收状态
static uint8_t gprs_i = 0; // GPRS接收指针
static uint8_t gprs_n = 0; // GPRS接收长度
static uint16_t gprs_ccrc = 0; // GPRS计算的CRC
static uint16_t gprs_rcrc = 0; // GPRS接收的CRC

static struct broadcast_conn mesh;
static struct unicast_conn unic;

#define APP_NODES_NUM 20
struct NODES {
  uint16_t magic;                // 初始化标志
  uint8_t  count;                // 保存的节点数量
  uint8_t  reserv[15];           // 保留
  uint16_t nodes[APP_NODES_NUM]; // 保存的各节点数据nv_node_entry的CRC
  uint32_t write;                // 本NODES结构体已写入次数
  uint16_t crc;                  // 本NODES结构体的CRC
};

static struct BOOT boot; // 启动信息
struct OTA  ota;  // 无线升级信息
static struct NIB  nib;  // 节点特定信息
static struct PIB  pib;  // 全网参数
#if OTA_NODES_NV
static struct NODES nodes; // 升级节点信息
#endif

static rimeaddr_t ota_rimeaddr = {{1,1}};

static int ota_fd = -1; // OTA文件句柄
static uint32_t ota_last = 0; // 上次接收OTA消息的数据地址
static uint32_t ota_addr = 0; // 本次接收OTA消息的写入地址
static uint32_t ota_size = 0; // 当前接收的OTA固件的大小
static uint16_t ota_crc = 0;  // 当前接收的OTA固件的校验和

#define OTA_REPORT_NPAGE 4
enum {
  OTA_PEND_NOT    = 0,
  OTA_PEND_START  = 1,
  OTA_PEND_FINISH = 2
};
struct app_otaexec_msg {
  struct app_msg_header header;
  uint8_t data[sizeof(struct app_data_ota_exec)];
  struct app_msg_footer footer;
};
static struct {
  uint16_t magic;             // 初始化标志
  uint8_t role;               // 更新的目标角色: VD/RP/AP
  uint8_t objid;              // 更新的镜像文件ID
  uint8_t ver;                // 更新的版本号
  uint8_t mode;               // 更新的模式
  uint8_t dest[MAC_LEN];      // 更新的目标MAC地址
  uint16_t npage;             // 更新固件的大小(以page计)
  uint8_t count;              // 更新的节点批量个数
  uint8_t autop;              // 是否自动更新，0:否，1:是
  uint8_t pend;               // 是否更新过程在进行
  struct app_otaexec_msg msg; // 更新命令的内容
  uint8_t commit[8];          // 更新固件的git版本号
  uint32_t write;             // 本ota_exec结构体已写入次数
  uint16_t crc;               // 本ota_exec结构体的CRC
} ota_exec;
static struct ctimer ota_exec_ct;

static uint8_t tsbuf[TSTAMP_LEN] = {0};

static struct app_msg txmsg;
static uint8_t txbuf[APP_DATA_MAXLEN]; // 发送消息缓冲区
static uint8_t txaes[APPMSG_AES_MAXLEN]; // 发送消息AES加密缓冲区
static uint8_t txb64[APPMSG_B64_MAXLEN]; // 发送消息Base64编码缓冲区

static uint8_t rxb64[240]; // 接收消息Base64编码缓冲区
static uint8_t rxaes[180]; // 接收消息AES加密缓冲区
static uint8_t rxbuf[180]; // 接收消息缓冲区

struct nv_node_entry {
  uint8_t mac[MAC_LEN]; // 节点MAC地址
  rimeaddr_t addr;      // 节点短地址
  uint8_t ver;          // 固件版本
  uint8_t pend;         // 更新状态
  int16_t done;         // 已接收页数
  int16_t last;         // 进度报告页号
};
struct node_entry {
  struct node_entry *next;
  struct nv_node_entry node;
  struct ctimer start_ct;
  struct ctimer radio_ct;
  struct ctimer reset_ct;
  uint8_t reset_n;
  uint8_t conned;
};
MEMB(app_node_mem, struct node_entry, APP_NODES_NUM);
LIST(app_node_list);

static struct node_entry *ota_waitne = NULL;
static uint8_t ota_waitack = 0;
/*---------------------------------------------------------------------------*/
static void app_send_to_gprs(uint8_t cmdop, uint8_t *payload, uint8_t len, uint8_t needrep, uint8_t retry);
//static void app_send_ack_gprs(struct app_msg *msg, uint8_t res);
static void app_send_radio_ack(uint8_t subop, uint8_t res, uint8_t *ts);
static void app_send_ota_dataack(struct app_msg *msg, uint8_t res, uint8_t *arg, uint8_t len);
static void app_send_ota_execack(struct app_msg *msg, uint8_t res, uint8_t ver);
static int app_gprs_send(struct app_msg *msg);

int app_factory_reset(const char *arg, int len);

static void ota_exec_save(void);
static void ota_nodes_save(void);
/*---------------------------------------------------------------------------*/
int app_gprs_rcvd(uint8_t *data, uint16_t len);

/*---------------------------------------------------------------------------*/
PROCESS(updater_process, "WVDS Updater");
AUTOSTART_PROCESSES(&updater_process);
/*---------------------------------------------------------------------------*/
/**
 * \brief  转义长度字节
 *
 * \param b 输入字节
 *
 * \retval 转义后字节。规则见文档《WVDS通讯协议》。
 */
static uint8_t
escape_len(uint8_t b)
{
  uint8_t c;

  switch(b) {
  case 0xFF:
    c = 0x01; break;
  case 0xAA:
    c = 0x02; break;
  case 0x1A:
    c = 0x03; break;
  case 0x1B:
    c = 0x04; break;
  case 0x08:
    c = 0x05; break;
  default:
    c = b; break;
  }

  return c;
}

static uint8_t
unescape_len(uint8_t b)
{
  uint8_t c;

  switch(b) {
  case 0x01:
    c = 0xFF; break;
  case 0x02:
    c = 0xAA; break;
  case 0x03:
    c = 0x1A; break;
  case 0x04:
    c = 0x1B; break;
  case 0x05:
    c = 0x08; break;
  default:
    c = b; break;
  }

  return c;
}

/**
 * \brief  转义CRC字节
 *
 * \param b 输入字节
 *
 * \retval 转义后字节。规则见文档《WVDS通讯协议》。
 */
static uint8_t
escape_crc(uint8_t b)
{
  uint8_t c;

  switch (b) {
  case 0xAA:
    c = 0x01; break;
  case 0xFF:
    c = 0xFE; break;
  case 0x1A:
    c = 0x03; break;
  case 0x1B:
    c = 0x04; break;
  case 0x08:
    c = 0x05; break;
  default:
    c = b; break;
  }

  return c;
}
/*---------------------------------------------------------------------------*/
void
app_msg_create(struct app_msg *msg, uint8_t cmdop, uint8_t *payload, uint8_t len, uint8_t dir, uint8_t rep)
{
  struct app_msg_header *header = (struct app_msg_header *)&msg->header;
  struct app_msg_footer *footer = (struct app_msg_footer *)(msg->data + len);
  uint16_t ccrc = 0;

  memset(msg, 0, sizeof(struct app_msg));
  header->beg = APPMSG_BEG;
  header->len = DEVNO_LEN + 1 + len;
  memcpy(header->devno, nib.devno, DEVNO_LEN);
  header->dir = dir;
  header->rep = rep;
  header->cmdop = cmdop;

  memcpy(msg->data, payload, len);

  ccrc = crc16_data(&header->len, (APPMSG_HEADER_LEN - 1 + len), 0x0000);
  footer->crc[0] = (ccrc >> 8);
  footer->crc[1] = (ccrc & 0xff);
  footer->end = APPMSG_END;
}

/*---------------------------------------------------------------------------*/
void
app_report_node(uint8_t *mac, rimeaddr_t *addr, uint8_t ver)
{
  struct app_msg *msg = &txmsg;
  uint8_t payload[MAC_LEN+3] = {0};
  uint8_t i = 0;

  memcpy(payload, mac, MAC_LEN); i += MAC_LEN;
  payload[i++] = ver;
  payload[i++] = addr->u8[1];
  payload[i++] = addr->u8[0];
  app_msg_create(msg, APPDATA_VD_CONN, payload, sizeof(payload), APPMSG_UP, 0);

  app_gprs_send(msg);
}

void
app_report_progress(uint8_t *mac, uint8_t ver, uint16_t done)
{
  struct app_msg *msg = &txmsg;
  uint8_t payload[MAC_LEN + 3] = {0};
  uint8_t i = 0;

  memcpy(payload, mac, MAC_LEN); i += MAC_LEN;
  payload[i++] = ver;
  payload[i++] = (done >> 8);
  payload[i++] = (done & 0xff);
  app_msg_create(msg, APPDATA_OTA_PROG, payload, sizeof(payload), APPMSG_UP, 0);

  app_gprs_send(msg);
}

struct node_entry *
find_node_by_addr(rimeaddr_t *addr)
{
  struct node_entry *ne = NULL;
  for (ne = list_head(app_node_list); ne != NULL; ne = list_item_next(ne)) {
    if (rimeaddr_cmp(&ne->node.addr, addr)) {
      return ne;
    }
  }
  return NULL;
}

struct node_entry *
find_node_by_mac(uint8_t *mac)
{
  struct node_entry *ne = NULL;
  for (ne = list_head(app_node_list); ne != NULL; ne = list_item_next(ne)) {
    if (memcmp(ne->node.mac, mac, MAC_LEN) == 0) {
      return ne;
    }
  }
  return NULL;
}

struct node_entry *
add_node(uint8_t *mac, rimeaddr_t *addr, uint8_t ver)
{
  struct node_entry *ne = NULL;
  ne = memb_alloc(&app_node_mem);
  if (ne == NULL) {
    ne = list_pop(app_node_list);
    if (ne != NULL) {
      ctimer_stop(&ne->start_ct);
      ctimer_stop(&ne->radio_ct);
      ctimer_stop(&ne->reset_ct);
      memset(&ne->node, 0, sizeof(struct nv_node_entry));
    }
  }
  if (ne != NULL) {
    memcpy(ne->node.mac, mac, MAC_LEN);
    rimeaddr_copy(&ne->node.addr, addr);
    ne->node.ver = ver;
    ne->node.pend = OTA_PEND_NOT;
    ne->node.done = 0;
    ne->node.last = 0 - OTA_REPORT_NPAGE;
    list_add(app_node_list, ne);
    return ne;
  }
  return NULL;
}

void
upd_node(uint8_t *mac, rimeaddr_t *addr, uint8_t ver)
{
  struct node_entry *ne = NULL;

  ne = find_node_by_mac(mac);
  if (ne == NULL) {
    add_node(mac, addr, ver);
  } else {
    if (!rimeaddr_cmp(&ne->node.addr, addr)) {
      rimeaddr_copy(&ne->node.addr, addr);
    }
    if (ver != ne->node.ver) {
      ne->node.ver = ver;
    }
  }
}

int
app_update_start(uint8_t *mac, rimeaddr_t *from)
{
  struct node_entry *ne = NULL;
  struct app_data_ota_exec *exec = NULL;
  struct netcmd_msg_req req = {0};
  uint8_t len = 0;

  ne = find_node_by_mac(mac);
  if (ne != NULL) {
    len = APPMSG_HEADER_LEN + sizeof(struct app_data_ota_exec) + APPMSG_FOOTER_LEN;
    if (ota_exec.autop == 1) {
      memcpy(&txmsg, &ota_exec.msg, len);
      exec = (struct app_data_ota_exec *)txmsg.data;
      memcpy(exec->target, ne->node.mac, MAC_LEN);
      memcpy(txmsg.header.devno, ne->node.mac+2, DEVNO_LEN);
      netcmd_send(&ne->node.addr, APP_WVDS, (uint8_t*)&txmsg, len);
    } else {
      netcmd_send(&ne->node.addr, APP_WVDS, (uint8_t*)&ota_exec.msg, len);
    }

    // 构造netcmd req交处理
    req.type = NETCMD_REQ;
    memcpy(req.mac, mac, MAC_LEN);
    req.fwver = ota_exec.ver-1;
    netcmd_handle((struct netcmd_msg *)&req, from);
  }
  else {
    log_w(E_NULL, mac, MAC_LEN);
  }

  return 0;
}

void
app_update_start_task(void *ptr)
{
  struct node_entry *ne = (struct node_entry *)ptr;
  ctimer_set(&ne->start_ct, OTA_START_RETRY, app_update_start_task, ne);
  app_update_start(ne->node.mac, &ne->node.addr);
}

int
app_update_finish(uint8_t *mac)
{
  struct node_entry *ne = NULL;
  uint8_t done = 0;

  ne = find_node_by_mac(mac);

  if ((ne == NULL) || (ne->node.pend != OTA_PEND_FINISH))
    return 1;

  if (ota_exec.mode == DELUGE_MODE_ONENODE) {
    ctimer_stop(&ne->start_ct);
    ctimer_stop(&ne->radio_ct);
    ctimer_stop(&ne->reset_ct);
    ne->node.pend = OTA_PEND_NOT; // 标记已完成更新

    if (ota_exec.autop == 0) {
      if (memcmp(mac, ota_exec.dest, MAC_LEN) == 0) {
        done = 1;
      }
    } else {
      ota_exec.pend = OTA_PEND_NOT; // 标记以更新下一个
      ota_exec_save();
    }
  }
  else if (ota_exec.mode == DELUGE_MODE_ONEHOP) {
    ctimer_stop(&ne->start_ct);
    ctimer_stop(&ne->radio_ct);
    ctimer_stop(&ne->reset_ct);
    ne->node.pend = OTA_PEND_NOT; // 标记已完成更新

    done = 1; // 假设所有已更新
    for (ne = list_head(app_node_list); ne != NULL; ne = list_item_next(ne)) {
      if (ne->node.pend != OTA_PEND_NOT) {
        done = 0; break;
      }
    }
  }
  log_i(I_OTA_FINISH, mac, MAC_LEN);

  if (done) {
    deluge_stop();

    watchdog_periodic();
    ota_exec.pend = OTA_PEND_NOT;
    ota_exec.ver  = 0;
    ota_exec.mode = 0;
    memset(ota_exec.dest, 0, MAC_LEN);
    ota_exec_save();
    ota_nodes_save();

    log_i(I_OTA_FINISH, NULL, 0);
    return 0;
  }

  return 2;
}

void
app_reset_node(uint8_t *mac, rimeaddr_t *addr)
{
  struct app_msg msg;
  struct app_data_reset payload;
  uint8_t len = sizeof(struct app_data_reset);
  struct netcmd_msg_req req = {0};

  unixtime_get(payload.tstamp);
  app_msg_create(&msg, APPDATA_RESET, (uint8_t*)&payload, len, APPMSG_DOWN, 0);

  len += APPMSG_HEADER_LEN + APPMSG_FOOTER_LEN;
  netcmd_send(addr, APP_WVDS, (uint8_t*)&msg, len);

  req.type = NETCMD_REQ;
  memcpy(req.mac, mac, MAC_LEN);
  req.fwver = ota_exec.ver;
  netcmd_handle((struct netcmd_msg *)&req, addr);
}

void
app_no_request(void *ptr)
{
  struct node_entry *ne = (struct node_entry *)ptr;
  if (--(ne->reset_n)) {
    app_reset_node(ne->node.mac, &ne->node.addr);
    ctimer_set(&ne->reset_ct, OTA_MAX_NOREQ_PERIOD, app_no_request, ne);
  }
}

void
app_send_act_resp(void *ptr)
{
  struct node_entry *ne = (struct node_entry *)ptr;
  uint8_t *mac = ne->node.mac;
  rimeaddr_t *addr = &ne->node.addr;
  struct app_msg msg = {{0}};
  struct app_data_vd_activate_resp resp = {{0}};
  uint8_t len = sizeof(struct app_data_vd_activate_resp);
  struct netcmd_msg_req req = {0};

  unixtime_get(resp.tstamp);
  resp.result = APP_OK;
  app_msg_create(&msg, APPDATA_VD_ACTIVATE, (uint8_t*)&resp, len, APPMSG_DOWN, 0);

  len += APPMSG_HEADER_LEN + APPMSG_FOOTER_LEN;
  netcmd_send(addr, APP_WVDS, (uint8_t*)&msg, len);

  req.type = NETCMD_REQ;
  memcpy(req.mac, mac, MAC_LEN);
  req.fwver = ota_exec.ver;
  netcmd_handle((struct netcmd_msg *)&req, addr);
}

void
app_send_conn_resp(void *ptr)
{
  struct node_entry *ne = (struct node_entry *)ptr;
  uint8_t *mac = ne->node.mac;
  rimeaddr_t *addr = &ne->node.addr;
  struct app_msg msg = {{0}};
  struct app_data_vd_faeconn_resp resp = {{0}};
  uint8_t len = sizeof(struct app_data_vd_faeconn_resp);
  struct netcmd_msg_req req = {0};

  unixtime_get(resp.tstamp);
  resp.result = APP_OK;
  app_msg_create(&msg, APPDATA_VD_FAECONN, (uint8_t*)&resp, len, APPMSG_DOWN, 0);

  len += APPMSG_HEADER_LEN + APPMSG_FOOTER_LEN;
  netcmd_send(addr, APP_WVDS, (uint8_t*)&msg, len);

  req.type = NETCMD_REQ;
  memcpy(req.mac, mac, MAC_LEN);
  req.fwver = ota_exec.ver;
  netcmd_handle((struct netcmd_msg *)&req, addr);
}

void
app_send_disconn_req(void *ptr)
{
  struct node_entry *ne = (struct node_entry *)ptr;
  uint8_t *mac = ne->node.mac;
  rimeaddr_t *addr = &ne->node.addr;
  struct app_msg msg = {{0}};
  struct app_data_vd_disconn resp = {{0}};
  uint8_t len = sizeof(struct app_data_vd_disconn);
  struct netcmd_msg_req req = {0};

  unixtime_get(resp.tstamp);
  resp.subop = 1;
  resp.channel = pib.radioChan;
  app_msg_create(&msg, APPDATA_VD_DISCONN, (uint8_t*)&resp, len, APPMSG_DOWN, 1);

  len += APPMSG_HEADER_LEN + APPMSG_FOOTER_LEN;
  netcmd_send(addr, APP_WVDS, (uint8_t*)&msg, len);

  req.type = NETCMD_REQ;
  memcpy(req.mac, mac, MAC_LEN);
  req.fwver = ota_exec.ver;
  netcmd_handle((struct netcmd_msg *)&req, addr);
}

void
app_get_algo(uint8_t *mac, rimeaddr_t *addr)
{
  struct app_msg msg = {{0}};
  struct app_data_algo algo = {{0}};
  uint8_t len = sizeof(struct app_data_algo);
  struct netcmd_msg_req req = {0};

  unixtime_get(algo.tstamp);
  algo.subop = APP_GET;
  app_msg_create(&msg, APPDATA_ALGO_PARAM, (uint8_t*)&algo, len, APPMSG_DOWN, 0);

  len += APPMSG_HEADER_LEN + APPMSG_FOOTER_LEN;
  netcmd_send(addr, APP_WVDS, (uint8_t*)&msg, len);

  req.type = NETCMD_REQ;
  memcpy(req.mac, mac, MAC_LEN);
  req.fwver = ota_exec.ver;
  netcmd_handle((struct netcmd_msg *)&req, addr);
}

void
app_keep_radio_on(void *ptr)
{
  struct node_entry *ne = (struct node_entry *)ptr;
  app_get_algo(ne->node.mac, &ne->node.addr);
  ctimer_set(&ne->radio_ct, OTA_KEEP_RADIO_PERIOD, app_keep_radio_on, ne);
}

void
app_autop_no_start(void *ptr)
{
  struct node_entry *ne = (struct node_entry *)ptr;
  ctimer_stop(&ne->start_ct);
  ctimer_stop(&ne->radio_ct);
  ctimer_stop(&ne->reset_ct);
  ne->node.pend = OTA_PEND_NOT;
  ota_exec.pend = OTA_PEND_NOT;
  app_send_disconn_req(ne);
}

void
app_try_update(uint8_t *mac, rimeaddr_t *from, uint8_t ver)
{
  struct node_entry *ne = NULL;

  ne = find_node_by_mac(mac);

  if (ne == NULL) {
    ne = add_node(mac, from, ver);
  }
  if (ne == NULL) {
    log_w(E_NULL, mac, MAC_LEN);
    return;
  }

  if (ota_exec.pend == OTA_PEND_START) {
    if (ne->node.ver < ota_exec.ver) {
      if (((ota_exec.mode == DELUGE_MODE_ONENODE) && (memcmp(ne->node.mac, ota_exec.dest, MAC_LEN) == 0))
          || ((ota_exec.mode == DELUGE_MODE_ONEHOP) && (ne->node.mac[2] == ota_exec.dest[2]))) {
        if ((ne->node.mac[2] == NODE_VD) && (ne->conned == 0)) {
          ota_waitne = ne;
          ota_waitack = APPDATA_VD_FAECONN;
          app_send_conn_resp(ne);
        }
        if (ne->node.pend == OTA_PEND_NOT) {
          ne->node.pend = OTA_PEND_START;
          ctimer_set(&ne->start_ct, OTA_START_DELAY, app_update_start_task, ne);
        }
      }
    }
  }
  else if (ota_exec.pend == OTA_PEND_FINISH) {
    if (ne->node.pend == OTA_PEND_FINISH) {
      if ((ne->node.mac[2] == NODE_VD) && (ne->conned == 0)) {
        ota_waitne = ne;
        ota_waitack = APPDATA_VD_FAECONN;
        app_send_conn_resp(ne);
      }
    }
    else if ((ne->node.pend == OTA_PEND_START)) {
      if ((ne->node.mac[2] == NODE_VD) && (ne->conned == 0)) {
        ota_waitne = ne;
        ota_waitack = APPDATA_VD_FAECONN;
        app_send_conn_resp(ne);
      }
    }
    else if ((ne->node.pend == OTA_PEND_NOT)) {
      if (ne->node.ver < ota_exec.ver) {
        if (((ota_exec.mode == DELUGE_MODE_ONENODE) && (memcmp(ne->node.mac, ota_exec.dest, MAC_LEN) == 0))
            || ((ota_exec.mode == DELUGE_MODE_ONEHOP) && (ne->node.mac[2] == ota_exec.dest[2]))) {
          if ((ne->node.mac[2] == NODE_VD) && (ne->conned == 0)) {
            ota_waitne = ne;
            ota_waitack = APPDATA_VD_FAECONN;
            app_send_conn_resp(ne);
          }
          ne->node.pend = OTA_PEND_START;
          ctimer_set(&ne->start_ct, OTA_START_DELAY, app_update_start_task, ne);
        }
      }
    }
  }
  else { // ota_exec.pend == OTA_PEND_NOT
    if ((ota_exec.autop == 1) && (ota_exec.mode == DELUGE_MODE_ONENODE)) {
      if ((ne->node.ver < ota_exec.ver) && (ne->node.mac[2] == ota_exec.dest[2])) {
        if ((ne->node.mac[2] == NODE_VD) && (ne->conned == 0)) {
          ota_waitne = ne;
          ota_waitack = APPDATA_VD_FAECONN;
          app_send_conn_resp(ne);
        }
        if (ne->node.pend == OTA_PEND_NOT) {
          ne->node.pend = OTA_PEND_START;
          ctimer_set(&ne->start_ct, OTA_START_DELAY, app_update_start_task, ne);
        }

        ota_exec.pend = OTA_PEND_START;
        ota_exec.count = 1;
        ctimer_set(&ota_exec_ct, OTA_MAX_NOREQ_PERIOD, app_autop_no_start, ne);
      } else {
        uint8_t arg[9];
        memcpy(arg, ne->node.mac, MAC_LEN);
        arg[8] = ne->node.ver;
        log_w(W_INVALID_OTA_EXEC, arg, sizeof(arg));
      }
    }
  }
}

void
app_sniff_rcvd(void)
{
  uint16_t chan = (uint16_t)packetbuf_attr(PACKETBUF_ATTR_CHANNEL);
  rimeaddr_t *from = (rimeaddr_t*)packetbuf_addr(PACKETBUF_ADDR_SENDER);
  rimeaddr_t *to = (rimeaddr_t*)packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  struct neighbor *nbr = NULL;
  struct node_entry *ne = NULL;
  struct netcmd_msg *nmsg = NULL;
  struct netcmd_msg_req *req = NULL;

  nbr = neighbor_find(from);
  if (nbr == NULL) {
    neighbor_add(from);
  }

  if (chan == NETCMD_CHANNEL) {
    nmsg = (struct netcmd_msg *)packetbuf_dataptr();
    if (nmsg->type == NETCMD_REQ) {
      req = (struct netcmd_msg_req *)packetbuf_dataptr();
      upd_node(req->mac, from, req->fwver);
      app_report_node(req->mac, from, req->fwver);

      ne = find_node_by_mac(req->mac);
      if (ne != NULL) {
        if (req->fwver < ota_exec.ver) {
          app_try_update(req->mac, from, req->fwver); // 监听到netcmd req时开启更新
        }
      }
    }
    else if (nmsg->type == NETCMD_ACK) {
      if (rimeaddr_cmp(to, &ota_rimeaddr)) {
        if (ota_waitack == APPDATA_VD_FAECONN) {
          if (ota_waitne != NULL) {
            ota_waitne->conned = 1;
            ctimer_set(&ota_waitne->radio_ct, OTA_KEEP_RADIO_PERIOD, app_keep_radio_on, ota_waitne);
            ota_waitne = NULL;
          }
          ota_waitack = 0;
        }
      }
    }
  }
  else if ((chan == DELUGE_BROADCAST_CHANNEL) || (chan == DELUGE_UNICAST_CHANNEL)) {
    struct deluge_msg_summary *dsum = NULL;
    struct deluge_msg_request *dreq = NULL;
    uint8_t ver = 0;
    uint16_t done = 0;
    uint8_t objid = 0;
    uint8_t mode = 0;
    uint8_t *mac = NULL;

    if (chan == DELUGE_BROADCAST_CHANNEL) {
      dsum = (struct deluge_msg_summary *)packetbuf_dataptr();
      if (dsum->cmd == DELUGE_CMD_SUMMARY) {
        objid = dsum->object_id;
        mode = dsum->mode;
        ver = dsum->version;
        done = (dsum->highest_available[0]<<8) + dsum->highest_available[1];
        mac = dsum->mac;

        ne = find_node_by_addr(from);
        if ((ne != NULL) && (ver == ota_exec.ver)) {
          ne->node.done = done;
          if ((done >= ne->node.last + OTA_REPORT_NPAGE) || (done >= ota_exec.npage-1)) {
            app_report_progress(ne->node.mac, ver, done);
            ne->node.last = done;
          }
        }
      }
    }
    else if (chan == DELUGE_UNICAST_CHANNEL) {
      dreq = (struct deluge_msg_request *)packetbuf_dataptr();
      objid = dreq->object_id;
      mode = dreq->mode;
      ver = dreq->version;
      done = (dreq->pagenum[0]<<8) + dreq->pagenum[1];

      ne = find_node_by_addr(from);
      if (ne == NULL) {
        ne = memb_alloc(&app_node_mem);
        if (ne == NULL) {
          ne = list_pop(app_node_list);
          ctimer_stop(&ne->start_ct);
          ctimer_stop(&ne->radio_ct);
          ctimer_stop(&ne->reset_ct);
          memset(&ne->node, 0, sizeof(struct nv_node_entry));
        }
        if (ne != NULL) {
          rimeaddr_copy(&ne->node.addr, from);
          ne->node.pend = OTA_PEND_FINISH;
          ne->node.ver = ver;
          ne->node.done = done;
          ne->node.last = 0 - OTA_REPORT_NPAGE;
        }
      }

      if (ne != NULL) {
        ctimer_stop(&ne->start_ct);
        if (ota_exec.pend == OTA_PEND_START) {
          ne->reset_n = OTA_MAX_RESET_TIMES;
          ctimer_set(&ne->reset_ct, OTA_MAX_NOREQ_PERIOD, app_no_request, ne);

          if (ne->node.pend != OTA_PEND_FINISH) {
            ne->node.pend = OTA_PEND_FINISH; // 标记该节点已开始更新
            ota_exec.count -= 1;
            log_i(I_OTA_PAUSED, &ota_exec.count, 1);
          }
          else {
            if (ota_exec.count > 0) {
              ota_exec.count -= 1;
              log_i(I_OTA_PAUSED, &ota_exec.count, 1);
            }
          }

          if (ota_exec.autop == 1) {
            ctimer_stop(&ota_exec_ct);
          }

          if (ota_exec.count == 0) {
            watchdog_periodic();
            ota_exec.pend = OTA_PEND_FINISH;
            ota_exec_save();
            ota_nodes_save();
            deluge_resume(); // 指定数量节点已开启后继续更新
            log_i(I_OTA_RESUME, NULL, 0);
          }

          if (ver == ota_exec.ver) {
            ne->node.done = done;
            if ((done >= ne->node.last + OTA_REPORT_NPAGE) || (done >= ota_exec.npage-1)) {
              app_report_progress(ne->node.mac, ver, done);
              ne->node.last = done;
            }
          }
        }
        else if (ota_exec.pend == OTA_PEND_FINISH) {
          ne->reset_n = OTA_MAX_RESET_TIMES;
          ctimer_set(&ne->reset_ct, OTA_MAX_NOREQ_PERIOD, app_no_request, ne);

          if (ver == ota_exec.ver) {
            ne->node.done = done;
            if ((done >= ne->node.last + OTA_REPORT_NPAGE) || (done >= ota_exec.npage-1)) {
              app_report_progress(ne->node.mac, ver, done);
              ne->node.last = done;
            }
          }
        }
      }
    }
  }
  else if (chan == APP_MESH_CHANNEL) {
    struct app_msg *msg = (struct app_msg *)((uint8_t*)packetbuf_dataptr() + 7);
    uint8_t mac[MAC_LEN] = {0};

    if (msg->header.dir == APPMSG_UP) {
      memcpy(mac+2, msg->header.devno, DEVNO_LEN);

      if (msg->header.cmdop == APPDATA_VD_CONN) {
        struct app_data_vdconn *conn = (struct app_data_vdconn *)msg->data;
        upd_node(mac, from, conn->FwVer);
        app_report_node(mac, from, conn->FwVer);

        if (conn->FwVer == ota_exec.ver) {
          app_update_finish(mac); // 监听到入网消息标记结束
        }
        else if (conn->FwVer < ota_exec.ver) {
          app_try_update(mac, from, conn->FwVer); // 监听到入网消息开启更新
        }
      }
      else if ((msg->header.cmdop == APPDATA_VD_HBEAT) || (msg->header.cmdop == APPDATA_PARK_EVT) || (msg->header.cmdop == APPDATA_MAG_DATA)) {
        ne = find_node_by_mac(mac);
        if (ne == NULL)
          ne = add_node(mac, from, 0x00);
        if (ne != NULL && !rimeaddr_cmp(&ne->node.addr, from))
          rimeaddr_copy(&ne->node.addr, from);

        if (ne != NULL) {
          if (ne->node.ver < ota_exec.ver) {
            app_try_update(mac, from, ne->node.ver); // 监听到心跳/事件/磁场波动消息开启更新
          }
        }
      }
    }
  }
  else if (chan == APP_UNICAST_CHANNEL) {
    struct app_msg *msg = (struct app_msg *)packetbuf_dataptr();
    uint8_t mac[8] = {0};

    if (msg->header.cmdop == APPDATA_VD_ACTIVATE) {
      memcpy(mac+2, msg->header.devno, DEVNO_LEN);
      ne = find_node_by_mac(mac);
      if (ne == NULL)
        ne = add_node(mac, from, 0x00);
      if (ne != NULL && !rimeaddr_cmp(&ne->node.addr, from))
        rimeaddr_copy(&ne->node.addr, from);

      if (ota_exec.mode == DELUGE_MODE_ONENODE) {
        if (memcmp(mac, ota_exec.dest, MAC_LEN) == 0) {
          if (ne->conned == 0) {
            ota_waitne = ne;
            ota_waitack = APPDATA_VD_ACTIVATE;
            app_send_act_resp(ne);
          }

          ne->node.pend = OTA_PEND_START;
          ctimer_set(&ne->start_ct, OTA_START_DELAY, app_update_start_task, ne);
        }
        else if (ota_exec.autop == 1) {
          if (ota_exec.pend == OTA_PEND_NOT) {
            if (ne->conned == 0) {
              ota_waitne = ne;
              ota_waitack = APPDATA_VD_ACTIVATE;
              app_send_act_resp(ne);
            }
            if (ne->node.pend == OTA_PEND_NOT) {
              ne->node.pend = OTA_PEND_START;
              ctimer_set(&ne->start_ct, OTA_START_DELAY, app_update_start_task, ne);

              ota_exec.pend = OTA_PEND_START;
              ota_exec.count = 1;
              ctimer_set(&ota_exec_ct, OTA_MAX_NOREQ_PERIOD, app_autop_no_start, ne);
            }
          }
        }
      }
    }
  }
  else if (chan == LOG_CHANNEL) {
    struct log_msg *msg = (struct log_msg *)packetbuf_dataptr();

    if (msg->level == LOG_INFO) {
      if (msg->code == I_REBOOT) {
        uint8_t *mac = msg->data;
        uint8_t ver = msg->data[MAC_LEN + 1];
        uint8_t *commit = &msg->data[MAC_LEN + 2];
        upd_node(mac, from, ver);

        ne = find_node_by_mac(mac);
        if (ne != NULL) {
          ne->conned = ((mac[2] == NODE_VD) ? 0 : 1); // 标记VD未连接
        }

        if (ver == ota_exec.ver) {
          if ((ne != NULL) && (ne->node.pend == OTA_PEND_FINISH)) {
            app_update_finish(mac); // 监听到重启消息标记结束
          }
          app_report_node(mac, from, ver);
        }
        else if (memcmp(commit, ota_exec.commit, 7) == 0) {
          if ((ne != NULL) && (ne->node.pend == OTA_PEND_FINISH)) {
            app_update_finish(mac); // 监听到重启消息标记结束
          }
          app_report_node(mac, from, ota_exec.ver);
        }
        else {
          app_report_node(mac, from, ver);
        }
      }
      else if (msg->code == I_OTA_START) {
        uint8_t *mac = msg->data;
        app_report_progress(mac, ota_exec.ver, 0);
      }
    }
    else if (msg->level == LOG_WARN) {
      if (msg->code == W_INVALID_OTA_EXEC) {
        ne = find_node_by_addr(from);
        if (ne != NULL) {
          ne->node.ver = ota_exec.ver;
        }
      }
    }
  }
}

void
app_sniff_sent(int mac_status)
{
}

RIME_SNIFFER(app_sniff, app_sniff_rcvd, app_sniff_sent);
/*---------------------------------------------------------------------------*/
static void
app_mesh_rcvd(struct broadcast_conn *c, const rimeaddr_t *from)
{
}

const static struct broadcast_callbacks mesh_cb = {app_mesh_rcvd};
/*---------------------------------------------------------------------------*/
static void
app_unic_rcvd(struct unicast_conn *c, const rimeaddr_t *from)
{
  struct app_msg *msg = (struct app_msg *)packetbuf_dataptr();
  uint8_t len = packetbuf_datalen();
  uint8_t mac[MAC_LEN] = {0};

  if (msg->header.cmdop == APPDATA_NODE_INFO) {
    struct app_data_nodeinfo_resp *resp = (struct app_data_nodeinfo_resp *)msg->data;
    memcpy(mac+2, msg->header.devno, DEVNO_LEN);
    upd_node(mac, (rimeaddr_t*)from, resp->FwVer);
    app_report_node(mac, (rimeaddr_t*)from, resp->FwVer);
  }
}

static void
app_unic_sent(struct unicast_conn *ptr, int status, int num_tx)
{
}

const static struct unicast_callbacks unic_cb = {app_unic_rcvd,app_unic_sent};
/*---------------------------------------------------------------------------*/
/**
 * \brief  将一个下行消息转发到目标网内节点
 *
 * \param msg 待转发的消息的指针
 * \param len 待转发的消息的长度
 */
static int
app_fwd_to_nwk(struct app_msg *msg, uint16_t len)
{
  uint8_t mac[MAC_LEN] = {0};
  struct node_entry *ne = NULL;
  uint8_t arg[10] = {0};
  int r = APP_OK;

  if (memcmp(mac+2, msg->header.devno, DEVNO_LEN) == 0) { // 目标是网内所有节点
    // 交给下行命令模块转发给所有节点，目标地址是广播地址
    r = netcmd_send((rimeaddr_t*)&rimeaddr_null, APP_WVDS, (uint8_t*)msg, len);

  } else { // 目标是某个网内节点
    memcpy(mac+2, msg->header.devno, DEVNO_LEN); // 构造MAC地址
    ne = find_node_by_mac(mac); // 根据MAC地址查询节点
    if (ne != NULL) { // 节点存在
      // 交给下行命令模块转发给指定节点，目标地址是该节点地址
      r = netcmd_send(&ne->node.addr, APP_WVDS, (uint8_t*)msg, len);
#if 1
      memcpy(arg, msg->header.devno, DEVNO_LEN);
      arg[DEVNO_LEN] = msg->header.cmdop;
      arg[DEVNO_LEN+1] = ne->node.addr.u8[1];
      arg[DEVNO_LEN+2] = ne->node.addr.u8[0];
      log_i(I_FWD_COMMAND, arg, 9); // 转发下行命令
#endif
    } else {
#if RWARN
      memcpy(arg, msg->header.devno, DEVNO_LEN);
      arg[DEVNO_LEN] = msg->header.cmdop;
      PRINTF("warn app no node %02X%02X%02X%02X%02X%02X\n", arg[0],arg[1],arg[2],arg[3],arg[4],arg[5]);
      log_w(W_UNKNOWN_TARGET, arg, 7); // 不存在下行命令的目标节点
#endif
      r = APP_ERR; // 不存在该目标节点，报错
    }
  }

  // 发送ACK告知AP已成功接收和处理该下行消息
  //app_send_ack_gprs(msg, r);

  return r;
}

static void
app_get_info(struct app_msg *msg, uint8_t len)
{
  packetbuf_copyfrom(msg, len);
  unicast_send(&unic, &rimeaddr_null);
}

/**
 * \brief  来自GPRS的下行消息的接收处理函数
 *
 * \param data 待处理的接收消息的缓冲区指针
 * \param len 待处理的接收消息的长度
 *
 * \retval 0 处理成功
 */
static int
app_handle_gprs(uint8_t *data, uint16_t len)
{
  struct app_msg *msg = (struct app_msg *)data;

  if (msg->header.cmdop == APPDATA_RADIO_PARAM) { // 射频参数消息
    struct app_data_radio *p = (struct app_data_radio *)msg->data;

    if (p->subop == APP_GET) { // 查询
      app_send_radio_ack(APP_GET, 0, p->tstamp); // 发送查询响应消息
    }
    else if (p->subop == APP_SET) { // 配置
      int err = 0;

      if (p->chan != pib.radioChan) {
        if (p->chan >= RFCHAN_MIN && p->chan <= RFCHAN_MAX) {
          radio_set_channel(p->chan);
        } else {
          err = 1; // 射频信道参数值非法
        }
      }

      if (p->power != pib.radioPower) {
        if (p->power >= RFPOWER_MIN && p->power <= RFPOWER_MAX) {
          radio_set_txpower(p->power); // 设置射频功率
        } else {
          err = 2; // 射频功率参数值非法
        }
      }

      if (err == 0) {
        if ((p->chan != pib.radioChan) || (p->power != pib.radioPower)) {
          pib.radioChan = p->chan;
          pib.radioPower = p->power;
          pib.crc = crc16_data((uint8_t*)&pib, sizeof(struct PIB)-2, CRC_INIT);
          nv_write(NV_PIB_ADDR, (uint8_t*)&pib, sizeof(struct PIB)); // 保存新的射频参数到NIB
        }

        app_send_radio_ack(APP_SET, APP_OK, p->tstamp); // 发送配置成功响应消息
      }
      else {
        app_send_radio_ack(APP_SET, APP_ERR, p->tstamp); // 发送配置失败响应消息
      }
    }
  }
  else if (msg->header.cmdop == APPDATA_RESET) { // 重启消息
    if (msg->header.devno[0] == NODE_VD) {
      app_fwd_to_nwk(msg, len); // 转发给网内节点
    } else {
      packetbuf_copyfrom(msg, len);
      unicast_send(&unic, &rimeaddr_null);
    }
  }
  else if (msg->header.cmdop == APPDATA_NODE_INFO) {
    app_get_info(msg, len);
  }
//  else {
//#if RWARN
//    uint8_t arg[1];
//    arg[0] = msg->header.cmdop;
//    log_w(E_INVAL, arg, 1); // 未处理的下行消息
//#endif
//    return 1;
//  }

  return 0;
}

/**
 * \brief  无线升级相关消息的处理函数
 *
 * \param msg 待处理的消息的指针
 *
 * \retval 0 处理成功
 *         1 处理失败
 */
static int
app_handle_otamsg(struct app_msg *msg)
{
  struct app_data_ota_data *dat = (struct app_data_ota_data *)msg->data;
  struct node_entry *ne = NULL;
  struct filedesc *f = NULL;
  uint16_t ccrc = 0, rcrc;
  uint32_t addr = 0;
  uint16_t len = 0;
  uint16_t i = 0;
  int r = 0;

  if (msg->header.cmdop == APPDATA_OTA_DATA) {
    if (dat->ver < ota.images[OTA_IMGID[dat->type]].ver) {
      app_send_ota_dataack(msg, 6, &(ota.images[OTA_IMGID[dat->type]].ver), 1); // version not newer
      return 1;
    }
    //else if (dat->ver == ota.images[OTA_IMGID[dat->type]].ver) {
    //  app_send_ota_dataack(msg, 7, &(ota.images[OTA_IMGID[dat->type]].ver), 1); // identical version
    //  return 1;
    //}

    rcrc = (dat->crc[0]<<8) + dat->crc[1];
    ccrc = crc16_data(msg->data + 6, msg->header.len - (6+1+6+2), 0x0000);
    if (ccrc != rcrc) {
      uint8_t arg[4];
      arg[0] = (rcrc >> 8); arg[1] = (rcrc & 0xff);
      arg[2] = (ccrc >> 8); arg[3] = (ccrc & 0xff);
      app_send_ota_dataack(msg, APP_ERR, arg, 4); // packet crc error
      return 1;
    }

    if (dat->beg) {
      if (ota_fd == -1) {
        ota_fd = cfs_open(OTA_FILE[dat->type], CFS_WRITE);
        if (ota_fd == -1) {
          app_send_ota_dataack(msg, 2, NULL, 0); // cannot open file
          return 1;
        }
        f = cfs_get(OTA_FILE[dat->type]);
        f->endptr = 0;
      }
      cfs_seek(ota_fd, 0, CFS_SEEK_SET);
      ota_addr = 0;
      ota_size = 0;
      ota_crc  = 0;
      ota_last = 0;
    }

    if (ota_fd == -1) {
      app_send_ota_dataack(msg, 3, NULL, 0); // file not open
      return 1;
    }

    addr = ((uint32_t)dat->addr[0]<<24) + ((uint32_t)dat->addr[1]<<16)
      + ((uint32_t)dat->addr[2]<<8) + ((uint32_t)dat->addr[3]);
    if (addr == ota_last) {
      app_send_ota_dataack(msg, 5, NULL, 0); // duplicate packet
      return 1;
    }

    len = (dat->len[0]<<8) + dat->len[1];
    cfs_seek(ota_fd, ota_addr, CFS_SEEK_SET);
    r = cfs_write(ota_fd, dat->data, len);
    if (r == -1) {
      app_send_ota_dataack(msg, 4, NULL, 0); // fail write packet
      return 1;
    }

    ota_last = addr;
    ota_addr += len;
    ota_size += len;
    for (i = 0; i < len; i++)
      ota_crc = crc16_add(dat->data[i], ota_crc);

    if (dat->fin) {
      cfs_close(ota_fd);
      ota_fd = -1;
      leds_off(LEDS_GREEN);
      leds_off(LEDS_ORANGE);

      for (i = 0; i < 3; i++) {
        if (ota.images[i].id == dat->type) {
          ota.images[i].ver = dat->ver;
          ota.images[i].crc = ota_crc;
          ota.images[i].size = ota_size;
          ota.images[i].rcvd = (ota_size + (S_PAGE-1)) / S_PAGE;
          ota.crc = crc16_data((uint8_t*)&ota, sizeof(struct OTA)-2, CRC_INIT);
          nv_write(NV_OTA_ADDR, (uint8_t*)&ota, sizeof(struct OTA));
          break;
        }
      }
    }

    app_send_ota_dataack(msg, APP_OK, NULL, 0);
  }
  else if (msg->header.cmdop == APPDATA_OTA_EXEC) {
    struct app_data_ota_exec *exec = (struct app_data_ota_exec *)msg->data;

    if (exec->mode == 0) {
      // 停止目前未完成的更新
      if ((ota_exec.pend != OTA_PEND_NOT) || (ota_exec.autop == 1)) {
        deluge_stop();

        ota_waitne = NULL;
        ota_waitack = 0;
        for (ne = list_head(app_node_list); ne != NULL; ne = list_item_next(ne)) {
          ctimer_stop(&ne->start_ct);
          ctimer_stop(&ne->radio_ct);
          ctimer_stop(&ne->reset_ct);
          ne->node.pend = OTA_PEND_NOT;
          if (ne->conned == 1) {
            app_send_disconn_req(ne);
            ne->conned = 0;
          }
        }

        ota_exec.role = 0;
        ota_exec.objid = 0;
        ota_exec.pend = OTA_PEND_NOT;
        ota_exec.autop = 0;
        ota_exec.mode = 0;
        ota_exec.ver  = 0;
        ota_exec.npage = 0;
        memset(ota_exec.dest, 0, MAC_LEN);
        memset(ota_exec.commit, 0, 8);
        ota_exec_save();
      }
      app_send_ota_execack(msg, APP_OK, exec->fwver);
      log_i(I_OTA_FINISH, NULL, 0);

    } else {
      // 根据目标节点类型确定固件镜像ID
      ota_exec.role = exec->type;
      if (exec->type == NODE_AP) {
        ota_exec.objid = OBJ_GATEWAY;
      } else if (exec->type == NODE_RP) {
        ota_exec.objid = OBJ_ROUTER;
      } else if (exec->type == NODE_VD) {
        ota_exec.objid = OBJ_DEVICE;
      }

      nv_read(NV_OTA_ADDR, (uint8_t*)&ota, sizeof(struct OTA));
      if (exec->fwver > ota.images[ota_exec.objid].ver) { // 升级版本号高于已下载固件版本号
        app_send_ota_execack(msg, 3, ota.images[ota_exec.objid].ver); // 对应版本新固件还未下载
      }
      else if (exec->fwver < ota.images[ota_exec.objid].ver) { // 升级版本号低于已下载固件版本号
        app_send_ota_execack(msg, 4, ota.images[ota_exec.objid].ver); // 与已下载版本的固件不区配
      }
      else {
        PRINTF("app rcvd ota %d/%d/%d\n", exec->type, exec->fwver, exec->mode);
        len = APPMSG_HEADER_LEN + sizeof(struct app_data_ota_exec) + APPMSG_FOOTER_LEN;

        deluge_stop();
        deluge_start(NULL);
        deluge_disseminate(ota_exec.objid, (char*)OTA_FILE[exec->type], exec->fwver, exec->mode, exec->target); // 调用无线升级模块分发新固件数据

        memcpy(msg->header.devno, exec->target+2, DEVNO_LEN); // 修改消息头的目标地址
        memcpy(&ota_exec.msg, msg, len);
        memcpy(ota_exec.dest, exec->target, MAC_LEN);
        memcpy(ota_exec.commit, exec->commit, 8);
        ota_exec.mode = exec->mode;
        ota_exec.ver = exec->fwver;
        ota_exec.npage = ota.images[OTA_IMGID[exec->target[2]]].size / S_PAGE;

        if (exec->mode == DELUGE_MODE_ONENODE) {
          ota_exec.count = exec->count;
          ota_exec.autop = exec->autop;
          ota_exec.pend = ((exec->autop == 0) ? OTA_PEND_START : OTA_PEND_NOT);
          ota_exec_save();

          if (ota_exec.autop == 0) { // 非自动更新
            ne = find_node_by_mac(exec->target);
            if (ne != NULL) {
              if (ne->node.mac[2] == NODE_VD) { // 更新VD
                r = app_fwd_to_nwk(msg, len); // 转发给网内节点
                if (r != 0) {
                  app_send_ota_execack(msg, 5, exec->fwver); // 发送节点未上线响应
                  return 0;
                }
              } else { // 更新RP或AP
                packetbuf_copyfrom(msg, len);
                unicast_send(&unic, &ne->node.addr);
              }
              app_send_ota_execack(msg, APP_OK, exec->fwver); // 发送OTA执行成功响应消息
            }
            else {
              app_send_ota_execack(msg, 5, exec->fwver); // 发送节点未上线响应
            }
          }
          else { // 自动更新
            app_send_ota_execack(msg, APP_OK, exec->fwver); // 发送OTA执行成功响应消息
          }
        }
        else if (exec->mode == DELUGE_MODE_ONEHOP) {
#if 0
          ota_exec.num  = exec->count;
          ota_exec.autop = exec->autop;
          ota_exec.pend = OTA_PEND_START;
          ota_exec.crc = crc16_data((uint8_t*)&ota_exec, sizeof(ota_exec)-2, CRC_INIT);
          nv_write(NV_STATS_ADDR, (uint8_t*)&ota_exec, sizeof(ota_exec));

          deluge_pause(); // 暂停以等待指定数量节点开启更新
          log_i(I_OTA_START, &ota_exec.num, 1);
          app_send_ota_execack(msg, 5, exec->fwver); // 发送节点未上线响应
#else
          app_send_ota_execack(msg, 6, exec->fwver); // 发送不支持该模式响应
#endif
        }
        else if (exec->mode == DELUGE_MODE_ALLNODE) {
          app_send_ota_execack(msg, 6, exec->fwver); // 发送不支持该模式响应
        }
      }
    }
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief  构造和通过GPRS发送一条消息到服务器
 *
 * \param cmdop  待发送消息的命令字
 * \param payload  消息有效载荷的指针
 * \param len  消息有效载荷的长度
 * \param needrep  是否需要服务器返回响应
 * \param retry  最大传输次数
 */
static void
app_send_to_gprs(uint8_t cmdop, uint8_t *payload, uint8_t len, uint8_t needrep, uint8_t retry)
{
  struct app_msg *msg = &txmsg;
  app_msg_create(msg, cmdop, payload, len, APPMSG_UP, needrep);
  app_gprs_send(msg);
}

static void
app_send_radio_ack(uint8_t subop, uint8_t res, uint8_t *ts)
{
  struct app_data_radio_ack *radio = (struct app_data_radio_ack *)txbuf;
  uint32_t freq = 0;

  memset(txbuf, 0, sizeof(txbuf));
  memcpy(radio->tstamp, ts, TSTAMP_LEN);
  radio->subop = subop;
  radio->res = res;
  if (subop == APP_GET) {
    freq = radio_get_frequency();
    radio->chan = radio_get_channel();
    radio->freq[0] = ((freq >> 24) & 0xff);
    radio->freq[1] = ((freq >> 16) & 0xff);
    radio->freq[2] = ((freq >>  8) & 0xff);
    radio->freq[3] = ((freq >>  0) & 0xff);
    radio->power = pib.radioPower;
  }

  PRINTF("app send radio\n");
  app_send_to_gprs(APPDATA_RADIO_PARAM, txbuf, sizeof(struct app_data_radio_ack), 0, 1);
}

//static void
//app_send_ack_gprs(struct app_msg *msg, uint8_t res)
//{
//  struct app_data_ack *ack = (struct app_data_ack *)txbuf;
//
//  memset(txbuf, 0, sizeof(txbuf));
//  memcpy(ack->tstamp, msg->data, TSTAMP_LEN);
//  ack->result = res;
//
//  app_send_to_gprs(msg->header.cmdop, txbuf, sizeof(struct app_data_ack), 0, 1);
//}

static void
app_send_ota_dataack(struct app_msg *msg, uint8_t res, uint8_t *arg, uint8_t len)
{
  struct app_data_ota_data *dat = (struct app_data_ota_data *)msg->data;
  struct app_data_ota_dataack *ack = (struct app_data_ota_dataack *)txbuf;
  uint8_t *addr = dat->addr;

  memset(txbuf, 0, sizeof(txbuf));
  memcpy(ack->tstamp, dat->tstamp, TSTAMP_LEN);
  ack->res = res;
  ack->addr[0] = addr[0];
  ack->addr[1] = addr[1];
  ack->addr[2] = addr[2];
  ack->addr[3] = addr[3];
  if (arg != NULL && len > 0)
    memcpy(ack->arg, arg, len);

  app_send_to_gprs(msg->header.cmdop, txbuf, sizeof(struct app_data_ota_dataack), 0, 1);
}

static void
app_send_ota_execack(struct app_msg *msg, uint8_t res, uint8_t ver)
{
  struct app_data_ota_execack *ack = (struct app_data_ota_execack *)txbuf;
  struct app_data_ota_exec *exec = (struct app_data_ota_exec *)msg->data;

  memset(txbuf, 0, sizeof(txbuf));
  memcpy(ack->tstamp, exec->tstamp, TSTAMP_LEN);
  ack->result = res;
  ack->version = ver;

  app_send_to_gprs(msg->header.cmdop, txbuf, sizeof(struct app_data_ota_execack), 0, 1);
}
/*---------------------------------------------------------------------------*/
static void
ota_exec_save(void)
{
#if OTA_EXEC_NV
  ota_exec.write += 1;
  ota_exec.crc = crc16_data((uint8_t*)&ota_exec, sizeof(ota_exec)-2, CRC_INIT);
  nv_write(NV_STATS_ADDR, (uint8_t*)&ota_exec, sizeof(ota_exec));
#endif
}

static void
ota_nodes_save(void)
{
#if OTA_NODES_NV
  struct node_entry *ne = NULL;
  uint32_t addr = 0;

  nodes.count = list_length(app_node_list);
  nodes.write += 1;
  nodes.crc = crc16_data((uint8_t*)&nodes, sizeof(struct NODES)-2, CRC_INIT);
  nv_write(NV_NODES_ADDR, (uint8_t*)&nodes, sizeof(struct NODES));

  addr = NV_NODES_ADDR + sizeof(struct NODES);
  for (ne = list_head(app_node_list); ne != NULL; ne = list_item_next(ne)) {
    watchdog_periodic();
    nv_write(addr, (uint8_t*)&ne->node, sizeof(struct nv_node_entry));
    addr += sizeof(struct nv_node_entry);
  }
#endif
}

/*---------------------------------------------------------------------------*/
/**
 * \brief  主应用的初始化
 *
 *         从存储器中加载节点之前运行保存的相关数据。第一次上电运行时将使用默
 *         认值初始化存储器的相关存储段。
 */
static void
app_init(void)
{
  uint16_t ccrc;
  uint8_t i;
#if OTA_NODES_NV
  struct node_entry *ne = NULL;
  uint32_t raddr = 0;
#endif

  // 设置节点短地址
  rimeaddr_set_node_addr(&ota_rimeaddr);

  // 初始化存储器访问模块
  nv_init();

  // 读取或初始化节点无线升级信息
  nv_read(NV_OTA_ADDR, (uint8_t*)&ota, sizeof(struct OTA));
  ccrc = crc16_data((uint8_t*)&ota, sizeof(struct OTA)-2, CRC_INIT);
  if ((ota.magic != NV_MAGIC) || (ccrc != ota.crc)) {
    memset(&ota, 0, sizeof(struct OTA));
    ota.magic = NV_MAGIC;
    for (i = 0; i < 3; i++) {
      ota.images[i].id  = OTA_ROLE[i];
    }
    ota.crc = crc16_data((uint8_t*)&ota, sizeof(struct OTA)-2, CRC_INIT);
    nv_write(NV_OTA_ADDR, (uint8_t*)&ota, sizeof(struct OTA));
  }

#if OTA_EXEC_NV
  /// 读取升级的进度信息
  nv_read(NV_STATS_ADDR, (uint8_t*)&ota_exec, sizeof(ota_exec));
  ccrc = crc16_data((uint8_t*)&ota_exec, sizeof(ota_exec)-2, CRC_INIT);
  if ((ota_exec.magic != NV_MAGIC) || (ccrc != ota_exec.crc)) {
    memset(&ota_exec, 0, sizeof(ota_exec));
    ota_exec.magic = NV_MAGIC;
    ota_exec.crc = crc16_data((uint8_t*)&ota_exec, sizeof(ota_exec)-2, CRC_INIT);
    nv_write(NV_STATS_ADDR, (uint8_t*)&ota_exec, sizeof(ota_exec));
  }
#endif

  // 读取或初始化节点特定信息
  nv_read(NV_NIB_ADDR, (uint8_t*)&nib, sizeof(struct NIB));
  ccrc = crc16_data((uint8_t*)&nib, sizeof(struct NIB)-2, CRC_INIT);
  if ((nib.magic != NV_MAGIC) || (ccrc != nib.crc) || (nib.devno[0] != 0x05)) {
    memset(&nib, 0, sizeof(struct NIB));
    nib.magic = NV_MAGIC;
    nib.devno[0] = 0x05; // 更新节点(不可更改)
    nib.devno[1] = 0xCA; // 厂商代码，CA拟为Cadre简称
    nib.devno[2] = 0x17; // 生产年份(BCD)
    nib.devno[3] = 0x03; // 生产月份(BCD)
    nib.devno[4] = 0x00; // 生产序号高字节(BCD)
    nib.devno[5] = 0x05; // 生产序号低字节(BCD)
    nib.hwver = 0x10; // 硬件版本号(BCD), v1.0
    nib.fwver = 0x10; // 固件版本号(BCD), v1.0
    nib.tricseq = 2;  // 全网参数序号
    nib.locked = 0;   // 网络锁定标志，默认0未锁定
    memcpy(nib.aeskey, APP_AES_KEY, 16);    // AES加密密钥
    memcpy(nib.aesiv, APP_AES_IV, 16);      // AES加密向量
    memcpy(nib.host, "58.250.57.68\0", 13); // 服务器IP地址
    nib.port = 62000;                        // 服务器端口号
    nib.crc = crc16_data((uint8_t*)&nib, sizeof(struct NIB)-2, CRC_INIT);
    nv_write(NV_NIB_ADDR, (uint8_t*)&nib, sizeof(struct NIB));
  }
  // 根据设备编码设置64位节点MAC地址
  memset(node_mac, 0, sizeof(node_mac));
  memcpy(node_mac + 2, nib.devno, sizeof(nib.devno));

  // 如果与启动信息中的固件版本不一致，更新其中版本号
  ReadFlash(FLASH_BOOT_ADDR, (uint8_t*)&boot, sizeof(struct BOOT));
  if (boot.version != nib.fwver) {
    nib.fwver = boot.version;
    nib.crc = crc16_data((uint8_t*)&nib, sizeof(struct NIB)-2, CRC_INIT);
    nv_write(NV_NIB_ADDR, (uint8_t*)&nib, sizeof(struct NIB));
  }

  // 读取或初始化全网基本参数
  nv_read(NV_PIB_ADDR, (uint8_t*)&pib, sizeof(struct PIB));
  ccrc = crc16_data((uint8_t*)&pib, sizeof(struct PIB)-2, CRC_INIT);
  if ((pib.magic != NV_MAGIC) || (ccrc != pib.crc)) {
    memset(&pib, 0, sizeof(struct PIB));
    pib.magic = NV_MAGIC;
    pib.apHbeatT = 30; // AP心跳周期
    pib.rpHbeatT = 600; // RP心跳周期
    pib.vdHbeatT = 600; // VD心跳周期
    pib.healthT  = 3600; // 自检周期
    pib.commandT = 60; // 下行命令轮询周期
    pib.paramT   = 512; // 全网参数轮询周期
    pib.batVoltageThr = 34; // 电池电压阈值
    pib.batQuantityThr = 20; // 电池电量阈值
    pib.solarVoltageThr = 200; // 太阳能电池电压阈值
    pib.radioChan  = RF_CHANNEL; // 默认信道 6/916MHz
    pib.radioPower = 24; // 默认功率 24dbm/250mW
    pib.crc = crc16_data((uint8_t*)&pib, sizeof(struct PIB)-2, CRC_INIT);
    nv_write(NV_PIB_ADDR, (uint8_t*)&pib, sizeof(struct PIB));
  }

  // 检查PIB中相关时间值，如果为0或过小，设置为默认值
  if (pib.apHbeatT < APHBEATT_MIN)
    pib.apHbeatT = APHBEATT_MIN;
  if (pib.rpHbeatT < RPHBEATT_MIN)
    pib.rpHbeatT = RPHBEATT_MIN;
  if (pib.vdHbeatT < VDHBEATT_MIN)
    pib.vdHbeatT = VDHBEATT_MIN;
  if (pib.healthT < HEALTHT_MIN)
    pib.healthT = HEALTHT_MIN;
  if (pib.commandT < COMMANDT_MIN)
    pib.commandT = COMMANDT_MIN;
  if (pib.paramT < PARAMT_MIN)
    pib.paramT = PARAMT_MIN;

  // 读取或初始化升级节点列表
#if OTA_NODES_NV
  nv_read(NV_NODES_ADDR, (uint8_t*)&nodes, sizeof(struct NODES));
  ccrc = crc16_data((uint8_t*)&nodes, sizeof(struct NODES)-2, CRC_INIT);
  if ((nodes.magic != NV_MAGIC) || (ccrc != nodes.crc)) {
    memset(&nodes, 0, sizeof(struct NODES));
    nodes.magic = NV_MAGIC;

    nv_erase(NV_NODES_ADDR, NV_NODES_SIZE);
    nodes.crc = crc16_data((uint8_t*)&nodes, sizeof(struct NODES)-2, CRC_INIT);
    nv_write(NV_NODES_ADDR, (uint8_t*)&nodes, sizeof(struct NODES));
  }
#endif

  memb_init(&app_node_mem);
  list_init(app_node_list);
#if OTA_NODES_NV
  for (i = 0; i < nodes.count; i++) {
    watchdog_periodic();
    ne = memb_alloc(&app_node_mem);
    if (ne != NULL) {
      raddr = NV_NODES_ADDR + sizeof(struct NODES) + sizeof(struct nv_node_entry) * i;
      nv_read(raddr, (uint8_t *)&ne->node, sizeof(struct nv_node_entry));
      list_add(app_node_list, ne);
    }
  }
#endif

#if 0
  ne = memb_alloc(&app_node_mem);
  ne->node.mac[0] = 0x00;
  ne->node.mac[1] = 0x00;
  ne->node.mac[2] = 0x01;
  ne->node.mac[3] = 0xCA;
  ne->node.mac[4] = 0x17;
  ne->node.mac[5] = 0x05;
  ne->node.mac[6] = 0x00;
  ne->node.mac[7] = 0x01;
  ne->node.addr.u8[0] = 25;
  ne->node.addr.u8[1] = 0;
  ne->node.ver = 0x18;
  ne->node.pend = OTA_PEND_FINISH;
  ne->node.done = 0;
  ne->node.last = 0;
  list_add(app_node_list, ne);
  ota_nodes_save();
#endif

#if WITH_OTA
  {
    int fd;
    // 初始化CFS文件系统
    cfs_init();

    // 为各程序镜像预留CFS存储空间
    fd = cfs_open(OTA_FILE[NODE_AP], CFS_READ);
    if (fd == -1) {
      fd = cfs_open(OTA_FILE[NODE_AP], CFS_WRITE);
      if (fd != -1) {
        cfs_reserv(fd, ((uint32_t)90<<10)/*AP_IMG_SIZE*/);
        cfs_close(fd);
      }
    } else {
      cfs_close(fd);
    }

    fd = cfs_open(OTA_FILE[NODE_RP], CFS_READ);
    if (fd == -1) {
      fd = cfs_open(OTA_FILE[NODE_RP], CFS_WRITE);
      if (fd != -1) {
        cfs_reserv(fd, RP_IMG_SIZE);
        cfs_close(fd);
      }
    } else {
      cfs_close(fd);
    }

    fd = cfs_open(OTA_FILE[NODE_VD], CFS_READ);
    if (fd == -1) {
      fd = cfs_open(OTA_FILE[NODE_VD], CFS_WRITE);
      if (fd != -1) {
        cfs_reserv(fd, VD_IMG_SIZE);
        cfs_close(fd);
      }
    } else {
      cfs_close(fd);
    }
  }
#endif /* WITH_OTA */
}

/*---------------------------------------------------------------------------*/
/**
 * \brief  通过GPRS发送消息
 *
 * \param msg 待发送消息的缓冲区指针(明文)
 *
 * \retval 0 发送成功
 */
static int
app_gprs_send(struct app_msg *msg)
{
  uint8_t buf[APPMSG_AES_MAXLEN];
  uint8_t len = 0, fill;
  uint8_t i, j;
  uint16_t crc;
  uint8_t crcH, crcL;
  int r = 0;

  // 复制待发送消息数据到临时数组以便进行数据填充
  i = 0;
  memcpy(buf, msg->header.devno, DEVNO_LEN); i += DEVNO_LEN;
  buf[i++] = msg->header.cmdop + (msg->header.rep << 6) + (msg->header.dir << 7);
  memcpy(buf + i, msg->data, msg->header.len);
  len = msg->header.len;
  PRINTF("app gprs op 0x%02X, len %d\n", msg->header.cmdop, msg->header.len);

  // 进行AES加密所需的数据填充，采用
  fill = (16 - (len & 0x0F));
  for (j = 0; j < fill; j++)
    buf[len + j] = fill;
  len += fill;
  //PRINTP("  app: (%2d)", len); PRINTD(buf, 0, len); PRINTP("\n");

  // 进行AES加密，加密结果在txaes中
  AES128_CBC_encrypt_buffer(txaes, buf, len, nib.aeskey, nib.aesiv);
  //PRINTP("  aes: (%2d)", len); PRINTD(txaes, 0, len); PRINTP("\n");

  // 进行base64编码，编码结果在txb64中
  len = base64_encode(txaes, len, txb64 + 2);

  // 构造完整WVDS数据帧
  i = 0;
  txb64[i++] = APPMSG_BEG;
  txb64[i++] = escape_len(len);
  i += len;
  crc = crc16_data(txb64 + 1, 1 + len, 0x0000);
  crcH = escape_crc(crc >> 8);
  crcL = escape_crc(crc & 0xff);
  txb64[i++] = crcH;
  txb64[i++] = crcL;
  txb64[i++] = APPMSG_END;
  len = i;
  //PRINTP("  b64: (%2d)", len); PRINTD(txb64, 0, len); PRINTP("\n");

  PRINTF("app gprs send msg 0x%02X, len %d\n", msg->header.cmdop, len);
  leds_toggle(LEDS_GREEN);
#if RDEBUG
  log_i(0, msg->header.mac, 7); // gprs send
#endif
  r = uart_send(txb64, len);
  if (r != M26_TX_OK) {
#if RWARN
    log_w(E_FAIL, &r, 1); // 串口发送失败
#endif
  }

  return r;
}

void
app_uart_sent(void)
{
}

/**
 * \brief    GPRS接收数据的回调处理函数
 *
 * \param data  接收到的数据的缓冲区指针
 * \param len   接收到的数据的长度
 *
 * \retval 0  已正常处理
 */
int
app_gprs_rcvd(uint8_t *data, uint16_t len)
{
  //struct msg_entry *me = NULL;
  struct app_msg *msg = NULL;
  uint16_t i;
  uint8_t crcH, crcL;

#define GPRS_RX_START 0
#define GPRS_RX_BEG   1
#define GPRS_RX_LEN   2
#define GPRS_RX_DATA  3
#define GPRS_RX_CRCH  4
#define GPRS_RX_CRCL  5
#define GPRS_RX_DONE  6
  PRINTF("app gprs rcvd\n");
  PRINTD(data,0,len);

  // 在输入字节序列中寻找并处理AT命令帧
  if (len >= 2 && data[0] == 'A' && data[1] == 'T') {
    for (i = 0; i < len; i++) {
      atcmd_input(data[i]);
    }
  }

  // 使用FSM在输入字节序列中寻找WVDS数据帧并处理
  for (i = 0; i < len; i++) {
    if (gprs_s == GPRS_RX_START) {
      if (data[i] == 0xAA) {
        gprs_s = GPRS_RX_BEG;
        gprs_i = 0;
        gprs_ccrc = 0x0000;
        rxb64[gprs_i++] = data[i];
      }
    }
    else if (gprs_s == GPRS_RX_BEG) {
      gprs_n = gprs_i + 1 + unescape_len(data[i]);
      rxb64[gprs_i++] = data[i];
      gprs_ccrc = crc16_add(data[i], gprs_ccrc);
      gprs_s = GPRS_RX_LEN;
    }
    else if (gprs_s == GPRS_RX_LEN) {
      rxb64[gprs_i++] = data[i];
      gprs_ccrc = crc16_add(data[i], gprs_ccrc);
      if (gprs_i == gprs_n)
        gprs_s = GPRS_RX_DATA;
    }
    else if (gprs_s == GPRS_RX_DATA) {
      rxb64[gprs_i++] = data[i];
      gprs_rcrc = data[i];
      gprs_s = GPRS_RX_CRCH;
    }
    else if (gprs_s == GPRS_RX_CRCH) {
      rxb64[gprs_i++] = data[i];
      gprs_rcrc = (gprs_rcrc << 8) + data[i];
      gprs_s = GPRS_RX_CRCL;
    }
    else if (gprs_s == GPRS_RX_CRCL) {
      if (data[i] == 0xFF) {
        rxb64[gprs_i++] = data[i];
        gprs_s = GPRS_RX_DONE;

        // 转义计算所得的CRC并与接收到的CRC进行比较
        crcH = escape_crc(gprs_ccrc >> 8);
        crcL = escape_crc(gprs_ccrc & 0xff);
        gprs_ccrc = (crcH << 8) + crcL;
        if (gprs_ccrc != gprs_rcrc) {
          PRINTF("warn app gprs crcx 0x%04X!=0x%04X\n", gprs_ccrc, gprs_rcrc);
          gprs_s = GPRS_RX_START; // 重置FSM以寻找新数据包
#if RWARN
          uint8_t arg[4];
          arg[0] = crcH;
          arg[1] = crcL;
          arg[2] = (gprs_rcrc >> 8);
          arg[3] = (gprs_rcrc & 0xff);
          log_w(E_CRCX, arg, 4); // 接收下行消息CRC出错
#endif
          continue; // CRC出错时忽略该数据包
        }

        // 已寻找到合法的WVDS数据帧
        len = gprs_i;
        PRINTP("  b64: (%2d)", len); PRINTD(rxb64, 0, len); PRINTP("\n");

        // 进行base64解码，结果在rxaes中
        memset(rxaes, 0, sizeof(rxaes));
        len = base64_decode(rxb64+2, len-5, rxaes);
        PRINTP("  aes: (%2d)", len); PRINTD(rxaes, 0, len); PRINTP("\n");

        // 进行AES解密，结果在rxbuf中
        memset(rxbuf, 0, sizeof(rxbuf));
        AES128_CBC_decrypt_buffer(rxbuf+2, rxaes, len, APP_AES_KEY, APP_AES_IV);
        PRINTP("  dec: (%2d)", len); PRINTD(rxbuf+2, 0, len); PRINTP("\n");
        len -= rxbuf[len+1]; // 排除尾部填充字节
        rxbuf[0] = APPMSG_BEG;
        rxbuf[1] = len;
        len += 2;
        PRINTP("  app: (%2d)", len); PRINTD(rxbuf, 0, len); PRINTP("\n");

#if GPRS_LED
        leds_toggle(GPRS_LED_RX);
#endif
        leds_toggle(LEDS_ORANGE);
        msg = (struct app_msg *)rxbuf;
        if (msg->header.cmdop == APPDATA_OTA_DATA || msg->header.cmdop == APPDATA_OTA_EXEC) {
          app_handle_otamsg(msg);
        }
        else {
          app_handle_gprs(rxbuf, len);
        }

        gprs_s = GPRS_RX_START;
      }
      else {
        gprs_s = GPRS_RX_START;
      }
    }
  }

  return 0;
}

/*---------------------------------------------------------------------------*/
int
netcmd_rcvd(rimeaddr_t *from, uint8_t msgid, uint8_t *data, uint8_t len)
{
  return 0;
}

struct netcmd_callback netc_cb = {netcmd_rcvd};
/*---------------------------------------------------------------------------*/
/**
 * \brief  查询设备编码的AT指令回调函数
 *
 * \param arg  AT指令参数的缓冲区指针
 * \param len  AT指令参数的长度
 *
 * \retval 0  已正常查询到设备编码
 */
int
app_get_devno(const char *arg, int len)
{
  char out[(DEVNO_LEN << 1) + 2];
  int i, j;

#define CHR(x) ((0<=(x) && (x)<=9) ? ((x)+'0') : ((x)-10+'A'))
  for (i = 0, j = 0; j < DEVNO_LEN; j++) {
    out[i++] = CHR(nib.devno[j] >> 4);
    out[i++] = CHR(nib.devno[j] & 0x0f);
  }
  out[i++] = '\r';
  out[i++] = '\n';
  atcmd_output(out, sizeof(out)); // 串口输出设备编码

  return -1;
}

/**
 * \brief  设置设备编码的AT指令回调函数
 *
 * \param arg  AT指令参数的缓冲区指针
 * \param len  AT指令参数的长度
 *
 * \retval 0  已正常设置设备编码
 *
 *         AT指令参数即为新的设备编码，是12字节的BCD编码字符串，例如'04CA16060001'。
 */
int
app_set_devno(const char *arg, int len)
{
  uint8_t *mac = nib.devno;
  int i, j;

#define INT(x) (('0'<=(x) && (x)<='9') ? ((x)-'0') : ((x)-'A'+10))
  nv_read(NV_NIB_ADDR, (uint8_t*)&nib, sizeof(struct NIB));
  for (i = 0, j = 0; i < DEVNO_LEN; i++, j += 2) {
    mac[i] = (INT(arg[j]) << 4) + INT(arg[j+1]);
  }
  nib.crc = crc16_data((uint8_t*)&nib, sizeof(struct NIB)-2, CRC_INIT);
  nv_write(NV_NIB_ADDR, (uint8_t*)&nib, sizeof(struct NIB)); // 写入新的设备编码

  return 0;
}

int
app_get_channel(const char *arg, int len)
{
  uint8_t chan = radio_get_channel();
  char out[4] = {0};
  uint8_t i = 0;

  if (chan >= 10)
    out[i++] = '0' + (chan / 10);
  out[i++] = '0' + (chan % 10);
  out[i++] = '\r';
  out[i++] = '\n';

  atcmd_output(out, i);
  return -1;
}

int
app_set_channel(const char *arg, int len)
{
  uint8_t chan = 0;
  uint8_t i;

  for (i = 0; i < len; i++) {
    if (('0' <= arg[i]) && (arg[i] <= '9')) {
      chan = (chan * 10) + (arg[i] - '0');
    } else {
      return 1;
    }
  }

  if ((chan < RFCHAN_MIN) || (chan > RFCHAN_MAX)) {
    return 2;
  }

  pib.radioChan = chan;
  pib.crc = crc16_data((uint8_t*)&pib, sizeof(struct PIB)-2, CRC_INIT);
  nv_write(NV_PIB_ADDR, (uint8_t *)&pib, sizeof(struct PIB));
  radio_set_channel(chan);
  return 0;
}

int
app_get_rfpower(const char *arg, int len)
{
  int8_t val = radio_get_txpower();
  char out[5] = {0};
  uint8_t i = 0;

  if (val < 0)
    out[i++] = '-';
  if (val >= 10) {
    out[i++] = '0' + (val / 10);
  } else if (val <= -10) {
    out[i++] = '0' + ((0-val) / 10);
  }
  if (val >= 0) {
    out[i++] = '0' + (val % 10);
  } else {
    out[i++] = '0' + ((0-val) % 10);
  }
  out[i++] = '\r';
  out[i++] = '\n';

  atcmd_output(out, i);
  return -1;
}

int
app_set_rfpower(const char *arg, int len)
{
  uint8_t i = 0;
  int8_t power = 0;
  int neg = 1;

  for (i = 0; i < len; i++) {
    if (arg[i] == '-') {
      neg = -1;
    } else if (('0' <= arg[i]) && (arg[i] <= '9')) {
      power = (power * 10) + (arg[i] - '0');
    } else {
      return 1;
    }
  }
  power *= neg;

  if ((power < RFPOWER_MIN) || (power > RFPOWER_MAX)) {
    return 2;
  }

  pib.radioPower = power;
  pib.crc = crc16_data((uint8_t*)&pib, sizeof(struct PIB)-2, CRC_INIT);
  nv_write(NV_PIB_ADDR, (uint8_t *)&pib, sizeof(struct PIB));
  radio_set_txpower(power);
  return 0;
}

/**
 * \brief  重启节点的AT指令的回调函数
 */
int
app_reboot(const char *arg, int len)
{
  atcmd_output("AT+OK\r\n", 7);
  watchdog_reboot();
  return 0;
}

int
app_update(const char *arg, int len)
{
  ReadFlash(FLASH_BOOT_ADDR, (uint8_t*)&boot, sizeof(struct BOOT));
  boot.update = 1;
  EraseFlash(FLASH_BOOT_ADDR);
  WriteFlash(FLASH_BOOT_ADDR, (uint8_t*)&boot, sizeof(struct BOOT));
  atcmd_output("AT+OK\r\n", 7);
  watchdog_reboot();
  return 0;
}

/**
 * \brief  恢复节点的出厂设置
 */
int
app_factory_reset(const char *arg, int len)
{
#if WITH_BLACKLIST
  struct blacklist blist;
#endif

  watchdog_periodic();

  // 重置OTA
  memset(&ota, 0x00, sizeof(struct OTA));
  nv_write(NV_OTA_ADDR, (uint8_t*)&ota, sizeof(struct OTA));

#if OTA_EXEC_NV
  memset(&ota_exec, 0x00, sizeof(ota_exec));
  nv_write(NV_STATS_ADDR, (uint8_t*)&ota_exec, sizeof(ota_exec));
#endif

  // 重置CFS
  cfs_factory_reset();

  // 重置NIB
  nv_read(NV_NIB_ADDR, (uint8_t*)&nib, sizeof(struct NIB));
  nib.magic = NV_MAGIC;
  nib.tricseq = 2;
  nib.locked = 0;
  nib.crc = crc16_data((uint8_t*)&nib, sizeof(struct NIB)-2, CRC_INIT);
  nv_write(NV_NIB_ADDR, (uint8_t*)&nib, sizeof(struct NIB));

  // 重置PIB
  memset(&pib, 0x00, sizeof(struct PIB));
  nv_write(NV_PIB_ADDR, (uint8_t*)&pib, sizeof(struct PIB));

#if OTA_NODES_NV
  // 重置更新节点列表
  memset(&nodes, 0x00, sizeof(struct NODES));
  nv_write(NV_NODES_ADDR, (uint8_t*)&nodes, sizeof(struct NODES));
#endif

  return 0;
}
/*---------------------------------------------------------------------------*/
void
app_log_reboot(void)
{
  uint8_t arg[25] = {0};
  uint8_t i = 0;
  memcpy(arg+i, node_mac, MAC_LEN); i += MAC_LEN;
  arg[i++] = nib.hwver;
  arg[i++] = nib.fwver;
  memcpy(arg+i, APP_COMMIT, 7); i += 7;
  memcpy(arg+i, APP_BUILD, 8); i += 8;
  log_i(I_REBOOT, arg, i); // 节点重启
}

static void
app_exit(void)
{
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(updater_process, ev, data)
{
  static struct etimer rt; // 运行定时器
  static uint8_t rtr = 0;

  PROCESS_EXITHANDLER(app_exit())
  PROCESS_BEGIN();

  app_init();
  ds3231_init();
  radio_set_channel(pib.radioChan); // 设置射频信道
  radio_set_txpower(pib.radioPower); // 设置射频功率
  cc11xx_set_promiscuous(1);

  rime_sniffer_add(&app_sniff);

  neighbor_init();

  atcmd_start(); // 启动AT指令模块
  atcmd_set_output(uart_writeb); // 设置AT模块反馈从串口输出
  atcmd_register("MAC?", app_get_devno); // 注册查询设备编码的回调函数
  atcmd_register("MAC=", app_set_devno); // 注册设置设备编码的回调函数
  atcmd_register("CHAN?", app_get_channel); // 注册查询射频信道的回调函数
  atcmd_register("CHAN=", app_set_channel); // 注册设置射频信道的回调函数
  atcmd_register("POWER?", app_get_rfpower); // 注册查询射频功率的回调函数
  atcmd_register("POWER=", app_set_rfpower); // 注册设置射频功率的回调函数
  atcmd_register("FRESET", app_factory_reset); // 注册恢复出厂设置的回调函数
  atcmd_register("REBOOT", app_reboot); // 注册重启节点的回调函数
  atcmd_register("UPDATE", app_update); // 注册进入串口升级模式的回调函数
  atcmd_output("OTA started\r\n", 13);

  enterS(S_START);
  PRINTF("app start\n");

  // 打开与网内节点的相关通信通道
  broadcast_open(&mesh, APP_MESH_CHANNEL, &mesh_cb);
  netcmd_open(0, pib.commandT, &netc_cb);
  unicast_open(&unic, APP_UNICAST_CHANNEL, &unic_cb);
  uart_set_frame_input(app_gprs_rcvd);
  uart_set_sent_callback(app_uart_sent);

  etimer_set(&rt, CLOCK_SECOND); rtr = 1;

  logger_start();
  app_log_reboot(); // 节点重启
  app_show_reboot();
  leds_off(LEDS_ALL);

  unixtime_get(tsbuf);
  app_send_radio_ack(APP_GET, APP_OK, tsbuf);

#if OTA_EXEC_NV
  // 自动继续重启前未完成的更新过程
  if ((ota_exec.pend != OTA_PEND_NOT) || (ota_exec.autop == 1)) {
    app_report_progress(ota_exec.dest, ota_exec.ver, 0xffff);
    deluge_start(NULL);
    deluge_disseminate(ota_exec.objid, (char*)OTA_FILE[ota_exec.role], ota_exec.ver, ota_exec.mode, ota_exec.dest);
  }
#endif

  while(1) {
    PROCESS_WAIT_EVENT();

    // 运行定时器到时
    if (rtr && etimer_expired(&rt)) {
      etimer_reset(&rt);
      leds_toggle(LEDS_RED);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
