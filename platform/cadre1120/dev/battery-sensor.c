#include "dev/battery-sensor.h"

#define delay_us(x) __delay_cycles((long)(F_CPU*(double)x/1000000.0))
#define delay_ms(x) __delay_cycles((long)(F_CPU*(double)x/1000.0))

const struct sensors_sensor battery_sensor;
/*---------------------------------------------------------------------------*/
static int
value(int type)
{
#define NUM 6
  uint8_t i;
  uint16_t ADC[NUM];
  uint16_t MAX;
  uint16_t MIN;
  uint32_t ch;

  for (i=0;i<NUM;i++)
  {
    ADC12CTL0|=ADC12SC;
    while(!(ADC12IFG & ADC12IFG0));
    ADC[i]=ADC12MEM0;
  }
  ADC12CTL0&=~ADC12SC;

  ch=ADC[0];
  MAX=ADC[0];
  MIN=ADC[0];
  for(i=1;i<NUM;i++)
  {
    if(MAX<ADC[i])
    {
      MAX=ADC[i];
    }
    if(MIN>ADC[i])
    {
      MIN=ADC[i];
    }
    ch+=ADC[i];
  }

  dint();
  ch-=(MAX+MIN);
  ch/=(NUM-2);
  ch*=100; // 2.5*4*10
  ch*=2; // input voltage divided by 5
  ch/=16384; // 2^12*4
  eint();

  return (int)ch;
}

static int
configure(int type, int value)
{
#if BOARD_CADRE1120_AP
  P6SEL|=BIT5;
  P6DIR&=~BIT5;
#elif BOARD_CADRE1120_VD
  P6SEL|=BIT4;
  P6DIR&=~BIT4;
#endif

  ADC12CTL0&=~ADC12ENC;
  REFCTL0 &= ~REFMSTR;
  ADC12CTL0=ADC12SHT0_10+ADC12REF2_5V_L+ADC12REFON_L+ADC12ON_L;
  ADC12CTL1=ADC12SHP;
#if BOARD_CADRE1120_AP
  ADC12MCTL0=ADC12SREF_1+ADC12INCH_5;
#elif BOARD_CADRE1120_VD
  ADC12MCTL0=ADC12SREF_1+ADC12INCH_4;
#endif
  ADC12CTL0|=ADC12ENC;

  return 0;
}

static int
status(int type)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(battery_sensor, BATTERY_SENSOR,
               value, configure, status);
