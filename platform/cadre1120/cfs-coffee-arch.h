#ifndef CFS_COFFEE_ARCH_H
#define CFS_COFFEE_ARCH_H

#include "contiki-conf.h"
#include "m25pe.h"

#define COFFEE_SECTOR_SIZE      65536UL
#define COFFEE_PAGE_SIZE        256UL
#define COFFEE_SIZE             (1024UL * 256UL - COFFEE_START)

#define COFFEE_NAME_LENGTH      16
#define COFFEE_MAX_OPEN_FILES   6
#define COFFEE_FD_SET_SIZE      8

#define COFFEE_LOG_TABLE_LIMIT  256
#define COFFEE_LOG_SIZE         1024
#define COFFEE_DYN_SIZE         256

typedef int16_t coffee_page_t;

#if BOARD_CADRE1120_VD
#define COFFEE_START            COFFEE_SECTOR_SIZE // must be COFFEE_SECTOR_SIZE*n
#define COFFEE_FLASH_ID         FLASH_A
#elif BOARD_CADRE1120_AP
#define COFFEE_START            0
#define COFFEE_FLASH_ID         FLASH_B
#else
#error "no support"
#endif

#define COFFEE_WRITE(buf, size, offset) \
  m25pe_write(COFFEE_FLASH_ID, COFFEE_START + (offset), (uint8_t*)buf, size)

#define COFFEE_READ(buf, size, offset) \
  m25pe_read(COFFEE_FLASH_ID, COFFEE_START + (offset), (uint8_t*)buf, size)

#define COFFEE_ERASE(sector) \
  m25pe_erase_sector(COFFEE_FLASH_ID, ((COFFEE_START/COFFEE_SECTOR_SIZE) + sector))

#endif /* !COFFEE_ARCH_H */
