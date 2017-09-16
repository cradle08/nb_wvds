#include "contiki.h"

#define HDLC_FRAME_MAXLEN 160

enum {
  HDLC_FLAG_BYTE = 0x7E,
  HDLC_ESC_BYTE  = 0x7D
};

enum {
  HDLC_PROTO_ACK = 0x43,
  HDLC_PROTO_PACKET_ACK = 0x44,
  HDLC_PROTO_PACKET_NOACK = 0x45,
  HDLC_PROTO_PACKET_UNKNOWN = 0xFF
};

enum {
  HDLC_ERR_BUSY     = 0x01,
  HDLC_ERR_CRCX     = 0x02,
  HDLC_ERR_TOOLONG  = 0x03,
  HDLC_ERR_TOOSMALL = 0x04
};

typedef struct {
  uint8_t B;
  uint8_t P;
  uint8_t S;
  uint8_t D;
} hdlc_header_t;

typedef struct {
  uint8_t CS[2];
  uint8_t E;
} hdlc_footer_t;

typedef struct {
  uint8_t B;
  uint8_t P;
  uint8_t S;
  uint8_t D;
  uint8_t data[1];
  uint8_t CS;
  uint8_t E;
} hdlc_msg_t;

typedef struct {
  uint8_t B;
  uint8_t P;
  uint8_t S;
  uint8_t E;
} hdlc_ack_t;

void hdlc_init(uint8_t baud, uint8_t mode);
int hdlc_send(uint8_t *data, uint8_t len, uint8_t proto);
void hdlc_set_input(void (*f)(uint8_t *data, uint8_t len));
int hdlc_byte_rcvd(uint8_t b);
void hdlc_encode(uint8_t *outbuf, uint8_t *outlen, uint8_t *inbuf, uint8_t inlen, uint8_t proto, uint8_t seqno, uint8_t D);
void hdlc_decode(uint8_t *outbuf, uint8_t *outlen, uint8_t *inbuf, uint8_t inlen);
