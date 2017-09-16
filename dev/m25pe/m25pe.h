#ifndef _M25PE20_H
#define _M25PE20_H

#include <stdint.h>
#include "c2195.h"

void m25pe_init();
int m25pe_write(uint32_t addr, uint8_t *data, uint16_t len);
uint16_t m25pe_read(uint32_t addr, uint8_t *buf, uint16_t len);
int m25pe_erase(uint32_t addr, uint16_t len);

#endif /* _M25PE20_H */
