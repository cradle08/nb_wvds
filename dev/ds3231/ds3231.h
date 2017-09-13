#ifndef _DS3231_H
#define _DS3231_H

#define DS3231_TIME_STRLEN  20

#define DS3231_ADDR_WRITE   0xd0
#define DS3231_ADDR_READ    0xd1

typedef struct ds3231_time {
  uint8_t second;
  uint8_t minute;
  uint8_t hour;
  uint8_t reserve;
  uint8_t day;
  uint8_t month;
  uint8_t year;
} ds3231_time_t;

void ds3231_init(void);
void ds3231_get_time(ds3231_time_t *time);
void ds3231_set_time(ds3231_time_t *time);
void ds3231_str_time(char *buf);

#endif /* _DS3231_H */
