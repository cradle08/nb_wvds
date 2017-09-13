#ifndef _M25PE20_H
#define _M25PE20_H

#define M25PE_PAGE_SIZE 256UL
#define M25PE_SECTOR_SIZE 65536UL

enum {
  M25PE_ACTIVE  = 1,
  M25PE_STANDBY = 2,
  M25PE_PWRDOWN = 3
};

void m25pe_init(int devid);
int m25pe_write(int devid, uint32_t addr, uint8_t *data, uint16_t len);
uint16_t m25pe_read(int devid, uint32_t addr, uint8_t *buf, uint16_t len);
int m25pe_erase(int devid, uint32_t addr, uint32_t len);
int m25pe_erase_sector(int devid, uint16_t sector);
int m25pe_powerdown(int devid);
int m25pe_standby(int devid);

#endif /* _M25PE20_H */
