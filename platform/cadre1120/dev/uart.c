#include "contiki.h"
#include "sys/ctimer.h"
#include "dev/uart.h"
#include "dev/uart1.h"
#include "dev/uart3.h"
#include <string.h>

static uint8_t  uart_rxbuf[UART_RXBUF_LEN];
static uint16_t uart_rxidx = 0;

static int (*uart_input_handler)(uint8_t *data, uint16_t len);
static void (*uart_sent_callback)(void);

static struct ctimer uart_ct;
static struct ctimer uart_st;
/*-----------------------------------------------------------------*/
static int uart_byte_rcvd(uint8_t c);
static void uart_pkt_rcvd(void *ptr);
/*-----------------------------------------------------------------*/
int
uart_init(unsigned long baud)
{
#if BOARD_CADRE1120_AP
  uart1_init(baud);
  uart1_set_input(uart_byte_rcvd);
#elif BOARD_CADRE1120_VD
  uart3_init(baud); /* Must come before first printf */
  uart3_set_input(uart_byte_rcvd);
#else
#error "no support"
#endif
  return 0;
}

uint8_t
uart_active(void)
{
#if BOARD_CADRE1120_AP
  return uart1_active();
#elif BOARD_CADRE1120_VD
  return uart3_active();
#else
#error "no support"
#endif
}

void
uart_set_input(int (*input)(unsigned char c))
{
#if BOARD_CADRE1120_AP
  uart1_set_input(input);
#elif BOARD_CADRE1120_VD
  uart3_set_input(input);
#else
#error "no support"
#endif
}

void
uart_set_sent_callback(void (*f)(void))
{
  uart_sent_callback = f;
}

#if 1
// For printf support
int
putchar(int c)
{
#if BOARD_CADRE1120_AP
  uart1_writeb((char)c);
#elif BOARD_CADRE1120_VD
  uart3_writeb((char)c);
#else
#error "no support"
#endif
#if WITH_LCD
  lcd_write_char((char)c);
#endif
  return c;
}
#endif

void
uart_writeb(uint8_t b)
{
#if BOARD_CADRE1120_AP
  uart1_writeb(b);
#elif BOARD_CADRE1120_VD
  uart3_writeb(b);
#else
#error "no support"
#endif
}

static void
uart_sent_task(void *ptr)
{
  if (uart_sent_callback)
    uart_sent_callback();
}

int
uart_send(uint8_t *data, uint16_t len)
{
  uint16_t i;
  for (i = 0; i < len; i++) {
#if BOARD_CADRE1120_AP
    uart1_writeb(data[i]);
#elif BOARD_CADRE1120_VD
    uart3_writeb(data[i]);
#else
#error "no support"
#endif
  }
  ctimer_set(&uart_st, (CLOCK_SECOND >> 7), uart_sent_task, NULL);
  return 0;
}

void
uart_set_frame_input(int (*f)(uint8_t *data, uint16_t len))
{
  uart_input_handler = f;
}

static void
uart_pkt_rcvd(void *ptr)
{
  if (uart_input_handler)
    uart_input_handler(uart_rxbuf, uart_rxidx);
  uart_rxidx = 0;
  memset(uart_rxbuf, 0, sizeof(uart_rxbuf));
}

static int
uart_byte_rcvd(uint8_t c)
{
  if (uart_rxidx < sizeof(uart_rxbuf)) {
    uart_rxbuf[uart_rxidx++] = c;
    ctimer_set(&uart_ct, (CLOCK_SECOND / 128), uart_pkt_rcvd, NULL);
  } else {
    uart_pkt_rcvd(NULL);
  }
  return 1;
}
