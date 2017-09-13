#ifndef _DS3231_ARCH_H
#define _DS3231_ARCH_H

#include "contiki.h"

#define delay_us(x) __delay_cycles((long)(F_CPU*(double)x/1000000.0))

#define SCL(reg)    P3##reg
#define SCL_PIN     BIT4
#define SDA(reg)    P3##reg
#define SDA_PIN     BIT5
#define INT(reg)    P2##reg
#define INT_PIN     BIT7

#define SET_SCL     do { SCL(SEL)&=~SCL_PIN;  SCL(DIR)|=SCL_PIN;  SCL(OUT)|=SCL_PIN; } while(0)
#define CLR_SCL     do { SCL(SEL)&=~SCL_PIN;  SCL(DIR)|=SCL_PIN;  SCL(OUT)&=~SCL_PIN; } while(0)

#define SET_SDA     do { SDA(SEL)&=~SDA_PIN;  SDA(DIR)|=SDA_PIN;  SDA(OUT)|=SDA_PIN; } while(0)
#define CLR_SDA     do { SDA(SEL)&=~SDA_PIN;  SDA(DIR)|=SDA_PIN;  SDA(OUT)&=~SDA_PIN; } while(0)

#define SDA_IN_P    do { SDA(SEL)&=~SDA_PIN;  SDA(DIR)&=~SDA_PIN; } while(0)
#define SDA_IN_DAT  (SDA(IN) & SDA_PIN)

#endif /* _DS3231_ARCH_H */
