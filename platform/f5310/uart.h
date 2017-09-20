#ifndef __UART_H__
#define __UART_H__

#ifndef UART_RXBUF_LEN
#define UART_RXBUF_LEN  512
#endif

int uart_init(unsigned long baud);
uint8_t uart_active(void);
void uart_writeb(uint8_t b);
int uart_send(uint8_t *data, uint16_t len);
void uart_set_input(int (*input)(unsigned char c));
void uart_set_frame_input(int (*f)(uint8_t *data, uint16_t len));
void uart_set_sent_callback(void (*f)(void));

#endif /* __UART_H__ */
