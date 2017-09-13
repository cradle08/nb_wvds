#include "contiki.h"
#include "node-id.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "net/rime.h"
#include "net/rime/broadcast.h"
#include "net/rime/unicast.h"
#include "net/rime/rimeaddr.h"
#include "net/rime/route.h"
#include "net/neighbor.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "sys/logger.h"
#include "dev/radio.h"
#include "dev/leds.h"
#include "netcmd.h"
#include "app.h"

#include <stddef.h>
#include <string.h>

#if WITH_TASKMON
#include "sys/taskmon.h"
#endif /* WITH_TASKMON */

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#if DEBUG
static char *MSG[4] = {"", "REQ", "CMD", "ACK"};
static char *STATE[6] = {"IDLE","SEND_REQ","SEND_CMD","SEND_ACK","WAIT_CMD","WAIT_ACK"};
#endif

// 是否广播发送REQ，定义为1时广播，为0时单播
#define NETCMD_BROADCAST 1

// REQ消息广播时的最大重发次数
#define NETCMD_REQ_REXMIT 1

#define NETCMD_CMD_WAIT (PACKET_TIME << 2)
#define NETCMD_ACK_WAIT (PACKET_TIME << 1)
#define NETCMD_SENT_TO  (PACKET_TIME << 2)

static uint8_t netcmd_lpm = 0;
static uint16_t netcmd_seq = 1;
#if WITH_LPM
static uint8_t netcmd_radio_off = 1;
static struct radio_res *netcmd_rs = NULL;
#endif
#if NETCMD_REQ_REXMIT > 1
static uint8_t netcmd_req_retry = 0;
#endif
static struct cmd_item *netcmd_tx = NULL;
static rimeaddr_t netcmd_tx_to = {{0,0}};
static struct ctimer netcmd_ct;
static uint8_t netcmd_wait = 0;
static struct netcmd_msg netcmd_rx_msg;
static rimeaddr_t netcmd_rx_from = {{0,0}};
static rimeaddr_t netcmd_dl_from = {{0,0}};
static uint16_t netcmd_period = 60;
#if WITH_OTA
static uint8_t nib_fwver = 0;
#endif /* WITH_OTA */

static process_event_t netcmd_rcvd_event = 0;
static process_event_t netcmd_period_change = 0;

enum {
  S_IDLE     = 0,
  S_SEND_REQ = 1,
  S_SEND_CMD = 2,
  S_SEND_ACK = 3,
  S_WAIT_CMD = 4,
  S_WAIT_ACK = 5,
};
static uint8_t netcmd_s = S_IDLE;
#define enterS(x) do { \
  PRINTF("netcmd s%d->s%d at %d\n", netcmd_s, (x), __LINE__); \
  netcmd_s = (x); \
} while(0)

struct reqcmd_item {
  struct reqcmd_item *next;
  struct cmd_item *cmd;
};
MEMB(netcmd_reqcmdm, struct reqcmd_item, (NETCMD_REQQ_MAXLEN * NETCMD_CMDQ_MAXLEN));
#if DEBUG
static uint8_t netcmd_reqcmdn = NETCMD_REQQ_MAXLEN * NETCMD_CMDQ_MAXLEN;
#endif

struct req_item {
  struct req_item *next;
  rimeaddr_t addr;
  uint8_t count;
  struct ctimer ct;
  LIST_STRUCT(cmds);
};
MEMB(netcmd_reqm, struct req_item, NETCMD_REQQ_MAXLEN);
LIST(netcmd_reqq);
#if DEBUG
static uint8_t netcmd_reqn = NETCMD_REQQ_MAXLEN;
#endif

struct ack_item {
  struct ack_item *next;
  rimeaddr_t addr;
};
MEMB(netcmd_ackm, struct ack_item, (NETCMD_CMDQ_MAX_UCAST + NETCMD_CMDQ_MAX_BCAST * NEIGHBORS_NUM));
#if DEBUG
static uint8_t netcmd_ackn = (NETCMD_CMDQ_MAX_UCAST + NETCMD_CMDQ_MAX_BCAST * NEIGHBORS_NUM);
#endif

struct cmd_item {
  struct cmd_item *next;
  struct netcmd_msg cmd;
  LIST_STRUCT(acks);
};
MEMB(netcmd_cmdm, struct cmd_item, NETCMD_CMDQ_MAXLEN);
LIST(netcmd_cmdq);
#if DEBUG
static uint8_t netcmd_cmdn = NETCMD_CMDQ_MAXLEN;
#endif

#define NETCMD_CACHE_NUM 4
struct seq_item {
  struct seq_item *next;
  uint16_t seqno;
};
MEMB(netcmd_seqm, struct seq_item, NETCMD_CACHE_NUM);
LIST(netcmd_seqc);

static netcmd_req_attach_func_t netcmd_req_attach_func = NULL;
static uint8_t netcmd_req_attach_len = 0;

static struct netcmd_callback *netcmd_cb;
struct unicast_conn netcmd_uc;

#if WITH_TASKMON
struct taskmon *netcmd = NULL;
#endif /* WITH_TASKMON */
/*---------------------------------------------------------------------------*/
static struct neighbor * netcmd_get_nbr(rimeaddr_t *to);
static int netcmd_enq(struct cmd_item *cmde, rimeaddr_t *from);
static struct cmd_item * netcmd_pop_cmd(void);

static void netcmd_no_cmd(void *ptr);
static void netcmd_no_ack(void *ptr);
static void netcmd_no_sent(void *ptr);

static int netcmd_cache_has(uint16_t seqno);
static int netcmd_cache_add(uint16_t seqno);
/*---------------------------------------------------------------------------*/
PROCESS(netcmd_process, "Network command");
/*---------------------------------------------------------------------------*/
static void
print_cmdq(void)
{
#if DEBUG
  struct cmd_item *cmde = NULL;

  for (cmde = list_head(netcmd_cmdq); cmde != NULL; cmde = list_item_next(cmde)) {
    PRINTF("  %p, dest %d.%d, seq %d, id 0x%02X, ack %d, next %p\n", cmde, cmde->cmd.dest.u8[0], cmde->cmd.dest.u8[1], cmde->cmd.seqno, cmde->cmd.msgid, list_length(cmde->acks), cmde->next);
  }
#endif
}

static void
print_acks(list_t acks)
{
#if DEBUG
  struct ack_item *acke = NULL;

  for (acke = list_head(acks); acke != NULL; acke = list_item_next(acke)) {
    PRINTF("  %p, addr %d.%d\n", acke, acke->addr.u8[0], acke->addr.u8[1]);
  }
#endif
}
/*---------------------------------------------------------------------------*/
void
netcmd_sniff_rcvd(void)
{
  uint16_t chan = (uint16_t)packetbuf_attr(PACKETBUF_ATTR_CHANNEL);
  const rimeaddr_t *from = packetbuf_addr(PACKETBUF_ADDR_SENDER);
  const rimeaddr_t *to = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  struct netcmd_msg *cmd = NULL;
  struct netcmd_msg *ack = NULL;
  struct cmd_item *cmde = NULL;
  struct ack_item *acke = NULL;
  struct req_item *reqe = NULL;
  struct reqcmd_item *rce = NULL;
  int done = 0;

  if (chan == NETCMD_CHANNEL) {
    ack = (struct netcmd_msg *)packetbuf_dataptr();
    if (ack->type == NETCMD_ACK) {
      if (!rimeaddr_cmp(to, &rimeaddr_node_addr)) {
        PRINTF("netcmd sniff ACK %d from %d.%d to %d.%d\n", ack->seqno, from->u8[0], from->u8[1], to->u8[0], to->u8[1]);
        done = 0;
        for (reqe = list_head(netcmd_reqq); (!done && (reqe != NULL)); reqe = list_item_next(reqe)) {
          for (rce = list_head(reqe->cmds); (!done && (rce != NULL)); rce = list_item_next(rce)) {
            cmd = &rce->cmd->cmd;
            if ((ack->seqno == cmd->seqno) && (rimeaddr_cmp(&cmd->dest, from) || rimeaddr_cmp(&cmd->dest, &rimeaddr_null))) {
              list_remove(reqe->cmds, rce); PRINTF("netcmd reqcmd del %p\n", rce);
              memb_free(&netcmd_reqcmdm, rce); PRINTF("netcmd reqcmd free %p, %d left\n", rce, ++netcmd_reqcmdn);
              done = 1;
            }
          }
          if (list_length(reqe->cmds) == 0) {
            ctimer_stop(&reqe->ct);
            list_remove(netcmd_reqq, reqe); PRINTF("netcmd req deq %p/%d/%d.%d at %d, %d in reqq\n", reqe, reqe->count, reqe->addr.u8[0], reqe->addr.u8[1], __LINE__, list_length(netcmd_reqq));
            memb_free(&netcmd_reqm, reqe); PRINTF("netcmd req free %p at %d, %d left\n", reqe, __LINE__, ++netcmd_reqn);
          }
        }

        done = 0;
        for (cmde = list_head(netcmd_cmdq); (!done && (cmde != NULL)); cmde = list_item_next(cmde)) {
          cmd = &cmde->cmd;
          if (ack->seqno == cmd->seqno) {
            for (acke = list_head(cmde->acks); (!done && (acke != NULL)); acke = list_item_next(acke)) {
              if (rimeaddr_cmp(from, &acke->addr)) {
                list_remove(cmde->acks, acke); PRINTF("netcmd ack del %p/%d.%d\n", acke, acke->addr.u8[0], acke->addr.u8[1]);
                memb_free(&netcmd_ackm, acke); PRINTF("netcmd ack free %p at %d, %d left\n", acke, __LINE__, ++netcmd_ackn);
                done = 1;
              }
            }
            if (list_length(cmde->acks) == 0) {
              list_remove(netcmd_cmdq, cmde); PRINTF("netcmd cmd deq %p/%d/%d.%d at %d, %d in cmdq\n", cmde, cmde->cmd.seqno, cmde->cmd.dest.u8[0], cmde->cmd.dest.u8[1], __LINE__, list_length(netcmd_cmdq));
              memb_free(&netcmd_cmdm, cmde); PRINTF("netcmd cmd free %p at %d, %d left\n", cmde, __LINE__, ++netcmd_cmdn);
            }
          }
        }
      }
    }
  }
}

void
netcmd_sniff_sent(int mac_status)
{
}

RIME_SNIFFER(netcmd_sniff, netcmd_sniff_rcvd, netcmd_sniff_sent);
/*---------------------------------------------------------------------------*/
void
netcmd_send_req(void)
{
  struct netcmd_msg_req msg = {0};
#if !NETCMD_BROADCAST
  struct route_entry *re = NULL;
  rimeaddr_t to;
#endif
  uint8_t len = 0;
#if WITH_OTA
  struct NIB nib;

  if (nib_fwver == 0) {
    nv_read(NV_NIB_ADDR, (uint8_t*)&nib, sizeof(struct NIB));
    nib_fwver = nib.fwver;
  }
#endif /* WITH_OTA */

  msg.type = NETCMD_REQ;
  memcpy(msg.mac, node_mac, 8);
#if WITH_OTA
  msg.fwver = nib_fwver;
#endif /* WITH_OTA */
  if (netcmd_req_attach_func != NULL) {
    memcpy(msg.data, (uint8_t*)netcmd_req_attach_func(), netcmd_req_attach_len);
    len = offsetof(struct netcmd_msg_req, data) + netcmd_req_attach_len;
  } else {
    len = offsetof(struct netcmd_msg_req, data);
  }

#if !NETCMD_BROADCAST
  re = route_lookup(&rimeaddr_root);
  if (re != NULL) {
    rimeaddr_copy(&to, &re->nexthop);
  } else {
    PRINTF("warn netcmd no parent\n");
    rimeaddr_copy(&to, &rimeaddr_root);
  }
#endif

#if WITH_LPM
  if (netcmd_lpm && netcmd_radio_off) {
    PRINTF("netcmd on radio to send req\n");
    radio_on(netcmd_rs, (PACKET_TIME * 9));
    netcmd_radio_off = 0;
  }
#endif

  packetbuf_copyfrom((uint8_t*)&msg, len);
#if !NETCMD_BROADCAST
  PRINTF("netcmd send REQ to %d.%d\n", to->u8[0], to->u8[1]);
  unicast_send(&netcmd_uc, &to);
#else
  PRINTF("netcmd send REQ\n");
  unicast_send(&netcmd_uc, &rimeaddr_null);
#endif
}

static void
netcmd_prep_cmd(struct req_item *req)
{
  struct cmd_item *cmde = NULL;
  struct ack_item *acke = NULL;
  struct reqcmd_item *rce = NULL;
  rimeaddr_t *dest = &req->addr;

  PRINTF("netcmd prep for req %p/%d/%d.%d, cmdq %d\n", req, req->count, dest->u8[0], dest->u8[1], list_length(netcmd_cmdq));
  for (cmde = list_head(netcmd_cmdq); cmde != NULL; cmde = list_item_next(cmde)) {
    if (rimeaddr_cmp(&cmde->cmd.dest, dest)) {
      rce = memb_alloc(&netcmd_reqcmdm); if (rce != NULL) { PRINTF("netcmd reqcmd alloc %p at %d, %d left\n", rce, __LINE__, --netcmd_reqcmdn); }
      if (rce != NULL) {
        rce->cmd = cmde;
        rce->next = NULL;
        req->count++;
        list_add(req->cmds, rce);
        PRINTF("netcmd need send cmd %p/%d/%d.%d to %d.%d\n", cmde, cmde->cmd.seqno, cmde->cmd.dest.u8[0], cmde->cmd.dest.u8[1], dest->u8[0], dest->u8[1]);
      } else {
        PRINTF("warn netcmd reqcmdm empty at %d\n", __LINE__);
        log_w(E_FULL, NULL, 0);
      }
    }
    else if (rimeaddr_cmp(&cmde->cmd.dest, &rimeaddr_null)) {
      for (acke = list_head(cmde->acks); acke != NULL; acke = list_item_next(acke)) {
        if (rimeaddr_cmp(&acke->addr, dest)) {
          rce = memb_alloc(&netcmd_reqcmdm); if (rce != NULL) { PRINTF("netcmd reqcmd alloc %p at %d, %d left\n", rce, __LINE__, --netcmd_reqcmdn); }
          if (rce != NULL) {
            rce->cmd = cmde;
            rce->next = NULL;
            req->count++;
            list_add(req->cmds, rce);
            PRINTF("netcmd need send cmd %p/%d/%d.%d to %d.%d\n", cmde, cmde->cmd.seqno, cmde->cmd.dest.u8[0], cmde->cmd.dest.u8[1], dest->u8[0], dest->u8[1]);
          } else {
            PRINTF("warn netcmd reqcmdm empty at %d\n", __LINE__);
            log_w(E_FULL, NULL, 0);
          }
        }
      }
    }
  }
}

static void
netcmd_send_cmd(void *ptr)
{
  struct req_item *reqe = NULL;
  struct reqcmd_item *rce = NULL;
  struct cmd_item *cmde = NULL;
  struct ack_item *acke = NULL;
  struct neighbor *nbr = NULL;
  struct netcmd_msg *msg = NULL;
  int done = 0;

  PRINTF("netcmd send cmd, %d reqs\n", list_length(netcmd_reqq));
  for (reqe = list_head(netcmd_reqq); !done && (reqe != NULL); reqe = list_item_next(reqe)) {
    for (rce = list_head(reqe->cmds); !done && (rce != NULL); rce = list_item_next(rce)) {
      cmde = rce->cmd;
      nbr = netcmd_get_nbr(&(reqe->addr));
      if (nbr != NULL) {
        msg = &cmde->cmd;
        msg->more = ((reqe->count > 1) ? 1 : 0);

        rimeaddr_copy(&netcmd_tx_to, &nbr->addr);
        PRINTF("netcmd send CMD %p/%d/%d.%d to %d.%d\n", cmde, msg->seqno, msg->dest.u8[0], msg->dest.u8[1], netcmd_tx_to.u8[0], netcmd_tx_to.u8[1]);
        enterS(S_SEND_CMD);
        netcmd_tx = cmde;
        ctimer_set(&netcmd_ct, NETCMD_SENT_TO, netcmd_no_sent, NULL);
        packetbuf_copyfrom(msg, offsetof(struct netcmd_msg,msgdata) + cmde->cmd.msglen);
        unicast_send(&netcmd_uc, &netcmd_tx_to);
      } else {
        PRINTF("warn netcmd not know nexthop dest %d.%d\n", reqe->addr.u8[0], reqe->addr.u8[1]);
      }
      done = 1; // only send one packet
    }
  }

  if(!done) {
    for (cmde = list_head(netcmd_cmdq); (!done && (cmde != NULL)); cmde = list_item_next(cmde)) {
      for (acke = list_head(cmde->acks); (!done && (acke != NULL)); acke = list_item_next(acke)) {
        nbr = neighbor_find(&acke->addr);
        if ((nbr != NULL) && (nbr->role == NODE_RP)) {
          msg = &cmde->cmd;
          msg->more = 0;

          rimeaddr_copy(&netcmd_tx_to, &nbr->addr);
          PRINTF("netcmd send CMD %p/%d/%d.%d to %d.%d\n", cmde, msg->seqno, msg->dest.u8[0], msg->dest.u8[1], netcmd_tx_to.u8[0], netcmd_tx_to.u8[1]);
          enterS(S_SEND_CMD);
          netcmd_tx = cmde;
          ctimer_set(&netcmd_ct, NETCMD_SENT_TO, netcmd_no_sent, NULL);
          packetbuf_copyfrom(msg, offsetof(struct netcmd_msg,msgdata) + cmde->cmd.msglen);
          unicast_send(&netcmd_uc, &netcmd_tx_to);
          done = 1;
        }
      }
    }
  }
}

static void
netcmd_send_ack(rimeaddr_t *from, struct netcmd_msg *msg)
{
  struct netcmd_msg ack = {0};

  ack.type = NETCMD_ACK;
  ack.seqno = msg->seqno;
  rimeaddr_copy(&ack.dest, &rimeaddr_node_addr);
  packetbuf_copyfrom(&ack, offsetof(struct netcmd_msg,dest));

  PRINTF("netcmd send ACK seq %d to %d.%d\n", ack.seqno, from->u8[0], from->u8[1]);
  unicast_send(&netcmd_uc, from);
}

void
netcmd_deliver(void *ptr)
{
  struct netcmd_msg *msg = (struct netcmd_msg *)ptr;

  if (netcmd_s == S_SEND_ACK)
    enterS(S_IDLE);

  if (!netcmd_cache_has(msg->seqno)) {
    if (!rimeaddr_cmp(&netcmd_dl_from, &rimeaddr_null)) {
      netcmd_cache_add(msg->seqno);
      if (netcmd_cb && netcmd_cb->rcvd) {
        PRINTF("netcmd deliver CMD seq %d dest %d.%d\n", msg->seqno, msg->dest.u8[0], msg->dest.u8[1]);
        netcmd_cb->rcvd(&netcmd_dl_from, msg->msgid, msg->msgdata, msg->msglen);
        rimeaddr_copy(&netcmd_dl_from, &rimeaddr_null);
      }
    }
  } else {
    PRINTF("netcmd ignore dup CMD seq %d dest %d.%d at %d\n", msg->seqno, msg->dest.u8[0], msg->dest.u8[1], __LINE__);
    log_w(E_FAIL, &msg->seqno, 1);
  }
}

void
netcmd_req_timedout(void *ptr)
{
  struct req_item *reqe = (struct req_item *)ptr;
  struct reqcmd_item *rce = NULL;

  PRINTF("warn netcmd req %p/%d/%d.%d timedout\n", reqe, reqe->count, reqe->addr.u8[0], reqe->addr.u8[1]);
  for (rce = list_head(reqe->cmds); rce != NULL; rce = list_item_next(rce)) {
    memb_free(&netcmd_reqcmdm, rce); PRINTF("netcmd reqcmd free %p, %d left\n", rce, ++netcmd_reqcmdn);
  }
  list_init(reqe->cmds);

  list_remove(netcmd_reqq, reqe); PRINTF("netcmd req deq %p/%d/%d.%d at %d, %d in reqq\n", reqe, reqe->count, reqe->addr.u8[0], reqe->addr.u8[1], __LINE__, list_length(netcmd_reqq));
  memb_free(&netcmd_reqm, reqe); PRINTF("netcmd req free %p at %d, %d left\n", reqe, __LINE__, ++netcmd_reqn);
}

void
netcmd_handle_req(struct netcmd_msg *msg, rimeaddr_t *from)
{
  struct req_item *req = NULL;
  struct req_item *reqe = NULL;
  struct reqcmd_item *rce = NULL;
  clock_time_t t = 0;

  print_cmdq();
  for (reqe = list_head(netcmd_reqq); reqe != NULL; reqe = list_item_next(reqe)) {
    if (rimeaddr_cmp(&reqe->addr, from))
      break;
  }

  if (reqe != NULL) {
    req = reqe;
    for (rce = list_head(reqe->cmds); rce != NULL; rce = list_item_next(rce)) {
      memb_free(&netcmd_reqcmdm, rce); PRINTF("netcmd reqcmd free %p/%p at %d", rce, rce->cmd, __LINE__);
    }
    list_init(reqe->cmds);
    list_remove(netcmd_reqq, reqe); PRINTF("netcmd remove req %p/%d/%d.%d\n", reqe, reqe->count, reqe->addr.u8[0], reqe->addr.u8[1]);
  } else {
    req = memb_alloc(&netcmd_reqm); if (req != NULL) PRINTF("netcmd req alloc %p, %d left\n", req, --netcmd_reqn);
    if (req != NULL) {
      LIST_STRUCT_INIT(req, cmds);
    }
  }

  if (req != NULL) {
    rimeaddr_copy(&req->addr, from);
    req->count = 0;
    ctimer_set(&req->ct, NETCMD_CMD_WAIT, netcmd_req_timedout, req);
    list_add(netcmd_reqq, req); PRINTF("netcmd req enq %p/%d/%d.%d, %d in reqq\n", req, req->count, from->u8[0], from->u8[1], list_length(netcmd_reqq));

    if (netcmd_s == S_IDLE) {
      netcmd_prep_cmd(req);

      if (req->count > 0) {
        PRINTF("netcmd has %d cmds dest %d.%d to send\n", req->count, from->u8[0], from->u8[1]);
        if (node_mac[2] == NODE_RP) {
          t = PACKET_TIME * (1 + (random_rand() & 0x01)); PRINTF("netcmd sched send after %d\n", (uint16_t)t);
          ctimer_set(&netcmd_ct, t, netcmd_send_cmd, NULL);
        } else {
          netcmd_send_cmd(NULL);
        }
      } else {
        PRINTF("netcmd no cmd dest %d.%d\n", from->u8[0], from->u8[1]);
        ctimer_stop(&req->ct);
        list_remove(netcmd_reqq, req); PRINTF("netcmd req deq %p/%d/%d.%d at %d, %d in reqq\n", req, req->count, req->addr.u8[0], req->addr.u8[1], __LINE__, list_length(netcmd_reqq));
        memb_free(&netcmd_reqm, req); PRINTF("netcmd req free %p at %d, %d left\n", req, __LINE__, ++netcmd_reqn);
      }
    } else {
      if (netcmd_tx != NULL) { PRINTF("warn netcmd busy cmd %p/%d/%d.%d to %d.%d at %s\n", netcmd_tx, netcmd_tx->cmd.seqno, netcmd_tx->cmd.dest.u8[0], netcmd_tx->cmd.dest.u8[1], netcmd_tx_to.u8[0], netcmd_tx_to.u8[1], STATE[netcmd_s]); } else { PRINTF("warn netcmd busy at %d/%s\n", netcmd_s, STATE[netcmd_s]); }
      log_w(E_BUSY, NULL, 0);
    }
  } else {
    PRINTF("warn netcmd null req\n");
    log_w(E_FULL, NULL, 0);
  }
}

static int
netcmd_cache_has(uint16_t seqno)
{
  struct seq_item *seqe = NULL;

  for (seqe = list_head(netcmd_seqc); seqe != NULL; seqe = list_item_next(seqe)) {
    if (seqno == seqe->seqno) {
      return 1;
    }
  }

  return 0;
}

static int
netcmd_cache_add(uint16_t seqno)
{
  struct seq_item *seqe = NULL;

  seqe = memb_alloc(&netcmd_seqm);
  if (seqe == NULL) {
    seqe = list_pop(netcmd_seqc); PRINTF("netcmd cache drop %d\n", seqe->seqno);
  }
  seqe->seqno = seqno;
  list_add(netcmd_seqc, seqe); PRINTF("netcmd cache add %d\n", seqe->seqno);

  return 0;
}

void
netcmd_handle_cmd(struct netcmd_msg *msg, rimeaddr_t *from)
{
  struct cmd_item *cmde = NULL;
  int r = 0;

  netcmd_wait = msg->more;

  if (netcmd_s == S_WAIT_CMD)
    ctimer_stop(&netcmd_ct);

  // send ACK for the command
  enterS(S_SEND_ACK);
  ctimer_set(&netcmd_ct, NETCMD_SENT_TO, netcmd_no_sent, NULL);
  netcmd_send_ack(from, msg);

  // mark the CMD to deliver at ACK sent if it is destinated to us or all nodes
  if (rimeaddr_cmp(&msg->dest, &rimeaddr_node_addr) || rimeaddr_cmp(&msg->dest, &rimeaddr_null)) {
    if (!netcmd_cache_has(msg->seqno)) {
      rimeaddr_copy(&netcmd_dl_from, from);
    }
    else {
      PRINTF("netcmd ignore dup CMD seq %d dest %d.%d at %d\n", msg->seqno, msg->dest.u8[0], msg->dest.u8[1], __LINE__);
      log_w(E_FAIL, &msg->seqno, 1);
    }
  }

  // if the CMD is destinated to all nodes or other node, intercept or forward it
  if (!rimeaddr_cmp(&msg->dest, &rimeaddr_node_addr)) {
    // let interceptor to handle the non-dest command
    PRINTF("netcmd intercept cmd dest %d.%d\n", msg->dest.u8[0], msg->dest.u8[1]);
    if (netcmd_cb && netcmd_cb->intercept) {
      netcmd_cb->intercept(&msg->dest, msg->msgid, msg->msgdata, msg->msglen);
    }

    // only non-LPM node enqueue the msg to do forwarding
    if (netcmd_lpm == 0) {
      cmde = memb_alloc(&netcmd_cmdm); if (cmde != NULL) { PRINTF("netcmd cmd alloc %p at %d, %d left\n", cmde, __LINE__, --netcmd_cmdn); }
      if (cmde == NULL) {
        cmde = netcmd_pop_cmd(); PRINTF("warn netcmd cmd drop %p/%d/%d.%d at %d\n", cmde, cmde->cmd.seqno, cmde->cmd.dest.u8[0], cmde->cmd.dest.u8[1], __LINE__);
        log_w(E_DROP, NULL, 0);
      } else {
        LIST_STRUCT_INIT(cmde, acks);
      }
      memcpy(&cmde->cmd, msg, offsetof(struct netcmd_msg,msgdata) + msg->msglen);

      r = netcmd_enq(cmde, from);
      if (r == 0) {
        if (netcmd_s == S_IDLE)
          process_poll(&netcmd_process);
      } else {
        PRINTF("warn netcmd cmd enq fail at %d\n", __LINE__);
      }
    }
  }
}

void
netcmd_handle_ack(struct netcmd_msg *msg, rimeaddr_t *from)
{
  struct cmd_item *cmde = NULL;
  struct req_item *reqe = NULL;
  struct reqcmd_item *rce = NULL;
  struct ack_item *acke = NULL;

  reqe = list_head(netcmd_reqq);
  if (reqe != NULL) { // sent command to device
    if (rimeaddr_cmp(&reqe->addr, from)) {
      rce = list_head(reqe->cmds);
      if (rce != NULL) {
        cmde = rce->cmd;
        ctimer_stop(&netcmd_ct);

        // dequeue msg if all acked
        for (acke = list_head(cmde->acks); acke != NULL; acke = list_item_next(acke)) {
          if (rimeaddr_cmp(&acke->addr, from)) {
            list_remove(cmde->acks, acke); PRINTF("netcmd ack del %p/%d.%d\n", acke, acke->addr.u8[0], acke->addr.u8[1]);
            memb_free(&netcmd_ackm, acke); PRINTF("netcmd ack free %p at %d, %d left\n", acke, __LINE__, ++netcmd_ackn);
            if (list_length(cmde->acks) == 0) {
              list_remove(netcmd_cmdq, cmde); PRINTF("netcmd cmd deq %p/%d/%d.%d at %d, %d in cmdq\n", cmde, cmde->cmd.seqno, cmde->cmd.dest.u8[0], cmde->cmd.dest.u8[1], __LINE__, list_length(netcmd_cmdq));
              memb_free(&netcmd_cmdm, cmde); PRINTF("netcmd cmd free %p at %d, %d left\n", cmde, __LINE__, ++netcmd_cmdn);
            }

            enterS(S_IDLE);
            netcmd_tx = NULL;
            rimeaddr_copy(&netcmd_tx_to, &rimeaddr_null);
            list_remove(reqe->cmds, rce); PRINTF("netcmd reqcmd del %p\n", rce);
            memb_free(&netcmd_reqcmdm, rce); PRINTF("netcmd reqcmd free %p at %d, %d left\n", rce, __LINE__, ++netcmd_reqcmdn);
            reqe->count--;
            if (reqe->count == 0) {
              ctimer_stop(&reqe->ct);
              list_remove(netcmd_reqq, reqe); PRINTF("netcmd req deq %p/%d/%d.%d at %d, %d in reqq\n", reqe, reqe->count, reqe->addr.u8[0], reqe->addr.u8[1], __LINE__, list_length(netcmd_reqq));
              memb_free(&netcmd_reqm, reqe); PRINTF("netcmd req free %p at %d, %d left\n", reqe, __LINE__, ++netcmd_reqn);
            } else {
              ctimer_set(&reqe->ct, NETCMD_CMD_WAIT, netcmd_req_timedout, reqe);
              if (list_length(netcmd_reqq) > 1) {
                PRINTF("netcmd tail req %d.%d\n", reqe->addr.u8[0], reqe->addr.u8[1]);
                reqe = list_pop(netcmd_reqq);
                list_add(netcmd_reqq, reqe);
              }
            }

            if (list_length(netcmd_reqq) > 0) {
              PRINTF("netcmd send next\n");
              netcmd_send_cmd(NULL);
            }
            return;
          }
        }
      } else {
        PRINTF("warn netcmd null req\n");
      }
    } else {
      PRINTF("warn netcmd not req head\n");
    }
  }
  else { // sent command to router
    int done = 0;
    for (cmde = list_head(netcmd_cmdq); (!done && (cmde != NULL)); cmde = list_item_next(cmde)) {
      for (acke = list_head(cmde->acks); (!done && (acke != NULL)); acke = list_item_next(acke)) {
        if (rimeaddr_cmp(from, &acke->addr)) {
          list_remove(cmde->acks, acke); PRINTF("netcmd ack del %p/%d.%d\n", acke, acke->addr.u8[0], acke->addr.u8[1]);
          memb_free(&netcmd_ackm, acke); PRINTF("netcmd ack free %p at %d, %d left\n", acke, __LINE__, ++netcmd_ackn);

          if (list_length(cmde->acks) == 0) {
            list_remove(netcmd_cmdq, cmde); PRINTF("netcmd cmd deq %p/%d/%d.%d at %d, %d in cmdq\n", cmde, cmde->cmd.seqno, cmde->cmd.dest.u8[0], cmde->cmd.dest.u8[1], __LINE__, list_length(netcmd_cmdq));
            memb_free(&netcmd_cmdm, cmde); PRINTF("netcmd cmd free %p at %d, %d left\n", cmde, __LINE__, ++netcmd_cmdn);
          }
          done = 1;
        }
      }
    }
    if (done) {
      enterS(S_IDLE);
      ctimer_stop(&netcmd_ct);
      rimeaddr_copy(&netcmd_tx_to, &rimeaddr_null);
      PRINTF("netcmd send next cmd\n");
      netcmd_send_cmd(NULL);
    }
  }
}

void
netcmd_handle(struct netcmd_msg *msg, rimeaddr_t *from)
{
  PRINTF("netcmd handle %s from %d.%d at %s\n", MSG[msg->type], from->u8[0], from->u8[1], STATE[netcmd_s]);
  if (msg->type == NETCMD_CMD) {
    netcmd_handle_cmd(msg, from);
  }
  else if (msg->type == NETCMD_REQ) {
    netcmd_handle_req(msg, from);
  }
  else if (msg->type == NETCMD_ACK) {
    netcmd_handle_ack(msg, from);
  }
  else {
    PRINTF("warn netcmd invalid msg %d\n", msg->type);
  }
}

static void
netcmd_no_cmd(void *ptr)
{
  PRINTF("netcmd no cmd at timeout\n");
#if NETCMD_REQ_REXMIT > 1
  if (--netcmd_req_retry) {
    enterS(S_SEND_REQ);
    netcmd_send_req();
  } else {
#if WITH_LPM
    if (netcmd_lpm) {
      PRINTF("netcmd off radio at no cmd\n");
      if (netcmd_radio_off == 0) {
        radio_off(netcmd_rs);
        netcmd_radio_off = 1;
      }
    }
#endif /* WITH_LPM */
    enterS(S_IDLE);
  }
#else /* NETCMD_REQ_REXMIT <= 1 */
#if WITH_LPM
  if (netcmd_lpm) {
    PRINTF("netcmd off radio at no cmd\n");
    if (netcmd_radio_off == 0) {
      radio_off(netcmd_rs);
      netcmd_radio_off = 1;
    }
  }
#endif /* WITH_LPM */
  enterS(S_IDLE);
#endif /* NETCMD_REQ_REXMIT */
}

static void
netcmd_no_ack(void *ptr)
{
  PRINTF("netcmd no ack at timeout\n");
  enterS(S_IDLE);
  rimeaddr_copy(&netcmd_tx_to, &rimeaddr_null);
  if (list_length(netcmd_reqq) > 0)
    netcmd_send_cmd(NULL);
}

static void
netcmd_no_sent(void *ptr)
{
  PRINTF("warn netcmd no sent at %d/%s\n", netcmd_s, STATE[netcmd_s]);
  if (netcmd_s == S_SEND_CMD) {
    enterS(S_IDLE);
    netcmd_tx = NULL;
  }
  else if (netcmd_s == S_SEND_REQ) {
    enterS(S_IDLE);
  }
  else if (netcmd_s == S_SEND_ACK) {
    enterS(S_IDLE);
    if (!rimeaddr_cmp(&netcmd_dl_from, &rimeaddr_null)) {
      PRINTF("netcmd deliver at ack no sent\n");
      netcmd_deliver(&netcmd_rx_msg);
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
netcmd_uc_rcvd(struct unicast_conn *c, const rimeaddr_t *from)
{
  struct netcmd_msg *msg = (struct netcmd_msg *)packetbuf_dataptr();
  uint8_t len = packetbuf_datalen();
  struct neighbor *nbr = NULL;

  if (msg->type == NETCMD_CMD) PRINTF("netcmd rcvd CMD seq %d dest %d.%d from %d.%d\n", msg->seqno, msg->dest.u8[0], msg->dest.u8[1], from->u8[0], from->u8[1]);
  else if (msg->type == NETCMD_ACK) PRINTF("netcmd rcvd ACK seq %d from %d.%d\n", msg->seqno, from->u8[0], from->u8[1]);
  else PRINTF("netcmd rcvd %s from %d.%d\n", MSG[msg->type], from->u8[0], from->u8[1]);

  if (msg->type == NETCMD_REQ) {
    if (netcmd_lpm == 0) {
      nbr = neighbor_find((rimeaddr_t*)from);
      if (nbr == NULL) {
        PRINTF("netcmd add nbr %d.%d\n", from->u8[0], from->u8[1]);
        nbr = neighbor_add((rimeaddr_t*)from);
        nbr->role = NODE_VD;
      }
    }
  }

  if (netcmd_s == S_SEND_ACK) {
    PRINTF("warn netcmd ignore recv %s at %s\n", MSG[msg->type], STATE[netcmd_s]);
  } else {
    if ((netcmd_lpm == 0) || (msg->type == NETCMD_CMD)) {
      memcpy(&netcmd_rx_msg, msg, len);
      rimeaddr_copy(&netcmd_rx_from, from);
      process_post(&netcmd_process, netcmd_rcvd_event, NULL);
    }
  }
}

void
netcmd_recv_uc(const rimeaddr_t *from)
{
  netcmd_uc_rcvd(&netcmd_uc, from);
}

static void
netcmd_uc_sent(struct unicast_conn *c, int status, int num_tx)
{
  struct netcmd_msg *msg = (struct netcmd_msg *)packetbuf_dataptr();
  PRINTF("netcmd sent %d, %d/%s\n", status, netcmd_s, STATE[netcmd_s]);

  if (netcmd_s == S_SEND_REQ) {
#if WITH_TASKMON
    if (status == MAC_TX_OK) {
      task_end(netcmd);
      task_begin(netcmd, ((uint32_t)CLOCK_SECOND * (netcmd_period << 2)));
    }
#endif /* WITH_TASKMON */
    enterS(S_WAIT_CMD);
    ctimer_set(&netcmd_ct, NETCMD_CMD_WAIT, netcmd_no_cmd, NULL);
  }
  else if (netcmd_s == S_SEND_CMD) {
#if 0
    enterS(S_IDLE);
    netcmd_tx = NULL;
#else
    PRINTF("netcmd wait ack\n");
    enterS(S_WAIT_ACK);
    netcmd_tx = NULL;
    ctimer_set(&netcmd_ct, NETCMD_ACK_WAIT, netcmd_no_ack, NULL);
#endif
  }
  else if (netcmd_s == S_SEND_ACK) {
    // stop no sent timeout
    ctimer_stop(&netcmd_ct);

    // wait subsequent message or turn off radio
    if (netcmd_wait) {
      PRINTF("netcmd wait more\n");
#if NETCMD_REQ_REXMIT > 1
      netcmd_req_retry = 1;
#endif
      enterS(S_WAIT_CMD);
      ctimer_set(&netcmd_ct, NETCMD_CMD_WAIT, netcmd_no_cmd, NULL);
    } else {
#if WITH_LPM
      if (netcmd_lpm) {
        enterS(S_IDLE);
        if (netcmd_radio_off == 0) {
          PRINTF("netcmd off radio at ack sent\n");
          radio_off(netcmd_rs);
          netcmd_radio_off = 1;
        }
      } else
#endif
      {
        enterS(S_IDLE);
        if (list_length(netcmd_cmdq) > 0) {
          process_poll(&netcmd_process);
        }
      }
    }

    // deliver message to application layer
    PRINTF("netcmd deliver at ack sent\n");
    netcmd_deliver(&netcmd_rx_msg);
  }
}

static const struct unicast_callbacks netcmd_uc_cb = { netcmd_uc_rcvd, netcmd_uc_sent };
/*---------------------------------------------------------------------------*/
void
netcmd_open(uint8_t lpm, uint16_t period, struct netcmd_callback *cb)
{
  PRINTF("netcmd open, lpm %d, period %d\n", lpm, period);
  netcmd_cb = cb;
  netcmd_lpm = lpm;
  netcmd_period = period;

  netcmd_rcvd_event = process_alloc_event();
  netcmd_period_change = process_alloc_event();

  memb_init(&netcmd_reqcmdm);
  memb_init(&netcmd_reqm);
  memb_init(&netcmd_ackm);
  memb_init(&netcmd_cmdm);
  memb_init(&netcmd_seqm);
  list_init(netcmd_reqq);
  list_init(netcmd_cmdq);
  list_init(netcmd_seqc);

  unicast_open(&netcmd_uc, NETCMD_CHANNEL, &netcmd_uc_cb);
  process_start(&netcmd_process, NULL);

  rime_sniffer_add(&netcmd_sniff);

#if WITH_LPM
  netcmd_rs = radio_alloc("netcmd");
#endif
}

void
netcmd_close(void)
{
  PRINTF("netcmd close\n");
#if WITH_LPM
  if (netcmd_lpm) {
    if (netcmd_radio_off == 0) {
      radio_off(netcmd_rs);
      netcmd_radio_off = 1;
    }
  }
#endif
#if WITH_TASKMON
  taskmon_free(netcmd);
#endif /* WITH_TASKMON */
  unicast_close(&netcmd_uc);
  process_exit(&netcmd_process);
}

void
netcmd_set_period(uint16_t period)
{
  PRINTF("netcmd set period %d\n", period);
  netcmd_period = period;
  if (netcmd_lpm)
    process_post(&netcmd_process, netcmd_period_change, NULL);
}

static int
netcmd_enq(struct cmd_item *cmde, rimeaddr_t *from)
{
  struct netcmd_msg *msg = &cmde->cmd;
  struct ack_item *acke = NULL;
  rimeaddr_t *to = &msg->dest;
  struct neighbor *nbr = NULL;
  uint8_t i, n;

  if (rimeaddr_cmp(&msg->dest, &rimeaddr_null)) {
    // add all neighbors into ack wait list if it is broadcast
    n = neighbor_num();
    for (i = 0; i < n; i++) {
      nbr = neighbor_get(i);
      if (rimeaddr_cmp(&nbr->addr, from)) {
        PRINTF("netcmd skip nbr %d.%d msg from\n", nbr->addr.u8[0], nbr->addr.u8[1]);
        continue; // skip the neighbor from which received the message
      }
      acke = memb_alloc(&netcmd_ackm); if (acke != NULL) { PRINTF("netcmd ack alloc %p at %d, %d left\n", acke, __LINE__, --netcmd_ackn); }
      if (acke != NULL) {
        PRINTF("netcmd ack add %p/%d.%d\n", acke, nbr->addr.u8[0], nbr->addr.u8[1]);
        rimeaddr_copy(&acke->addr, &nbr->addr);
        list_add(cmde->acks, acke); print_acks(cmde->acks);
      } else {
        PRINTF("warn netcmd ackm full at %d\n", __LINE__);
        log_w(E_FULL, NULL, 0);
      }
    }
  }
  else {
    // add neighbor to command dest into ack wait list
    nbr = netcmd_get_nbr(to);
    if (nbr != NULL) {
      acke = memb_alloc(&netcmd_ackm); if (acke != NULL) { PRINTF("netcmd ack alloc %p at %d, %d left\n", acke, __LINE__, --netcmd_ackn); }
      if (acke != NULL) {
        PRINTF("netcmd ack add %p/%d.%d\n", acke, nbr->addr.u8[0], nbr->addr.u8[1]);
        rimeaddr_copy(&acke->addr, &nbr->addr);
        list_add(cmde->acks, acke); print_acks(cmde->acks);
      } else {
        PRINTF("warn netcmd ackm full at %d\n", __LINE__);
        log_w(E_FULL, NULL, 0);
      }
    } else {
      PRINTF("netcmd ignore msg dest %d.%d\n", to->u8[0], to->u8[1]);
      return 1; // not know nexthop to command dest
    }
  }

  if (list_length(cmde->acks) > 0) {
    list_add(netcmd_cmdq, cmde); PRINTF("netcmd cmd enq %p/%d/%d.%d, %d in cmdq\n", cmde, msg->seqno, msg->dest.u8[0], msg->dest.u8[1], list_length(netcmd_cmdq));
    print_cmdq();
    return 0; // command enqueue succeed
  } else {
    memb_free(&netcmd_cmdm, cmde); PRINTF("netcmd cmd free %p at %d, %d left\n", cmde, __LINE__, ++netcmd_cmdn);
    return 2; // no neighbor need send to
  }
}

static struct neighbor *
netcmd_get_nbr(rimeaddr_t *to)
{
  struct neighbor *nbr = NULL;
  struct route_entry *re = NULL;

  nbr = neighbor_find(to);
  if (nbr != NULL) PRINTF("netcmd found nbr %d.%d\n", nbr->addr.u8[0], nbr->addr.u8[1]);
  if (nbr == NULL) {
    re = route_lookup(to);
    if (re == NULL) PRINTF("warn netcmd no route dest %d.%d\n", to->u8[0], to->u8[1]);
    if (re != NULL)
      nbr = neighbor_find(&re->nexthop);
    if (nbr != NULL) PRINTF("netcmd found nbr %d.%d to %d.%d\n", nbr->addr.u8[0], nbr->addr.u8[1], to->u8[0], to->u8[1]);
  }

  return nbr;
}

static struct cmd_item *
netcmd_pop_cmd(void)
{
  struct cmd_item *cmde = NULL;
  struct req_item *reqe = NULL;
  struct ack_item *acke = NULL;
  struct reqcmd_item *rce = NULL;

  cmde = list_pop(netcmd_cmdq);

  for (acke = list_head(cmde->acks); acke != NULL; acke = list_item_next(acke)) {
    memb_free(&netcmd_ackm, acke); PRINTF("netcmd ack free %p at %d, %d left\n", acke, __LINE__, ++netcmd_ackn);
  }
  list_init(cmde->acks);

  for (reqe = list_head(netcmd_reqq); reqe != NULL; reqe = list_item_next(reqe)) {
    for (rce = list_head(reqe->cmds); rce != NULL; rce = list_item_next(rce)) {
      if (rce->cmd == cmde) {
        list_remove(reqe->cmds, rce);
        memb_free(&netcmd_reqcmdm, rce); PRINTF("netcmd reqcmd free %p at %d, %d left\n", rce, __LINE__, ++netcmd_reqcmdn);

        reqe->count--;
        if (reqe->count == 0) {
          ctimer_stop(&reqe->ct);
          list_remove(netcmd_reqq, reqe); PRINTF("netcmd req deq %p/%d/%d.%d at %d, %d in reqq\n", reqe, reqe->count, reqe->addr.u8[0], reqe->addr.u8[1], __LINE__, list_length(netcmd_reqq));
          memb_free(&netcmd_reqm, reqe); PRINTF("netcmd req free %p at %d, %d left\n", reqe, __LINE__, ++netcmd_reqn);
        }
        break;
      }
    }
  }

  return cmde;
}

int
netcmd_send(rimeaddr_t *to, uint8_t msgid, uint8_t *data, uint8_t len)
{
  struct cmd_item *cmde = NULL;
  struct netcmd_msg *msg = NULL;
  int r = 0;

  PRINTF("netcmd send dest %d.%d msg %d len %d, %s\n", to->u8[0], to->u8[1], msgid, len, STATE[netcmd_s]);
  if (!rimeaddr_cmp(to, &rimeaddr_node_addr)) {
    cmde = memb_alloc(&netcmd_cmdm); if (cmde != NULL) { PRINTF("netcmd cmd alloc %p at %d, %d left\n", cmde, __LINE__, --netcmd_cmdn); }
    if (cmde == NULL) {
      cmde = netcmd_pop_cmd(); PRINTF("warn netcmd cmd drop %p/%d/%d.%d at %d\n", cmde, cmde->cmd.seqno, cmde->cmd.dest.u8[0], cmde->cmd.dest.u8[1], __LINE__);
      log_w(E_DROP, NULL, 0);
    }
    else {
      LIST_STRUCT_INIT(cmde, acks);
    }

    msg = &cmde->cmd;
    msg->type = NETCMD_CMD;
    msg->opt  = 0;
    msg->more = 0;
    msg->seqno = netcmd_seq;
    rimeaddr_copy(&msg->dest, to);
    msg->msgid = msgid;
    msg->msglen = len;
    memcpy(&msg->msgdata, data, len);

    if (++netcmd_seq == 0)
      netcmd_seq = 1;

    r = netcmd_enq(cmde, (rimeaddr_t*)&rimeaddr_null);
    if (r == 0) {
      if (netcmd_s == S_IDLE) {
        process_poll(&netcmd_process);
      }
    } else {
      return r;
    }
  }

  return 0;
}

void
netcmd_req_attach(netcmd_req_attach_func_t func, uint8_t len)
{
  netcmd_req_attach_func = func;
  netcmd_req_attach_len = len;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(netcmd_process, ev, data)
{
  static struct etimer et;
  static uint8_t etr = 0;

  PROCESS_BEGIN();

  if (netcmd_lpm) {
    etimer_set(&et, ((uint32_t)CLOCK_SECOND * netcmd_period)); etr = 1;
#if WITH_TASKMON
    netcmd = taskmon_create("netcmd");
    task_begin(netcmd, ((uint32_t)CLOCK_SECOND * (netcmd_period << 2)));
#endif /* WITH_TASKMON */
  }

  while (1) {
    PROCESS_WAIT_EVENT();

    if (ev == PROCESS_EVENT_POLL) {
      PRINTF("netcmd poll at %s\n", STATE[netcmd_s]);
      netcmd_send_cmd(NULL);
    }
    else if (ev == netcmd_rcvd_event) {
      netcmd_handle(&netcmd_rx_msg, &netcmd_rx_from);
    }
    else if (ev == netcmd_period_change) {
      if (netcmd_period > 0) {
        etimer_set(&et, ((uint32_t)CLOCK_SECOND * netcmd_period)); etr = 1;
#if WITH_TASKMON
        task_end(netcmd);
        task_begin(netcmd, ((uint32_t)CLOCK_SECOND * (netcmd_period << 2)));
#endif /* WITH_TASKMON */
#if 1
        if (netcmd_s == S_IDLE) {
          enterS(S_SEND_REQ);
          netcmd_send_req();
        }
#endif
      } else {
        etimer_stop(&et); etr = 0;
#if WITH_TASKMON
        task_end(netcmd);
#endif /* WITH_TASKMON */
      }
    }
    else if (etr && etimer_expired(&et)) {
      etimer_reset(&et);

#if NETCMD_REQ_REXMIT > 1
      netcmd_req_retry = NETCMD_REQ_REXMIT;
#endif
      enterS(S_SEND_REQ);
      netcmd_send_req();
    }
  }

  PROCESS_END();
}
