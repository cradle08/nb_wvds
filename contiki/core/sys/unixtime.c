#include "unixtime.h"
#include <time.h>
#include <string.h>
#include <stdio.h>
#include "rtc.h"

#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


//#if UNIXTIME_ARCH
//#warning "using hardware unixtime"
//#else
//#warning "using software unixtime"
//#endif

#define BCD(x) ((((x)/10)<<4) + ((x)%10))
#define DeBCD(x) (((x)>>4)*10 + ((x)&0x0f))


#if !UNIXTIME_ARCH
static time_t utime
#endif
static struct tm utime_tm;
static uint8_t utime_ts[6];


void unixtime_init(uint8_t* ts)
{
#if UNIXTIME_ARCH
  rtc_arch_init(ts);
#else
  static time_t utime = 1451606400L; // 2016-01-01 00:00:00 UTC
#endif
  
}

uint32_t
unixtime_now(void)
{
#if UNIXTIME_ARCH
  uint8_t *ts = utime_ts;
  rtc_arch_get(ts);
  return unixtime_sec(ts);
#else
  return ((uint32_t)utime + clock_seconds());
#endif
}

uint32_t
unixtime_sec(uint8_t *ts)
{
  struct tm *t = &utime_tm;;
  time_t v;

  t->tm_year = DeBCD(ts[0]) + 100;
  t->tm_mon  = DeBCD(ts[1]) - 1;
  t->tm_mday = DeBCD(ts[2]);
  t->tm_hour = DeBCD(ts[3]);
  t->tm_min  = DeBCD(ts[4]);
  t->tm_sec  = DeBCD(ts[5]);
  v = mktime(t);
  PRINTF("unixtime_sec %02X%02X%02X%02X%02X%02X -> %lu\n", ts[0],ts[1],ts[2],ts[3],ts[4],ts[5], v);

  return (uint32_t)v;
}

void
unixtime_ts(uint8_t *ts, uint32_t sec)
{
  struct tm *t = NULL;
  uint8_t i = 0;

  if (ts == NULL) {
    PRINTF("unixtime_ts null ts\n");
    return;
  }

  t = gmtime((const time_t *)&sec);
  if (t == NULL) {
    PRINTF("unixtime_ts null t for %lu/%lu\n", sec, sec);
    return;
  }

  ts[i++] = BCD(t->tm_year - 100);
  ts[i++] = BCD(t->tm_mon + 1);
  ts[i++] = BCD(t->tm_mday);
  ts[i++] = BCD(t->tm_hour);
  ts[i++] = BCD(t->tm_min);
  ts[i++] = BCD(t->tm_sec);
  PRINTF("unixtime_ts %lu -> %02X%02X%02X%02X%02X%02X\n", sec, ts[0],ts[1],ts[2],ts[3],ts[4],ts[5]);
}

void
unixtime_get(uint8_t *ts)
{
#if UNIXTIME_ARCH
  rtc_arch_get(ts);
#else
  uint32_t now = utime;
  now += clock_seconds();
  unixtime_ts(ts, now);
#endif
}

void
unixtime_set(uint8_t *ts)
{
#if UNIXTIME_ARCH
  rtc_arch_set(ts);
#else
  uint32_t now = 0;
  dint();
  now = unixtime_sec(ts);
  utime = now - clock_seconds();
  eint();
#endif
}


int
unixtime_eq(uint8_t *ts)
{
#if UNIXTIME_ARCH
  return unixtime_arch_eq(ts);
#else
  uint8_t *buf = utime_ts;
  unixtime_get(buf);
  return (memcmp(buf, ts, sizeof(struct unixtime)) == 0);
#endif
}
