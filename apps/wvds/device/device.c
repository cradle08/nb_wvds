#include "contiki.h"
#include "node-id.h"
#include "lib/crc16.h"
#include "net/netstack.h"
#include "net/rime.h"
#include "net/rime/mesh.h"
#include "net/rime/trickle.h"
#include "net/rime/unicast.h"
#include "net/rime/rimeaddr.h"
#include "net/netcmd.h"
#include "lib/random.h"
#include "dev/radio.h"
#include "dev/uart.h"
#include "dev/watchdog.h"
#include "dev/battery-sensor.h"
#include "dev/leds.h"
#include "sys/unixtime.h"
#include "sys/taskmon.h"
#include "sys/logger.h"
#include "m25pe.h"
#include "cfs/cfs.h"
#include "cfs-flash.h"
#include "deluge.h"
#include "at-cmd.h"
#include "app.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifndef APP_VER_MAJOR
#define APP_VER_MAJOR 1
#endif
#ifndef APP_VER_MINOR
#define APP_VER_MINOR 4
#endif
#ifndef APP_VER_PATCH
#define APP_VER_PATCH 1
#endif

#define SUBWAY 0
#if SUBWAY==1
#include "xyzfuedetection.h"
#else
#include "VehicleDetection.h"
#endif

#define DEBUG 0
#if DEBUG
#define PRINTF(...)  printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

// 如果要在运行时诊断重要消息是否存储和再发送，定义为1
#define DEBUG_MSG_SAVE 0

// 是否支持通过串口AT指令设置节点参数，定义为1时支持
#define WITH_ATCMD 0

// 是否只在READY状态下发送心跳，定义为1时是
#define HBEAT_READY 0

//#if WITH_MMC3316
//#warning "using sensor MMC3316"
//#elif WITH_HMC5983
//#warning "using sensor HMC5983"
//#elif WITH_QMC5883
//#warning "using sensor QMC5883"
//#else
//#error "no support"
//#endif

//#if WITH_LPM
//#warning "with Low Power Mode support"
//#else
//#warning "without Low Power Mode support"
//#endif
/*---------------------------------------------------------------------------*/
enum {
  S_IDLE = 0,
  S_SEND = 1,
  S_WAIT = 2
};

uint8_t enter_leave_branch=0; //标记车辆驶入或驶离的分支，调试用
/*
*case 0: CRB7~5=000, 1370 gain, 0x00, 0.73mg/LSB
  *case 1: CRB7~5=001, 1090 gain, 0x20, 0.92mg/LSB
  *case 2: CRB7~5=010, 820 gain, 0x40, 1.22mg/LSB
  *case 3: CRB7~5=011, 660 gain, 0x60, 1.52mg/LSB
  *case 4: CRB7~5=100, 440 gain, 0x80, 2.27mg/LSB (milli-Gauss per count, LSB)
  *case 5: CRB7~5=101, 390 gain, 0xA0, 2.56mg/LSB
  *case 6: CRB7~5=110, 330 gain, 0xC0, 2.56mg/LSB
  *case 7: CRB7~5=111, 230 gain, 0xE0, 4.35mg/LSB
*/
//uint8_t extern_gain_hmc5983 = 1; //外部变量
uint16_t digital_resolution = 100; //外部变量，与algo.gain_hmc5983相关，默认值为100
uint8_t hist_gain=0;
bool reset_flag=false;

const rimeaddr_t act_addr = {{2,1}}; // 激活节点的rime短地址
static uint8_t act_conned = 0; // 是否与激活节点已连接

static uint8_t app_s = S_START; // 主线程的状态
static uint8_t prev_s = S_START;
static uint8_t app_send_s = S_IDLE; // 发送状态
static uint8_t park_s = 2; // 检测算法状态，初始化为待激活

static uint8_t app_time_synced = 0;
static uint8_t app_req_reboot = 0; // 重启标记
static struct ctimer app_reboot_ct;

static uint8_t reconn_n = 0; // 入网重试计数
const uint16_t reconn_ints[] = {8,8,15,15,30,30,60,60,120,120,300,600,1800}; // 入网重试间隔，以秒为单位
const uint8_t reconn_num = sizeof(reconn_ints) / sizeof(uint16_t);
#define RECONN_INTERVAL() \
  ((reconn_n < reconn_num) ? reconn_ints[reconn_n] : reconn_ints[reconn_num-1])

const uint16_t resend_ints[] = {0,8,8,15,15,30,60,120,300,600,1800}; // 重发间隔，以秒为单位
const uint8_t resend_num = sizeof(resend_ints) / sizeof(uint16_t);
static struct ctimer resend_ct;
#define RESEND_INTERVAL() \
  ((app_noack_n < resend_num) ? resend_ints[app_noack_n] : resend_ints[resend_num-1])

static process_event_t pib_changed_event; // 全网参数改变事件
static process_event_t addr_changed_event; // 短地址改变事件
static process_event_t data_ready_event; // 传感器数据就绪事件

#define APP_MAX_REXMIT 1
static uint8_t app_hbeat_seq = 0; // 心跳序号
static uint8_t app_parkevt_seq = 0; // 泊位事件序号
static uint8_t app_magdata_seq = 0; // 波动磁场数据序号
static uint8_t app_noack_n = 0; // 未收到ACK计数

static struct app_msg txmsg;
static uint8_t txbuf[APPMSG_MAXLEN]; // 发送消息缓冲区
static uint8_t txlen = 0; // 发送消息长度

static struct ctimer app_act_ct;
static struct ctimer app_radio_ct;
static uint8_t app_keep_radio_on = 0;
static uint8_t app_new_chan = RF_CHANNEL;
static uint8_t app_in_activate = 0;

#define APP_HBEAT_MAX_RETRY 1
static struct ctimer app_ack_ct; // ACK等待定时器
static struct ctimer app_hb_ct; // 心跳重传定时器
static uint8_t app_hb_retry = 0; // 心跳剩余发送次数

static uint16_t sampleT = CLOCK_SECOND; // 磁场采样周期

static struct BOOT boot; // 启动信息
struct OTA  ota;  // 无线升级信息
static struct NIB  nib;  // 节点特定信息
static struct PIB  pib;  // 全网参数
static struct ALGO algo; // 算法参数
static struct MSGS msgs; // 已存储消息
static struct PIB rcvpib; // 新接收的全网参数

static struct mesh_conn mesh; // mesh连接
static struct trickle_conn tric; // trickle连接
static struct unicast_conn unic;
static uint8_t unic_opened = 0;
static uint8_t fae_req = 0;

struct taskmon *act = NULL;
struct taskmon *send = NULL;
struct taskmon *join = NULL;
struct taskmon *hbeat = NULL;
struct taskmon *sample = NULL;
struct taskmon *sensor = NULL;

struct radio_res *app_fae_rs  = NULL;
struct radio_res *app_mesh_rs = NULL;
struct radio_res *app_tric_rs = NULL;

static uint8_t req_ts[TSTAMP_LEN];

enum {
  ENQ_HEAD = 0, // 加为队列头
  ENQ_TAIL = 1, // 加为队列尾
  ENQ_PRIO = 2  // 根据优先级入队
};

// 发送消息队列
#define APP_MSGMEM_LEN  6
struct msg_entry {
  struct msg_entry *next;
  struct app_msg  msg;   // 消息数据
  rimeaddr_t      to;    // 目的节点
  uint8_t         cmdop; // 消息命令字
  uint8_t         len;   // 消息长度
  uint8_t         retry; // 最大发送次数
  uint8_t         ack;   // 是否要等待ACK
  uint8_t         prio;  // 消息优先级
  int             index; // 消息在Flash中的存储索引，未存储为-1
  uint8_t         sync;  // 时间是否已同步, 0:未同步, 1:已同步
};
MEMB(app_msg_mem, struct msg_entry, APP_MSGMEM_LEN);
LIST(app_send_queue);

static uint8_t app_sendq_n = 0; // 在队列中的消息个数
static struct msg_entry *app_tx = NULL; // 当前发送的消息项

// RSSI平滑队列
#define APP_RSSI_QLEN 4
struct {
  uint8_t head;
  uint8_t tail;
  int16_t rssi_sum;
  int16_t lqi_sum;
  uint8_t count;
   int8_t rssi[APP_RSSI_QLEN + 1];
   int8_t lqi[APP_RSSI_QLEN + 1];
} app_rssi_q = {0};

/*---------------------------------------------------------------------------*/
static void app_enq_msg(uint8_t flag, const rimeaddr_t *to, uint8_t cmdop, uint8_t *payload, uint8_t len, uint8_t rep, uint8_t ack, uint8_t try, int idx, int sync);
static void app_do_send(void *ptr);
static void app_send_next(void *ptr);
static void app_end_send(uint8_t res);

static int app_load_msg(uint8_t *cmdop, uint8_t *buf, uint8_t *len, struct msgmeta *meta);
static int app_save_msg(uint8_t cmdop, uint8_t *data, uint8_t len);
static int app_erase_msg(uint16_t idx);
static int app_send_saved_msg(void);

static int app_get_batV(void);

void app_handle_msg(rimeaddr_t *from, struct app_msg *msg);
int app_factory_reset(const char *arg, int len);

void app_send_activate_req(void);
void app_no_command(void *ptr);
void app_fail_activate(void *ptr);

void app_magdata_ready(unsigned char *data, unsigned char *temp);

static void app_log_reboot(void);
//static void app_log_save(uint16_t idx, uint8_t cmdop, uint8_t *ts);

extern int cfs_reserv(int fd, uint32_t size);
/*---------------------------------------------------------------------------*/
PROCESS(device_process, "WVDS Device");
AUTOSTART_PROCESSES(&device_process);
/*---------------------------------------------------------------------------*/
static void
enterS(uint8_t s)
{
  PRINTF("app s%d->s%d\n", app_s, s);
  prev_s = app_s;
  app_s = s;
}

/**
 * \brief  根据输入时间戳设置本地时间
 *
 * \param ts  输入时间戳，为6字节BCD编码值，如0x16 0x01 0x01 0x00 0x00 0x00
 */
static void
app_set_time(uint8_t *ts)
{
  uint32_t now;
  uint32_t newt;
  int32_t  diff;
  uint16_t ptr;
  uint32_t addr;
  struct msgmeta meta;

  now = unixtime_now();
  newt = unixtime_sec(ts);
  PRINTF("app set time now %lu, new %lu/0x%02X%02X%02X%02X%02X%02X\n", now, newt, ts[0],ts[1],ts[2],ts[3],ts[4],ts[5]);
  if ((newt - now > 1) || (now - newt > 1)) { // 与本地时间存在差距
    // 设置节点的本地时间
    unixtime_set(ts);
    //log_i(I_UPDATE_LOCAL_TIME, ts, 6); // 更新节点的本地时间

    // 修正已存储消息的时间
    if (!app_time_synced) {
      diff = newt - now;
      if (msgs.count > 0) {
        ptr = msgs.readptr;
        while (ptr != msgs.writeptr) {
          watchdog_periodic();
          addr = NV_MSGS_ADDR + sizeof(struct MSGS) + (NV_MSGS_MAXLEN * ptr);
          nv_read(addr, (uint8_t*)&meta, sizeof(struct msgmeta));
          if (!meta.synced) {
            PRINTF("app fix ts %lu+%ld at 0x%06X\n", meta.ts, diff, addr);
            meta.ts += diff;
            meta.synced = 1;
            meta.fixed = 1;
            meta.mcrc = crc16_data((uint8_t*)&meta, sizeof(struct msgmeta)-2, CRC_INIT);
            nv_write(addr, (uint8_t*)&meta, sizeof(struct msgmeta));
          }
          if (++ptr >= NV_MSGS_MAXNUM) ptr = 0;
        }
      }
      app_time_synced = 1;
    }
  }
}
/*---------------------------------------------------------------------------*/
/**
 * \brief  当前发送消息未收到ACK的回调函数
 *
 * \param ptr  ctimer用回调指针
 */
void
app_msg_noack(void *ptr)
{
  if (app_send_s == S_WAIT) {
    PRINTF("warn app msg 0x%02X noack\n", app_tx->cmdop);
    //log_w(W_MSG_NO_ACK, NULL, 0); // 发送消息未收到ACK
    ++app_noack_n; // 增加无ACK计数
    app_end_send(APP_TX_NOACK); // 以无ACK结束当前发送
  }
}

/**
 * \brief  当前发送消息接收到ACK的处理函数
 */
void
app_msg_acked(void)
{
  if (app_send_s == S_WAIT || app_send_s == S_SEND) {
    PRINTF("app msg 0x%02X acked\n", app_tx->cmdop);
    app_noack_n = 0; // 清零无ACK计数
#if MESH_BROADCAST
    mesh_cancel(&mesh);
#endif
    ctimer_stop(&app_ack_ct); // 停止ACK超时
    app_end_send(APP_TX_OK); // 以发送正常结束当前发送
  }
}

/**
 * \brief  mesh发送成功的回调函数
 *
 * \param c  对应mesh连接的指针
 */
static void
app_mesh_sent(struct mesh_conn *c)
{
  PRINTF("app sent ok at s%d,%d\n", app_s, app_send_s);

  if (app_send_s == S_SEND) {
    if (app_tx->ack) { // 需要等待ACK
      app_send_s = S_WAIT; // 标记进行等待ACK状态
      ctimer_set(&app_ack_ct, APP_ACK_WAIT, app_msg_noack, NULL); // 启动ACK等待超时
    } else { // 无需等待ACK
      app_end_send(APP_TX_OK); // 以发送成功结束当前发送
    }
  }
}

/**
 * \brief  mesh发送失败的回调函数
 *
 * \param c  对应mesh连接的指针
 */
static void
app_mesh_timedout(struct mesh_conn *c)
{
  PRINTF("app sent fail at s%d,%d\n", app_s, app_send_s);

  if (app_send_s == S_SEND) {
    ++app_noack_n;
    app_end_send(APP_TX_FAIL); // 以发送失败结束当前发送
  }
}

static void
app_enq_rssi(int8_t rssi, int8_t lqi)
{
  uint8_t i = app_rssi_q.tail;

  app_rssi_q.rssi[i] = rssi;
  app_rssi_q.lqi[i] = lqi;
  app_rssi_q.rssi_sum += rssi;
  app_rssi_q.lqi_sum += lqi;
  app_rssi_q.count += 1;
  if (++app_rssi_q.tail > APP_RSSI_QLEN)
    app_rssi_q.tail = 0;

  if (app_rssi_q.tail == app_rssi_q.head) {
    i = app_rssi_q.head;
    app_rssi_q.rssi_sum -= app_rssi_q.rssi[i];
    app_rssi_q.lqi_sum -= app_rssi_q.lqi[i];
    app_rssi_q.count -= 1;
    if (++app_rssi_q.head > APP_RSSI_QLEN)
      app_rssi_q.head = 0;
  }
}

/**
 * \brief  处理mesh接收消息的回调函数
 *
 * \param c  对应的mesh连接的指针
 * \param from  消息的源节点地址
 * \param hops  消息从源节点到本节点经过的跳数
 */
static void
app_mesh_recv(struct mesh_conn *c, const rimeaddr_t *from, uint8_t hops)
{
  const rimeaddr_t *sender = packetbuf_addr(PACKETBUF_ADDR_SENDER);
  int8_t rssi = (int8_t)packetbuf_attr(PACKETBUF_ATTR_RSSI);
  int8_t lqi = (int8_t)packetbuf_attr(PACKETBUF_ATTR_LINK_QUALITY);
  struct app_msg *msg = (struct app_msg *)packetbuf_dataptr();

  PRINTF("app rcvd orig %d.%d from %d.%d: msg 0x%02X, len %d, s %d\n",
      from->u8[0], from->u8[1], sender->u8[0], sender->u8[1], msg->header.cmdop, msg->header.len, app_s);

  // 将当前消息的RSSI和LQI加入平滑队列
  app_enq_rssi(rssi, lqi);

#if MESH_BROADCAST
  route_add(from, sender, hops, 0);
#endif

  if (app_s == S_JOIN) {
    // 当前是入网状态，处理相关消息
    if (msg->header.cmdop == APPDATA_VD_CONN) {
      struct app_data_vdconnack *ack = (struct app_data_vdconnack *)msg->data;

      PRINTF("app rcvd conn ack, res %d, dest %02X%02X%02X%02X%02X%02X%02X%02X\n",
          ack->res, ack->argd[0], ack->argd[1], ack->argd[2], ack->argd[3],
          ack->argd[4], ack->argd[5], ack->argd[6], ack->argd[7]);
      app_msg_acked();

      if (ack->res == APP_ACCEPT) { // 收到同意入网消息
        if (memcmp(ack->argd, node_mac, 8) == 0) {
          enterS(S_READY); // 标记进入已入网状态
          app_set_time(ack->tstamp); // 根据同意入网消息的时间戳更新本地时间
          process_poll(&device_process);
          log_i(I_NETWORK_JOINED, NULL, 0);
        }
      }
      else if (ack->res == APP_REJECT) { // 收到拒绝入网消息
        if (memcmp(ack->argd, node_mac, 8) == 0) {
          PRINTF("app rejected\n");
          log_w(W_JOIN_REJECTED, NULL, 0); // 被拒绝入网
          //enterS(S_SLEEP); // 进入休眠状态
          //process_poll(&device_process);
        }
      }
      else if (ack->res == APP_ASSIGN) { // 收到短地址分配消息
        if (memcmp(ack->argd, node_mac, 8) == 0) {
          rimeaddr_t addr;

          // 更新使用新的短地址
          addr.u8[0] = ack->argd[8];
          addr.u8[1] = ack->argd[9];
          node_id = addr.u8[0] + (addr.u8[1]<<8);
          rimeaddr_set_node_addr(&addr);
          PRINTF("app assigned %d.%d\n", addr.u8[0], addr.u8[1]);

          // 将新的短地址写入NIB，重启后使用新短地址
          nib.addr = node_id;
          nib.crc = crc16_data((uint8_t*)&nib, sizeof(struct NIB)-2, CRC_INIT);
          nv_write(NV_NIB_ADDR, (uint8_t*)&nib, sizeof(struct NIB));

          process_post(&device_process, addr_changed_event, NULL);
        }
      }
    }
  }
  else if (app_s == S_READY) {
    if (app_send_s == S_WAIT || app_send_s == S_SEND) {
      if (msg->header.cmdop == APPDATA_VD_CONN) {
        struct app_data_vdconnack *ack = (struct app_data_vdconnack *)msg->data;
        if (ack->res == APP_RECONN) {
          PRINTF("app rejoin at request\n");
          enterS(S_JOIN); // 重新入网
          process_poll(&device_process);

          ctimer_stop(&app_ack_ct);
          ctimer_stop(&app_hb_ct);
          app_end_send(APP_TX_NOACK);
        }
      }
      else {
        if (msg->header.cmdop == APPDATA_VD_HBEAT) {
          if (app_tx->cmdop == APPDATA_VD_HBEAT) {
            struct app_data_vdhback *ack = (struct app_data_vdhback *)msg->data;

            PRINTF("app hbeat acked\n");
            app_set_time(ack->tstamp); // 根据心跳ACK的时间戳更新本地时间
            ctimer_stop(&app_hb_ct); // 停止心跳重传超时
          }
        }

        if (app_tx != NULL) {
          if (msg->header.cmdop == app_tx->cmdop) {
            app_msg_acked(); // 处理消息接收到ACK
          } else {
            uint8_t arg[2];
            arg[0] = app_tx->cmdop;
            arg[1] = msg->header.cmdop;
            PRINTF("app got 0x%02X when wait 0x%02X\n", arg[1],arg[0]);
            log_w(W_ACK_NOT_MATCH, arg, 2); // 收到的ACK不匹配当前发送消息
          }
        }
      }
    }
  }
  else {
    uint8_t arg[2];
    arg[0] = app_s;
    arg[1] = msg->header.cmdop;
    log_w(W_RECV_AT_WRONG_STATE, arg, 2); // 在错误状态接收到消息
  }
}

const static struct mesh_callbacks mesh_cb = {app_mesh_recv, app_mesh_sent, app_mesh_timedout};
/*---------------------------------------------------------------------------*/
/**
 * \brief  接收到全网参数消息的处理函数
 *
 * \param c 对应trickle连接的指针
 */
static void
app_tric_recv(struct trickle_conn *c)
{
  struct PIB *rcv = (struct PIB *)packetbuf_dataptr();

  if ((rcv->vdHbeatT < VDHBEATT_MIN) || (rcv->healthT < HEALTHT_MIN)
      || (rcv->commandT < COMMANDT_MIN) || (rcv->paramT < PARAMT_MIN)) {
    log_w(W_INVALID_PIB, NULL, 0); // 非法的全网参数消息
    return; // 如果新参数的一些时间值过小，忽略该参数
  }

  PRINTF("app tric rcvd\n"); log_i(I_NEW_PIB, NULL, 0); // 接收到新的全网参数
  memcpy(&rcvpib, rcv, sizeof(struct PIB)); // 新参数保存到缓存变量

  process_post(&device_process, pib_changed_event, NULL); // 提交事件让主线程处理参数变化
}

const static struct trickle_callbacks tric_cb = {app_tric_recv};
/*---------------------------------------------------------------------------*/
static void
app_unic_rcvd(struct unicast_conn *c, const rimeaddr_t *from)
{
  PRINTF("app unic rcvd\n");
}

static void
app_unic_sent(struct unicast_conn *ptr, int status, int num_tx)
{
  if (app_req_reboot) {
    app_req_reboot = 0;
    watchdog_reboot();
  }

  if (!app_keep_radio_on) {
    unicast_close(&unic); unic_opened = 0;

    if (app_s == S_PREACT) {
      app_in_activate = 2;
      if (nib.activated) {
        // 关闭激活通道
        netcmd_close();

        // 将NIB激活标志写入Flash
        nib.crc = crc16_data((uint8_t*)&nib, sizeof(struct NIB)-2, CRC_INIT);
        nv_write(NV_NIB_ADDR, (uint8_t*)&nib, sizeof(struct NIB));

        // 更改信道并关闭射频
        if (app_new_chan != pib.radioChan) {
          pib.radioChan = app_new_chan;
          pib.crc = crc16_data((uint8_t*)&pib, sizeof(struct PIB)-2, CRC_INIT);
          nv_write(NV_PIB_ADDR, (uint8_t*)&pib, sizeof(struct PIB));

          radio_set_channel(app_new_chan);
        }
        log_i(I_VD_RADIO_OFF, NULL, 0); radio_off(app_fae_rs); // 关闭射频case2: 激活完成

        // 标记进入已激活状态，让主线程开始相关操作
        enterS(S_ACTED);
        process_poll(&device_process);
        ctimer_stop(&app_act_ct);
        task_end(act); // 激活成功
      }
      else {
        enterS(S_START);
        process_poll(&device_process);
      }
    }
    else if (app_s == S_READY || app_s == S_JOIN) {
      log_s(LOG_INFO, I_VD_RADIO_OFF, NULL, 0); // 维护完成关闭射频

      // 更改信道并关闭射频
      if (app_new_chan != pib.radioChan) {
        pib.radioChan = app_new_chan;
        pib.crc = crc16_data((uint8_t*)&pib, sizeof(struct PIB)-2, CRC_INIT);
        nv_write(NV_PIB_ADDR, (uint8_t*)&pib, sizeof(struct PIB));

        radio_set_channel(app_new_chan);
      }

      radio_off(app_fae_rs); // 关闭射频case3: 维护完成
      task_end(act); // 维护成功
    }
    else {
      log_w(E_STATE, &app_s, 1);
    }
  }
}

const static struct unicast_callbacks unic_cb = {app_unic_rcvd,app_unic_sent};
/*---------------------------------------------------------------------------*/
/**
 * \brief  获取消息的优先级
 *
 * \param msg 待获取优先级的消息指针
 *
 * \retval prio 消息的优先级
 */
static uint8_t
app_msg_prio(struct app_msg *msg)
{
  uint8_t prio = 0;

  switch(msg->header.cmdop) {
    case APPDATA_VD_CONN:
      prio = 4; break;
    case APPDATA_PARK_EVT:
      prio = 3; break;
    case APPDATA_MAG_DATA:
      prio = 2; break;
    case APPDATA_ALARM:
      prio = 1; break;
    default:
      prio = 0; break;
  }

  return prio;
}

/**
 * \brief  向AP发送消息的公用函数
 *
 * \param to 消息的目的节点地址
 * \param cmdop 消息的命令字
 * \param payload 消息的有效载荷指针
 * \param len 消息的有效载荷长度
 * \param rep 是否需要服务器响应
 * \param ack 是否需要AP的确认
 * \param try 最大发送次数
 * \param idx 消息在Flash中的存储索引
 */
static void
app_send(const rimeaddr_t *to, uint8_t cmdop, uint8_t *payload, uint8_t len, uint8_t rep, uint8_t ack, uint8_t try, int idx, int sync)
{
  app_enq_msg(ENQ_PRIO, &rimeaddr_root, cmdop, payload, len, rep, ack, try, idx, sync);
  app_send_next(NULL);
}

/**
 * \brief  根据发送队列执行发送
 *
 * \param ptr ctimer用回调指针
 */
static void
app_do_send(void *ptr)
{
  struct msg_entry *me = NULL;
#if WITH_TSTAMP
  uint8_t *buf = NULL;
  uint32_t now = 0;
  uint8_t i = 0;
#endif

  if (app_send_s != S_IDLE) {
    return; // 将在app_mesh_sent()中重试
  }

  if (list_length(app_send_queue) == 0) {
    return; // 队列中没有消息，不发送
  }

  radio_on(app_mesh_rs, MESH_PKT_TIMEOUT); // 开启射频case1: 发送上行消息

  me = list_head(app_send_queue); // 发送队列头的消息
#if WITH_TSTAMP
  now = unixtime_now();
  packetbuf_clear();
  buf = packetbuf_dataptr(); i = 0;
  memcpy(buf, (uint8_t*)&me->msg, me->len); i += me->len;
  memcpy(buf+i, (uint8_t*)&now, sizeof(uint32_t)); i += sizeof(uint32_t); // 附加4字节unix时间
  buf[i] = me->sync; i++; // 附加时间是否已同步标志
  packetbuf_set_datalen(i);
#else
  packetbuf_copyfrom(&me->msg, me->len);
#endif

  PRINTF("app send msg 0x%02X,%d to %d.%d\n", me->cmdop, me->len, me->to.u8[0], me->to.u8[1]);
  app_tx = me;
  app_send_s = S_SEND;
  task_begin(send, (MESH_PKT_TIMEOUT + (CLOCK_SECOND >> 4)));
  mesh_send(&mesh, &me->to); // 调用mesh发送数据
}

/**
 * \brief  结束当前发送
 *
 * \param res 发送结果
 */
static void
app_end_send(uint8_t res)
{
  struct msg_entry *me = NULL;
  //struct msgmeta meta;
  //uint32_t addr;

  PRINTF("app end send %d\n", res);
  radio_off(app_mesh_rs); // 关闭射频case1: 上行发送完成

  me = list_head(app_send_queue);
  if (res == APP_TX_OK) { // 发送成功
    // 如果是已存储消息，擦除该消息
    if (me->index != -1) {
      app_erase_msg(me->index);
    }

    // 从发送队列中删除，释放消息项
    list_pop(app_send_queue); --app_sendq_n;
    memb_free(&app_msg_mem, me);
    PRINTF("app sendq deq at ok, %d\n", app_sendq_n);

  } else { // 发送失败
    if (--(me->retry) == 0) { // 已达到最大重传次数
      if (me->index == -1) {
        // 从发送队列中删除，释放消息项
        list_pop(app_send_queue); --app_sendq_n;
        memb_free(&app_msg_mem, me);
        PRINTF("app sendq deq at fail, %d\n", app_sendq_n);
      }
      else {
        // 存储重要消息必须要发送成功
      }

      // 标记存储消息已发送
      //if (me->index != -1) {
      //  addr = NV_MSGS_ADDR + sizeof(struct MSGS) + (NV_MSGS_MAXLEN * me->index);
      //  nv_read(addr, (uint8_t*)&meta, sizeof(struct msgmeta));
      //  meta.sent = 1;
      //  meta.mcrc = crc16_data((uint8_t*)&meta, sizeof(struct msgmeta)-2, CRC_INIT);
      //  nv_write(addr, (uint8_t*)&meta, sizeof(struct msgmeta));
      //  log_i(0, NULL, 0);
      //}
    } else {
      // 稍后将重传
    }
  }

  // 重置相关状态变量
  app_tx = NULL;
  app_send_s = S_IDLE;
  task_end(send);

  // 如果标记了要重启，此处可能是ACK已发送
  if (app_req_reboot) {
    app_req_reboot = 0;
    watchdog_reboot(); // 节点重启
  }

  // 根据当前通信情况，计划下次发送的时刻
  ctimer_set(&resend_ct, ((uint32_t)CLOCK_SECOND * RESEND_INTERVAL()), app_send_next, NULL);
}

void
app_send_next(void *ptr)
{
  if (app_send_s == S_IDLE) {
    if (list_length(app_send_queue) > 0) {
      app_do_send(NULL); // 队列不空，发送下一个
    } else {
      if (msgs.count > 0) {
        app_send_saved_msg(); // 发送存储的消息
      }
    }
  }
}

void
app_unic_send(uint8_t cmdop, uint8_t *payload, uint8_t len, uint8_t rep)
{
  struct app_msg *msg = (struct app_msg *)&txmsg;
  struct app_msg_header *header = (struct app_msg_header *)msg;
  struct app_msg_footer *footer = (struct app_msg_footer *)(msg->data + len);
  uint16_t crc = 0;

  memset(msg, 0, sizeof(struct app_msg));
  header->beg = APPMSG_BEG;
  header->len = DEVNO_LEN + 1 + len;
  memcpy(header->devno, nib.devno, sizeof(nib.devno));
  header->dir = APPMSG_UP;
  header->rep = rep;
  header->cmdop = cmdop;
  memcpy(msg->data, payload, len);
  crc = crc16_data(&header->len, (APPMSG_HEADER_LEN - 1 + len), CRC_INIT);
  footer->crc[0] = (crc >> 8);
  footer->crc[1] = (crc & 0xff);
  footer->end = APPMSG_END;
  len += APPMSG_HEADER_LEN + APPMSG_FOOTER_LEN;

  packetbuf_copyfrom(&txmsg, len);
  unicast_send(&unic, &act_addr);
}

void
app_send_activate_req(void)
{
  struct app_data_vd_activate *req = (struct app_data_vd_activate *)txbuf;

  unixtime_get(req->tstamp);
  req->radio = 0;
  req->sensor = 0;
  req->flash = 0;
  req->batV = app_get_batV();
  req->batQ = 100;

  app_unic_send(APPDATA_VD_ACTIVATE, txbuf, sizeof(struct app_data_vd_activate), 1);
}

void
app_send_disconn_ack(struct app_data_vd_disconn *req)
{
  struct app_data_vd_disconn_resp *ack = (struct app_data_vd_disconn_resp *)txbuf;

  memcpy(ack->tstamp, req->tstamp, TSTAMP_LEN);
  ack->result = 0;

  app_unic_send(APPDATA_VD_DISCONN, txbuf, sizeof(struct app_data_vd_disconn_resp), 0);
}

void
app_send_disconn_req(void)
{
  struct app_data_vd_disconn_resp *ack = (struct app_data_vd_disconn_resp *)txbuf;

  unixtime_get(ack->tstamp);
  ack->result = 1;

  app_unic_send(APPDATA_VD_DISCONN, txbuf, sizeof(struct app_data_vd_disconn_resp), 0);
}

uint8_t *
app_netcmd_req_attach(void)
{
  struct app_msg *msg = (struct app_msg *)&txmsg;
  uint8_t len = sizeof(struct app_data_vd_faeconn);
  struct app_msg_header *header = (struct app_msg_header *)msg;
  struct app_msg_footer *footer = (struct app_msg_footer *)(msg->data + len);
  struct app_data_vd_faeconn *req = (struct app_data_vd_faeconn *)msg->data;
  uint16_t crc = 0;

  header->beg = APPMSG_BEG;
  header->len = DEVNO_LEN + 1 + len;
  memcpy(header->devno, nib.devno, DEVNO_LEN);
  header->dir = APPMSG_UP;
  header->rep = 0;
  header->cmdop = APPDATA_VD_FAECONN;

  unixtime_get(req->tstamp);
  req->radio = 0;
  req->sensor = 0;
  req->flash = 0;
  req->batV = app_get_batV();
  req->batQ = 100;

  crc = crc16_data(&header->len, (APPMSG_HEADER_LEN - 1 + len), CRC_INIT);
  footer->crc[0] = (crc >> 8);
  footer->crc[1] = (crc & 0xff);
  footer->end = APPMSG_END;

  return (uint8_t*)msg;
}

/**
 * \brief  发送入网消息
 */
void
app_send_join(void)
{
  struct app_data_vdconn *payload = (struct app_data_vdconn *)txbuf;

  memset(txbuf, 0, sizeof(struct app_data_vdconn));
  unixtime_get(payload->tstamp);
  payload->HwVer = nib.hwver;
  payload->FwVer = nib.fwver;

  PRINTF("app send join\n");
  app_send(&rimeaddr_root, APPDATA_VD_CONN, (uint8_t *)payload, sizeof(struct app_data_vdconn), 1, 1, 1, -1, app_time_synced);
}

int8_t
app_get_temperature(void)
{
#if WITH_HMC5983
  return hmc5983_get_temperature();
#elif WITH_QMC5883
  return qmc5883_get_temperature();
#else
  return 0;
#endif
}

/**
 * \brief  填充三轴磁场数据
 *
 * \param buf 填充的缓冲区的指针
 */
static void
app_fill_magnetic(uint8_t *buf)
{
  uint8_t i = 0;

#if WITH_MMC3316
  buf[i++] = (MMC3316[SENSOR_PORT].x >> 8);
  buf[i++] = (MMC3316[SENSOR_PORT].x & 0xff);
  buf[i++] = (MMC3316[SENSOR_PORT].y >> 8);
  buf[i++] = (MMC3316[SENSOR_PORT].y & 0xff);
  buf[i++] = (MMC3316[SENSOR_PORT].z >> 8);
  buf[i++] = (MMC3316[SENSOR_PORT].z & 0xff);
#elif WITH_HMC5983 || WITH_QMC5883
  buf[i++] = (One_Sample.x >> 8); // XH
  buf[i++] = (One_Sample.x & 0xff); // XL
  buf[i++] = (One_Sample.y >> 8); // YH
  buf[i++] = (One_Sample.y & 0xff); // YL
  buf[i++] = (One_Sample.z >> 8); // ZH
  buf[i++] = (One_Sample.z & 0xff); // ZL
#else
#error "no support"
#endif
}

static int
app_get_batV(void)
{
  int battery = 0;
  battery_sensor.configure(0, 0);
  battery = battery_sensor.value(0);
  return battery;
}

/**
 * \brief  发送心跳消息
 */
void
app_send_hbeat(void)
{
  struct app_data_vdhbeat *payload = (struct app_data_vdhbeat *)txbuf;

  memset(txbuf, 0, sizeof(struct app_data_vdhbeat));
  unixtime_get(payload->tstamp);
  payload->status = park_s; // 泊位状态
  payload->temperature = app_get_temperature();
  payload->seqno = app_hbeat_seq;
  app_fill_magnetic(payload->magnetic);
  payload->batVoltage = app_get_batV();
  payload->batQuantity = 100;
  if (app_rssi_q.count > 0) { // 填充平均的接收RSSI和LQI
    payload->recvRSSI = app_rssi_q.rssi_sum / app_rssi_q.count;
    payload->recvLQI = app_rssi_q.lqi_sum / app_rssi_q.count;
  }

  PRINTF("app send hbeat\n");
  app_send(&rimeaddr_root, APPDATA_VD_HBEAT, (uint8_t *)payload, sizeof(struct app_data_vdhbeat), 0, 1, 1, -1, app_time_synced);
}

/**
 * \brief  心跳发送回调函数
 *
 * \param ptr ctimer用回调指针
 */
void
app_hbeat_task(void *ptr)
{
  if (app_hb_retry) { // 未达到心跳最大重传次数
    // 为下一次重传定时，防止本次发送收不到ACK
    ctimer_set(&app_hb_ct, (CLOCK_SECOND * 10), app_hbeat_task, NULL);

    // 发送心跳消息并减少重传次数
    --app_hb_retry;
    app_send_hbeat();
  }
  else {
    //log_w(E_FAIL, NULL, 0);
#if HBEAT_READY
    enterS(S_JOIN);
    process_poll(&device_process);
#endif
  }
}

static void
app_enq_msg(uint8_t flag, const rimeaddr_t *to, uint8_t cmdop, uint8_t *payload, uint8_t len, uint8_t rep, uint8_t ack, uint8_t try, int idx, int sync)
{
  struct app_msg *msg = NULL;
  struct app_msg_header *header = NULL;
  struct app_msg_footer *footer = NULL;
  struct msg_entry *me = NULL;
  struct msg_entry *ce = NULL;
  struct msg_entry *prev = NULL;
  uint16_t crc = 0;
  uint8_t done = 0;

  me = memb_alloc(&app_msg_mem); // 申请消息项
  if (me == NULL) {
    for (me = list_head(app_send_queue); me != NULL; me = list_item_next(me)) {
      if (me->prio < 2) { // 优先级低于APPDATA_MAG_DATA
        list_remove(app_send_queue, me);
        break; // 找到一个可丢弃的消息
      }
    }
  }
  if (me == NULL) { // 没有可丢弃的消息
    if (cmdop == APPDATA_VD_CONN) {
      me = list_chop(app_send_queue); // 为入网消息删除最后一条消息
    } else {
      return; // 入队失败
    }
  }

  msg = (struct app_msg *)&me->msg;
  header = (struct app_msg_header *)msg;
  footer = (struct app_msg_footer *)(msg->data + len);

  // 填充消息内容
  memset(msg, 0, sizeof(struct app_msg));
  header->beg = APPMSG_BEG;
  header->len = DEVNO_LEN + 1 + len;
  memcpy(header->devno, nib.devno, sizeof(nib.devno));
  header->dir = APPMSG_UP;
  header->rep = rep;
  header->cmdop = cmdop;

  memcpy(msg->data, payload, len);

  crc = crc16_data(&header->len, (APPMSG_HEADER_LEN - 1 + len), CRC_INIT);
  footer->crc[0] = (crc >> 8);
  footer->crc[1] = (crc & 0xff);
  footer->end = APPMSG_END;

  // 填充消息项内容
  rimeaddr_copy(&me->to, to);
  me->cmdop = cmdop;
  me->len = APPMSG_HEADER_LEN + len + APPMSG_FOOTER_LEN;
  me->ack = ack;
  me->retry = try;
  me->prio = app_msg_prio(msg);
  me->index = idx;
  me->sync = sync;

  if (flag == ENQ_HEAD) {
    list_push(app_send_queue, me); ++app_sendq_n; // 加为队列头
  }
  else if (flag == ENQ_TAIL) {
    list_add(app_send_queue, me); ++app_sendq_n; // 加为队列尾
  }
  else if (flag == ENQ_PRIO) {
    // 根据优先级插入队列
    done = 0;
    prev = NULL;
    for (ce = list_head(app_send_queue); ce != NULL; ce = list_item_next(ce)) {
      if (me->prio > ce->prio) {
        if (prev == NULL) {
          list_push(app_send_queue, me); ++app_sendq_n; // 加为队列头
        } else {
          list_insert(app_send_queue, prev, me); ++app_sendq_n; PRINTF("app sendq ins %p %p\n", me, prev); // 插入为同优先级的最后一项
        }
        done = 1; break;
      }
      prev = ce;
    }
    if (!done) {
      PRINTF("app sendq add %p\n", me);
      list_add(app_send_queue, me); ++app_sendq_n; // 加为队列尾
    }
  }
  PRINTF("app sendq enq, %d\n", app_sendq_n);
}

/**
 * \brief  发送泊位事件消息
 *
 * \param status 泊位当前状态。0: 无车，1: 有车，2: 待激活，3: 初始化。
 */
void
app_send_parkevt(uint8_t status)
{
  struct app_data_parkevt *payload = (struct app_data_parkevt *)txbuf;
  int idx = -1;

  memset(txbuf, 0, sizeof(txbuf));
  unixtime_get(payload->tstamp);
  payload->status = park_s; // 泊位状态
  payload->temperature = app_get_temperature();
  payload->seqno = app_parkevt_seq++;
  app_fill_magnetic(payload->magnetic);
  payload->batVoltage = app_get_batV();
  payload->batQuantity = 100;

  PRINTF("app send parkevt\n");
  if ((payload->status == 0) || (payload->status == 1)) { // 只保存车驶入或离开的数据
    idx = app_save_msg(APPDATA_PARK_EVT, txbuf, sizeof(struct app_data_parkevt)); //app_log_save(idx, APPDATA_PARK_EVT, payload->tstamp); // 先保存泊位事件数据
  }
  app_enq_msg(ENQ_PRIO, &rimeaddr_root, APPDATA_PARK_EVT, txbuf, sizeof(struct app_data_parkevt), 1, 1, APP_MAX_REXMIT, idx, app_time_synced);
  app_send_next(NULL);
}

#if UP_HILLS==1
/**
 * \brief  发送磁场波动消息
 *
 * \param info 磁场波动数据的指针
 */
void
app_send_magdata(upload_wave *info)
{
  struct app_data_magdata *mag = (struct app_data_magdata *)txbuf;
  uint8_t i, j;
  uint16_t val;
  int idx;

  memset(txbuf, 0, sizeof(txbuf));
  unixtime_get(mag->tstamp);
  mag->status = park_s;
  mag->seqno = app_magdata_seq;
  for (i = 0; i < APPDATA_MAGDATA_NUM; i++) {
    for (j = 0; j < 3; j++) {
      val = (uint16_t)info->upload_hill[i][j];
      mag->hillvalleys[i][2*j] = (val >> 8);
      mag->hillvalleys[i][2*j+1] = (val & 0xff);
    }
  }
  for (i = 0; i < 3; i++) {
    val = (uint16_t)info->upload_bs[i];
    mag->baselines[2*i] = (val >> 8);
    mag->baselines[2*i+1] = (val & 0xff);
    val = (uint16_t)info->upload_smooth[i];
    mag->smooths[2*i] = (val >> 8);
    mag->smooths[2*i+1] = (val & 0xff);
  }
  mag->judgebranch = info->judge_branch;

  PRINTF("app send magdata\n");
  idx = app_save_msg(APPDATA_MAG_DATA, txbuf, sizeof(struct app_data_magdata)); //app_log_save(idx, APPDATA_MAG_DATA, mag->tstamp); // 先保存磁场波动数据
  app_enq_msg(ENQ_PRIO, &rimeaddr_root, APPDATA_MAG_DATA, txbuf, sizeof(struct app_data_magdata), 1, 1, APP_MAX_REXMIT, idx, app_time_synced);
  app_send_next(NULL);
}
#endif

/**
 * \brief  发送算法参数消息的响应
 *
 * \param msg 算法参数消息的指针
 * \param result 执行结果
 */
void
app_send_algo_ack(struct app_msg *msg, uint8_t result)
{
  struct app_data_algo *req = (struct app_data_algo *)msg->data;
  struct app_data_algo_ack *ack = (struct app_data_algo_ack *)txbuf;

  memset(txbuf, 0, sizeof(txbuf));
  memcpy(ack->tstamp, req->tstamp, TSTAMP_LEN);
  ack->subop = req->subop;
  ack->res = result;
  if (req->subop == APP_GET) {
    uint8_t i = 0;
    Get_Algorithm_Parameters(&algo);
    ack->param[i++] = (algo.normalT >> 8);
    ack->param[i++] = (algo.normalT & 0xff);
    ack->param[i++] = (algo.flunctT >> 8);
    ack->param[i++] = (algo.flunctT & 0xff);
    ack->param[i++] = algo.big_occ_thresh;
    ack->param[i++] = algo.mid_occ_thresh;
    ack->param[i++] = algo.litt_occ_thresh;
    ack->param[i++] = algo.unocc_thresh;
    ack->param[i++] = (algo.base_line[0] >> 8);
    ack->param[i++] = (algo.base_line[0] & 0xff);
    ack->param[i++] = (algo.base_line[1] >> 8);
    ack->param[i++] = (algo.base_line[1] & 0xff);
    ack->param[i++] = (algo.base_line[2] >> 8);
    ack->param[i++] = (algo.base_line[2] & 0xff);
    ack->param[i++] = algo.gain_hmc5983;
  }

  if (fae_req) {
    fae_req = 0;
    app_unic_send(APPDATA_ALGO_PARAM, txbuf, sizeof(struct app_data_algo_ack), 0);
  } else {
    app_send(&rimeaddr_root, APPDATA_ALGO_PARAM, txbuf, sizeof(struct app_data_algo_ack), 0, 1, 1, -1, app_time_synced);
  }
}

/**
 * \brief  发送通用的确认消息
 *
 * \param msg 待确认的消息的指针
 * \param result 执行结果
 */
void
app_send_ack(struct app_msg *msg, uint8_t result)
{
  struct app_data_ack *ack = (struct app_data_ack *)txbuf;

  memset(txbuf, 0, sizeof(txbuf));
  memcpy(ack->tstamp, msg->data, TSTAMP_LEN);
  ack->result = result;

  PRINTF("app send ack\n");
  if (fae_req) {
    fae_req = 0;
    app_unic_send(msg->header.cmdop, (uint8_t *)ack, sizeof(struct app_data_ack), 0);
  } else {
    app_send(&rimeaddr_root, msg->header.cmdop, (uint8_t *)ack, sizeof(struct app_data_ack), 0, 0, 1, -1, app_time_synced);
  }
}

/*---------------------------------------------------------------------------*/
/**
 * \brief  从存储器读取一条已存储消息
 *
 * \param cmdop 存放读取的消息命令字的变量指针
 * \param buf 存放读取的消息内容的缓冲区指针
 * \param len 存放读取的消息长度的变量指针
 *
 * \retval idx 读取的消息对应的存储索引
 *         -1  未读取到消息
 */
static int
app_load_msg(uint8_t *cmdop, uint8_t *buf, uint8_t *len, struct msgmeta *metaptr)
{
  uint32_t addr = 0;
  struct msgmeta meta;
  uint16_t idx = 0;
  uint16_t ccrc = 0;

  idx = msgs.readptr;
  while(idx != msgs.writeptr) {
    watchdog_periodic();
    addr = NV_MSGS_ADDR + sizeof(struct MSGS) + (NV_MSGS_MAXLEN * idx); // 根据索引计算读取地址
    nv_read(addr, (uint8_t*)&meta, sizeof(struct msgmeta)); // 读取元数据
    if (meta.erased == 0) { // 消息未被擦除
      //if (meta.sent == 0) {
        nv_read(addr + sizeof(struct msgmeta), buf, meta.len); // 读取消息数据
        PRINTF("app load msg 0x%02X at 0x%04X, len %d, crc 0x%04X, ts 0x%02X%02X%02X%02X%02X%02X/%lu\n",
            meta.id, addr, meta.len, meta.crc, buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],meta.ts);

        ccrc = crc16_data(buf, meta.len, CRC_INIT);
        if (meta.crc == ccrc) { // CRC正确
          *cmdop = meta.id;
          *len = meta.len;
          memcpy(metaptr, &meta, sizeof(struct msgmeta));
          if (meta.fixed) {
            unixtime_ts(buf, meta.ts); // 将修正的时间戳替换原消息载荷的时间戳
          }
          return idx; // 读取成功
        }
        else {
#if 1
          uint8_t arg[7];
          arg[0] = (idx >> 8);
          arg[1] = (idx & 0xff);
          arg[2] = meta.id;
          arg[3] = (meta.crc >> 8);
          arg[4] = (meta.crc & 0xff);
          arg[5] = (ccrc >> 8);
          arg[6] = (ccrc & 0xff);
          log_w(W_READ_MSG_CRCX, arg, 7); // 从Flash读取消息出现CRC错误
#endif
          // CRC错误，擦取数据出错的该消息
          app_erase_msg(idx);
        }
      //} else {
      //  // 取下一条消息
      //}
    } else { // 消息已被擦除
      if (idx == msgs.readptr) {
        if (++msgs.readptr >= NV_MSGS_MAXNUM) // 移动读取指针
          msgs.readptr = 0;
      }
      if (msgs.readptr == msgs.writeptr) {
        msgs.count = 0;
        break;
      }
    }
    if (++idx >= NV_MSGS_MAXNUM)
      idx = 0;
  }

  return -1; // 读取失败
}

/**
 * \brief  向存储器保存一条消息
 *
 * \param cmdop 待保存的消息命令字
 * \param data 待保存的消息有效载荷
 * \param len 待保存的消息有效载荷的长度
 *
 * \retval idx 消息保存后对应的存储索引值
 */
static int
app_save_msg(uint8_t cmdop, uint8_t *data, uint8_t len)
{
  uint32_t addr = 0;
  struct msgmeta meta = {0};
  uint16_t idx = 0;
#if DEBUG_MSG_SAVE
  uint8_t arg[10];
#endif

  // 填充元数据
  memset(&meta, 0, sizeof(struct msgmeta));
  meta.id = cmdop;
  meta.len = len;
  meta.crc = crc16_data(data, len, CRC_INIT);
  meta.ts = unixtime_now();
  meta.synced = app_time_synced;
  meta.fixed = 0;
  meta.erased = 0;
  meta.sent = 0;

  // 将元数据和消息数据写入存储器
  idx = msgs.writeptr;
  addr = NV_MSGS_ADDR + sizeof(struct MSGS) + (NV_MSGS_MAXLEN * msgs.writeptr); // 根据写入指针计算写入地址
  PRINTF("app save msg 0x%02X to 0x%04X, len %d, crc 0x%04X, ts 0x%02X%02X%02X%02X%02X%02X/%lu\n",
      meta.id, addr, meta.len, meta.crc, data[0],data[1],data[2],data[3],data[4],data[5],meta.ts);
  meta.mcrc = crc16_data((uint8_t*)&meta, sizeof(struct msgmeta)-2, CRC_INIT);
  nv_write(addr, (uint8_t*)&meta, sizeof(struct msgmeta)); // 保存消息时写消息元数据
  nv_write(addr + sizeof(struct msgmeta), data, len); // 保存消息时写消息数据

  // 更新存储的写入指针和消息个数
  if (++msgs.writeptr >= NV_MSGS_MAXNUM)
    msgs.writeptr = 0;
  msgs.count++; msgs.changed = 1;
  PRINTF("app msgs count %d, [%d,%d)\n", msgs.count, msgs.readptr, msgs.writeptr);

#if DEBUG_MSG_SAVE
  arg[0] = (idx >> 8);
  arg[1] = (idx & 0xff);
  arg[2] = cmdop;
  arg[3] = meta.len;
  arg[4] = (meta.crc >> 8);
  arg[5] = (meta.crc & 0xff);
  arg[6] = (addr >> 24);
  arg[7] = (addr >> 16);
  arg[8] = (addr >>  8);
  arg[9] = (addr >>  0);
  log_i(I_VD_SAVE_MSG, arg, 10); // 保存消息
#endif

  return idx;
}

/**
 * \brief  发送一条已保存的消息
 *
 * \retval 0 可忽略
 */
static int
app_send_saved_msg(void)
{
  uint8_t cmdop;
  struct msgmeta meta;
  int r = 0;
#if DEBUG_MSG_SAVE
  uint8_t arg[4];
#endif

  while (msgs.count > 0) {
    r = app_load_msg(&cmdop, txbuf, &txlen, &meta);
    if (r != -1) {
      app_send(&rimeaddr_root, cmdop, txbuf, txlen, 1, 1, APP_MAX_REXMIT, r, meta.synced);
#if DEBUG_MSG_SAVE
      arg[0] = (r >> 8);
      arg[1] = (r & 0xff);
      arg[2] = cmdop;
      arg[3] = txlen;
      log_i(I_VD_SEND_SAVE, arg, 4); // 发送保存消息
#endif
      break; // 只发送一条消息
    }
  }

  return 0;
}

/**
 * \brief  根据索引擦除一条已存储消息
 *
 * \param idx 待擦除的消息对应的存储索引
 *
 * \retval 0 擦除成功
 *         1 已擦除
 */
static int
app_erase_msg(uint16_t idx)
{
  uint32_t addr;
  struct msgmeta meta;
#if 0
  uint8_t tmp[NV_MSGS_MAXLEN];
  uint8_t i;
#endif
#if DEBUG_MSG_SAVE
  uint8_t arg[6];
#endif

  // 根据索引读取元数据
  addr = NV_MSGS_ADDR + sizeof(struct MSGS) + (NV_MSGS_MAXLEN * idx);
  nv_read(addr, (uint8_t*)&meta, sizeof(struct msgmeta));
  if (meta.erased > 0) {
    return 1; // 已擦除
  }

  // 标记已擦除并更新元数据
  meta.erased = 1;
  meta.mcrc = crc16_data((uint8_t*)&meta, sizeof(struct msgmeta)-2, CRC_INIT);
  nv_write(addr, (uint8_t*)&meta, sizeof(struct msgmeta));

#if 0
  for (i = 0; i < NV_MSGS_MAXLEN; i++)
    tmp[i] = 0xFF;
  nv_write(addr, tmp, NV_MSGS_MAXLEN);
#endif

  // 更新读取指针和消息个数
  if (idx == msgs.readptr) {
    if (++msgs.readptr >= NV_MSGS_MAXNUM)
      msgs.readptr = 0;
  }
  msgs.count--; msgs.changed = 1;
  PRINTF("app msgs count %d, [%d,%d)\n", msgs.count, msgs.readptr, msgs.writeptr);

#if DEBUG_MSG_SAVE
  arg[0] = (idx >> 8);
  arg[1] = (idx & 0xff);
  arg[2] = (msgs.readptr >> 8);
  arg[3] = (msgs.readptr & 0xff);
  arg[4] = (msgs.count >> 8);
  arg[5] = (msgs.count & 0xff);
  log_i(I_VD_ERASE_MSG, arg, 6); // 擦除保存消息
#endif
  return 0;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief  主应用的初始化
 */
static void
app_init(void)
{
  rimeaddr_t addr;
  uint16_t msgs_readptr;
  uint16_t msgs_writeptr;
  uint16_t msgs_count;
  uint16_t msgs_ptr;
  uint16_t msgs_end;
  uint32_t msg_addr;
  struct msgmeta meta;
  uint16_t mcrc;
  uint16_t ccrc;
  uint8_t fwver;
  uint8_t i;

  watchdog_periodic();

  // 存储器访问接口的初始化
  nv_init();

  // 读取或初始化启动信息，根据APP_VER_xxx修正固件版本号
  ReadFlash(FLASH_BOOT_ADDR, (unsigned char*)&boot, sizeof(struct BOOT));
  if (boot.magic != NV_MAGIC) {
    memset(&boot, 0, sizeof(struct BOOT));
    boot.magic = NV_MAGIC;
    boot.role = NODE_VD;
    boot.version = 0x01;
    EraseFlash(FLASH_BOOT_ADDR);
    WriteFlash(FLASH_BOOT_ADDR, (unsigned char*)&boot, sizeof(struct BOOT));
  }
  fwver = ((APP_VER_MAJOR << 4) + APP_VER_MINOR);
  if (boot.version != fwver) {
    boot.version = fwver;
    EraseFlash(FLASH_BOOT_ADDR);
    WriteFlash(FLASH_BOOT_ADDR, (unsigned char*)&boot, sizeof(struct BOOT));
  }

  // 读取或初始化节点无线升级信息
  nv_read(NV_OTA_ADDR, (uint8_t*)&ota, sizeof(struct OTA));
  ccrc = crc16_data((uint8_t*)&ota, sizeof(struct OTA)-2, CRC_INIT);
  if ((ota.magic != NV_MAGIC) || (ccrc != ota.crc)) {
    memset(&ota, 0, sizeof(struct OTA));
    ota.magic = NV_MAGIC;
    for (i = 0; i < 3; i++) {
      ota.images[i].id  = OTA_ROLE[i];
      ota.images[i].ver = 0x00;
      ota.images[i].rcvd = 0;
      ota.images[i].addr = 0x0000;
      ota.images[i].size = 0;
      ota.images[i].crc  = CRC_INIT;
      ota.images[i].pend = 0;
    }
    ota.crc = crc16_data((uint8_t*)&ota, sizeof(struct OTA)-2, CRC_INIT);
    nv_write(NV_OTA_ADDR, (uint8_t*)&ota, sizeof(struct OTA));
  }

#if 0
  // 读取或初始化节点的特定信息
  nv_read(NV_NIB_ADDR, (uint8_t*)&nib, sizeof(struct NIB));
  ccrc = crc16_data((uint8_t*)&nib, sizeof(struct NIB)-2, CRC_INIT);
  if ((nib.magic != NV_MAGIC-1) || (ccrc != nib.crc) || (nib.devno[0] != NODE_VD)) {
    PRINTF("app init nib\n");
    memset(&nib, 0, sizeof(struct NIB));
    nib.magic = NV_MAGIC;
    nib.devno[0] = NODE_VD; // VD(不可更改)
    nib.devno[1] = 0xCA; // 厂商代码，CA拟为Cadre简称
    nib.devno[2] = 0x17; // 生产年份(BCD)
    nib.devno[3] = 0x03; // 生产月份(BCD)
    nib.devno[4] = 0x00; // 生产序号高字节(BCD)
    nib.devno[5] = node_id; // 生产序号低字节(BCD)
    nib.addr  = 0x200 + nib.devno[5]; // 默认短地址(不要更改本行)
    nib.hwver = 0x10; // 硬件版本号(BCD), v1.0
    nib.fwver = 0x10; // 固件版本号(BCD), v1.0
    nib.activated = 1; // 激活标志, 0:未激活, 1:已激活
    nib.tricseq = 0; // 全网参数序号
    nib.crc = crc16_data((uint8_t*)&nib, sizeof(struct NIB)-2, CRC_INIT);
    nv_write(NV_NIB_ADDR, (uint8_t*)&nib, sizeof(struct NIB));
  }
#else
  nv_read(NV_NIB_ADDR, (uint8_t*)&nib, sizeof(struct NIB));
  ccrc = crc16_data((uint8_t*)&nib, sizeof(struct NIB)-2, CRC_INIT);
  if ((nib.magic != NV_MAGIC) || (ccrc != nib.crc) || (nib.devno[0] != NODE_VD)) {
    log_e(E_IDENTITY_ERR, nib.devno, DEVNO_LEN);
    app_fatal();
  }
#endif
  // 如果NIB中的固件版本号不正确，根据APP_VER_xxx修正
  fwver = ((APP_VER_MAJOR << 4) + APP_VER_MINOR);
  if (nib.fwver != fwver) {
    nib.fwver = fwver;
    nib.crc = crc16_data((uint8_t*)&nib, sizeof(struct NIB)-2, CRC_INIT);
    nv_write(NV_NIB_ADDR, (uint8_t*)&nib, sizeof(struct NIB));
  }

  // 根据设备编码设置64位节点MAC地址
  memset(node_mac, 0, sizeof(node_mac));
  memcpy(node_mac + 2, nib.devno, sizeof(nib.devno));

  // 根据NIB中的短地址设置node_id和rime地址
  node_id = nib.addr;
  addr.u8[0] = (nib.addr & 0xff);
  addr.u8[1] = (nib.addr >> 8);
  rimeaddr_set_node_addr(&addr);

  // 读取或初始化全网参数
  nv_read(NV_PIB_ADDR, (uint8_t*)&pib, sizeof(struct PIB));
  ccrc = crc16_data((uint8_t*)&pib, sizeof(struct PIB)-2, CRC_INIT);
  if ((pib.magic != NV_MAGIC) || (ccrc != pib.crc)) {
    memset(&pib, 0, sizeof(struct PIB));
    pib.magic = NV_MAGIC;
    pib.apHbeatT = 30; // AP心跳周期
    pib.rpHbeatT = 1800; // RP心跳周期
    pib.vdHbeatT = 1800; // VD心跳周期
    pib.healthT  = 3600; // 自检周期
    pib.commandT = 10; // 下行命令轮询周期
    pib.paramT   = 4096; // 全网参数轮询周期
    pib.batVoltageThr = 30; // 电池电压阈值
    pib.batQuantityThr = 10; // 电池电量阈值
    pib.solarVoltageThr = 20; // 太阳能电池电压阈值
    pib.radioChan  = 2; // 默认信道 2/908MHz
    pib.radioPower = 24; // 默认功率 24dBm
    pib.crc = crc16_data((uint8_t*)&pib, sizeof(struct PIB)-2, CRC_INIT);
    nv_write(NV_PIB_ADDR, (uint8_t*)&pib, sizeof(struct PIB));
  }
  app_new_chan = pib.radioChan;

  // 检查PIB中时间值，如果小于最小允许值，校正为最小值
  if (pib.vdHbeatT < VDHBEATT_MIN)
    pib.vdHbeatT = VDHBEATT_MIN;
  if (pib.healthT < HEALTHT_MIN)
    pib.healthT = HEALTHT_MIN;
  if (pib.commandT < COMMANDT_MIN)
    pib.commandT = COMMANDT_MIN;
  if (pib.paramT < PARAMT_MIN)
    pib.paramT = PARAMT_MIN;

  // 读取或初始化检测算法参数
  nv_read(NV_ALGO_ADDR, (uint8_t*)&algo, sizeof(struct ALGO));
  ccrc = crc16_data((uint8_t*)&algo, sizeof(struct ALGO)-2, CRC_INIT);
  if ((algo.magic != NV_MAGIC) || (ccrc != algo.crc)) {
    memset(&algo, 0, sizeof(struct ALGO));
    algo.magic = NV_MAGIC;
    algo.normalT = STABLE_SAMPLE_PERIOD;//1000; // 平稳采样周期(ms)
    algo.flunctT =  DISTURB_SAMPLE_PERIOD;//200; // 波动采样周期(ms)
    algo.gain_hmc5983 = DEFAULT_GAIN; //case 3: CRB7~5=011, 660 gain, 0x60, 1.52mg/LSB
    algo.big_occ_thresh=PARKING_THRESHOLD_F; //停车快速检测阈值
    algo.mid_occ_thresh=PARKING_THRESHOLD_B; //停稳后检测阈值
    algo.litt_occ_thresh=PARKING_THRESHOLD_L; //弱磁信号车辆检测阈值
    algo.unocc_thresh=UNOCC_THRSHOLD;         //车辆驶离检测阈值
    algo.status = 2; // 默认待激活状态
    algo.crc = crc16_data((uint8_t*)&algo, sizeof(struct ALGO)-2, CRC_INIT);
    nv_write(NV_ALGO_ADDR, (uint8_t*)&algo, sizeof(struct ALGO));
  }
  Set_Algorithm_Parameters(&algo,false);
  sampleT = ((uint32_t)CLOCK_SECOND * algo.normalT) / 1000;
  park_s = algo.status;

  // 读取存储消息队列的元数据或进行相应的初始化
  nv_read(NV_MSGS_ADDR, (uint8_t*)&msgs, sizeof(struct MSGS));
  ccrc = crc16_data((uint8_t*)&msgs, sizeof(struct MSGS)-2, 0x0000);
  if ((msgs.magic != NV_MAGIC) || (ccrc != msgs.crc)) {
    memset(&msgs, 0, sizeof(struct MSGS));
    msgs.magic = NV_MAGIC;
    msgs.count = 0;
    msgs.readptr = 0;
    msgs.writeptr = 0;
    msgs.wcount = 1;
    msgs.crc = crc16_data((uint8_t*)&msgs, sizeof(struct MSGS)-2, 0x0000);

    PRINTF("msgs init\n");
    nv_erase(NV_MSGS_ADDR, NV_MSGS_SIZE);
    nv_write(NV_MSGS_ADDR, (uint8_t*)&msgs, sizeof(struct MSGS)); // 初始化消息队列时写元数据
  }

  /// 修正已存储消息队列的读写指针和消息个数
  if ((msgs.readptr >= NV_MSGS_MAXNUM) && (msgs.writeptr >= NV_MSGS_MAXNUM)) {
    msgs.readptr = 0; msgs.writeptr = NV_MSGS_MAXNUM-1;
  } else if (msgs.readptr >= NV_MSGS_MAXNUM) {
    msgs.readptr = ((msgs.writeptr < NV_MSGS_MAXNUM-1) ? (msgs.writeptr + 1) : 0);
  } else if (msgs.writeptr >= NV_MSGS_MAXNUM) {
    msgs.writeptr = ((msgs.readptr > 0) ? (msgs.readptr - 1) : (NV_MSGS_MAXNUM - 1));
  }

  msgs_count = 0;
  msgs_ptr = msgs.readptr;
  //// 修正队列头指针
  msgs_end = ((msgs.readptr > 0) ? (msgs.readptr - 1) : (NV_MSGS_MAXNUM - 1));
  while (msgs_ptr != msgs_end) {
    watchdog_periodic();
    msg_addr = NV_MSGS_ADDR + sizeof(struct MSGS) + (NV_MSGS_MAXLEN * msgs_ptr);
    nv_read(msg_addr, (uint8_t*)&meta, sizeof(struct msgmeta));
    mcrc = crc16_data((uint8_t*)&meta, sizeof(struct msgmeta)-2, CRC_INIT);
    if (mcrc != meta.mcrc) {
      msgs_readptr = msgs_ptr;
      break; // 找到第一条未使用消息时结束
    } else {
      if (meta.erased) { // 当前消息已擦除
        if (++msgs_ptr >= NV_MSGS_MAXNUM)
          msgs_ptr = 0;
      } else {
        msgs_readptr = msgs_ptr;
        break; // 找到第一条未擦除消息时结束
      }
    }
  }

  if (msgs_ptr == msgs_end) { // 全部是已擦除消息
    msgs_readptr = msgs_ptr;
    msgs_writeptr = msgs_ptr;
    msgs_count = 0;
  }
  else {
    //// 修正队列尾指针
    msgs_end = ((msgs_ptr > 0) ? (msgs_ptr - 1) : (NV_MSGS_MAXNUM - 1));
    while (msgs_ptr != msgs_end) {
      watchdog_periodic();
      msg_addr = NV_MSGS_ADDR + sizeof(struct MSGS) + (NV_MSGS_MAXLEN * msgs_ptr);
      nv_read(msg_addr, (uint8_t*)&meta, sizeof(struct msgmeta));
      mcrc = crc16_data((uint8_t*)&meta, sizeof(struct msgmeta)-2, CRC_INIT);
      if (mcrc != meta.mcrc) {
        msgs_writeptr = msgs_ptr;
        break; // 找到第一条未使用消息时结束
      } else {
        if (meta.erased) {
          msgs_writeptr = msgs_ptr;
          break; // 找到第一条已擦除消息时结束
        } else {
          if (++msgs_ptr >= NV_MSGS_MAXNUM)
            msgs_ptr = 0;
          msgs_count++;
        }
      }
    }
  }

  if ((msgs_count != msgs.count) || (msgs_readptr != msgs.readptr) || (msgs_writeptr != msgs.writeptr)) {
    msgs.count = msgs_count;
    msgs.readptr = msgs_readptr;
    msgs.writeptr = msgs_writeptr;
    msgs.wcount += 1;
    msgs.crc = crc16_data((uint8_t*)&msgs, sizeof(struct MSGS)-2, CRC_INIT);
    nv_write(NV_MSGS_ADDR, (uint8_t*)&msgs, sizeof(struct MSGS)); // 重启后修正队列时写元数据
  }

  // 初始化磁传感器
  sensor = taskmon_create("sensor");
#if WITH_MMC3316
  MMC3316_Port_Init(SENSOR_PORT);
  while(MMC3316_Check(SENSOR_PORT) == FALSE);
  MMC3316_Reset(SENSOR_PORT);
  MMC3316_Init(SENSOR_PORT);
#elif WITH_HMC5983
  if (hmc5983_init() == 0) {
    //hmc5983_self_test();
    //dig_resolution_change();
    //hmc5983_sample_read(algo.gain_hmc5983);
    hmc5983_set_callback(app_magdata_ready);
  }
  else {
    log_e(E_SENSOR_FAIL, NULL, 0); // 传感器初始化出错
#if NODEALARM
    nodealarm.sensor1 = 1;
#endif
  }
#elif WITH_QMC5883
  if (qmc5883_init() == 0) {
    //qmc5883_self_test();
    //dig_resolution_change();
    //qmc5883_sample_read(algo.gain_hmc5983);
    qmc5883_set_callback(app_magdata_ready);
  }
  else {
    log_e(E_SENSOR_FAIL, NULL, 0); // 传感器初始化出错
#if NODEALARM
    nodealarm.sensor1 = 1;
#endif
  }
#else
#error "no support"
#endif

  // 实例化任务监视器以监视程序正常运行
  act = taskmon_create("act");
  send = taskmon_create("send");
  join = taskmon_create("join");
  hbeat = taskmon_create("hbeat");
  sample = taskmon_create("sample");

  // 申请radio共享资源控制块指针
  app_fae_rs  = radio_alloc("app-fae");
  app_mesh_rs = radio_alloc("app-mesh");
  app_tric_rs = radio_alloc("app-tric");

  // 申请事件编号供后续提交相关事件给主线程处理
  addr_changed_event = process_alloc_event();
  pib_changed_event = process_alloc_event();
  data_ready_event = process_alloc_event();

#if WITH_OTA
  {
    int fd;
    // 初始化CFS文件系统
    cfs_init();

    // 为存储数据和程序镜像预留CFS存储空间
    //fd = cfs_open("NV", CFS_READ);
    //if(fd == -1) {
    //  fd = cfs_open("NV", CFS_WRITE);
    //  if(fd != -1) {
    //    cfs_reserv(fd, NV_SIZE);
    //    cfs_close(fd);
    //  }
    //} else {
    //  cfs_close(fd);
    //}

    //fd = cfs_open("GOLDEN", CFS_WRITE);
    //cfs_reserv(fd, VD_IMG_SIZE);
    //cfs_close(fd);

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

#if 0
#define SIZE 1024
    uint16_t len;
    uint8_t buf[S_PAGE];
    fd = cfs_open(OTAFILE[NODE_VD], CFS_READ);
    cfs_seek(fd, SIZE, CFS_SEEK_SET);
    cfs_seek(fd, 0, CFS_SEEK_SET);
    for (len = SIZE; len > 0; ) {
      cfs_read(fd, buf, S_PAGE);
      len -= S_PAGE;
    }
    cfs_close(fd);
#endif
  }
#endif /* WITH_OTA */
}

/**
 * \brief  节点健康检查函数
 *
 * \retval 0 可忽略
 *
 *         检查节点的各硬件模块是否工作正常、电池电压等指标是否正常，如果至少
 *         有一项出现异常，发送报警消息向服务器报告。
 */
int
app_check_health(void)
{
  struct app_data_alarm alarm = {0};
  int error = 0;

  if (msgs.changed) {
    msgs.changed = 0;
    msgs.wcount += 1;
    msgs.crc = crc16_data((uint8_t*)&msgs, sizeof(struct MSGS)-2, CRC_INIT);
    nv_write(NV_MSGS_ADDR, (uint8_t*)&msgs, sizeof(struct MSGS)); // 周期性时写消息队列的元数据
  }

  if (app_get_batV() < pib.batVoltageThr) { // 电池电压小于阈值
    alarm.battery = 1; error = 1;
  }
#if NODEALARM
  if (nodealarm.sensor1) {
    alarm.sensor1 = 1; error = 1;
  }
#endif

  if (error) {
    // 存在异常，发送报警消息
    PRINTF("app send alarm\n");
    unixtime_get(alarm.tstamp);
    app_send(&rimeaddr_root, APPDATA_ALARM, (uint8_t*)&alarm, sizeof(struct app_data_alarm), 1, 1, 1, -1, app_time_synced);
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief  检测算法模块设置采样周期
 *
 * \param period 新采样周期，以毫秒为单位
 */
void Set_AMR_Period(int period)
{
  if (period < 16) {
    return;
  }
  sampleT = ((uint32_t)period * CLOCK_SECOND) / 1000; // 将在本次采样完成后生效
}

/**
 * \brief  重新标定传感器
 *
 * \param none
 */
void Re_Calibrate_AMR()
{
  Re_Init_Algorithm(); //重新标定
}

/**
 * \brief  重新标定成功
 *
 * \param none
 */
void Re_Calibrate_Success(bool is_success)
{
  struct app_data_reinit_resp *resp = (struct app_data_reinit_resp *)txbuf;
  uint8_t i = 0;

  Get_Algorithm_Parameters(&algo);

  memset(txbuf, 0, sizeof(txbuf));
  memcpy(resp->tstamp, req_ts, TSTAMP_LEN);
  resp->result = (is_success)?APP_OK:APP_ERR;
  resp->param[i++] = (algo.normalT >> 8);
  resp->param[i++] = (algo.normalT & 0xff);
  resp->param[i++] = (algo.flunctT >> 8);
  resp->param[i++] = (algo.flunctT & 0xff);
  resp->param[i++] = algo.big_occ_thresh;
  resp->param[i++] = algo.mid_occ_thresh;
  resp->param[i++] = algo.litt_occ_thresh;
  resp->param[i++] = algo.unocc_thresh;
  resp->param[i++] = (algo.base_line[0] >> 8);
  resp->param[i++] = (algo.base_line[0] & 0xff);
  resp->param[i++] = (algo.base_line[1] >> 8);
  resp->param[i++] = (algo.base_line[1] & 0xff);
  resp->param[i++] = (algo.base_line[2] >> 8);
  resp->param[i++] = (algo.base_line[2] & 0xff);
  resp->param[i++] = algo.gain_hmc5983;

  if ((app_s != S_PREACT) && !ctimer_expired(&app_radio_ct) && (act_conned == 0))
    netcmd_set_period(pib.commandT); // 远程标定结束，重新发送req

  if (fae_req) {
    fae_req = 0;
    app_unic_send(APPDATA_VD_REINIT, txbuf, sizeof(struct app_data_reinit_resp), 0);
  } else {
    app_send(&rimeaddr_root, APPDATA_VD_REINIT, txbuf, sizeof(struct app_data_reinit_resp), 0, 1, 1, -1, app_time_synced);
  }
}

#if UP_HILLS==1
/**
 * \brief  检测算法模块触发发送磁场波动数据
 *
 * \param info 磁场波动数据的指针
 */
void Set_UP_HILLS(upload_wave * info)
{
  ++app_magdata_seq;
  app_send_magdata(info);
}
#endif

void Set_Gain_HMC5983(uint8_t gain)
{
  algo.gain_hmc5983 = gain;
}

uint8_t Get_Gain_HMC5983(void)
{
  return algo.gain_hmc5983;
}

#if UNIT_MILLI_GAUSS==1
/*********************************************************************
* @fn      Float_Compute
*
* @brief   为了计算时不产生溢出，采用按位计算
*          x1--磁采样值
*          x2--数字分辨率
* @param   none
*
* @return  计算结果
*/
int16_t Float_Compute(int16_t x1, int16_t x2)
{
  int16_t t1=0, t2=0, t3=0, t4=0;
  t1 = x2/100;           //提取百位的值
  t2 = (x2-t1*100)/10;   //提取十位的值
  t3 = x2%10;            //提取个位的值
  t4 = t1*x1+t2*x1/10+t3*x1/100;
  return t4;
}

void dig_resolution_change()
{
  switch(algo.gain_hmc5983) {
  case 0:
    digital_resolution=73; //Gain Configuration 1370
    break;
  case 1:
    digital_resolution=92;    //Gain Configuration 1090
    break;
  case 2:
    digital_resolution=122;   //Gain Configuration 820
    break;
  case 3:
    digital_resolution=152;   //Gain Configuration 660
    break;
  case 4:
    digital_resolution=227;  //Gain Configuration 440
    break;
  case 5:
    digital_resolution=256;  //Gain Configuration 440
    break;
  case 6:
    digital_resolution=303;   //Gain Configuration 330
    break;
  case 7:
    digital_resolution=435;  //Gain Configuration 230
    break;
  default:
    break;
  }
}
#endif

#if WITH_HMC5983 || WITH_QMC5883
void app_magdata_ready(unsigned char *data, unsigned char *temp)
{
  One_Sample.x=(int16_t)((data[0]<<8) + data[1]);
  One_Sample.y=(int16_t)((data[2]<<8) + data[3]);
  One_Sample.z=(int16_t)((data[4]<<8) + data[5]);
  //temperature= ((temp[0]<<8)+temp[1])/128+25;

  //将采样数据转换为毫高斯
//#if UNIT_MILLI_GAUSS==1
//  One_Sample.x = Float_Compute(One_Sample.x, digital_resolution);
//  One_Sample.y = Float_Compute(One_Sample.y, digital_resolution);
//  One_Sample.z = Float_Compute(One_Sample.z, digital_resolution);
//#endif

  task_end(sensor);
  process_post(&device_process, data_ready_event, NULL); // 提交事件让主线程处理传感器采样结果
}
#endif

/*---------------------------------------------------------------------------*/
#if WITH_OTA
void
app_ota_completed(uint8_t objid, uint8_t version, uint32_t size)
{
  struct filedesc *f;

  f = cfs_get(OTA_FILE[NODE_VD]);
  if (f == NULL) {
    log_w(E_NULL, NULL, 0);
    return;
  }

  __disable_interrupt();
  watchdog_periodic();
  ReadFlash(FLASH_BOOT_ADDR, (uint8_t*)&boot, sizeof(struct BOOT));

  boot.update = 2; // 重启后从外部Flash升级
  boot.updver = version;
  boot.addr = CFS_FILES_ADDR + f->offset;
  boot.size = size;
  boot.crc  = 0;

  EraseFlash(FLASH_BOOT_ADDR);
  WriteFlash(FLASH_BOOT_ADDR, (uint8_t*)&boot, sizeof(struct BOOT));
  __enable_interrupt();

  watchdog_reboot();
}

const struct deluge_callbacks deluge_cb = { app_ota_completed };
#endif /* WITH_OTA */
/*---------------------------------------------------------------------------*/
/**
 * \brief  下行命令模块收到消息的回调函数
 *
 * \param msgid 消息类型(可忽略，目前仅一种消息)
 * \param data 消息数据缓冲区的指针，为struct app_msg类型
 * \param len 消息数据的长度
 *
 * \retval 0 可忽略
 */
int
netcmd_rcvd(rimeaddr_t *from, uint8_t msgid, uint8_t *data, uint8_t len)
{
  struct app_msg *msg = (struct app_msg *)data;

  PRINTF("app rcvd cmd 0x%02X, op 0x%02X\n", msgid, msg->header.cmdop);
  app_handle_msg(from, msg);

  return 0;
}

struct netcmd_callback netc_cb = {netcmd_rcvd};
/*---------------------------------------------------------------------------*/
void
app_reboot_cb(void *ptr)
{
  watchdog_reboot();
}

/**
 * \brief  处理接收到的下行消息
 *
 * \param msg 接收的消息的指针
 */
void
app_handle_msg(rimeaddr_t *from, struct app_msg *msg)
{
  uint8_t op = msg->header.cmdop;

  if (op == APPDATA_RESET) { // 重启消息
    if (rimeaddr_cmp(from, &act_addr)) {
      fae_req = 1;
    }

    ctimer_set(&app_reboot_ct, (CLOCK_SECOND * 2), app_reboot_cb, NULL);
    app_req_reboot = 1; // 标记ACK发送后重启
    app_send_ack(msg, APP_OK); // 发送ACK
  }
  else if (op == APPDATA_FACT_RESET) { // 恢复出厂设置
    if (rimeaddr_cmp(from, &act_addr)) {
      fae_req = 1;
    }

    ctimer_set(&app_reboot_ct, (CLOCK_SECOND * 6), app_reboot_cb, NULL);
    app_factory_reset(NULL, 0);
    app_req_reboot = 1; // 标记ACK发送后重启
    app_send_ack(msg, APP_OK); // 发送ACK
  }
  else if (op == APPDATA_ALGO_PARAM) { // 算法参数消息
    struct app_data_algo *p = (struct app_data_algo *)msg->data;

    if (rimeaddr_cmp(from, &act_addr)) {
      fae_req = 1;
    }
    if (app_keep_radio_on) {
      radio_on(app_fae_rs, (APP_FAECMD_WAIT + (CLOCK_SECOND<<1))); // 延长射频开启
      ctimer_set(&app_radio_ct, APP_FAECMD_WAIT, app_no_command, NULL);
      if (app_s == S_PREACT) {
        ctimer_set(&app_act_ct, (APP_FAECMD_WAIT + CLOCK_SECOND/2), app_fail_activate, NULL);
      }
      task_end(act); task_begin(act, (APP_FAECMD_WAIT + (CLOCK_SECOND<<1)));
    }

    if (p->subop == APP_SET) {
      uint8_t i = 0;
      int r = 0;

      // 更改算法参数并写入Flash
      algo.normalT = (p->param[i] << 8) + p->param[i+1]; i += 2;
      algo.flunctT = (p->param[i] << 8) + p->param[i+1]; i += 2;
      algo.big_occ_thresh = p->param[i++];
      algo.mid_occ_thresh = p->param[i++];
      algo.litt_occ_thresh = p->param[i++];
      algo.unocc_thresh = p->param[i++];
      algo.base_line[0] = (p->param[i] << 8) + p->param[i+1]; i += 2;
      algo.base_line[1] = (p->param[i] << 8) + p->param[i+1]; i += 2;
      algo.base_line[2] = (p->param[i] << 8) + p->param[i+1]; i += 2;
      algo.gain_hmc5983 = p->param[i++];
      algo.crc = crc16_data((uint8_t*)&algo, sizeof(struct ALGO)-2, 0x0000);
      nv_write(NV_ALGO_ADDR, (uint8_t*)&algo, sizeof(struct ALGO));

      // 新算法参数应用于检测算法模块
      r = Set_Algorithm_Parameters(&algo,true);

      app_send_algo_ack(msg, r);
    }
    else if (p->subop == APP_GET) {
      app_send_algo_ack(msg, APP_OK);
    }
  }
  else if (op == APPDATA_VD_REINIT) { // 重新标定
    struct app_data_reinit *req = (struct app_data_reinit *)msg->data;

    if (rimeaddr_cmp(from, &act_addr)) {
      fae_req = 1;
    }
    if (app_keep_radio_on) {
      radio_on(app_fae_rs, (APP_FAECMD_WAIT + VD_REINIT_WAIT + (CLOCK_SECOND<<1))); // 延长射频开启
      ctimer_set(&app_radio_ct, (APP_FAECMD_WAIT + VD_REINIT_WAIT), app_no_command, NULL);
      if (app_s == S_PREACT) {
        ctimer_set(&app_act_ct, (APP_FAECMD_WAIT + VD_REINIT_WAIT + CLOCK_SECOND/2), app_fail_activate, NULL);
      }
      task_end(act); task_begin(act, (APP_FAECMD_WAIT + VD_REINIT_WAIT + (CLOCK_SECOND<<1)));
    }

    park_s = 2;
    memcpy(req_ts, req->tstamp, TSTAMP_LEN);
    Re_Calibrate_AMR(); //log_i(0, NULL, 0);

    if ((app_s != S_PREACT) && !ctimer_expired(&app_radio_ct) && (act_conned == 0))
      netcmd_set_period(0); // 远程标定开始，停止发送req
  }
  else if (op == APPDATA_VD_ACTIVATE) {
    struct app_data_vd_activate_resp *resp = (struct app_data_vd_activate_resp *)msg->data;
    if (resp->result == 0) {
      if (app_s == S_PREACT) {
        app_in_activate = 1;
        act_conned = 1;
        ctimer_set(&app_radio_ct, APP_FAECMD_WAIT, app_no_command, NULL);
        ctimer_set(&app_act_ct, (APP_FAECMD_WAIT + CLOCK_SECOND/2), app_fail_activate, NULL);
        task_end(act); task_begin(act, (APP_FAECMD_WAIT + (CLOCK_SECOND<<1)));
      } else {
        log_w(E_STATE, &app_s, 1);
      }
    } else {
      log_w(E_INVAL, &(resp->result), 1);
    }
  }
  else if (op == APPDATA_VD_FAECONN) {
    radio_on(app_fae_rs, APP_FAECMD_WAIT); // 开启射频case3: 维护连接成功
    app_keep_radio_on = 1; // 激活RF
    log_i(I_VD_ACT_CONNED, NULL, 0); // 与激活节点已连接
    act_conned = 1;
    netcmd_set_period(0); // 与激活节点已连接，停止发送req
    unicast_open(&unic, APP_UNICAST_CHANNEL, &unic_cb); unic_opened = 1; // 打开fae连接
    ctimer_set(&app_radio_ct, APP_FAECMD_WAIT, app_no_command, NULL);
  }
  else if (op == APPDATA_VD_DISCONN) {
    struct app_data_vd_disconn *req = (struct app_data_vd_disconn *)msg->data;

    ctimer_stop(&app_radio_ct);
    act_conned = 0;
    app_keep_radio_on = 0;
    if (req->subop == 1) {
      if (app_s == S_PREACT) {
        nib.activated = 1; // 标记已激活
      }
      app_new_chan = req->channel;
      app_send_disconn_ack(req);
    } else {
      log_w(E_INVAL, &(req->subop), 1);
      radio_off(app_fae_rs); // 关闭射频case3: 维护连接断开
    }
  }
#if WITH_OTA
  else if (op == APPDATA_OTA_EXEC) {
    struct app_data_ota_exec *exec = (struct app_data_ota_exec *)msg->data;

    if ((exec->type == NODE_VD)
        && (exec->fwver > nib.fwver)
        && ((memcmp(exec->target, node_mac, MAC_LEN) == 0)
            || (memcmp(exec->target, deluge_all, MAC_LEN) == 0)
            || ((exec->target[2] == NODE_VD)
                && (memcmp(exec->target+3, deluge_all+3, MAC_LEN-3) == 0)))) {
      // 启动无线更新模块
      deluge_start(&deluge_cb);
      deluge_disseminate(OBJ_DEVICE, "VD", nib.fwver, DELUGE_MODE_ALLNODE, (uint8_t*)deluge_all);
      netcmd_set_period(0); // 开始无线更新，停止发送req
      log_i(I_OTA_START, node_mac, MAC_LEN); // OTA升级开始
    } else {
      log_w(W_INVALID_OTA_EXEC, NULL, 0); // 非法的OTA升级命令
    }
  }
#endif /* WITH_OTA */
  else {
    log_w(W_MSG_NOT_HANDLE, &op, 1); // 消息未添加处理支持
  }
}
/*---------------------------------------------------------------------------*/
/**
 * \brief  节点激活的回调函数
 */
void
app_activated(void)
{
#if 0
  // 标记已激活并写入Flash
  nib.activated = 1;
  nib.crc = crc16_data((uint8_t*)&nib, sizeof(struct NIB)-2, 0x0000);
  nv_write(NV_NIB_ADDR, (uint8_t*)&nib, sizeof(struct NIB));

  // 标记进入已激活状态，让主线程开始相关操作
  enterS(S_ACTED);
  process_poll(&device_process);
#else
  task_begin(act, (APP_FAECMD_WAIT + CLOCK_SECOND)); // 激活VD
#if DEBUG_VD
  printf("act vd\n");
#endif
  enterS(S_PREACT); log_i(I_VD_PREACT, NULL, 0); // 初次激活等待激活器交互
  process_poll(&device_process);
#endif
}

#if ACTIVATE_RF_BY_AMR==1
/**
 * \brief  激活RF
 *
 */
void Activate_RF()
{
  task_begin(act, (APP_FAECMD_WAIT + CLOCK_SECOND)); // 激活RF
#if DEBUG_VD
  printf("act rf\n");
#endif
  netcmd_set_period(pib.commandT); // 强磁激活射频，重新发送req
  ctimer_set(&app_radio_ct, APP_FAECMD_WAIT, app_no_command, NULL);
}
#endif

void
app_no_command(void *ptr)
{
  if ((app_s == S_READY) || (app_s == S_JOIN)) { // 维护阶段
    if (act_conned == 0) { // 未与激活器连接
      log_i(I_VD_NO_FAECMD, &app_s, 1); // 超时无下行命令
      netcmd_set_period(0); // 超时无命令，停止发送req
      task_end(act); // 维护失败
    }
    else { // 已连接激活器
      act_conned = 0;
      app_keep_radio_on = 0;
      app_send_disconn_req();
    }
  }
  else if (app_s == S_PREACT) { // 激活阶段
    if (app_keep_radio_on == 1) {
      app_keep_radio_on = 0;
      if (act_conned) { // 已连接激活器
        act_conned = 0;
        app_send_disconn_req();
      }
      else { // 未与激活器连接
        enterS(S_START);
        process_poll(&device_process);
      }
    }
  }
  else {
    log_w(E_STATE, &app_s, 1);
  }
}

void
app_fail_activate(void *ptr)
{
  if (app_s != S_START) {
    enterS(S_START);
    process_poll(&device_process);
  }
}
/*---------------------------------------------------------------------------*/
#if WITH_ATCMD
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
  atcmd_output(out, sizeof(out));

  return 0;
}

/**
 * \brief  设置设备编码的AT指令回调函数
 *
 * \param arg  AT指令参数的缓冲区指针
 * \param len  AT指令参数的长度
 *
 * \retval 0  已正常设置设备编码
 *
 *         AT指令参数即为新的设备编码，是12字节的BCD编码字符串，例如'01CA16060001'。
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
  nib.crc = crc16_data((uint8_t*)&nib, sizeof(struct NIB)-2, 0x0000);
  nv_write(NV_NIB_ADDR, (uint8_t*)&nib, sizeof(struct NIB));

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
  atcmd_output("AT+OK\r\n", 7);

  ReadFlash(FLASH_BOOT_ADDR, (uint8_t*)&boot, sizeof(struct BOOT));
  boot.update = 1;
  EraseFlash(FLASH_BOOT_ADDR);
  WriteFlash(FLASH_BOOT_ADDR, (uint8_t*)&boot, sizeof(struct BOOT));

  watchdog_reboot();
  return 0;
}

int
app_get_activate(const char *arg, int len)
{
  char out[3] = {0};
  uint8_t i = 0;

  out[i++] = '0' + nib.activated;
  out[i++] = '\r';
  out[i++] = '\n';

  atcmd_output(out, i);
  return -1;
}

int
app_set_activate(const char *arg, int len)
{
  if ((arg[0] != '0') && (arg[0] != '1'))
    return 1;

  nib.activated = arg[0] - '0';
  nib.crc = crc16_data((uint8_t*)&nib, sizeof(struct NIB)-2, 0x0000);
  nv_write(NV_NIB_ADDR, (uint8_t*)&nib, sizeof(struct NIB));
  return 0;
}
#endif /* WITH_ATCMD */

/**
 * \brief  恢复节点的出厂设置
 */
int
app_factory_reset(const char *arg, int len)
{
  watchdog_periodic();

  // 重置NIB
  nv_read(NV_NIB_ADDR, (uint8_t*)&nib, sizeof(struct NIB));
  nib.magic = NV_MAGIC;
  nib.addr = 0x200 + nib.devno[5];
  nib.activated = 0; // 未激活
  nib.tricseq = 0;
  nib.crc = crc16_data((uint8_t*)&nib, sizeof(struct NIB)-2, 0x0000);
  nv_write(NV_NIB_ADDR, (uint8_t*)&nib, sizeof(struct NIB));

  // 重置PIB
  memset(&pib, 0x00, sizeof(struct PIB));
  nv_write(NV_PIB_ADDR, (uint8_t*)&pib, sizeof(struct PIB));

  // 重置ALGO
  memset(&algo, 0x00, sizeof(struct ALGO));
  nv_write(NV_ALGO_ADDR, (uint8_t*)&algo, sizeof(struct ALGO));

  // 清除存储消息
  memset(&msgs, 0x00, sizeof(struct MSGS));
  nv_write(NV_MSGS_ADDR, (uint8_t*)&msgs, sizeof(struct MSGS)); // 恢复出厂设置时写元数据

  // 重置CFS
  cfs_factory_reset();

  log_i(I_FACTORY_RESET, NULL, 0); // 已恢复出厂设置
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
app_log_reboot(void)
{
  uint8_t arg[26] = {0};
  uint8_t i = 0;
  memcpy(arg+i, node_mac, MAC_LEN); i += MAC_LEN;
  arg[i++] = nib.hwver;
  arg[i++] = nib.fwver;
  memcpy(arg+i, APP_COMMIT, 7); i += 7;
  memcpy(arg+i, APP_BUILD, 8); i += 8;
  arg[i++] = pib.radioPower;
  log_i(I_REBOOT, arg, i); // 节点重启
}

//static void
//app_log_save(uint16_t idx, uint8_t cmdop, uint8_t *ts)
//{
//  uint8_t arg[9] = {0};
//  uint8_t i = 0;
//  arg[i++] = (idx >> 8);
//  arg[i++] = (idx & 0xff);
//  arg[i++] = cmdop;
//  memcpy(arg+i, ts, TSTAMP_LEN); i += TSTAMP_LEN;
//  log_i(0, arg, i);
//}
/*---------------------------------------------------------------------------*/
void
app_exit(void)
{
  mesh_close(&mesh);
  trickle_close(&tric);
  unicast_close(&unic);
  netcmd_close();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(device_process, ev, data)
{
  static struct etimer et; // 公用定时器
  static uint8_t et_r = 0;
  static struct etimer hb; // 心跳定时器
  static uint8_t hb_r = 0;
  static struct etimer st; // 采样定时器
  static uint8_t st_r = 0;
  static struct etimer hc; // 自检定时器
  static uint8_t hc_r = 0;

  PROCESS_EXITHANDLER(app_exit())
  PROCESS_BEGIN();


  logger_start(); // 启动运行日志模块
  app_show_reboot();
  leds_off(LEDS_ALL);

  enterS(S_START);
  app_init(); // 主线程的初始化
  Variant_Init(); // 算法模块初始化
  radio_set_channel(pib.radioChan); // 设置射频信道
  radio_set_txpower(pib.radioPower); // 设置射频功率
  NETSTACK_MAC.off(0); // 关闭射频

  // 初始化发送队列
  memb_init(&app_msg_mem);
  list_init(app_send_queue);

  app_log_reboot(); // 节点重启
  //log_i(I_SAVED_MSG_COUNT, (uint8_t*)&msgs.count, 2); // 已存储消息的个数

  // 启动磁场采样定时器
  etimer_set(&st, (CLOCK_SECOND >> 6)); st_r = 1;
  task_begin(sample, (CLOCK_SECOND));
  task_begin(sensor, (CLOCK_SECOND << 2));
#if WITH_TASKMON
  taskmon_init_end();
#endif /* WITH_TASKMON */

  // 如果已激活，开始后续动作，否则等待激活
  if (nib.activated) { // 已激活
    Set_Activated_Flag(1); // 告知算法模块跳过激活
    enterS(S_ACTED); // 标记进入已激活状态
    process_poll(&device_process); // 请求主线程后续动作
  }
  else {
    park_s = 2;
    Set_Activated_Flag(0); // 告知算法模块等待激活
    Set_Activated_Callback(app_activated); // 设置检测到激活磁信号时的回调函数
    log_i(I_VD_WAIT_ACT, NULL, 0); // 等待初次激活
  }

#if WITH_ATCMD
  atcmd_start(); // 启动AT指令模块
  uart_set_input(atcmd_input); // 设置串口输入由AT模块处理
  atcmd_set_output(uart_writeb); // 设置AT模块反馈从串口输出
  atcmd_register("MAC?", app_get_devno); // 注册查询设备编码的回调函数
  atcmd_register("MAC=", app_set_devno); // 注册设置设备编码的回调函数
  atcmd_register("CHAN?", app_get_channel); // 注册查询射频信道的回调函数
  atcmd_register("CHAN=", app_set_channel); // 注册设置射频信道的回调函数
  atcmd_register("POWER?", app_get_rfpower); // 注册查询射频功率的回调函数
  atcmd_register("POWER=", app_set_rfpower); // 注册设置射频功率的回调函数
  atcmd_register("ACT?", app_get_activate); // 注册查询节点是否已激活的回调函数
  atcmd_register("ACT=", app_set_activate); // 注册激活节点的回调函数
  atcmd_register("FRESET", app_factory_reset); // 注册恢复出厂设置的回调函数
  atcmd_register("REBOOT", app_reboot); // 注册重启节点的回调函数
  atcmd_register("UPDATE", app_update); // 注册进入串口升级模式的回调函数
  atcmd_output("VD started\r\n", 12);
#else
#if DEBUG_VD
  printf("VD started\n");
#endif
#endif


  while(1) {
    PROCESS_WAIT_EVENT();

    if (ev == PROCESS_EVENT_POLL) {
      if (app_s == S_START) {
        log_i(I_VD_BACK_SLEEP, NULL, 0); // 未激活返回出厂休眠
        app_in_activate = 0;
        Variant_Init();
        Set_Activated_Flag(0); // 告知算法模块等待激活
        radio_off(app_fae_rs); // 关闭射频case2: 激活未完成
        netcmd_close();
        etimer_stop(&et); et_r = 0; // 停止激活请求定时器
        ctimer_stop(&app_act_ct);
        task_end(act); // 激活失败
      }
      else if (app_s == S_PREACT) {
        radio_on(app_fae_rs, APP_FAECMD_WAIT); // 开启射频case2: 激活VD开始
        app_keep_radio_on = 1; // 激活VD

        unicast_open(&unic, APP_UNICAST_CHANNEL, &unic_cb); unic_opened = 1; // 打开fae连接
        netcmd_open(0, pib.commandT, &netc_cb); // 打开netcmd连接

        etimer_set(&et, (CLOCK_SECOND >> 6)); et_r = 1; // 启动激活请求定时器
        ctimer_set(&app_radio_ct, APP_FAECMD_WAIT, app_no_command, NULL); // 启动超时
        ctimer_set(&app_act_ct, (APP_FAECMD_WAIT + CLOCK_SECOND), app_fail_activate, NULL);
      }
      else if (app_s == S_ACTED) {
        mesh_open(&mesh, APP_MESH_CHANNEL, &mesh_cb); // 打开mesh连接
        trickle_open(&tric, (CLOCK_SECOND), APP_TRICKLE_CHANNEL, &tric_cb); // 打开trickle连接
        trickle_set_max_interval(&tric, pib.paramT);
        route_set_lifetime(3600); // 设置路由过期时间
        netcmd_open(1, pib.commandT, &netc_cb); // 打开netcmd连接
        netcmd_req_attach(app_netcmd_req_attach, APPMSG_HEADER_LEN + sizeof(struct app_data_vd_faeconn) + APPMSG_FOOTER_LEN);
        netcmd_set_period(0); // 正常运行，不主动发送req
        PRINTF("app start\n");

        // 停止激活请求定时器
        etimer_stop(&et); et_r = 0;

        // 发送全网参数以获取可能的新参数
        tric.seqno = nib.tricseq;
        packetbuf_copyfrom(&pib, sizeof(struct PIB));
        trickle_send(&tric);

#if !HBEAT_READY
        // 启动心跳定时器
        etimer_set(&hb, (CLOCK_SECOND * 2)); hb_r = 1;
        task_begin(hbeat, (CLOCK_SECOND * 3));
#endif

        // 启动自检定时器
        etimer_set(&hc, (CLOCK_SECOND * 60)); hc_r = 1;

        // 标记进入入网状态
        enterS(S_JOIN);
        process_poll(&device_process);

#if WITH_OTA
        // 如果之前更新未完成，重新启动
        if (ota.images[OBJ_DEVICE].pend != 0) {
          deluge_start(&deluge_cb);
          deluge_disseminate(OBJ_DEVICE, "VD", ota.images[OBJ_DEVICE].ver, ota.images[OBJ_DEVICE].mode, (uint8_t*)deluge_all);
        }
#endif /* WITH_OTA */
      }
      else if (app_s == S_JOIN) {
        if (prev_s == S_ACTED) {
          PRINTF("app start join\n");
          reconn_n = 0;
          etimer_set(&et, (CLOCK_SECOND)); et_r = 1; // 启动入网定时器
          task_begin(join, (CLOCK_SECOND * 2));
        }
        else if (prev_s == S_READY) {
          PRINTF("app restart join\n");

#if HBEAT_READY
          // 停止心跳定时器
          etimer_stop(&hb); hb_r = 0;
          ctimer_stop(&app_hb_ct);
          task_end(hbeat);
#endif

          // 启动入网定时器
          reconn_n = 0;
          etimer_set(&et, (CLOCK_SECOND >> 6)); et_r = 1;
        }
      }
      else if (app_s == S_READY) {
        if (prev_s == S_JOIN) {
          PRINTF("app ready\n");
          task_end(join);

          // 停止入网定时器
          etimer_stop(&et); et_r = 0;

#if HBEAT_READY
          // 启动心跳定时器
          etimer_set(&hb, (CLOCK_SECOND / 1)); hb_r = 1;
          task_begin(hbeat, (CLOCK_SECOND * 2));
#endif

          // 如果有已存储消息，进行发送
          app_send_next(NULL);
        }
        else {
          log_w(E_STATE, &prev_s, 1);
        }
      }
      else if (app_s == S_SLEEP) {
        // 进入休眠状态，关闭所有定时器
        etimer_stop(&et); et_r = 0;
        etimer_stop(&hb); hb_r = 0;
        etimer_stop(&st); st_r = 0;
        etimer_stop(&hc); hc_r = 0;
      }
      else {
        log_w(E_STATE, &app_s, 1);
      }
    }
    else if (ev == addr_changed_event) {
      PRINTF("app rejoin at addr change\n");
      // 重新启动入网定时器
      etimer_set(&et, (CLOCK_SECOND >> 6)); et_r = 1;

      // 以新的短地址发送请求通知激活节点更改短地址
      if (unic_opened)
        app_send_activate_req();
    }
    else if (ev == data_ready_event) {
      uint8_t status = 0;
      //printf("n:%lu, x:%d, y:%d, z:%d\n", unixtime_now(), One_Sample.x, One_Sample.y, One_Sample.z);
#if DEBUG_VD
      printf("x:%d, y:%d, z:%d, s:%d\n", One_Sample.x, One_Sample.y, One_Sample.z, park_s);
#endif
      /*status
       *parking space status
       *0 is vacant
       *1 is occupance
       *2 待激活节点
       *3 初始化
       *4 待复位
       *5 待激活RF
       */
      dint();
      status = Parking_Algorithm(); // 检测结果交算法模块处理
      eint();

      if (status != park_s) { // 算法检测结果与上次采样后结果不同
        PRINTF("app parkevt %d\n", status);
        park_s = status; // 记录新的泊位状态
        //printf("x:%d, y:%d, z:%d, s:%d\n", One_Sample.x, One_Sample.y, One_Sample.z, park_s);
        if ((app_s == S_READY) || (app_s == S_JOIN)) {
          if (park_s != 2) {
            app_send_parkevt(park_s); // 发送泊位事件消息
          }
        }
      }
    }
    else if (ev == pib_changed_event) { // 全网参数发生变化
      if (rcvpib.vdHbeatT != pib.vdHbeatT) {
        etimer_stop(&hb);
        etimer_set(&hb, (CLOCK_SECOND >> 6)); // 重新启动心跳定时器
      }
      if (rcvpib.healthT != pib.healthT) {
        etimer_stop(&hc);
        etimer_set(&hc, (CLOCK_SECOND >> 6)); // 重新启动自检定时器
      }
      //if (rcvpib.commandT != pib.commandT) {
      //  netcmd_set_period(rcvpib.commandT); // 设置下行命令轮询周期
      //}
      if (rcvpib.paramT != pib.paramT) {
        trickle_set_max_interval(&tric, rcvpib.paramT); // 设置全网参数轮询周期
      }
      if (rcvpib.radioPower != pib.radioPower) {
        radio_on(app_tric_rs, CLOCK_SECOND); // 开启射频case4: 设置射频参数
        radio_set_txpower(rcvpib.radioPower); // 设置射频功率
        radio_off(app_tric_rs); // 关闭射频case4: 完成射频设置
      }

      // 将新的全网参数保存到Flash
      memcpy(&pib, &rcvpib, sizeof(struct PIB));
      pib.crc = crc16_data((uint8_t*)&pib, sizeof(struct PIB)-2, CRC_INIT);
      nv_write(NV_PIB_ADDR, (uint8_t*)&pib, sizeof(struct PIB));

      // 保存全网参数的序号到Flash
      nib.tricseq = tric.seqno - 1;
      nib.crc = crc16_data((uint8_t*)&nib, sizeof(struct NIB)-2, 0x0000);
      nv_write(NV_NIB_ADDR, (uint8_t*)&nib, sizeof(struct NIB));
    }

    // 公用定时器到时
    if (et_r && etimer_expired(&et)) {
      if (app_s == S_PREACT) {
        if (app_in_activate == 0) {
          etimer_set(&et, (CLOCK_SECOND * 10)); // 为下一次发送激活请求定时
          app_send_activate_req();
        }
      }
      else if (app_s == S_JOIN) { // 入网状态
        clock_time_t period = ((uint32_t)CLOCK_SECOND * RECONN_INTERVAL());
        etimer_set(&et, period); // 为下一次发送入网消息定时
        if (reconn_n < sizeof(reconn_ints))
          ++reconn_n;

        task_end(join);
        task_begin(join, (period + (CLOCK_SECOND >> 4)));

        PRINTF("app retry join\n");
        app_send_join(); // 发送入网消息
      }
      else {
        log_w(E_STATE, &app_s, 1);
      }
    }

    // 心跳定时器到时
    if (hb_r && etimer_expired(&hb)) {
      // 为下一次心跳定时
      etimer_set(&hb, (CLOCK_SECOND * pib.vdHbeatT));
      task_end(hbeat);
      task_begin(hbeat, (CLOCK_SECOND * (pib.vdHbeatT + 1)));

      // 调用心跳发送任务发送心跳消息
      ++app_hbeat_seq;
      app_hb_retry = APP_HBEAT_MAX_RETRY; // 设置最多心跳发送次数
      ctimer_set(&app_hb_ct, (CLOCK_SECOND >> 6), app_hbeat_task, NULL);
    }

    // 采样定时器到时
    if (st_r && etimer_expired(&st)) {
      etimer_set(&st, sampleT); // 为下一次采样定时
      task_begin(sensor, (CLOCK_SECOND << 2));
      task_end(sample);
      task_begin(sample, (CLOCK_SECOND << 2));

      //PRINTF("app sample\n");
#if WITH_MMC3316
      MMC3316_Read(SENSOR_PORT); // 进行磁场采样

#elif WITH_HMC5983
      if(reset_flag)
      {
        hmc5983_self_test();
        reset_flag=false;
        continue;
      }
      else
      {
        hmc5983_sample_read(algo.gain_hmc5983);
      }

      //gain test
      // 如果gain改变，则丢掉一次采样
      // gain改变，需要重新计算数字分辨率digital_resolution
      if(hist_gain!=algo.gain_hmc5983)
      {
        hist_gain=algo.gain_hmc5983;
#if UNIT_MILLI_GAUSS==1
        dig_resolution_change();
#endif
        continue; //丢掉一个包
      }

#elif WITH_QMC5883
      if(reset_flag)
      {
        qmc5883_self_test();
        reset_flag=false;
        continue;
      }
      else
      {
        qmc5883_sample_read(algo.gain_hmc5983);
      }

      //gain test
      // 如果gain改变，则丢掉一次采样
      // gain改变，需要重新计算数字分辨率digital_resolution
      if(hist_gain!=algo.gain_hmc5983)
      {
        hist_gain=algo.gain_hmc5983;
#if UNIT_MILLI_GAUSS==1
        dig_resolution_change();
#endif
        continue; //丢掉一个包
      }

#else
#error "no support"
#endif
    }

    // 自检定时器到时
    if (hc_r && etimer_expired(&hc)) {
      etimer_set(&hc, (CLOCK_SECOND * pib.healthT)); // 为下一次自检定时

      app_check_health(); // 检查节点状态
    }

  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
