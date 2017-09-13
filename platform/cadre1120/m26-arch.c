#include "dev/uart3.h"

int
m26_arch_init(void)
{
  P8SEL &= ~(BIT1 + BIT3);
  P8DIR |=  (BIT1 + BIT3);
  P8OUT &= ~(BIT1 + BIT3);

  uart3_init(BAUD2UBR(115200));
  return 0;
}

// P8.1 and P8.3 both set high to turn on the module
void
m26_arch_turnon(void)
{
  P8OUT |=  (BIT1 + BIT3);
}

// P8.1 set low to turn off the module
void
m26_arch_turnoff(void)
{
  P8OUT &= ~(BIT1);
}

// P8.3 set low to power off the module
void
m26_arch_poweroff(void)
{
  P8OUT &= ~(BIT3);
}

int
m26_arch_send(uint8_t b)
{
  uart3_writeb(b);
  return 0;
}

void
m26_arch_set_input(int (*input)(unsigned char c))
{
  uart3_set_input(input);
}
