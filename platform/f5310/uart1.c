#include "contiki.h"
#include "sys/ctimer.h"
#include "uart1.h"
///#include "dev/uart3.h"
#include <string.h>

static uint8_t  uart_rxbuf[UART_RXBUF_LEN];
static uint16_t uart_rxidx = 0;
static volatile uint8_t transmitting;

//static int (*uart1_input_handler)(uint8_t *data, uint16_t len);
static int (*uart1_input_handler)(int8_t c);
static void (*uart1_sent_callback)(void);

static struct ctimer uart_ct;
static struct ctimer uart_st;

static int uart1_byte_rcvd(uint8_t c);
static void uart1_pkt_rcvd(void *ptr);


//***
void
uart1_init(uint16_t baud) // baud = 8M/9600
{
#if OS_DEBUG
  // set as UCSI p4.5 R, p4.4 T
  P4SEL |= BIT4 + BIT5;
  P4DIR |= BIT4;
  P4DIR &= ~BIT5;
  //hold peripheral in reset state
  UCA1CTL1 |= UCSWRST;
  // smclk
  UCA1CTL1 |= UCSSEL__SMCLK; // 8M
  // set baud rate
  UCA1BR0 = baud & 0xff;
  UCA1BR1 = baud >> 8;
  // Modulation UCBRSx = 3 
  //UCA1MCTL = UCBRS_3 | UCBRF_0;
  
  transmitting = 0;
  // clear r x flag
  UCA1IE &= ~(UCRXIFG + UCRXIFG);
  // Initialize USCI state machine **before** enabling interrupts
  UCA1CTL1 &= ~UCSWRST;
  UCA1IE |= UCRXIE;
  
#else
  
  P4SEL |= BIT4+BIT5;			
  UCA1CTL1 |= UCSWRST;	
  UCA1CTL1 |= UCSSEL_2;		//SMCLK   1048576HZ
  //according differ rate
  UCA1BR0=(1058576/9600);
//  UCA1BR0=(1048576/9600);
  UCA1BR1=0;

  UCA1CTL1 &= ~UCSWRST; 		
  UCA1IFG	&= ~UCRXIFG;    
  UCA1IE |= UCRXIE;  
  UCA1IE &= ~UCTXIE;

#endif
}

//***
uint8_t
uart1_active(void)
{
  uint8_t busy = (UCA1STAT & UCBUSY);
  return (busy | transmitting);

}

//***
void
uart1_set_input(int (*input)(int8_t c))
{
  uart1_input_handler = input;
}

//***
void
uart1_set_sent_callback(void (*f)(void))
{
  uart1_sent_callback = f;
}



//*** For printf support
/*
int
putchar(int c)
{
  uart1_writeb((char)c);
  return c;
}
*/

//***
void
uart1_writeb(uint8_t c)
{
//  //  watchdog_periodic(); // ...
    while((UCA1STAT & UCBUSY));
    UCA1TXBUF = c;

//    UCA1TXBUF = c;
//    while(!(UCA1IFG & UCTXIFG));

}

//***
static void
uart1_sent_task(void *ptr)
{
  if (uart1_sent_callback)
    uart1_sent_callback();
}

//***
int
uart1_send(uint8_t *data, uint16_t len)
{
  uint16_t i;
  for (i = 0; i < len; i++) {
    uart1_writeb(data[i]);
//    uart3_writeb(data[i]);
  }
 // ctimer_set(&uart_st, (CLOCK_SECOND >> 7), uart1_sent_task, NULL);
  return 0;
}

/*
void
uart1_set_frame_input(int (*f)(uint8_t *data, uint16_t len))
{
  uart1_input_handler = f;
}
*/

//***
static void
uart1_pkt_rcvd(void *ptr)
{
//  if (uart1_input_handler)
//    uart1_input_handler(uart_rxbuf, uart_rxidx);
//  uart_rxidx = 0;
//  memset(uart_rxbuf, 0, sizeof(uart_rxbuf));
}

//***
static int
uart1_byte_rcvd(uint8_t c)
{
  if (uart_rxidx < sizeof(uart_rxbuf)) {
    uart_rxbuf[uart_rxidx++] = c;
    ctimer_set(&uart_ct, (CLOCK_SECOND / 128), uart1_pkt_rcvd, NULL);
  } else {
    uart1_pkt_rcvd(NULL);
  }
  return 1;
}

/*
//***
ISR(USCI_A1, uart1_rx_interrupt)
{
  uint8_t c;
  //leds_toggle(LEDS_ALL);
  if(UCA1IV == 2) {
    if(UCA1STAT & UCRXERR) {
      c = UCA1RXBUF;   // Clear error flags by forcing a dummy read
    } else {
      c = UCA1RXBUF;
      if(uart1_input_handler != NULL) {
        if(uart1_input_handler(c)) {
          LPM4_EXIT;
        }
      }
    }
  }
}

*/


//#pragma vector=USCI_A1_VECTOR
//__interrupt void USCI_A1_ISR(void)
//{
//    uartRMsg.rxbuf[0] = UCA1RXBUF;
//    _DINT();
//    UCA1IE &=~UCRXIE;     
//    UART1_cmd();
//    uartRMsg.rxS = UART_TIMEOUT;
//    uartRMsg.rxOK = TRUE;
//    UCA1IE |=UCRXIE;  
//    _EINT();
//
//}
