#include "platform-conf.h"
#include "rtc.h"

void rtc_arch_init(uint8_t* tstamp)
{
  RTCCTL1 = RTCBCD|RTCHOLD;                      
  rtc_arch_set(tstamp);
  RTCCTL1 &= ~(RTCHOLD);
}

void rtc_arch_set(uint8_t* tstamp)
{
  RTCYEAR = 0x2000|tstamp[0];     // 如果高位大于0x20，会有异常现象
  RTCMON = tstamp[1]&0x1F;
  RTCDAY = tstamp[2]&0x3F;
  RTCHOUR = tstamp[3]&0x3F;
  RTCMIN = tstamp[4]&0X7F;
  RTCSEC = tstamp[5]&0X7F;
}

void rtc_arch_read(uint8_t* tstamp)
{
  while(!(RTCCTL1&RTCRDY))
  {
    tstamp[0] =  RTCYEARH; 
    tstamp[1] =  RTCYEARL ; 
    tstamp[2] =  RTCMON ; 
    tstamp[3] =  RTCDAY ; 
    tstamp[4] =  RTCHOUR; 
    tstamp[5] =  RTCMIN ; 
    tstamp[6] =  RTCSEC; 
    break;
  }
}

