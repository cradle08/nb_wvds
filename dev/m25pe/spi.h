#include "system.h"

#ifndef _SPI_H
#define _SPI_H

/* bit banging */ 
#define SPI_PORT(reg)    P1##reg

#define SCK_PIN         BIT3    // SPI Serial Clock
#define MISO_PIN        BIT4    // SPI Master in
#define MOSI_PIN        BIT5    // SPI Master out
#define CS_PIN          BIT6    // Chip Select
#define W_PIN           BIT7    // Write Protect

#define CS_LOW          P1OUT &= ~BIT6   
#define CS_HIGH         P1OUT |= BIT6            
#define SCK_LOW         P1OUT &= ~BIT3    
#define SCK_HIGH        P1OUT |= BIT3   
#define MISO_LOW        ~(P1IN & BIT4)     
#define MISO_HIGH       P1IN  & BIT4     
#define MOSI_LOW        P1OUT &= ~BIT5     
#define MOSI_HIGH       P1OUT |= BIT5   
#define TSL_LOW         P1OUT &= ~BIT7;
#define TSL_HIGH        P1OUT |= BIT7;

#define M25PE_SELECT() do { \
          CS_LOW;  \
} while(0)

#define M25PE_DESELECT() do { \
          CS_HIGH; \
} while(0)


void spi_init(void);
void m25pe_write_byte(u8 data);
u8 m25pe_read_byte(void);

#endif