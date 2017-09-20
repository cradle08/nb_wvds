#ifndef __UART0_H__
#define __UART0_H__

#ifndef UART_RXBUF_LEN
#define UART_RXBUF_LEN  160
#endif

void uart0_init(unsigned long baud);
uint8_t uart0_active(void);
void uart0_writeb(uint8_t b);
int uart0_send(uint8_t *data, uint16_t len);
void uart0_set_input(int (*input)(unsigned char c));
void uart0_set_frame_input(int (*f)(uint8_t *data, uint16_t len));
void uart0_set_sent_callback(void (*f)(void));

#endif /* __UART0_H__ */
