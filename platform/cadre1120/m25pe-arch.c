#include "contiki.h"
#include "m25pe-arch.h"

void
m25pe_arch_init(void)
{
  // init CS and RST ports
  M25PE_CS0(DIR) |= M25PE_CS0_PIN;
  M25PE_CS0(OUT) |= M25PE_CS0_PIN;
  M25PE_CS1(DIR) |= M25PE_CS1_PIN;
  M25PE_CS1(OUT) |= M25PE_CS1_PIN;
  M25PE_RST(DIR) |= M25PE_RST_PIN;
  M25PE_RST(OUT) |= M25PE_RST_PIN;
  M25PE_PWR(SEL) &= ~M25PE_PWR_PIN;
  M25PE_PWR(DIR) |= M25PE_PWR_PIN;
  M25PE_PWR(OUT) |= M25PE_PWR_PIN;

  // init SPI interface
    /* Initalize ports for communication with SPI units. */
  M25PE_SPI(CTL1) |= UCSWRST;                /* Reset USCI */
  M25PE_SPI(CTL1) |= UCSSEL_2;               /* smclk while usci is reset */
  /* MSB-first 8-bit, Master, Synchronous, 3 pin SPI master, no ste,
     watch-out for clock-phase UCCKPH */
  M25PE_SPI(CTL0) |= UCMST | UCMSB | UCCKPL | UCSYNC;

  /*
   * Set up SPI bus speed. If CPU is too fast, we need to lower speed to
   * accomodate CC11xx max SPI speeds.
   */
  M25PE_SPI(BR1) = 0x00;
#if F_CPU==16000000uL
  /* Baud rate is F_CPU/4 */
  M25PE_SPI(BR0) = 0x02;
#elif F_CPU==8000000uL
  /* Baud rate is F_CPU/2 */
  M25PE_SPI(BR0) = 0x01;
#else
#error Unknown SPI UBR config for F_CPU, please configure and verify SPI comm.
#endif

  /* Dont need modulation control. */
  /* M25PE_SPI(MCTL) = 0; */

  /* Select Peripheral functionality */
  M25PE_SPI_PORT(SEL) |= BV(SCK) | BV(MOSI) | BV(MISO);
  /* Configure as outputs(SIMO,CLK). */
  M25PE_SPI_PORT(DIR) |= BV(SCK) | BV(MOSI);

  /* Clear pending interrupts before enabling. */
  M25PE_SPI(IE) &= ~UCRXIFG;
  M25PE_SPI(IE) &= ~UCTXIFG;
  /* Remove RESET before enabling interrupts */
  M25PE_SPI(CTL1) &= ~UCSWRST;

  /* Enable UCBx Interrupts */
  /* Enable USCI_Bx TX Interrupts */
  /* IE2 |= M25PE_SPI(TXIE); */
  /* Enable USCI_Bx RX Interrupts */
  /* IE2 |= M25PE_SPI(RXIE); */
}

void
m25pe_arch_on(void)
{
  M25PE_PWR(OUT) |= M25PE_PWR_PIN;
  __delay_cycles((long)F_CPU>>9);
  m25pe_arch_init();
}

void
m25pe_arch_off(void)
{
  M25PE_PWR(OUT) &= ~M25PE_PWR_PIN;
  M25PE_SPI_PORT(SEL) &= ~(BV(SCK) | BV(MOSI) | BV(MISO));
  M25PE_SPI_PORT(DIR) &= ~(BV(SCK) | BV(MOSI) | BV(MISO));
  M25PE_SPI_PORT(REN) |=  (BV(SCK) | BV(MOSI) | BV(MISO));
  M25PE_SPI_PORT(OUT) |=  (BV(SCK) | BV(MOSI) | BV(MISO));
}