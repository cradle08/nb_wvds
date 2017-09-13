#include "contiki.h"
#include "ds3231.h"
#include <string.h>

#define MAX_FAIL 4

static ds3231_time_t last = {0};
static uint8_t last_cnt = 0;

int
unixtime_arch_get(uint8_t *ts)
{
  ds3231_time_t now;
  uint8_t nil[sizeof(ds3231_time_t)] = {0};
  uint8_t i = 0;
  int ok = 0, r = 0;

  do {
    ds3231_get_time(&now);
    ok = (memcmp(&now, nil, sizeof(ds3231_time_t)) != 0);
  } while(!ok && (++i < MAX_FAIL));

  if (!ok) {
    r = 3;
#if NODEALARM
    nodealarm.rtc = 3;
#endif
  }

  if (memcmp(&now, &last, sizeof(ds3231_time_t)) != 0) {
    memcpy(&last, &now, sizeof(ds3231_time_t));
    last_cnt = 0;
  } else {
    if (last_cnt < 255)
      ++last_cnt;
    if (last_cnt >= 3) {
      r = 1;
#if NODEALARM
      nodealarm.rtc = 1;
#endif
    }
  }

  i = 0;
  ts[i++] = now.year;
  ts[i++] = now.month;
  ts[i++] = now.day;
  ts[i++] = now.hour;
  ts[i++] = now.minute;
  ts[i++] = now.second;

  return r;
}

int
unixtime_arch_set(uint8_t *ts)
{
  ds3231_time_t t, c;
  uint8_t i = 0;
  int ok = 0, r = 0;

  i = 0;
  t.year   = ts[i++];
  t.month  = ts[i++];
  t.day    = ts[i++];
  t.hour   = ts[i++];
  t.minute = ts[i++];
  t.second = ts[i++];
  t.reserve = 0;

  i = 0;
  do {
    ds3231_set_time(&t);
    ds3231_get_time(&c);
    ok = (memcmp(&t, &c, sizeof(ds3231_time_t)) == 0);
  } while (!ok && (++i < MAX_FAIL));

  if (!ok) {
    r = 4;
#if NODEALARM
    nodealarm.rtc = 4;
#endif
  }

  return r;
}

int
unixtime_arch_eq(uint8_t *ts)
{
  ds3231_time_t now;

  ds3231_get_time(&now);

  return ((now.second == ts[5]) &&
      (now.minute == ts[4]) &&
      (now.hour   == ts[3]) &&
      (now.day    == ts[2]) &&
      (now.month  == ts[1]) &&
      (now.year   == ts[0]));
}
