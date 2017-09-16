//#include "contiki.h"
#include "m25pe.h"
#include "m25pe-arch.h"
#include "c2195.h"
#include "Serialize.h"
//#include "dev/watchdog.h"

/*-----------------------------------------------------------*/
#if 0
static ST_uint8 buf[3];
static ST_uint16 id;
#endif
int m25pe_dev = -1;
/*-----------------------------------------------------------*/
void
m25pe_init(int devid)
{
  m25pe_dev = devid;
  m25pe_arch_init();
#if 0
  if (FlashReadManufacturerIdentification(buf) != Flash_Success)
    while (1);
  if (FlashReadDeviceIdentification(&id) != Flash_Success)
    while (1);
#endif
}

int
m25pe_write(int devid, uint32_t addr, uint8_t *data, uint16_t len)
{
  m25pe_dev = devid;
  return FlashWrite(addr, data, len);
}

uint16_t
m25pe_read(int devid, uint32_t addr, uint8_t *buf, uint16_t len)
{
  m25pe_dev = devid;
  return FlashRead(addr, buf, len);
}

int
m25pe_erase(int devid, uint32_t addr, uint16_t len)
{
  uint16_t page;
  uint16_t count = 0;
  ReturnType r;

  if (((addr & 0xff) != 0) || ((len & 0xff) != 0))
    return 0;

  m25pe_dev = devid;
  page = (addr >> 8);
  while (len > 0) {
    //watchdog_periodic();
    r = FlashPageErase(page);
    if (r != Flash_Success)
      return 0;
    count += 1;
    page += 1;
    len -= 256;
  }

  return count;
}
/*-----------------------------------------------------------*/
