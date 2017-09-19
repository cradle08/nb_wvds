#ifndef __UART_H__
#define __UART_H__

#ifndef UART_RXBUF_LEN
#define UART_RXBUF_LEN  160
#endif

int uart1_init(unsigned long baud);
uint8_t uart1_active(void);
void uart1_writeb(uint8_t b);
int uart1_send(uint8_t *data, uint16_t len);
void uart1_set_input(int (*input)(unsigned char c));
void uart1_set_frame_input(int (*f)(uint8_t *data, uint16_t len));
void uart1_set_sent_callback(void (*f)(void));

#endif /* __UART_H__ */
