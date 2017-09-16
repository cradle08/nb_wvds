#include "contiki.h"
#include "net/netstack.h"
#include "sys/taskmon.h"
#include "dev/leds.h"
#include "dev/watchdog.h"
#include "m25pe.h"
#include "app.h"

#if CONTIKI_TARGET_COOJA
#include "cfs/cfs.h"
#elif BOARD_CADRE1120_AP || BOARD_CADRE1120_VD
#include "m25pe.h"
#else
#warning "no support"
#endif

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...)  printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

const char* OTA_FILE[5] = {"NA","VD","RP","NA","AP"};
const int  OTA_IMGID[5] = {  -1,   2,   1,  -1,   0};
const uint8_t OTA_ROLE[3] = {NODE_AP,NODE_RP,NODE_VD};
#if BOARD_CADRE1120_AP
const uint32_t OTA_ADDR[3] = {0x00000ul,0x18000ul,0x28000ul}; // AP,RP,VD
#elif BOARD_CADRE1120_VD
const uint32_t OTA_ADDR[3] = {0x40000ul,0x40000ul,0x20000ul}; // AP,RP,VD
#else
#error "no support"
#endif

/*------------------------------------------------------------------*/
int
nv_init(void)
{
#if CONTIKI_TARGET_COOJA
  int fd;
  fd = cfs_open(NV_FILE, CFS_WRITE);
  cfs_reserv(fd, 5120);
  cfs_close(fd);
  return 0;

#elif BOARD_CADRE1120_AP
  m25pe_init(FLASH_A);
  m25pe_init(FLASH_B);
  return 0;

#elif BOARD_CADRE1120_VD
  m25pe_init(FLASH_A);
  return 0;

#else
#warning "no support"
  return 0;
#endif
}

int
nv_read(uint32_t addr, uint8_t *buf, uint8_t len)
{
#if CONTIKI_TARGET_COOJA
  int fd;
  int ret;

  fd = cfs_open(NV_FILE, CFS_READ);
  if (fd == -1) {
    PRINTF("nv_read fail open\n");
  } else {
    ret = cfs_seek(fd, addr, CFS_SEEK_SET);
    if (ret == -1) {
      PRINTF("nv_read fail seek\n");
    } else {
      ret = cfs_read(fd, buf, len);
      PRINTF("nv_read 0x%04X,%d,%d\n", addr, len, ret);
      cfs_close(fd);
      return ret;
    }
    cfs_close(fd);
  }
  return -1;

#elif BOARD_CADRE1120_AP || BOARD_CADRE1120_VD
  int ret = -1;
  ret = m25pe_read(FLASH_A, addr, buf, len);
  return ret;

#else
#warning "no support"
#endif
}

int
nv_write(uint32_t addr, uint8_t *buf, uint8_t len)
{
#if CONTIKI_TARGET_COOJA
  int fd;
  int ret;

  fd = cfs_open(NV_FILE, CFS_WRITE);
  if (fd == -1) {
    PRINTF("nv_write fail open\n");
  } else {
    ret = cfs_seek(fd, addr, CFS_SEEK_SET);
    if (ret == -1) {
      PRINTF("nv_write fail seek\n");
    } else {
      ret = cfs_write(fd, buf, len);
      PRINTF("nv_write 0x%04X,%d,%d\n", addr, len, ret);
      cfs_close(fd);
      return ret;
    }
    cfs_close(fd);
  }
  return -1;

#elif BOARD_CADRE1120_AP || BOARD_CADRE1120_VD
  int ret = -1;
  ret = m25pe_write(FLASH_A, addr, buf, len);
  return ret;

#else
#warning "no support"
#endif
}

int
nv_erase(uint32_t addr, uint32_t len)
{
#if CONTIKI_TARGET_COOJA
  int fd;
  int ret;
  uint8_t buf[128] = {0};
  uint8_t wlen;
  uint8_t i;

  for (i=0; i<sizeof(buf); i++)
    buf[i] = 0xff;

  fd = cfs_open(NV_FILE, CFS_WRITE);
  if (fd == -1) {
    PRINTF("nv_erase fail open\n");
  } else {
    ret = cfs_seek(fd, addr, CFS_SEEK_SET);
    if (ret == -1) {
      PRINTF("nv_erase fail seek\n");
    } else {
      while (len > 0) {
        wlen = (len<128?len:128);
        ret = cfs_write(fd, buf, wlen);
        len -= wlen;
      }
      PRINTF("nv_erase 0x%04X,%d,%d\n", addr, len, ret);
      cfs_close(fd);
      return ret;
    }
    cfs_close(fd);
  }
  return -1;

#elif BOARD_CADRE1120_AP || BOARD_CADRE1120_VD
  uint16_t n;
  n = m25pe_erase(FLASH_A, addr, len);
  return (n > 0 ? 0 : -1);

#else
#warning "no support"
#endif
}
/*---------------------------------------------------------------------------*/
void EraseFlash(unsigned long waddr)
{
  dint();//_BIC_SR(GIE);                         // 关闭总中断

  //FCTL2 = FWKEY + FSSEL0+FN1;           // 选择DC0作为LFASH操作时钟源,MCLK/2
  FCTL3 = FWKEY;
  FCTL1 = FWKEY + ERASE;                // 擦除操作
  *(unsigned char*)waddr=0;             // 虚拟的擦除段操作
  while(FCTL3 & BUSY);
  FCTL3=FWKEY+LOCK;

  eint();//_BIS_SR(GIE);                         //  再次开总中断使能
}

unsigned char WriteFlash(unsigned long addr,unsigned char *pdata,
    unsigned int length)
{
  unsigned char ErrorFlag = 0;
  unsigned int i;

  //while(FCTL3 & BUSY);

  dint();//_BIC_SR(GIE);

  //FCTL2 = FWKEY + FSSEL0+FN1;
  FCTL3 = FWKEY;                            // 清除锁
  while(FCTL3 & BUSY);
  FCTL1 = FWKEY + WRT;                      // 设置WRT位为写操作
  for(i=0;i<length;i++)
  {
    *(unsigned char*)addr=*pdata;          // 写一个字节
    while(FCTL3 & BUSY);
    if(ReadFlashByte(addr)!=*pdata)            // 验证,写比较.正确或错误
    {
      ErrorFlag = 1;                       // 设置错误标志
      break;
    }
    addr++;pdata++;
  }
  FCTL1=FWKEY;
  FCTL3=FWKEY+LOCK;

  eint();//_BIS_SR(GIE);

  return ErrorFlag;
}

unsigned char ReadFlashByte(unsigned long waddr)
{
  unsigned char value;
  //while(FCTL3 & BUSY);
  value = *(unsigned char*)waddr;
  return value;
}

int ReadFlash(unsigned long addr, unsigned char *buf, unsigned int len)
{
  unsigned int i;
  for (i = 0; i < len; i++)
    buf[i] = *(unsigned char*)(addr + i);
  return 0;
}

/*---------------------------------------------------------------------------*/
void
app_show_reboot(void)
{
  uint8_t led = 0;
  uint8_t i;

#if BOARD_CADRE1120_AP
  led = LEDS_RED;
#elif BOARD_CADRE1120_VD
  led = LEDS_GREEN;
#else
#warning "no support"
  return;
#endif

  leds_off(led);
  for (i = 0; i < 5; i++) {
    leds_on(led);
    __delay_cycles((long)F_CPU >> 4);
    leds_off(led);
    __delay_cycles((long)F_CPU >> 4);
  }
}

void
app_show_inact(void)
{
  uint8_t i;

  leds_off(LEDS_ALL);
  for (i = 0; i < 3; i++) {
    leds_on(LEDS_ALL);
    __delay_cycles((long)F_CPU >> 1);
    watchdog_periodic();
    leds_off(LEDS_ALL);
    __delay_cycles((long)F_CPU >> 1);
    watchdog_periodic();
  }
}

void
app_show_chan(uint8_t chan)
{
  leds_off(LEDS_ALL);
  if (chan & 0x08) leds_on(LEDS_ORANGE);
  if (chan & 0x04) leds_on(LEDS_GREEN);
  if (chan & 0x02) leds_on(LEDS_BLUE);
  if (chan & 0x01) leds_on(LEDS_RED);
}

void
app_fatal(void)
{
    uint8_t led = 0;
  uint8_t i;

#if BOARD_CADRE1120_AP
  led = LEDS_RED;
#elif BOARD_CADRE1120_VD
  led = LEDS_GREEN;
#else
#warning "no support"
  return;
#endif

  leds_off(led);
  for (i = 0; i < 4; i++) {
    leds_on(led);
    __delay_cycles((long)F_CPU >> 2);
    leds_off(led);
    __delay_cycles((long)F_CPU >> 2);
  }

  dint();
  LPM4;
}
