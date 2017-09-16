#ifndef _UART_H_
#define _UART_H_

#define BUF_SIZE  512

/***********************************************************/

typedef struct{
  u8 rxOK;
  u16 rxIndex;
  u16 rxS;
  u8 rxbuf[BUF_SIZE];  
}UART_RX_STR;

/***********************************************************/

void uart1_init(u32 baudrate);
void uart1_Tx(u8 nums,u8* buf);
void uart_send(u8 *dat,u16 count);
void get_UART1_data(void);
/***********************************************************/

extern UART_RX_STR uartRMsg;

#endif