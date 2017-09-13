#include "unixtime.h"
#include <time.h>
#include <string.h>
#include <stdio.h>

#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#if CONTIKI_TARGET_COOJA
#undef UNIXTIME_ARCH
#define UNIXTIME_ARCH 0
#endif

//#if UNIXTIME_ARCH
//#warning "using hardware unixtime"
//#else
//#warning "using software unixtime"
//#endif

#define BCD(x) ((((x)/10)<<4) + ((x)%10))
#define DeBCD(x) (((x)>>4)*10 + ((x)&0x0f))

#if UNIXTIME_ARCH
extern int unixtime_arch_get(uint8_t *ts);
extern int unixtime_arch_set(uint8_t *ts);
extern int unixtime_arch_eq(uint8_t *ts);
#endif

#if !UNIXTIME_ARCH
static time_t utime = 1451606400L; // 2016-01-01 00:00:00 UTC
#endif
static struct tm utime_tm;
static uint8_t utime_ts[6];
/*---------------------------------------------------------------------------*/
#if UNIXTIME_PROCESS
PROCESS(unixtime_process, "Unixtime");
#endif
/*---------------------------------------------------------------------------*/
uint32_t
unixtime_now(void)
{
#if UNIXTIME_ARCH
  uint8_t *ts = utime_ts;
  unixtime_arch_get(ts);
  return unixtime_sec(ts);
#elif UNIXTIME_PROCESS
  return (uint32_t)utime;
#else
  return ((uint32_t)utime + clock_seconds());
#endif
}

void
unixtime_inc(void)
{
#if !UNIXTIME_ARCH
#if UNIXTIME_PROCESS
  ++utime;
#endif
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
  unixtime_arch_get(ts);
#elif UNIXTIME_PROCESS
  unixtime_ts(ts, utime);
  PRINTF("unixtime_get %lu -> %02X%02X%02X%02X%02X%02X\n", utime, ts[0],ts[1],ts[2],ts[3],ts[4],ts[5]);
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
  unixtime_arch_set(ts);
#elif UNIXTIME_PROCESS
  dint();
  utime = unixtime_sec(ts);
  eint();
  PRINTF("unixtime_set %02X%02X%02X%02X%02X%02X -> %lu\n", ts[0],ts[1],ts[2],ts[3],ts[4],ts[5], utime);
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
/*---------------------------------------------------------------------------*/
#if UNIXTIME_PROCESS
PROCESS_THREAD(unixtime_process, ev, data)
{
#if !UNIXTIME_ARCH
  static struct etimer et;
#endif

  PROCESS_BEGIN();

#if 0
  unixtime_ts(utime_ts, utime);
  utime = unixtime_sec(utime_ts);
#endif

#if !UNIXTIME_ARCH
  etimer_set(&et, CLOCK_SECOND);
#endif

  while (1) {
    PROCESS_WAIT_EVENT();

#if !UNIXTIME_ARCH
    if (etimer_expired(&et)) {
      etimer_reset(&et);
      unixtime_inc();
    }
#endif
  }

  PROCESS_END();
}
#endif
