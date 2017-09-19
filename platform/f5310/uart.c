#include "contiki.h"
#include "sys/ctimer.h"
#include "uart1.h"
///#include "dev/uart3.h"
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

  uart1_init(baud);
//  uart3_init(baud); /* Must come before first printf */
//  uart3_set_input(uart_byte_rcvd);

  return 0;
}

uint8_t
uart_active(void)
{

  return uart1_active();
//  return uart3_active();

}

void
uart_set_input(int (*input)(unsigned char c))
{
  uart1_set_input(input);
//  uart3_set_input(input);
}

void
uart_set_sent_callback(void (*f)(void))
{
  uart_sent_callback = f;
}

// For printf support
int
putchar(int c)
{
  uart1_writeb((char)c);
 // uart3_writeb((char)c);
  return c;
}

//***
void
uart_writeb(uint8_t b)
{
  uart1_writeb(b);
//  uart3_writeb(b);
}

//***
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
    uart1_writeb(data[i]);
//    uart3_writeb(data[i]);

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
