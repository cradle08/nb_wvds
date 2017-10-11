#ifndef __RTC_H__
#define __RTC_H__

void rtc_arch_init(uint8_t* tstamp);
void rtc_arch_set(uint8_t* tstamp);
void rtc_arch_read(uint8_t* tstamp);

#endif