#ifndef _M25PE_ARCH_H
#define _M25PE_ARCH_H

#define M25PE_SPI(reg)       UCB2##reg
#define M25PE_SPI_PORT(reg)  P9##reg

#define M25PE_SPI_TXBUF UCB2TXBUF
#define M25PE_SPI_RXBUF UCB2RXBUF

#define M25PE_SPI_WAITFOREOTx() while ((UCB2STAT & UCBUSY) != 0)
                                /* USARTx Rx ready? */
#define M25PE_SPI_WAITFOREORx() while ((UCB2IFG & UCRXIFG) == 0)
                                /* USARTx Tx buffer ready? */
#define M25PE_SPI_WAITFORTxREADY() while ((UCB2IFG & UCTXIFG) == 0)

#define M25PE_SPI_WAITFORTx_BEFORE() M25PE_SPI_WAITFORTxREADY()


#define M25PE_RST(reg)    P8##reg
#define M25PE_RST_PIN     BIT7

#define M25PE_PWR(reg)    P10##reg
#define M25PE_PWR_PIN     BIT0

// 66 - P8.6/TA1.1
#define M25PE_CS0(reg)    P8##reg
#define M25PE_CS0_PIN     BIT6
// 65 - P8.5/TA1.0
#define M25PE_CS1(reg)    P8##reg
#define M25PE_CS1_PIN     BIT5

extern int m25pe_dev;

#define M25PE_SELECT(x) do { \
  if ((x) == 0) M25PE_CS0(OUT) &= ~M25PE_CS0_PIN; \
  if ((x) == 1) M25PE_CS1(OUT) &= ~M25PE_CS1_PIN; \
} while(0)

#define M25PE_DESELECT(x) do { \
  if ((x) == 0) M25PE_CS0(OUT) |=  M25PE_CS0_PIN; \
  if ((x) == 1) M25PE_CS1(OUT) |=  M25PE_CS1_PIN; \
} while(0)

void m25pe_arch_init(void);
void m25pe_arch_on(void);
void m25pe_arch_off(void);

#endif /* _M25PE_ARCH_H */
