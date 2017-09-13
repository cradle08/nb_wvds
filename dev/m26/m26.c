#include "contiki.h"
#include "sys/ctimer.h"
#include "sys/logger.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "dev/watchdog.h"
#include "dev/leds.h"
#include "m26.h"
#include "m26-arch.h"
#include "app.h"

#include <string.h>
#include <stdio.h>

#if 0
#define LEDS_ON(x)   leds_on(x)
#define LEDS_OFF(x)  leds_off(x)
#else
#define LEDS_ON(x)
#define LEDS_OFF(x)
#endif

enum {
  M26_START = 0,
  M26_RDY,
  M26_SMSRDY,
  M26_QIFGCNT,
  M26_QIMODE,
  M26_QICSGP,
  M26_ATV1,
  M26_QIDEACT_PRE,
  M26_QIREGAPP,
  M26_QIACT,
  M26_QIHEAD,
  M26_QIDNSIP,
  M26_CSQ,
  M26_QIOPEN,
  M26_READY,
  M26_QISEND,
  M26_WAITOK,
  M26_WAITACK,
  M26_QICLOSE,
  M26_CLOSED,
  M26_QIDEACT,
  M26_DEACTED
};
static uint8_t m26_s = 0;

static int m26_last = -1;
static uint8_t m26_cmdn = 0;

enum {
  M26_TX_IDLE = 0,
  M26_TX_SEND,
  M26_TX_WAIT
};
static volatile uint8_t m26_tx = M26_TX_IDLE;
static uint8_t  m26_txbuf[M26_TXBUF_LEN];
static uint16_t m26_txlen = 0;

enum {
  M26_RX_START = 0,
  M26_RX_BEG_CR,
  M26_RX_BEG_LF,
  M26_RX_PROMPT,
  M26_RX_DATA,
  M26_RX_END_CR,
  M26_RX_END_LF,
  M26_RX_BEG_A,
  M26_RX_BEG_T,
  M26_RX_CMD,
  M26_RX_END_CMD,
  M26_RX_IPD_I,
  M26_RX_IPD_P,
  M26_RX_IPD_D,
  M26_RX_IPD_LEN,
  M26_RX_IPD_DATA,
  M26_RX_AA,
  M26_RX_LEN,
  M26_RX_DAT,
  M26_RX_FF,
  M26_RX_DONE
};
static volatile uint8_t m26_rx = M26_RX_START;
static uint8_t  m26_rxbuf[M26_RXBUF_LEN];
static uint16_t m26_rxbeg = 0;
static uint16_t m26_rxend = 0;
static uint16_t m26_rxlen = 0;
#define INC(x) do { if ((++(x)) > sizeof(m26_rxbuf)) (x) = 0; } while (0)

static uint8_t  m26_ipdn = 0;
static uint8_t  m26_ipdi = 0;
static uint8_t  m26_pkt[M26_RXBUF_LEN];
static uint16_t m26_pkti = 0;

static int8_t m26_rssi = -127;

#define DEBUG_PKT 0
#if DEBUG_PKT
static uint8_t  m26_pktq[1][M26_RXBUF_LEN];
static uint8_t  m26_pktn = 0;
#endif

static char m26_host[16];
static uint16_t m26_port = 9000;
static char m26_opencmd[43] = {0}; // 17+15+3+5+2+1=43
static uint8_t m26_fail = 0;
static uint8_t m26_conn_fail = 0;
//static uint8_t m26_reconn = 0;
static uint8_t m26_echo = 1;

static struct ctimer m26_ct;

const struct m26_callbacks *m26_callback;

/*-------------------------------------------------------*/
int m26_byte_rcvd(uint8_t b);
void m26_frame_rcvd(uint8_t *ptr, uint16_t len);

static int m26_send(uint8_t *data, uint16_t len);

void m26_timedout(void *ptr);
/*-------------------------------------------------------*/
PROCESS(m26_process, "M26 GPRS");
/*-------------------------------------------------------*/
int
m26_open(const char *ip, uint16_t port, const struct m26_callbacks *callback)
{
  m26_set_host(ip, port);

  m26_arch_init();
  m26_arch_set_input(m26_byte_rcvd);

  m26_arch_turnon();
  m26_s = M26_START;
  ctimer_set(&m26_ct, (CLOCK_SECOND * 10), m26_timedout, NULL);

  m26_callback = callback;
  process_start(&m26_process, NULL);

  return 0;
}

int
m26_close(void)
{
  m26_s = M26_QICLOSE;
  //m26_reconn = 0;
  ctimer_set(&m26_ct, (CLOCK_SECOND >> 6), m26_timedout, NULL);
  return 0;
}

int
m26_reopen(void)
{
  m26_s = M26_CSQ;
  ctimer_stop(&m26_ct);
  ctimer_set(&m26_ct, (CLOCK_SECOND >> 6), m26_timedout, NULL);
  return 0;
}

int
m26_set_host(const char *host, uint16_t port)
{
  char str[6] = {0};
  char *p = m26_opencmd;

  memset(m26_host, 0, sizeof(m26_host));
  memcpy(m26_host, host, strlen(host));
  m26_port = port;

  sprintf(str, "%u", m26_port);
  strncpy(p, "AT+QIOPEN=\"TCP\",\"", 17); p += 17;
  strncpy(p, m26_host, strlen(m26_host)); p += strlen(m26_host);
  strncpy(p, "\",\"", 3); p += 3;
  strncpy(p, str, strlen(str)); p += strlen(str);
  strncpy(p, "\"\r", 2); p += 2;
  *p = '\0';

  return 1;
}

int
m26_reset(void)
{
  m26_s = M26_START;
  m26_txlen = 0;
  ctimer_set(&m26_ct, (CLOCK_SECOND >> 6), m26_timedout, NULL);
  return 0;
}

int
m26_cmd(uint8_t *data, uint16_t len)
{
  return 0;
}

int
m26_send_data(uint8_t *data, uint16_t len)
{
  if (m26_s != M26_READY) {
    log_w(E_FAIL, &m26_s, 1);
    return M26_TX_NOCONN; // wrong state, no tcp connection
  }
  if (len > sizeof(m26_txbuf)) {
    uint8_t arg[2];
    arg[0] = len;
    arg[1] = sizeof(m26_txbuf);
    log_w(E_FAIL, arg, sizeof(arg));
    return M26_TX_TOOLONG; // packet too long
  }
  if (m26_txlen > 0) {
    log_w(E_FAIL, NULL, 0);
    return M26_TX_BUSY; // busy, last packet not sent
  }

  m26_s = M26_QISEND;
  m26_send("AT+QISEND\r", 10);

  m26_txlen = len;
  memcpy(m26_txbuf, data, len);
  ctimer_set(&m26_ct, (CLOCK_SECOND >> 6), m26_timedout, NULL);

  return M26_TX_OK;
}

int
m26_get_rssi(void)
{
  m26_send("AT+CSQ\r", 7); // for next read
  return m26_rssi;
}
/*-------------------------------------------------------*/
static int
m26_send(uint8_t *data, uint16_t len)
{
  uint16_t i;

  watchdog_periodic();
  for (i = 0; i < len; i++)
    m26_arch_send(data[i]);

  return 0;
}

/*-------------------------------------------------------*/
int
m26_handle_byte(uint8_t b)
{
  if (m26_rx == M26_RX_START) {
    if (b == 0x0D) {
      m26_rx = M26_RX_BEG_CR;
      m26_pkt[m26_pkti++] = b;
    }
    else if (b == 'A') {
      m26_rx = M26_RX_BEG_A;
      m26_pkt[m26_pkti++] = b;
    }
    else if (b == 'I') {
      m26_rx = M26_RX_IPD_I;
      m26_pkt[m26_pkti++] = b;
    }
    else if (b == 0xAA) {
      m26_rx = M26_RX_AA;
      m26_pkt[m26_pkti++] = b;
    }
  }
  else if (m26_rx == M26_RX_BEG_A) {
    if (b == 'T') {
      m26_rx = M26_RX_BEG_T;
      m26_pkt[m26_pkti++] = b;
    }
    else if (b == 'A') {
      // maybe new start
    }
    else {
      m26_rx = M26_RX_START;
      m26_pkti = 0;
    }
  }
  else if (m26_rx == M26_RX_BEG_T) {
    if (b == 0x0D) {
      m26_rx = M26_RX_END_CMD;
      m26_pkt[m26_pkti++] = b;
      m26_rx = M26_RX_DONE;
      m26_frame_rcvd(m26_pkt, m26_pkti);
      return 0;
    }
    else {
      m26_rx = M26_RX_CMD;
      m26_pkt[m26_pkti++] = b;
    }
  }
  else if (m26_rx == M26_RX_CMD) {
    if (b == 0x0D) {
      m26_rx = M26_RX_END_CMD;
      m26_pkt[m26_pkti++] = b;
      m26_rx = M26_RX_DONE;
      m26_frame_rcvd(m26_pkt, m26_pkti);
      return 0;
    }
    else {
      m26_pkt[m26_pkti++] = b;
    }
  }
  else if (m26_rx == M26_RX_BEG_CR) {
    if (b == 0x0A) {
      m26_rx = M26_RX_BEG_LF;
      m26_pkt[m26_pkti++] = b;
    }
    else if (b == 0x0D) {
      // maybe new start
    }
    else {
      m26_rx = M26_RX_START;
      m26_pkti = 0;
    }
  }
  else if (m26_rx == M26_RX_BEG_LF) {
    if (b == 0x0D) {
      m26_rx = M26_RX_BEG_CR;
      m26_pkti = 0;
      m26_pkt[m26_pkti++] = b;
    }
    else if (b == 0x3E) { // >
      m26_rx = M26_RX_PROMPT;
      m26_pkt[m26_pkti++] = b;
    }
    else {
      m26_rx = M26_RX_DATA;
      m26_pkt[m26_pkti++] = b;
    }
  }
  else if (m26_rx == M26_RX_PROMPT) {
    if (b == 0x20) { // space
      m26_pkt[m26_pkti++] = b;
      m26_rx = M26_RX_DONE;
      m26_frame_rcvd(m26_pkt, m26_pkti);
      return 0;
    } else {
      m26_rx = M26_RX_DATA;
      m26_pkt[m26_pkti++] = b;
    }
  }
  else if (m26_rx == M26_RX_DATA) {
    if (b == 0x0D) {
      m26_rx = M26_RX_END_CR;
      m26_pkt[m26_pkti++] = b;
    } else {
      m26_pkt[m26_pkti++] = b;
    }
  }
  else if (m26_rx == M26_RX_END_CR) {
    if (b == 0x0A) {
      m26_rx = M26_RX_END_LF;
      m26_pkt[m26_pkti++] = b;
      m26_rx = M26_RX_DONE;
      m26_frame_rcvd(m26_pkt, m26_pkti);
      return 0;
    } else {
      m26_rx = M26_RX_START;
      m26_pkti = 0;
    }
  }
  else if (m26_rx == M26_RX_IPD_I) {
    if (b == 'P') {
      m26_rx = M26_RX_IPD_P;
      m26_pkt[m26_pkti++] = b;
    } else if (b == 'I') {
      // may be new start
    } else {
      m26_rx = M26_RX_START;
      m26_pkti = 0;
    }
  }
  else if (m26_rx == M26_RX_IPD_P) {
    if (b == 'D') {
      m26_rx = M26_RX_IPD_D;
      m26_pkt[m26_pkti++] = b;
#if GPRS_LED
      leds_toggle(GPRS_LED_RX);
#endif
    } else {
      m26_rx = M26_RX_START;
      m26_pkti = 0;
    }
  }
  else if (m26_rx == M26_RX_IPD_D) {
    if ('0' <= b && b <= '9') {
      m26_rx = M26_RX_IPD_LEN;
      m26_pkt[m26_pkti++] = b;
      m26_ipdn = b - '0';
    } else {
      m26_rx = M26_RX_START;
      m26_pkti = 0;
    }
  }
  else if (m26_rx == M26_RX_IPD_LEN) {
    if (b == ':') {
      m26_pkt[m26_pkti++] = b;
      m26_rx = M26_RX_IPD_DATA;
      m26_ipdi = 0;
    } else if ('0' <= b && b <= '9') {
      m26_pkt[m26_pkti++] = b;
      m26_ipdn = (m26_ipdn * 10) + (b - '0');
    } else {
      m26_rx = M26_RX_START;
      m26_pkti = 0;
    }
  }
  else if (m26_rx == M26_RX_IPD_DATA) {
    m26_pkt[m26_pkti++] = b;
    if (++m26_ipdi == m26_ipdn) {
      m26_rx = M26_RX_DONE;
      m26_frame_rcvd(m26_pkt, m26_pkti);
      return 0;
    }
  }
  else if (m26_rx == M26_RX_AA) {
    m26_rx = M26_RX_LEN;
    m26_pkt[m26_pkti++] = b;
    m26_rxlen = b + 5;
  }
  else if (m26_rx == M26_RX_LEN) {
    m26_pkt[m26_pkti++] = b;
    if (m26_pkti == m26_rxlen) {
      //m26_rx = M26_RX_DAT;
      if (b == 0xFF) {
        //m26_rx = M26_RX_FF;
        m26_rx = M26_RX_DONE;
        m26_frame_rcvd(m26_pkt, m26_pkti);
        return 0;
      } else {
        m26_rx = M26_RX_START;
        m26_pkti = 0;
      }
    }
  }

  return 1;
}

#define BUSYWAIT_UNTIL(cond, max_time) do { \
  rtimer_clock_t t0; \
  t0 = RTIMER_NOW(); \
  while(!(cond) && RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + (max_time))) {} \
} while(0)

int
m26_byte_rcvd(uint8_t b)
{
#if 0
  m26_rxbuf[m26_rxend] = b;
  INC(m26_rxend);
  if (b == '\n')
    process_poll(&m26_process);

#else
  UCA3IE &= ~UCRXIE; // disable UCA3 RX interrupt

  UCA3IFG &= ~UCRXIFG;
  m26_rxbuf[m26_rxend] = b;
  INC(m26_rxend);

  watchdog_periodic();
  do {
    BUSYWAIT_UNTIL((UCA3IFG & UCRXIFG), (RTIMER_SECOND/1024));
    if (!(UCA3IFG & UCRXIFG))
      break;

    UCA3IFG &= ~UCRXIFG;
    m26_rxbuf[m26_rxend] = UCA3RXBUF;
    INC(m26_rxend);
    if (m26_rxend == m26_rxbeg) {
      INC(m26_rxbeg); // drop first
      log_w(0, NULL, 0); // rxbuf full
      break;
    }
  } while(1);

  UCA3IE |= UCRXIE; // enable UCA3 RX interrupt

  process_poll(&m26_process);
#endif

  return 1; // must be non-zero to leave LPM state
}

void
m26_frame_rcvd(uint8_t *ptr, uint16_t len)
{
#if DEBUG_PKT
  memset(m26_pktq[m26_pktn], 0, sizeof(m26_pktq[m26_pktn]));
  memcpy(m26_pktq[m26_pktn], ptr, len);
#endif

  // dispatch state transition
  if (len >= 7 && (memcmp(ptr, "\r\nRDY\r\n", 7) == 0 || memcmp(ptr, "\r\n+CFUN", 7) == 0)) {
    ctimer_stop(&m26_ct);
    m26_s = M26_RDY;

    if (!m26_echo) {
      m26_send("ATE0\r", 5); // disable command echo
    }
    ctimer_set(&m26_ct, (CLOCK_SECOND * 10), m26_timedout, NULL);
  }
  else if (len >= 6 && memcmp(ptr, "IPD", 3) == 0) {
      // deliver to callback
      if (m26_callback->rcvd)
        m26_callback->rcvd(m26_pkt + m26_pkti - m26_ipdn, m26_ipdn);
  }
  else if (len >= 4 && memcmp(ptr, "\r\n> ", 4) == 0) {
    if (m26_s == M26_QISEND) {
      m26_send(m26_txbuf, m26_txlen);
      m26_arch_send(0x1A); // Ctrl-Z

      m26_s = M26_WAITOK;
      ctimer_set(&m26_ct, (CLOCK_SECOND >> 2), m26_timedout, NULL);
    } else {
      log_w(E_STATE, (uint8_t*)&m26_s, 1);
    }
  }
  else if (len >= 9 && memcmp(ptr, "\r\nSEND OK", 9) == 0) {
    if (m26_s == M26_WAITOK) {
      ctimer_stop(&m26_ct);
      m26_s = M26_READY;
      m26_txlen = 0;
      m26_fail = 0;
      if (m26_callback->sent)
        m26_callback->sent(M26_TX_OK);
    } else {
      log_w(E_STATE, (uint8_t*)&m26_s, 1);
    }
  }
  else if (len >= 13 && (memcmp(ptr, "\r\n+CGREG: 0,5", 13) == 0 || memcmp(ptr, "\r\n+CGREG: 0,1", 13) == 0)) {
    if (m26_s == M26_RDY) {
      m26_s = M26_QIFGCNT;
      ctimer_stop(&m26_ct);
      ctimer_set(&m26_ct, (CLOCK_SECOND >> 6), m26_timedout, NULL);
    } else {
      log_w(E_STATE, (uint8_t*)&m26_s, 1);
    }
  }
  else if (len >= 8 && memcmp(ptr, "\r\n+CSQ: ", 8) == 0) {
    uint8_t *p = ptr + 8;
    //log_i(0, ptr + 8, 4);

    m26_rssi = 0;
    while ((*p >= '0') && (*p <= '9')) {
      m26_rssi = (m26_rssi * 10) + (*p - '0');
      p++;
    }
    m26_rssi = -53 + (m26_rssi - 30) * 2;
  }
  else if (len >= 4 && memcmp(ptr, "\r\nOK", 4) == 0) {
    if (m26_s == M26_QIFGCNT) {
      m26_s = M26_QIMODE;
      ctimer_stop(&m26_ct);
      ctimer_set(&m26_ct, (CLOCK_SECOND >> 6), m26_timedout, NULL);
    }
    else if (m26_s == M26_QIMODE) {
      m26_s = M26_QICSGP;
      ctimer_stop(&m26_ct);
      ctimer_set(&m26_ct, (CLOCK_SECOND >> 6), m26_timedout, NULL);
    }
    else if (m26_s == M26_QICSGP) {
      m26_s = M26_ATV1;
      ctimer_stop(&m26_ct);
      ctimer_set(&m26_ct, (CLOCK_SECOND >> 6), m26_timedout, NULL);
    }
    else if (m26_s == M26_ATV1) {
      m26_s = M26_QIDEACT_PRE;
      ctimer_stop(&m26_ct);
      ctimer_set(&m26_ct, (CLOCK_SECOND >> 6), m26_timedout, NULL);
    }
    else if (m26_s == M26_QIREGAPP) {
      m26_s = M26_QIACT;
      ctimer_stop(&m26_ct);
      ctimer_set(&m26_ct, (CLOCK_SECOND >> 6), m26_timedout, NULL);
    }
    else if (m26_s == M26_QIACT) {
      m26_s = M26_QIHEAD;
      LEDS_ON(LEDS_GREEN);
      ctimer_stop(&m26_ct);
      ctimer_set(&m26_ct, (CLOCK_SECOND >> 6), m26_timedout, NULL);
    }
    else if (m26_s == M26_QIHEAD) {
      m26_s = M26_QIDNSIP;
      ctimer_stop(&m26_ct);
      ctimer_set(&m26_ct, (CLOCK_SECOND >> 6), m26_timedout, NULL);
    }
    else if (m26_s == M26_QIDNSIP) {
      m26_s = M26_CSQ;
      ctimer_stop(&m26_ct);
      ctimer_set(&m26_ct, (CLOCK_SECOND >> 6), m26_timedout, NULL);
    }
    else if (m26_s == M26_CSQ) {
#if 1
      uint8_t arg[4];
      arg[0] = (m26_rssi <= -100) ? '-' : ' ';
      arg[1] = (m26_rssi <= -100) ? '1' : '-';
      arg[2] = (((0 - m26_rssi) / 10) % 10) + '0';
      arg[3] = ((0 - m26_rssi) % 10) + '0';
      log_i(I_GPRS_RSSI, arg, 4);
#endif
      m26_s = M26_QIOPEN;
      ctimer_stop(&m26_ct);
      ctimer_set(&m26_ct, (CLOCK_SECOND >> 6), m26_timedout, NULL);
    } else {
      //log_w(E_STATE, (uint8_t*)&m26_s, 1);
    }
  }
  else if (len >= 8 && memcmp(ptr, "\r\nCLOSED", 8) == 0) {
    log_w(E_FAIL, NULL, 0);
    m26_s = M26_CLOSED;
    if (m26_callback->closed)
      m26_callback->closed();
  }
  else if (len >= 7 && memcmp(ptr, "\r\nERROR", 7) == 0) {
    if (m26_s == M26_QISEND) {
      m26_send("AT+QISTAT\r", 10);
    }
    else {
      m26_send("AT+QISTATE\r", 11);

      log_w(E_FAIL, (uint8_t*)&m26_s, 1);
      ++m26_fail;
      if (m26_fail >= M26_MAX_FAIL) {
        log_w(E_FAIL, (uint8_t*)&m26_s, 1);
        m26_reset();
      }
    }
  }
  else if ((len >= 12 && memcmp(ptr, "\r\nCONNECT OK", 12) == 0)
        || (len >= 17 && memcmp(ptr, "\r\nALREADY CONNECT", 17) == 0)) {
    m26_conn_fail = 0;
    if (m26_s == M26_QIOPEN) {
      m26_s = M26_READY;
      ctimer_stop(&m26_ct);
      if (m26_callback->opened)
        m26_callback->opened();
    } else {
      log_w(E_STATE, (uint8_t*)&m26_s, 1);
    }
  }
  else if (len >= 14 && memcmp(ptr, "\r\nCONNECT FAIL", 14) == 0) {
    ++m26_conn_fail;
    {
      uint8_t arg[2];
      arg[0] = m26_s;
      arg[1] = m26_conn_fail;
      log_w(E_FAIL, arg, 2);
    }
    if (m26_conn_fail >= 5) {
      m26_reset();
    } else {
      m26_send("AT+QISTATE\r", 11);
    }
  }
  else if (len >= 10 && memcmp(ptr, "\r\nCLOSE OK", 10) == 0) {
    if (m26_s == M26_QICLOSE) {
      ctimer_stop(&m26_ct);
      m26_s = M26_CLOSED;

      m26_s = M26_QIDEACT;
      ctimer_set(&m26_ct, (CLOCK_SECOND >> 6), m26_timedout, NULL);
    } else {
      log_w(E_STATE, (uint8_t*)&m26_s, 1);
    }
  }
  else if (len >= 10 && memcmp(ptr, "\r\nDEACT OK", 10) == 0) {
    if (m26_s == M26_QIDEACT_PRE) {
      m26_s = M26_QIREGAPP;
      ctimer_stop(&m26_ct);
      ctimer_set(&m26_ct, (CLOCK_SECOND >> 6), m26_timedout, NULL);
    }
    else if (m26_s == M26_QIDEACT) {
      ctimer_stop(&m26_ct);
      m26_s = M26_DEACTED;
      log_w(E_FAIL, (uint8_t*)&m26_s, 1);
      if (m26_callback->closed)
        m26_callback->closed();
    }
  }
  else if (len >= 14 && memcmp(ptr, "\r\n+PDP DEACT\r\n", 14) == 0) {
    log_w(E_FAIL, (uint8_t*)&m26_s, 1);
    m26_reset();
  }
  else if (len >= 3 && memcmp(ptr, "AT+", 3) == 0) {
    if (!m26_echo) {
      m26_send("ATE0\r", 5); // disable command echo
    }
  }
  else if (len >= 15 && memcmp(ptr, "\r\nUNDER_VOLTAGE", 15) == 0) {
    if (m26_callback->alarm)
      m26_callback->alarm(M26_ALARM_UNDER_VOLTAGE);
  }
  else if (len >= 17 && memcmp(ptr, "\r\nSTATE: IP CLOSE", 17) == 0) {
    log_w(E_FAIL, NULL, 0);
    m26_txlen = 0; // cancel sending
    m26_s = M26_CLOSED;
    if (m26_callback->closed)
      m26_callback->closed();
  }
  else if (len >= 18 && memcmp(ptr, "\r\nSTATE: PDP DEACT", 18) == 0) {
    log_w(E_FAIL, NULL, 0);
    m26_txlen = 0; // cancel sending
    m26_s = M26_QIACT;
    ctimer_stop(&m26_ct);
    ctimer_set(&m26_ct, (CLOCK_SECOND >> 6), m26_timedout, NULL);
  }
#if DEBUG_PKT
  else {
    if (m26_s > M26_START) {
      log_w(E_NULL, ptr, len);
    }
  }
#endif

  // reset reception FSM
  m26_rx = M26_RX_START;
  m26_pkti = 0;
  memset(m26_pkt, 0, sizeof(m26_pkt));
}

/*-------------------------------------------------------*/
void
m26_timedout(void *ptr)
{
  if (m26_s != m26_last) {
    m26_last = m26_s;
    m26_cmdn = 0;
    m26_fail = 0;
  }
  if (++m26_cmdn >= 30) {
    m26_cmdn = 0;
    log_w(E_FAIL, (uint8_t*)&m26_s, 1);
    m26_reset();
  }

  if (m26_s == M26_START) {
    m26_arch_turnoff();
    m26_arch_poweroff();
    clock_wait(CLOCK_SECOND << 1);
    m26_arch_turnon();

    ctimer_set(&m26_ct, (CLOCK_SECOND * 60), m26_timedout, NULL);
  }
  else if (m26_s == M26_RDY) {
    m26_send("AT+CGREG?\r", 10);
    ctimer_set(&m26_ct, (CLOCK_SECOND * 2), m26_timedout, NULL);
  }
  else if (m26_s == M26_QIFGCNT) {
    m26_send("AT+QIFGCNT=0\r", 12);
    ctimer_set(&m26_ct, (CLOCK_SECOND * 1), m26_timedout, NULL);
  }
  else if (m26_s == M26_QIMODE) {
    m26_send("AT+QIMODE=0\r", 12);
    ctimer_set(&m26_ct, (CLOCK_SECOND * 1), m26_timedout, NULL);
  }
  else if (m26_s == M26_QICSGP) {
    m26_send("AT+QICSGP=1,\"CMNET\"\r", 20);
    ctimer_set(&m26_ct, (CLOCK_SECOND * 1), m26_timedout, NULL);
  }
  else if (m26_s == M26_QIREGAPP) {
    m26_send("AT+QIREGAPP\r", 12);
    ctimer_set(&m26_ct, (CLOCK_SECOND * 1), m26_timedout, NULL);
  }
  else if (m26_s == M26_QIACT) {
    m26_send("AT+QIACT\r", 9);
    ctimer_set(&m26_ct, (CLOCK_SECOND * 1), m26_timedout, NULL);
  }
  else if (m26_s == M26_ATV1) {
    m26_send("ATV1\r", 5);
    ctimer_set(&m26_ct, (CLOCK_SECOND * 1), m26_timedout, NULL);
  }
  else if (m26_s == M26_QIHEAD) {
    m26_send("AT+QIHEAD=1\r", 12);
    ctimer_set(&m26_ct, (CLOCK_SECOND * 1), m26_timedout, NULL);
  }
  else if (m26_s == M26_QIDNSIP) {
    m26_send("AT+QIDNSIP=0\r", 13);
    ctimer_set(&m26_ct, (CLOCK_SECOND * 1), m26_timedout, NULL);
  }
  else if (m26_s == M26_CSQ) {
    m26_send("AT+CSQ\r", 7);
    ctimer_set(&m26_ct, (CLOCK_SECOND * 1), m26_timedout, NULL);
  }
  else if (m26_s == M26_QIOPEN) {
    m26_send((uint8_t *)m26_opencmd, strlen(m26_opencmd));
    ctimer_set(&m26_ct, (CLOCK_SECOND * 6), m26_timedout, NULL);
  }
  else if (m26_s == M26_QISEND) {
    m26_send(m26_txbuf, m26_txlen);
    m26_arch_send(0x1A); // Ctrl-Z

    m26_s = M26_WAITOK;
    ctimer_set(&m26_ct, (CLOCK_SECOND >> 2), m26_timedout, NULL);
  }
  else if (m26_s == M26_WAITOK) {
    m26_s = M26_READY;
    m26_txlen = 0;
    if (m26_callback->sent)
      m26_callback->sent(M26_TX_FAIL);
  }
  else if (m26_s == M26_QICLOSE) {
    m26_send("AT+QICLOSE\r", 11);
    ctimer_set(&m26_ct, CLOCK_SECOND, m26_timedout, NULL);
  }
  else if (m26_s == M26_QIDEACT || m26_s == M26_QIDEACT_PRE) {
    m26_send("AT+QIDEACT\r", 11);
    ctimer_set(&m26_ct, CLOCK_SECOND, m26_timedout, NULL);
  }
  else {
    log_w(E_STATE, &m26_s, 1);
  }
}
/*-------------------------------------------------------*/
PROCESS_THREAD(m26_process, ev, data)
{
  static uint8_t b;

  PROCESS_BEGIN();

  while (1) {
    PROCESS_WAIT_EVENT();

    if (ev == PROCESS_EVENT_POLL) {
      watchdog_periodic();
      while (m26_rxbeg != m26_rxend) {
        b = m26_rxbuf[m26_rxbeg];
        INC(m26_rxbeg);
        m26_handle_byte(b);
      }
    }
  }

  PROCESS_END();
}
