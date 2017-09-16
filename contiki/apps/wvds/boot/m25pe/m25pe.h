#ifndef _M25PE20_H
#define _M25PE20_H

#include <stdint.h>

void m25pe_init(int devid);
int m25pe_write(int devid, uint32_t addr, uint8_t *data, uint16_t len);
uint16_t m25pe_read(int devid, uint32_t addr, uint8_t *buf, uint16_t len);
int m25pe_erase(int devid, uint32_t addr, uint16_t len);

#endif /* _M25PE20_H */
