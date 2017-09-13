#include "contiki.h"
#include "m25pe.h"
#include "m25pe-arch.h"
#include "c2195.h"
#include "Serialize.h"
#include "dev/watchdog.h"
#include <string.h>

#ifndef M25PE_LPM
#define M25PE_LPM  0
#endif

#ifndef M25PE_WRITE_CHECK
#define M25PE_WRITE_CHECK  1
#endif

#ifndef M25PE_WRITE_TIMES
#define M25PE_WRITE_TIMES  4
#endif
/*-----------------------------------------------------------*/
#if 0
static ST_uint8 m25pe_manu[3];
static ST_uint16 m25pe_id;
#endif
#if M25PE_WRITE_CHECK
static uint8_t m25pe_buf[FLASH_WRITE_BUFFER_SIZE];
#endif

int m25pe_dev = -1;
static uint8_t m25pe_state[2];
/*-----------------------------------------------------------*/
void
m25pe_init(int devid)
{
  m25pe_dev = devid;
  m25pe_arch_init();
#if 0
  if (FlashReadManufacturerIdentification(m25pe_manu) != Flash_Success) {
    dint();
    LPM4;
  }
  if (FlashReadDeviceIdentification(&m25pe_id) != Flash_Success) {
    dint();
    LPM4;
  }
#endif
#if M25PE_LPM
  m25pe_powerdown(devid);
#else
  m25pe_state[devid] = M25PE_STANDBY;
#endif
}

int
m25pe_write(int devid, uint32_t addr, uint8_t *data, uint16_t len)
{
  int r = Flash_Success;
#if M25PE_WRITE_CHECK
  uint16_t idx, left, cnt;
#endif
  uint8_t num = M25PE_WRITE_TIMES;

  m25pe_dev = devid;
#if M25PE_LPM
  if (m25pe_state[devid] == M25PE_PWRDOWN) {
    m25pe_standby(devid);
  }
#endif

  while (1) {
write_again:
    if ((num--) == 0)
      break;
#if WITH_WDT
    watchdog_periodic();
#endif
    r = FlashWrite(addr, data, len);
    if (r != Flash_Success) {
      r = -1; goto write_again;
    } else {
      r = 0;
    }
#if M25PE_WRITE_CHECK
    for (idx = 0, left = len; left > 0; ) {
      cnt = ((len < sizeof(m25pe_buf)) ? len : sizeof(m25pe_buf));
      r = FlashRead(addr + idx, m25pe_buf, cnt);
      if (r != Flash_Success) {
        r = -2; goto write_again;
      } else {
        r = 0;
      }
      if (!((cnt > 0) && (memcmp(data + idx, m25pe_buf, cnt) == 0))) {
        r = -3; goto write_again;
      }
      idx += cnt;
      left -= cnt;
    }
#endif
    if (r == 0)
      break;
  }

#if M25PE_LPM
  m25pe_powerdown(devid);
#endif
  return r;
}

uint16_t
m25pe_read(int devid, uint32_t addr, uint8_t *buf, uint16_t len)
{
  ReturnType r = Flash_Success;

  m25pe_dev = devid;
#if M25PE_LPM
  if (m25pe_state[devid] == M25PE_PWRDOWN) {
    m25pe_standby(devid);
  }
#endif
  r = FlashRead(addr, buf, len);
#if M25PE_LPM
  m25pe_powerdown(devid);
#endif

  return ((r == Flash_Success) ? 0 : -1);
}

int
m25pe_erase(int devid, uint32_t addr, uint32_t len)
{
  uint16_t page;
  uint16_t count = 0;
  ReturnType r;

  if (((addr & 0xff) != 0) || ((len & 0xff) != 0))
    return 0;

  m25pe_dev = devid;
#if M25PE_LPM
  if (m25pe_state[devid] == M25PE_PWRDOWN) {
    m25pe_standby(devid);
  }
#endif

  page = (addr >> 8);
  while (len > 0) {
#if WITH_WDT
    watchdog_periodic();
#endif
    r = FlashPageErase(page);
    if (r != Flash_Success) {
      count = 0;
      goto erase_done;
    }
    count += 1;
    page += 1;
    len -= M25PE_PAGE_SIZE;
  }

erase_done:
#if M25PE_LPM
  m25pe_powerdown(devid);
#endif
  return count;
}

int
m25pe_erase_sector(int devid, uint16_t sector)
{
  uint8_t buf[M25PE_PAGE_SIZE] = {0};
  uint32_t addr;
  uint32_t left;
  ReturnType r;

  m25pe_dev = devid;
#if M25PE_LPM
  if (m25pe_state[devid] == M25PE_PWRDOWN) {
    m25pe_standby(devid);
  }
#endif
  r = FlashSectorErase(sector);
  if (r == Flash_Success) {
    addr = M25PE_SECTOR_SIZE * sector;
    left = M25PE_SECTOR_SIZE;
    while (left > 0) {
      r = FlashPageWrite(addr, buf, M25PE_PAGE_SIZE);
      if (r != Flash_Success) {
        break;
      } else {
        addr += M25PE_PAGE_SIZE;
        left -= M25PE_PAGE_SIZE;
      }
    }
  }
#if M25PE_LPM
  m25pe_powerdown(devid);
#endif

  return ((r == Flash_Success) ? 0 : -1);
}

int
m25pe_powerdown(int devid)
{
  m25pe_dev = devid;
  FlashDeepPowerDown();
  m25pe_arch_off();
  m25pe_state[devid] = M25PE_PWRDOWN;
  return 0;
}

int
m25pe_standby(int devid)
{
  m25pe_dev = devid;
  m25pe_arch_on();
  FlashReleaseFromDeepPowerDown();
  m25pe_state[devid] = M25PE_STANDBY;
  return 0;
}
/*-----------------------------------------------------------*/
