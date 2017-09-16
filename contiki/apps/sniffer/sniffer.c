#include "contiki.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "net/rime/rimeaddr.h"
#include "net/rime/unicast.h"
#include "dev/leds.h"
#include "dev/rs232.h"
#include "dev/uart.h"
#include "dev/flash.h"
#include "dev/watchdog.h"
#include "hdlc.h"

#if CC11xx_CC1120
#include "cc11xx.h"
#endif

#include <string.h>
#include <stddef.h>

/*----------------------------------------------------------------------------*/
#ifndef APP_VER_MAJOR
#define APP_VER_MAJOR 1
#endif
#ifndef APP_VER_MINOR
#define APP_VER_MINOR 1
#endif
#ifndef APP_VER_PATCH
#define APP_VER_PATCH 2
#endif

#ifndef APP_COMMIT
#define APP_COMMIT "dee7f32c"
#endif
#ifndef APP_BUILD
#define APP_BUILD  "20170713"
#endif

/*----------------------------------------------------------------------------*/
#ifndef WITH_WVDS
#define WITH_WVDS  1
#endif

/*----------------------------------------------------------------------------*/
#define MAX_PKTLEN    128
#define PKT_QUEUELEN  10

#define RFCHAN_ADDR  0x1800
#define RFPWR_ADDR   0x1802

extern void network_set_rcvd_cb(void (*f)(void));

extern int radio_get_channel(void);
extern void radio_set_channel(int ch);
#if CC11xx_CC1120
extern void cc11xx_set_promiscuous(char p);
#endif

enum {
  SNIFF_MSG  = 0x01,
  SNIFF_DATA = 0x02,
  SNIFF_CMD  = 0x03,
  SNIFF_ACK  = 0x04
};

enum {
  SNIFF_CMD_GETVER  = 0x00,
  SNIFF_CMD_SETCHAN = 0x01,
  SNIFF_CMD_GETCHAN = 0x02,
  SNIFF_CMD_SENDPKT = 0x03,
  SNIFF_CMD_SETPWR  = 0x05,
  SNIFF_CMD_GETPWR  = 0x06
};

enum {
  SNIFF_DATA_START = 0x01,
  SNIFF_DATA_CHAN  = 0x02,
  SNIFF_DATA_POWER = 0x03,
  SNIFF_DATA_ERROR = 0x11
};

struct sniff_header {
  uint8_t type;
  uint8_t seqno;
   int8_t rssi;
   int8_t lqi;
  uint8_t len;
};

struct sniff_footer {
  uint8_t crc;
};

struct sniff_msg {
  struct sniff_header header;
  uint8_t data[MAX_PKTLEN];
  struct sniff_footer footer;
};

struct sniff_data {
  uint8_t type;
  uint8_t seqno;
  uint8_t dtype;
  uint8_t len;
  uint8_t data[20];
};

struct sniff_cmd {
  uint8_t type;
  uint8_t seqno;
  uint8_t ack:1;
  uint8_t reserved:7;
  uint8_t op;
  uint8_t len;
  uint8_t arg[MAX_PKTLEN];
};

struct sniff_ack {
  uint8_t type;
  uint8_t seqno;
  uint8_t op;
  uint8_t res;
};

/*---------------------------------------------------------------------------*/
#if WITH_WVDS
enum {
  WVDS_CMD = 0x11
};

static rimeaddr_t actaddr = {{2,1}};
static rimeaddr_t txaddr;
static struct unicast_conn txunic;
#endif /* WITH_WVDS */

/*---------------------------------------------------------------------------*/
//static rimeaddr_t addr = {{0xFD,0xFF}};
static uint8_t seqno = 0;
static uint8_t recvn = 0;

#define RXBUF_LEN sizeof(struct sniff_cmd)
static uint8_t rxbuf[RXBUF_LEN];
static uint8_t rxlen = 0;

#define TXBUF_LEN 108
static uint8_t txbuf[TXBUF_LEN];
static uint8_t txlen = 0;
static uint16_t txseq = 1;

struct msg_entry {
  struct msg_entry *next;
  struct sniff_msg msg;
};
LIST(msg_queue);
MEMB(msg_mem, struct msg_entry, PKT_QUEUELEN);
/*---------------------------------------------------------------------------*/
PROCESS(sniffer_process, "Sniffer process");
AUTOSTART_PROCESSES(&sniffer_process);
/*---------------------------------------------------------------------------*/
static uint8_t
crc8(uint8_t *data, uint8_t len)
{
  uint8_t csum = 0;
  uint8_t i;

  for (i = 0; i < len; i++)
    csum += data[i];

  return csum;
}

static void
app_send_data(uint8_t type, uint8_t *data, uint8_t len)
{
  struct sniff_data sdata;

  sdata.type = SNIFF_DATA;
  sdata.seqno = seqno++;
  sdata.dtype = type;
  sdata.len = len;
  if (data != NULL && len > 0)
    memcpy(sdata.data, data, len);

  hdlc_send((uint8_t*)&sdata, offsetof(struct sniff_data,data) + sdata.len, HDLC_PROTO_PACKET_NOACK);
}
/*---------------------------------------------------------------------------*/
void
app_uart_rcvd(uint8_t *data, uint8_t len)
{
  if (rxlen == 0) {
    memcpy(rxbuf, data, len);
    rxlen = len;
    process_poll(&sniffer_process);
  }
}

static void
app_radio_rcvd(void)
{
  struct msg_entry *e = NULL;
  struct sniff_msg *msg = NULL;
  struct sniff_header *header = NULL;
  struct sniff_footer *footer = NULL;

  ++recvn;
  e = memb_alloc(&msg_mem);
  if (e == NULL) {
    leds_toggle(LEDS_RED);
  } else {
    msg = &(e->msg);
    header = (struct sniff_header *)msg;
    footer = (struct sniff_footer *)(msg->data + packetbuf_datalen());

    header->type = SNIFF_MSG;
    header->seqno = seqno++;
    header->rssi = (int8_t)packetbuf_attr(PACKETBUF_ATTR_RSSI);
    header->lqi = (int8_t)packetbuf_attr(PACKETBUF_ATTR_LINK_QUALITY);
    header->len = packetbuf_datalen();
    memcpy(msg->data, packetbuf_dataptr(), packetbuf_datalen());
    footer->crc = crc8((uint8_t *)packetbuf_dataptr(), packetbuf_datalen());

    leds_toggle(LEDS_YELLOW);
    list_add(msg_queue, e);
    process_poll(&sniffer_process);
  }
}
/*---------------------------------------------------------------------------*/
#if WITH_WVDS
static void
wvds_unic_rcvd(struct unicast_conn *c, const rimeaddr_t *from)
{
}

static void
wvds_unic_sent(struct unicast_conn *ptr, int status, int num_tx)
{
}

const static struct unicast_callbacks unic_cb = {wvds_unic_rcvd,wvds_unic_sent};
#endif /* WITH_WVDS */
/*---------------------------------------------------------------------------*/
static void
app_save_radio(uint8_t chan, int8_t power)
{
  flash_clear(RFCHAN_ADDR);
  flash_write(RFCHAN_ADDR, (uint16_t)chan);
  flash_write(RFPWR_ADDR, (uint16_t)power);
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sniffer_process, ev, data)
{
  static struct msg_entry *e = NULL;
  static uint8_t *buf = NULL;
  static int len;
  static uint8_t chan = RF_CHANNEL;
  static int8_t power = 17;

  PROCESS_BEGIN();

  hdlc_init(USART_BAUD_115200, USART_PARITY_NONE);
  hdlc_set_input(app_uart_rcvd);
  uart_set_input(hdlc_byte_rcvd);
  network_set_rcvd_cb(app_radio_rcvd);

#if WITH_WVDS
#define WVDS_NETCMD_CHANNEL 0x87
  unicast_open(&txunic, WVDS_NETCMD_CHANNEL, &unic_cb);
#endif /* WITH_WVDS */

  list_init(msg_queue);
  memb_init(&msg_mem);

#if CC11xx_CC1120
  cc11xx_set_promiscuous(1);
#endif

  // read radio channel from internal flash of MCU
  chan = (uint8_t)flash_read(RFCHAN_ADDR);
  if ((chan < RFCHAN_MIN) || (chan > RFCHAN_MAX)) {
    chan = RF_CHANNEL;
    app_save_radio(chan, power);
  }
  power = (int8_t)flash_read(RFPWR_ADDR);
  if ((power < RFPOWER_MIN) || (power > RFPOWER_MAX)) {
    power = RFPOWER_MIN;
    app_save_radio(chan, power);
  }
  radio_set_channel(chan);
  radio_set_txpower(power);

  // report the node start and radio channel
  app_send_data(SNIFF_DATA_START, NULL, 0);
  app_send_data(SNIFF_DATA_CHAN, &chan, 1);
  app_send_data(SNIFF_DATA_POWER, (uint8_t*)&power, 1);
  leds_on(LEDS_GREEN);

  while (1) {
    PROCESS_WAIT_EVENT();

    if (ev == PROCESS_EVENT_POLL) {
      if (rxlen > 0) {
        if (rxbuf[0] == SNIFF_CMD) {
          struct sniff_cmd *cmd = (struct sniff_cmd *)rxbuf;
          struct sniff_ack ack;
          int ret = 0;

          if (cmd->op == SNIFF_CMD_GETVER) { // query firmware version
            struct sniff_data data;
            uint8_t i = 0;

            data.type = SNIFF_DATA;
            data.seqno = seqno++;
            data.dtype = cmd->op;
            data.data[i++] = APP_VER_MAJOR;
            data.data[i++] = APP_VER_MINOR;
            data.data[i++] = (APP_VER_PATCH >> 8);
            data.data[i++] = (APP_VER_PATCH & 0xff);
            memcpy(data.data + i, APP_COMMIT, 7); i += 8;
            memcpy(data.data + i, APP_BUILD, 8); i += 8;
            data.len = i;

            hdlc_send((uint8_t*)&data, offsetof(struct sniff_data,data) + data.len, HDLC_PROTO_PACKET_NOACK);
          }
          else if (cmd->op == SNIFF_CMD_SETCHAN) { // set radio channel
            chan = cmd->arg[0];
            radio_set_channel(chan);
            ret = (radio_get_channel() == chan) ? 0 : 1;
            if (ret == 0) {
              app_save_radio(chan, power);
            }
          }
          else if (cmd->op == SNIFF_CMD_GETCHAN) { // get radio channel
            struct sniff_data data;

            chan = radio_get_channel();
            data.type = SNIFF_DATA;
            data.seqno = seqno++;
            data.dtype = SNIFF_DATA_CHAN;
            data.len = 1;
            data.data[0] = chan;

            hdlc_send((uint8_t*)&data, offsetof(struct sniff_data,data) + data.len, HDLC_PROTO_PACKET_NOACK);
          }
          else if (cmd->op == SNIFF_CMD_SETPWR) { // set radio txpower
            power = cmd->arg[0];
            radio_set_txpower(power);
            ret = (radio_get_txpower() == power) ? 0 : 1;
            if (ret == 0) {
              app_save_radio(chan, power);
            }
          }
          else if (cmd->op == SNIFF_CMD_GETPWR) { // get radio txpower
            struct sniff_data data;

            power = radio_get_txpower();
            data.type = SNIFF_DATA;
            data.seqno = seqno++;
            data.dtype = SNIFF_DATA_POWER;
            data.len = 1;
            data.data[0] = power;

            hdlc_send((uint8_t*)&data, offsetof(struct sniff_data,data) + data.len, HDLC_PROTO_PACKET_NOACK);
          }
          else if (cmd->op == SNIFF_CMD_SENDPKT) { // send packet
            ret = NETSTACK_RADIO.send(cmd->arg, cmd->len);
          }
          else {
            ret = 2;
          }

          if (cmd->ack) {
            ack.type = SNIFF_ACK;
            ack.seqno = cmd->seqno;
            ack.op = cmd->op;
            ack.res = ret;
            hdlc_send((uint8_t*)&ack, sizeof(ack), HDLC_PROTO_PACKET_NOACK);
          }
        }
#if WITH_WVDS
        else if (rxbuf[0] == WVDS_CMD) {
          uint8_t i;
          txaddr.u8[0] = rxbuf[2];
          txaddr.u8[1] = rxbuf[1];

          memset(txbuf, 0, sizeof(txbuf));
          i = 0;
          txbuf[i++] = 0x02; // type: netcmd cmd
          txbuf[i++] = 0x00; // more: 0
          txbuf[i++] = (txseq & 0xff); // seqnoL
          txbuf[i++] = (txseq >> 8); // seqnoH
          txbuf[i++] = txaddr.u8[0]; // destL
          txbuf[i++] = txaddr.u8[1]; // destH
          txbuf[i++] = 0x01; // msgid: wvds
          txbuf[i++] = rxlen - 3; // msglen
          memcpy(txbuf + i, rxbuf + 3, rxlen - 3); i += rxlen - 3;
          txlen = i;
          txseq++;

          rimeaddr_set_node_addr(&actaddr);
          packetbuf_copyfrom(txbuf, txlen);
          unicast_send(&txunic, &txaddr);
          //rimeaddr_set_node_addr(&addr);

#ifdef NETSTACK_DECRYPT
          NETSTACK_DECRYPT();
#endif
          i = 0;
          txbuf[i++] = SNIFF_MSG;
          txbuf[i++] = seqno++;
          txbuf[i++] = 0;
          txbuf[i++] = 0;
          txbuf[i++] = packetbuf_totlen();
          memcpy(txbuf + i, (uint8_t*)packetbuf_hdrptr(), packetbuf_totlen()); i += packetbuf_totlen();
          txbuf[i++] = crc8((uint8_t*)packetbuf_hdrptr(), packetbuf_totlen());
          txlen = i;
          hdlc_send(txbuf, txlen, HDLC_PROTO_PACKET_NOACK);
        }
#endif /* WITH_WVDS */
        rxlen = 0;
      }

      while (list_length(msg_queue) > 0) {
        e = list_head(msg_queue);

        len = sizeof(struct sniff_header) + e->msg.header.len + sizeof(struct sniff_footer);
        buf = ((uint8_t *)&e->msg);
        hdlc_send(buf, len, HDLC_PROTO_PACKET_NOACK);
        leds_toggle(LEDS_GREEN);

        list_pop(msg_queue);
        memb_free(&msg_mem, e);
      }
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
