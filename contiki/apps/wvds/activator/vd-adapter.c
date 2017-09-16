#include "contiki.h"
#include "net/rime.h"
#include "net/rime/rimeaddr.h"
#include "net/rime/unicast.h"
#include "net/packetbuf.h"
#include "net/netcmd.h"
#include "lib/crc16.h"
#include "sys/unixtime.h"
#include "sys/logger.h"
#include "cc11xx.h"
#include "app.h"
#include <string.h>
#include <stddef.h>

extern uint8_t rf_work_chan;

static struct app_msg txmsg;
static struct netcmd_cmd txcmd;
static uint16_t cmdseq = 1;

const uint8_t devmac_null[DEVMAC_LEN] = {0};
static uint8_t vd_devmac[DEVMAC_LEN] = {0};
static rimeaddr_t vd_addr = {{0,0}};
static uint8_t vd_connected = 0;

static struct app_data_algo vd_algo;

static process_event_t activate_event = 0;
static process_event_t maintain_event = 0;

static struct unicast_conn unicast_c;
static struct unicast_conn netcmd_c;
/*------------------------------------------------------------------*/
PROCESS(vdadapter_process, "VD Adapter");
/*------------------------------------------------------------------*/
void
vda_set_target(uint8_t *devmac)
{
  memcpy(vd_devmac, devmac, DEVMAC_LEN);
}

/*------------------------------------------------------------------*/
static void
vda_netcmd_send(rimeaddr_t *to, struct app_msg *msg, uint8_t len)
{
  struct netcmd_cmd *cmd = (struct netcmd_cmd *)&txcmd;

  cmd->type = NETCMD_CMD;
  cmd->more = 0; // 表示无后续消息(不要更改)
  cmd->seqno = cmdseq;
  rimeaddr_copy(&cmd->dest, to);
  cmd->subid = APP_WVDS; // 表示data部分是WVDS数据帧即app_msg
  cmd->len = len;
  memcpy(cmd->data, msg, len);

  if (++cmdseq == 0)
    cmdseq = 1;

  len += offsetof(struct netcmd_cmd, data);
  packetbuf_copyfrom((uint8_t*)cmd, len);

  unicast_send(&netcmd_c, to);
}

static void
vda_netcmd_send_cmd(rimeaddr_t *to, uint8_t *devmac, uint8_t cmdop, uint8_t *data, uint8_t len)
{
  struct app_msg *msg = (struct app_msg *)&txmsg;
  struct app_msg_header *header = (struct app_msg_header *)msg;
  struct app_msg_footer *footer = (struct app_msg_footer *)(msg->data + len);
  uint16_t ccrc = 0;

  header->beg = APPMSG_BEG;
  header->len = DEVMAC_LEN + 1 + len;
  memcpy(header->devmac, devmac, DEVMAC_LEN);
  header->cmdop = cmdop;
  header->rep = 1;
  header->dir = APPMSG_DOWN;

  memcpy(msg->data, data, len);

  ccrc = crc16_data(&header->len, (APPMSG_HEADER_LEN - 1 + header->len), 0x0000);
  footer->crc[0] = (ccrc >> 8);
  footer->crc[1] = (ccrc & 0xff);

  len = APPMSG_HEADER_LEN + len + APPMSG_FOOTER_LEN;
  vda_netcmd_send(to, msg, len);
}

static void
vda_netcmd_send_algo_get(rimeaddr_t *to, uint8_t *devmac)
{
  struct app_data_algo algo;

  memset(&algo, 0, sizeof(struct app_data_algo));
  unixtime_get(algo.tstamp);
  algo.subop = APP_GET;

  vda_netcmd_send_cmd(to, devmac, APPDATA_ALGO_PARAM, (uint8_t*)&algo, sizeof(struct app_data_algo));
}

static void
vda_netcmd_send_algo_set(rimeaddr_t *to, uint8_t *devmac, struct app_data_algo *algo)
{
  unixtime_get(algo->tstamp);
  algo->subop = APP_SET;

  vda_netcmd_send_cmd(to, devmac, APPDATA_ALGO_PARAM, (uint8_t*)algo, sizeof(struct app_data_algo));
}

static void
vda_netcmd_send_reinit(rimeaddr_t *to, uint8_t *devmac)
{
  struct app_data_reinit req;

  unixtime_get(req.tstamp);

  vda_netcmd_send_cmd(to, devmac, APPDATA_VD_REINIT, (uint8_t*)&req, sizeof(struct app_data_reinit));
}

static void
vda_netcmd_send_disconn(rimeaddr_t *to, uint8_t *devmac, uint8_t chan)
{
  struct app_data_vd_disconn req;

  unixtime_get(req.tstamp);
  req.subop = 1;
  req.channel = chan;

  vda_netcmd_send_cmd(to, devmac, APPDATA_VD_DISCONN, (uint8_t*)&req, sizeof(struct app_data_vd_disconn));
}

static void
vda_netcmd_send_disconn_ack(rimeaddr_t *to, uint8_t *devmac)
{
  struct app_data_vd_disconn req;

  unixtime_get(req.tstamp);
  req.subop = 2;

  vda_netcmd_send_cmd(to, devmac, APPDATA_VD_DISCONN, (uint8_t*)&req, sizeof(struct app_data_vd_disconn));
}

static void
vda_netcmd_send_resp(rimeaddr_t *from, struct app_msg *req, uint8_t *resp, uint8_t resp_len)
{
  struct app_msg *msg = (struct app_msg *)&txmsg;
  struct app_msg_header *header = (struct app_msg_header *)msg;
  struct app_msg_footer *footer = (struct app_msg_footer *)(msg->data + resp_len);
  uint16_t ccrc = 0;
  uint8_t len = APPMSG_HEADER_LEN + resp_len + APPMSG_FOOTER_LEN;

  header->beg = APPMSG_BEG;
  header->len = DEVMAC_LEN + 1 + resp_len;
  memcpy(header->devmac, req->header.devmac, DEVMAC_LEN);
  header->cmdop = req->header.cmdop;
  header->rep = 0;
  header->dir = APPMSG_DOWN;

  memcpy(msg->data, resp, resp_len);

  ccrc = crc16_data(&header->len, (APPMSG_HEADER_LEN - 1 + header->len), 0x0000);
  footer->crc[0] = (ccrc >> 8);
  footer->crc[1] = (ccrc & 0xff);

  vda_netcmd_send(from, msg, len);
}

static void
vda_netcmd_send_activate_resp(rimeaddr_t *from, struct app_msg *msg)
{
  uint8_t buf[sizeof(struct app_data_vd_activate_resp)];
  struct app_data_vd_activate *req = (struct app_data_vd_activate *)msg->data;
  struct app_data_vd_activate_resp *resp = (struct app_data_vd_activate_resp *)buf;

  memcpy(resp->tstamp, req->tstamp, TSTAMP_LEN);
  resp->result = 0;

  vda_netcmd_send_resp(from, msg, buf, sizeof(struct app_data_vd_activate_resp));
}

static void
vda_netcmd_send_faeconn_resp(rimeaddr_t *from, struct app_msg *msg)
{
  struct app_data_vd_faeconn *req = (struct app_data_vd_faeconn *)msg->data;
  struct app_data_vd_faeconn_resp resp = {0};

  memcpy(resp.tstamp, req->tstamp, TSTAMP_LEN);
  resp.result = 0;

  vda_netcmd_send_resp(from, msg, (uint8_t*)&resp, sizeof(struct app_data_vd_faeconn_resp));
}

/*------------------------------------------------------------------*/
static void
vda_unicast_rcvd(struct unicast_conn *c, const rimeaddr_t *from)
{
  struct app_msg *msg = (struct app_msg *)packetbuf_dataptr();

  if (memcmp(vd_devmac, devmac_null, DEVMAC_LEN) == 0) {
    if (msg->header.cmdop == APPDATA_VD_ACTIVATE) {
      memcpy(vd_devmac, msg->header.devmac, DEVMAC_LEN);
      vd_connected = 1;
    } else {
      return;
    }
  } else {
    if (memcmp(msg->header.devmac, vd_devmac, DEVMAC_LEN) != 0) {
      return;
    }
  }

  // 更新当前VD的短地址(激活入网时短地址会由AP分配而改变)
  if (!rimeaddr_cmp(&vd_addr, from)) {
    rimeaddr_copy(&vd_addr, from);
  }

  if (msg->header.cmdop == APPDATA_VD_ACTIVATE) {
    //struct app_data_vd_activate *req = (struct app_data_vd_activate *)msg->data;
    // TODO: 通知APP激活

    vda_netcmd_send_activate_resp(&vd_addr, msg);
    process_post(&vdadapter_process, activate_event, NULL);
  }
  else if (msg->header.cmdop == APPDATA_VD_REINIT) {
    // TODO: 通知APP标定结果
  }
  else if (msg->header.cmdop == APPDATA_ALGO_PARAM) {
    struct app_data_algo_resp *resp = (struct app_data_algo_resp *)msg->data;
    if (resp->subop == APP_GET) {
      memcpy(((uint8_t*)&vd_algo) + offsetof(struct app_data_algo, normalT),
          ((uint8_t*)resp) + offsetof(struct app_data_algo_resp, normalT),
          sizeof(struct app_data_algo) - offsetof(struct app_data_algo, normalT));
    }
    // TODO: 通知APP参数查询或配置结果
  }
  else if (msg->header.cmdop == APPDATA_VD_DISCONN) {
    struct app_data_vd_disconn_resp *resp = (struct app_data_vd_disconn_resp *)msg->data;
    // TODO: 通知APP已断开连接

    if (resp->result == 0) { // 收到VD的断开确认
      vd_connected = 0;
      memset(vd_devmac, 0, DEVMAC_LEN);
      rimeaddr_copy(&vd_addr, &rimeaddr_null);
    }
    else if (resp->result == 1) { // VD主动因超时断开
      vd_connected = 0;
      vda_netcmd_send_disconn_ack(&vd_addr, vd_devmac);
      rimeaddr_copy(&vd_addr, &rimeaddr_null);
      memset(vd_devmac, 0, DEVMAC_LEN);
    }
  }
}

static const struct unicast_callbacks unicast_cb = {vda_unicast_rcvd};
/*------------------------------------------------------------------*/
static void
vda_netcmd_rcvd(struct unicast_conn *c, const rimeaddr_t *from)
{
  struct netcmd_req *req = (struct netcmd_req *)packetbuf_dataptr();
  struct app_msg *msg = (struct app_msg *)req->data;

  if (req->type != NETCMD_REQ)
    return; // 忽略类型不对的netcmd消息

  if (memcmp(msg->header.devmac, vd_devmac, DEVMAC_LEN) != 0) {
    log_w(E_INVAL, vd_devmac, 6); return;
  }

  if (!rimeaddr_cmp(&vd_addr, from)) {
    rimeaddr_copy(&vd_addr, from);
  }

  if (msg->header.cmdop == APPDATA_VD_FAECONN) {
    //struct app_data_vd_faeconn *conn = (struct app_data_vd_faeconn *)msg->data;

    if (!vd_connected) {
      vd_connected = 1;
      // TODO: 通知APP连接建立

      // 向VD发送响应
      vda_netcmd_send_faeconn_resp(&vd_addr, msg);
      process_post(&vdadapter_process, maintain_event, NULL);
    }
  }
}

static const struct unicast_callbacks netcmd_cb = {vda_netcmd_rcvd};
/*------------------------------------------------------------------*/
static void
vda_sniff_rcvd(void)
{
  uint16_t chan = (uint16_t)packetbuf_attr(PACKETBUF_ATTR_CHANNEL);
  const rimeaddr_t *from = (rimeaddr_t*)packetbuf_addr(PACKETBUF_ADDR_SENDER);

  if (chan == APP_NETCMD_CHANNEL) {
    vda_netcmd_rcvd(&netcmd_c, from);
  }
}

static void
vda_sniff_sent(int mac_status)
{
}

RIME_SNIFFER(vda_sniff, vda_sniff_rcvd, vda_sniff_sent);
/*------------------------------------------------------------------*/
static void
vda_exit(void)
{
  unicast_close(&unicast_c);
  unicast_close(&netcmd_c);
}
/*------------------------------------------------------------------*/
PROCESS_THREAD(vdadapter_process, ev, data)
{
  static rimeaddr_t addr;
  static struct etimer et;
  uint8_t test[DEVMAC_LEN] = {0x01,0xCA,0x17,0x03,0x00,0x03};

  PROCESS_EXITHANDLER(vda_exit());
  PROCESS_BEGIN();

  activate_event = process_alloc_event();
  maintain_event = process_alloc_event();

  addr.u8[0] = 2; addr.u8[1] = 1;
  rimeaddr_set_node_addr(&addr);  // 设置激活节点自己的短地址为258

  unicast_open(&unicast_c, APP_UNICAST_CHANNEL, &unicast_cb);
  unicast_open(&netcmd_c, APP_NETCMD_CHANNEL, &netcmd_cb);
  //vda_set_target(test);

  cc11xx_set_promiscuous(1); // 接收目标地址不是自己的数据包
  rime_sniffer_add(&vda_sniff);

  while (1) {
    PROCESS_WAIT_EVENT();

    if (ev == activate_event) {
      etimer_set(&et, (CLOCK_SECOND * 5));
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
      vda_netcmd_send_reinit(&vd_addr, vd_devmac);

      etimer_set(&et, (CLOCK_SECOND * 48));
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
      vda_netcmd_send_disconn(&vd_addr, vd_devmac, rf_work_chan);
#if 0
      etimer_set(&et, (CLOCK_SECOND * 10));
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
      vda_set_target(test); log_i(0, NULL, 0);
#endif
    }

    if (ev == maintain_event) {
      etimer_set(&et, (CLOCK_SECOND * 2));
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
      vda_netcmd_send_algo_get(&vd_addr, vd_devmac);

      etimer_set(&et, (CLOCK_SECOND * 2));
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
      vd_algo.bigOccThr += 2;
      vda_netcmd_send_algo_set(&vd_addr, vd_devmac, &vd_algo);

      etimer_set(&et, (CLOCK_SECOND * 2));
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
      vda_netcmd_send_algo_get(&vd_addr, vd_devmac);
#if 1
      etimer_set(&et, (CLOCK_SECOND * 2));
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
      vda_netcmd_send_reinit(&vd_addr, vd_devmac);
      etimer_set(&et, (CLOCK_SECOND * 60));
#else
      etimer_set(&et, (CLOCK_SECOND * 2));
#endif
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
      vda_netcmd_send_disconn(&vd_addr, vd_devmac, rf_work_chan);

      etimer_set(&et, (CLOCK_SECOND * 10));
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
      vda_set_target(test); log_i(0, NULL, 0);
    }
  }

  PROCESS_END();
}
