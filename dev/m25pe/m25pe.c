#include "m25pe.h"
#include "c2195.h"
#include "Serialize.h"
#include "watchdog.h"
#include "spi.h"
/*-----------------------------------------------------------*/
static ST_uint8 buf[3];
static ST_uint16 id;

/*-----------------------------------------------------------*/
void
m25pe_init()
{
  spi_init();
  if (FlashReadManufacturerIdentification(buf) != Flash_Success)
    while (1);
  if (FlashReadDeviceIdentification(&id) != Flash_Success)
    while (1);
}

int
m25pe_write(uint32_t addr, uint8_t *data, uint16_t len)
{
  return FlashWrite(addr, data, len);
}

uint16_t
m25pe_read(uint32_t addr, uint8_t *buf, uint16_t len)
{
  return FlashRead(addr, buf, len);
}

int
m25pe_erase(uint32_t addr, uint16_t len)
{
  uint16_t page;
  uint16_t count = 0;
  ReturnType r;

  if (((addr & 0xff) != 0) || ((len & 0xff) != 0))
    return 0;

  page = (addr >> 8);
  while (len > 0) {
    watchdog_periodic();
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
