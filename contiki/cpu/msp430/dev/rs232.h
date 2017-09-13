#ifndef _RS232_H
#define _RS232_H

#include "contiki.h"

#define RS232_PORT_0  0

#define USART_BAUD_115200  7

#define USART_PARITY_NONE  0

int rs232_send(int port, uint8_t b);

#endif /* _RS232_H */
