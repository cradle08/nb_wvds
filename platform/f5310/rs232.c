#include "dev/rs232.h"
#include "dev/uart1.h"
#include "dev/uart3.h"

int
rs232_send(int port, uint8_t b)
{
#if BOARD_CADRE1120_AP
  uart1_writeb(b);
#elif BOARD_CADRE1120_VD
  uart3_writeb(b);
#else
#warning "no support"
#endif
  return 0;
}