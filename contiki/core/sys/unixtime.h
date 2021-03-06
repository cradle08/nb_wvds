#ifndef _UNIXTIME_H
#define _UNIXTIME_H

#include "contiki.h"

// 定义为1时使用硬件RTC芯片或MCU内部RTC，定义为0时使用软件实现
#ifndef UNIXTIME_ARCH
#define UNIXTIME_ARCH 1
#endif


struct unixtime {
  uint8_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
};

uint32_t unixtime_now(void);
uint32_t unixtime_sec(uint8_t *ts);
void unixtime_init();
void unixtime_ts(uint8_t *ts, uint32_t sec);
void unixtime_inc(void);
void unixtime_get(uint8_t *ts);
void unixtime_set(uint8_t *ts);
int unixtime_eq(uint8_t *ts);

#endif /* _UNIXTIME_H */
