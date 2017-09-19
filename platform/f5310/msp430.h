#ifndef __MSP430_H__
#define __MSP430_H__

static void port_init(void);
static void port_mapping(void);
void msp430_dco_init(uint32_t sped);
void msp430_cpu_init(uint32_t sped);

#endif 


