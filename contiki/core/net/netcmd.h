#ifndef _NETCMD_H
#define _NETCMD_H

#include "net/rime/rimeaddr.h"

#ifndef NETCMD_DATA_MAXLEN
#define NETCMD_DATA_MAXLEN 48
#endif

#ifndef NETCMD_CMDQ_MAX_BCAST
#define NETCMD_CMDQ_MAX_BCAST 2
#endif

#ifndef NETCMD_CMDQ_MAX_UCAST
#define NETCMD_CMDQ_MAX_UCAST 4
#endif

#define NETCMD_CMDQ_MAXLEN (NETCMD_CMDQ_MAX_UCAST + NETCMD_CMDQ_MAX_BCAST)

#ifndef NETCMD_REQQ_MAXLEN
#define NETCMD_REQQ_MAXLEN 1
#endif

#define NETCMD_CHANNEL  0x87

struct netcmd_msg_req {
  uint8_t    type;
  uint8_t    mac[8];
#if WITH_OTA
  uint8_t    fwver;
#else
  uint8_t    reserv;
#endif /* WITH_OTA */
  uint8_t    data[NETCMD_DATA_MAXLEN];
};

struct netcmd_msg {
  uint8_t    type;    // netcmd message type: REQ,CMD,ACK
  uint8_t    opt:7;   // reserved for other options
  uint8_t    more:1;  // has more cmd for the request node
  uint16_t   seqno;   // sequence number
  rimeaddr_t dest;    // destination, 0.0 for all
  uint8_t    msgid;   // sub message type
  uint8_t    msglen;  // number of bytes of argument data
  uint8_t    msgdata[NETCMD_DATA_MAXLEN]; // bytes of argument data
};

struct netcmd_callback {
  int (* rcvd)(rimeaddr_t *from, uint8_t msgid, uint8_t *data, uint8_t len);
  int (* intercept)(rimeaddr_t *dest, uint8_t msgid, uint8_t *data, uint8_t len);
};

enum {
  NETCMD_REQ = 1,
  NETCMD_CMD = 2,
  NETCMD_ACK = 3
};

extern struct unicast_conn netcmd_uc;

typedef uint8_t* (*netcmd_req_attach_func_t)(void);

void netcmd_open(uint8_t lpm, uint16_t period, struct netcmd_callback *cb);
void netcmd_close(void);
void netcmd_set_period(uint16_t period);
int netcmd_send(rimeaddr_t *to, uint8_t msgid, uint8_t *data, uint8_t len);
void netcmd_send_req(void);
void netcmd_req_attach(netcmd_req_attach_func_t func, uint8_t len);
void netcmd_handle(struct netcmd_msg *msg, rimeaddr_t *from);

void netcmd_recv_uc(const rimeaddr_t *from);

#endif /* _NETCMD_H */
