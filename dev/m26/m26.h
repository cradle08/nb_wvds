#ifndef _M26_H
#define _M26_H

#include "contiki.h"

#ifndef M26_RXBUF_LEN
#define M26_RXBUF_LEN  160
#endif

#ifndef M26_TXBUF_LEN
#define M26_TXBUF_LEN  160
#endif

#ifndef M26_PKT_LEN
#define M26_PKT_LEN  80
#endif

#ifndef M26_QUEUE_LEN
#define M26_QUEUE_LEN  4
#endif

#ifndef M26_MAX_FAIL
#define M26_MAX_FAIL  10
#endif

enum {
  M26_TX_OK      = 0,
  M26_TX_NOCONN  = 1,
  M26_TX_TOOLONG = 2,
  M26_TX_BUSY    = 3,
  M26_TX_FAIL    = 4
};

enum {
  M26_ALARM_UNDER_VOLTAGE = 1
};

struct m26_callbacks {
  int (* opened)(void);
  int (* closed)(void);
  int (* sent)(int res);
  int (* rcvd)(uint8_t *data, uint16_t len);
  int (* alarm)(uint8_t code);
};

int m26_open(const char *ip, uint16_t port, const struct m26_callbacks *callback);
int m26_close(void);
int m26_reopen(void);
int m26_set_host(const char *host, uint16_t port);
int m26_reset(void);
int m26_cmd(uint8_t *data, uint16_t len);
int m26_send_data(uint8_t *data, uint16_t len);
int m26_get_rssi(void);

#endif /* _M26_H */
