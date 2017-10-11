#ifndef __PLATFORM_CONF_H__
#define __PLATFORM_CONF_H__

#include <msp430F5310.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <in430.h>
#include <msp430.h>

typedef unsigned long   clock_time_t;
typedef unsigned short  rtimer_clock_t;


#define OS_DEBUG                0
#define TSTAMP_LEN              7
#define AXIS_NUM                3
#define UPLOAD_MAGDATA_NUM      10
#define RECV_PLAYDATA_LEN       40
#define SEND_PLAYDATA_LEN       90
#define RECV_MSGMEM_NUM         3
#define SEND_MSGMEM_NUM         5
#define RECV_MSG_BUF_LEN        512
#define SEND_MSG_BUF_LEN        165
#define DEVICE_NO_LEN           6
#define ACLK                    32768ul
#define INTERVAL                ACLK/CLOCK_CONF_SECOND
#define F_CPU                   8000000uL   // CPU target speed in Hz; works fine at 8, 16, 18 MHz but not higher. 
#define CLOCK_CONF_SECOND       128UL  // Our clock resolution, this is the same as Unix HZ.
#define MS2JIFFIES(ms)         ((clock_time_t)CLOCK_SECOND * (ms)) / 1000  // ms to jiffies
#define BAUD2UBR(baud)         (F_CPU/(baud))  //#define BAUD2UBR(baud) ((F_CPU/baud))
#define JIFFIES_NUM(x)         (clock_time_t)(x)*CLOCK_CONF_SECOND/1000
#define RTIMER_CLOCK_LT(a,b)   ((signed short)((a)-(b)) < 0)
#define RTIMER_CLOCK_LT(a,b)   ((signed short)((a)-(b)) < 0)




#define __PRAGMA__(x) _Pragma(#x)
#define ISR(a,b) \
__PRAGMA__(vector=a ##_VECTOR) \
__interrupt void b(void)

#define CCIF
#define CLIF
#define dint()       __disable_interrupt()
#define eint()       __enable_interrupt()
#define delay_us(x)  __delay_cycles((long)(F_CPU*(double)x/1000000.0))
#define delay_ms(x)  __delay_cycles((long)(F_CPU*(double)x/1000.0))
#define BCD(x)       ((((x)/10)<<4) + ((x)%10))
#define DeBCD(x)     (((x)>>4)*10 + ((x)&0x0f))

//typedef unsigned long   clock_time_t;
//typedef unsigned short  rtimer_clock_t;





#endif /* __PLATFORM_CONF_H__ */



