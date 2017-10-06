#ifndef system_H
#define system_H

#include <msp430F5310.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#define HW_VER  1 

#define F_CPU	1000000uL

#define SystemLED_OFF	P1OUT |=BIT2;
#define SystemLED_ON	P1OUT &= ~BIT2;
#define SystemLED_FLA P1OUT ^=BIT2;

#define WDT_OPEN_1S  WDTCTL = WDT_ARST_1000                 // (WDTPW + WDTCNTCL + WDTSSEL)
#define WDT_CLOSE    WDTCTL = WDTPW + WDTHOLD               //  关看门狗

#define delay_us(x) __delay_cycles((long)(F_CPU*(double)x/1000000.0))
#define delay_ms(x) __delay_cycles((long)(F_CPU*(double)x/1000.0))

#ifndef TRUE
#define TRUE    (0)
#endif
#ifndef FALSE
#define FALSE   (0XFF)
#endif

typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;

/***********************************************************/
void msp430_cpu_init(void);	//系统初始化函数
void osc_init(void);  //时钟初始化函数
void ADC_DMA_init(void); //端口初始化函数
u16 get_bat_valu(void);//电池电压转换
/***********************************************************/



#endif
