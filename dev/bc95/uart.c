#include "system.h"
#include "uart.h"

#define UART_TIMEOUT  0X8000

__no_init UART_RX_STR uartRMsg;

/*================================================================
【名 称】void uart1_init(u32 baudrate)
【功 能】串口初始化
【备 注】
================================================================*/
void uart1_init(u32 baudrate)
{
  P4SEL |= BIT4+BIT5;			

  UCA1CTL1 |= UCSWRST;	
  UCA1CTL1 |= UCSSEL_2;		//SMCLK   1048576HZ

  //according differ rate
  UCA1BR0 = (1068576/baudrate);//1058576
  UCA1BR1 = 0;

  UCA1CTL1 &= ~UCSWRST; 		
  UCA1IFG	&= ~UCRXIFG;    
  UCA1IE |= UCRXIE;  
  UCA1IE &= ~UCTXIE;
  
  return;
}



void uart1_Tx(u8 nums,u8* buf)
{
  while(nums--)
  {
    UCA1TXBUF = *buf;
    buf++;
    while(!(UCA1IFG & UCTXIFG));
  }
}

void UART1_cmd(void)
{
  uartRMsg.rxIndex = 0;
  uartRMsg.rxS = UART_TIMEOUT;
  
  uartRMsg.rxbuf[uartRMsg.rxIndex] = UCA1RXBUF;
  uartRMsg.rxbuf[uartRMsg.rxIndex+1] = '\0';
  
  while((uartRMsg.rxS > 0) && (uartRMsg.rxIndex < BUF_SIZE))
  {
    uartRMsg.rxS = UART_TIMEOUT;
    while (!(UCA1IFG & UCRXIFG) && (--uartRMsg.rxS));
    uartRMsg.rxbuf[++uartRMsg.rxIndex] = UCA1RXBUF;
    uartRMsg.rxbuf[uartRMsg.rxIndex+1] = '\0';  
  }
  uartRMsg.rxOK = TRUE;
  return;
   
}

void get_UART1_data(void)
{
  u8 i;
  switch(__even_in_range(UCA1IV,4))
  {
    case 0: break;                          // Vector 0 - no interrupt
    case 2:                                 // Vector 2 - RXIFG
      
      UART1_cmd();
      for(i = 20; i>0; i--);                // Add time between transmissions to                                            
                                             // make sure slave can process information
      break;
    case 4: break;                          // Vector 4 - TXIFG
    default: break;
  }
    
}


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
