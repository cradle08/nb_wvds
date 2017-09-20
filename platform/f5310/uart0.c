#include "contiki.h"
#include "sys/ctimer.h"
#include "uart0.h"
///#include "dev/uart3.h"
#include <string.h>

static uint8_t  uart_rxbuf[UART_RXBUF_LEN];
static uint16_t uart_rxidx = 0;
static volatile uint8_t transmitting;

//static int (*uart0_input_handler)(uint8_t *data, uint16_t len);
static int (*uart0_input_handler)(unsigned char c);
static void (*uart0_sent_callback)(void);

static struct ctimer uart_ct;
static struct ctimer uart_st;

static int uart0_byte_rcvd(uint8_t c);
static void uart0_pkt_rcvd(void *ptr);


//***
void
uart0_init(unsigned long baud)
{
  // set as UCSI p4.5 X,p4.4 R
  P4SEL |= 0x30;
  P4DIR |= BIT5;
  P4DIR &= ~BIT4;
  //
  UCA1CTL1 |= UCSWRST;
  UCA1CTL1 |= UCSSEL_2;
  UCA1BR0 = baud & 0xff;
  UCA1BR1 = UCBRS_3;

  transmitting = 0;
  // clear r x flag
  UCA1IE &= ~(UCRXIFG + UCRXIFG);
  UCA1CTL1 &= ~UCSWRST;
  UCA1IE |= UCRXIE;
}

//***
uint8_t
uart0_active(void)
{
  uint8_t busy = (UCA1STAT & UCBUSY);
  return (busy | transmitting);

}

//***
void
uart0_set_input(int (*input)(unsigned char c))
{
  uart0_input_handler = input;
}

//***
void
uart0_set_sent_callback(void (*f)(void))
{
  uart0_sent_callback = f;
}

//***
void
uart0_writeb(uint8_t c)
{
//  watchdog_periodic();
  /* Loop until the transmission buffer is available. */
  while((UCA1STAT & UCBUSY));

  /* Transmit the data. */
  UCA1TXBUF = c;
}

//*** For printf support
int
putchar(int c)
{
  uart0_writeb((char)c);
 // uart3_writeb((char)c);
  return c;
}

//***
static void
uart0_sent_task(void *ptr)
{
  if (uart0_sent_callback)
    uart0_sent_callback();
}

//***
int
uart0_send(uint8_t *data, uint16_t len)
{
  uint16_t i;
  for (i = 0; i < len; i++) {
    uart0_writeb(data[i]);
//    uart3_writeb(data[i]);

  }
  ctimer_set(&uart_st, (CLOCK_SECOND >> 7), uart0_sent_task, NULL);
  return 0;
}

/*
void
uart0_set_frame_input(int (*f)(uint8_t *data, uint16_t len))
{
  uart0_input_handler = f;
}
*/

//***
static void
uart0_pkt_rcvd(void *ptr)
{
//  if (uart0_input_handler)
//    uart0_input_handler(uart_rxbuf, uart_rxidx);
//  uart_rxidx = 0;
//  memset(uart_rxbuf, 0, sizeof(uart_rxbuf));
}

//***
static int
uart0_byte_rcvd(uint8_t c)
{
//  if (uart_rxidx < sizeof(uart_rxbuf)) {
//   uart_rxbuf[uart_rxidx++] = c;
//   ctimer_set(&uart_ct, (CLOCK_SECOND / 128), uart0_pkt_rcvd, NULL);
//  } else {
//    uart0_pkt_rcvd(NULL);/
//  }
  return 1;
}

//***
ISR(USCI_A1, uart0_rx_interrupt)
{
  uint8_t c;
  /*leds_toggle(LEDS_ALL);*/
  if(UCA1IV == 2) {
    if(UCA1STAT & UCRXERR) {
      c = UCA1RXBUF;   /* Clear error flags by forcing a dummy read. */
    } else {
      c = UCA1RXBUF;
      if(uart0_input_handler != NULL) {
        if(uart0_input_handler(c)) {
          LPM4_EXIT;
        }
      }
    }
  }
}


