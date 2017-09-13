#include "contiki.h"
#include "dev/rs232.h"
#include "dev/leds.h"
#include "hdlc.h"

#include <string.h>

#include <stdio.h>
#if CONTIKI_TARGET_COOJA
#define DEBUG 0
#else
#define DEBUG 0
#endif
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static uint8_t insyn = 0;
static uint8_t inesc = 0;

static uint8_t rxbuf[HDLC_FRAME_MAXLEN];
static uint8_t rxidx = 0;
static uint8_t rxlen = 0;

static uint8_t txbuf[HDLC_FRAME_MAXLEN];
static uint8_t txlen = 0;
static uint8_t txseq = 0;

void (* input_cb)(uint8_t *data, uint8_t len);

static int hdlc_err(uint8_t e);
/*---------------------------------------------------------------------------*/
PROCESS(hdlc_process, "hdlc process");
/*---------------------------------------------------------------------------*/
void
printd(uint8_t *data, uint8_t len)
{
#if DEBUG
  uint8_t i;

  for (i = 0; i < len; i++)
    printf(" %02X", data[i]);
  printf("\n");
#endif
}
/*---------------------------------------------------------------------------*/
int
hdlc_valid(uint8_t *frame, uint8_t len)
{
  return (frame[len-1]==0x7E && frame[0]==0x7E) ? 1 : 0;
}

int
hdlc_byte_rcvd(uint8_t b)
{
  if (rxlen == 0) {
    if (!insyn) {
      if (b == HDLC_FLAG_BYTE) {
        insyn = 1;
        rxidx = 0;
        rxbuf[rxidx++] = b;
      }
    }
    else {
      if (inesc) {
        if (b == HDLC_FLAG_BYTE) {
          insyn = 0;
          rxidx = 0;
          memset(rxbuf, 0, HDLC_FRAME_MAXLEN);
          return 1;
        }
        rxbuf[rxidx++] = (b ^ 0x20);
        inesc = 0;
      }
      else if (b == HDLC_ESC_BYTE) {
        inesc = 1;
      }
      else if (b == HDLC_FLAG_BYTE) {
        if (rxidx == 1) {
          // this should be head of next packet
          rxidx = 0;
          rxbuf[rxidx++] = b;
          return 0;
        }
        else if (rxidx < 4) {
          // too small
          insyn = 0;
          rxidx = 0;
          memset(rxbuf, 0, HDLC_FRAME_MAXLEN);
          hdlc_err(HDLC_ERR_TOOSMALL);
          return 1;
        }
        rxbuf[rxidx++] = b;
        rxlen = rxidx;
        insyn = 0;
        rxidx = 0;
        PRINTF("hdlc poll, len %d\n", rxlen);
        process_poll(&hdlc_process);
        return 0;
      }
      else {
        rxbuf[rxidx++] = b;
      }

      if (rxidx > HDLC_FRAME_MAXLEN) {
        // too long
        insyn = 0;
        rxidx = 0;
        memset(rxbuf, 0, HDLC_FRAME_MAXLEN);
        hdlc_err(HDLC_ERR_TOOLONG);
        return 1;
      }
    }
  }
  else {
    hdlc_err(HDLC_ERR_BUSY); // rx busy
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
void
hdlc_init(uint8_t baud, uint8_t mode)
{
#if CONTIKI_TARGET_COOJA
  PRINTF("hdlc init\n");
  //rs232_init();
  //rs232_set_input(hdlc_byte_rcvd);
  sim_host_start();
  sim_host_set_output(hdlc_byte_rcvd);

#elif CONTIKI_TARGET_MICAA
  rs232_init(RS232_PORT_0, baud,
    mode | USART_STOP_BITS_1 | USART_DATA_BITS_8);
  rs232_set_input(RS232_PORT_0, hdlc_byte_rcvd);

#elif CONTIKI_TARGET_STM8
  rs232_init(0, baud, mode);
  rs232_set_input(0, hdlc_byte_rcvd);

#elif CONTIKI_TARGET_CADRE1120
  

#else
#error "not support this platform"
#endif

  process_start(&hdlc_process, NULL);
}

static uint16_t
hdlc_crc_byte(uint16_t crc, uint8_t b)
{
  uint8_t i;

  crc = crc ^ (b << 8);
  i = 8;
  do {
    if (crc & 0x8000)
      crc = (crc << 1) ^ 0x1021;
    else
      crc = (crc << 1);
  } while (--i);

  return crc;
}

static uint16_t
hdlc_crc(uint8_t *buf, uint8_t len)
{
  uint16_t crc = 0x0000;
  uint8_t i;

  for (i = 0; i < len; i++)
    crc = hdlc_crc_byte(crc, buf[i]);

  return crc;
}

void
hdlc_decode(uint8_t *outbuf, uint8_t *outlen, uint8_t *inbuf, uint8_t inlen)
{
  uint8_t i = 0;
  uint8_t j;
  uint8_t k = 0;
  uint8_t b;
  uint8_t inesc = 0;
  uint8_t proto = inbuf[1];

  for (j = 0; j < inlen; j++) {
    if (inbuf[j] == HDLC_FLAG_BYTE) {
      b = inbuf[j]; k++;
    } else if (inbuf[j] == HDLC_ESC_BYTE) {
      inesc = 1;
    } else if (inesc) {
      inesc = 0; b = (inbuf[j] ^ 0x20); k++;
    } else {
      b = inbuf[j]; k++;
    }

    if (proto == HDLC_PROTO_PACKET_ACK || proto == HDLC_PROTO_PACKET_NOACK) {
      if (k > 4) outbuf[i++] = b;
    } else if (proto == HDLC_PROTO_ACK) {
      if (k > 2) outbuf[i++] = b;
    }
  }
  *outlen = i - 3; // exclude two crc bytes and end flag

  //PRINTF("hdlc decoded, ptr %d, len %d\n", outbuf, *outlen);
  //printd(outbuf, *outlen);
}

void
hdlc_encode(uint8_t *outbuf, uint8_t *outlen, uint8_t *inbuf, uint8_t inlen, uint8_t proto, uint8_t seqno, uint8_t D)
{
  uint8_t i, j, b;
  uint16_t ccrc = 0x0000;

  i = 0;
  outbuf[i++] = HDLC_FLAG_BYTE;
  for (j = 0; j < 3+inlen+2; j++) {
    if (proto == HDLC_PROTO_PACKET_ACK || proto == HDLC_PROTO_PACKET_NOACK) {
      if (j < 1) b = proto;
      else if (j < 2) b = seqno;
      else if (j < 3) b = D;
      else if (j < 3+inlen) b = inbuf[j-3];
      else if (j < 3+inlen+1) b = (ccrc & 0xFF);
      else if (j < 3+inlen+2) b = (ccrc >> 8);

      if (j < 3+inlen)
        ccrc = hdlc_crc_byte(ccrc, b);
    }
    else if (proto == HDLC_PROTO_ACK) {
      if (j < 1) b = proto;
      else if (j < 2) b = inbuf[j-1];
      else if (j < 3) b = (ccrc & 0xFF);
      else if (j < 4) b = (ccrc >> 8);
      else continue;

      if (j < 2)
        ccrc = hdlc_crc_byte(ccrc, b);
    }

    if (b == HDLC_FLAG_BYTE || b == HDLC_ESC_BYTE) {
      outbuf[i++] = HDLC_ESC_BYTE;
      outbuf[i++] = (b ^ 0x20);
    } else {
      outbuf[i++] = b;
    }
  }
  outbuf[i++] = HDLC_FLAG_BYTE;
  *outlen = i;

  //PRINTF("hdlc encoded, ptr %d, len %d\n", outbuf, *outlen);
  //printd(outbuf, *outlen);
}

int
hdlc_send(uint8_t *data, uint8_t len, uint8_t proto)
{
  uint8_t i;

  if (len > HDLC_FRAME_MAXLEN - 6) {
    PRINTF("warn hdlc send fail %d>%d\n", len, (HDLC_FRAME_MAXLEN-6));
    return 0;
  }

  PRINTF("hdlc send len %d\n", len);
  hdlc_encode(txbuf, &txlen, data, len, proto, txseq++, 0);
  printd(txbuf, txlen);

  for (i = 0; i < txlen; i++) {
#if CONTIKI_TARGET_COOJA
    //rs232_send(txbuf[i]);
    //printf("%02X ", txbuf[i]);
#else
    rs232_send(RS232_PORT_0, txbuf[i]);
#endif
  }
#if CONTIKI_TARGET_COOJA
  //rs232_send('\n');
  //printf("\n");
  sim_host_rcvd(txbuf, txlen);
#endif

  return txlen;
}

void
hdlc_set_input(void (*f)(uint8_t *data, uint8_t len))
{
  input_cb = f;
}

int
hdlc_ack(uint8_t seq)
{
  uint8_t buf[1];

  PRINTF("hdlc ack %d\n", seq);
  buf[0] = seq;
  hdlc_send(buf, 1, HDLC_PROTO_ACK);

  return 0;
}

static int
hdlc_err(uint8_t e)
{
#if 0
  uint8_t buf[1];

  buf[0] = e;
  hdlc_send(buf, 1, HDLC_PROTO_PACKET_NOACK);
#endif
  return 0;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(hdlc_process, ev, data)
{
  static hdlc_header_t *header;
  static hdlc_footer_t *footer;
  static uint8_t *payload;
  static uint8_t payload_len;

  PROCESS_BEGIN();

  while (1) {
    PROCESS_WAIT_EVENT();

    if (ev == PROCESS_EVENT_POLL) {
      header = (hdlc_header_t *)rxbuf;
      footer = (hdlc_footer_t *)(rxbuf + rxlen - sizeof(hdlc_footer_t));

      // send ACK if the packet required
      if (header->P == HDLC_PROTO_PACKET_ACK) {
        uint16_t rcrc = 0;
        uint16_t ccrc = 0;

        rcrc = footer->CS[0] + (footer->CS[1] << 8);
        ccrc = hdlc_crc(rxbuf + 1, rxlen - 4);

        if (ccrc == rcrc)
          hdlc_ack(header->S);
        else
          hdlc_err(HDLC_ERR_CRCX);
      }

      // deliver the packet to upper layer
      if (header->P == HDLC_PROTO_PACKET_ACK || header->P == HDLC_PROTO_PACKET_NOACK) {
        PRINTF("hdlc rcvd, len %d\n", rxlen);
        printd(rxbuf, rxlen);

        if (input_cb) {
          payload = (uint8_t *)(rxbuf + sizeof(hdlc_header_t));
          payload_len = rxlen - sizeof(hdlc_header_t) - sizeof(hdlc_footer_t);
          input_cb(payload, payload_len);
        }
      }

      // unlock the rxbuf to receive next
      rxlen = 0;
      rxidx = 0;
      memset(rxbuf, 0, HDLC_FRAME_MAXLEN);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
