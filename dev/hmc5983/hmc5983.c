#include <string.h>
#include "hmc5983.h"
#include "dev/leds.h"
#include "sys/clock.h"

// 是否使用硬件I2C接口，定义为1时使用
#define USE_I2C    0
// 是否采用中断处理方式
#define USE_ISR    0

#ifndef F_CPU
#define F_CPU  16000000UL
#warning "using default 16MHz clock"
//#else
//#warning "using custom clock"
#endif

// I2C接口速率(Hz)
#define I2C_RATE 400000UL

#if 0
#define LEDS_OFF(x) leds_off(x)
#define LEDS_ON(x) leds_on(x)
#else
#define LEDS_OFF(x)
#define LEDS_ON(x)
#endif

unsigned char HMC5883L_Buf[6] = { 0 };
unsigned char TEMPERATURE_Buf[2] = { 0 };
//HMC5983_Struct HMC5983;
static hmc5983_callback_t callback;
static unsigned char in_sample = 0;
//static unsigned long sampleT = 0;

#if   BOARD_CADRE1120_VD
// -----------------------
//  HMC5983  MSP430F5418A
// -----------------------
//  SCL      P10.1
//  SDA      P10.2
//  CS       Px.x
//  DRDY     Px.x
// -----------------------
#include <msp430f5438a.h>

#define SCL_PORT(type)    P10##type
#define SCL_PIN           2
#define SDA_PORT(type)    P10##type
#define SDA_PIN           1
#define CS_PORT(type)     P1##type
#define CS_PIN            0
#define DRDY_PORT(type)   P2##type
#define DRDY_PIN          6

#else
#warning "no support"
#endif

#if BOARD_CADRE1120_VD
#define CS_H()     do { } while(0)
#define CS_L()     do { } while(0)
#define SCL_H()    do { SCL_PORT(OUT) |=  BV(SCL_PIN); } while(0)
#define SCL_L()    do { SCL_PORT(OUT) &= ~BV(SCL_PIN); } while(0)
#define SDA_H()    do { SDA_PORT(OUT) |=  BV(SDA_PIN); } while(0)
#define SDA_L()    do { SDA_PORT(OUT) &= ~BV(SDA_PIN); } while(0)
#define SDA()      (SDA_PORT(IN) & BV(SDA_PIN))
#define SDA_IN()   do { SDA_PORT(DIR) &= ~BV(SDA_PIN); } while(0)
#define SDA_OUT()  do { SDA_PORT(DIR) |=  BV(SDA_PIN); } while(0)
#define DRDY()     (1)

#define enableInterrupts()   do {} while(0)
#define disableInterrupts()  do {} while(0)

#else
#define CS_H()     do { CS_PORT(OUT) |=  BV(CS_PIN); } while(0)
#define CS_L()     do { CS_PORT(OUT) &= ~BV(CS_PIN); } while(0)
#define SCL_H()    do { SCL_PORT(OUT) |=  BV(SCL_PIN); } while(0)
#define SCL_L()    do { SCL_PORT(OUT) &= ~BV(SCL_PIN); } while(0)
#define SDA_H()    do { SDA_PORT(OUT) |=  BV(SDA_PIN); } while(0)
#define SDA_L()    do { SDA_PORT(OUT) &= ~BV(SDA_PIN); } while(0)
#define SDA()      (SDA_PORT(IN) & BV(SDA_PIN))
#define SDA_IN()   do { SDA_PORT(DIR) &= ~BV(SDA_PIN); } while(0)
#define SDA_OUT()  do { SDA_PORT(DIR) |=  BV(SDA_PIN); } while(0)
#define DRDY()     (DRDY_PORT(IN) & BV(DRDY_PIN))

#define enableInterrupts()   do {} while(0)
#define disableInterrupts()  do {} while(0)
#endif

/*********************************************************************
 * LOCAL FUNCTIONS
 */
#if !USE_I2C
void Delay(void);
void init_i2c(void);
void i2c_start(void);
void i2c_stop(void);
void i2c_ack(void);
void i2c_noack(void);
void Write_Byte(unsigned char data);
unsigned char Read_Byte(void);
#endif
void Write_IIC(unsigned char addr,unsigned char data);
unsigned char Read_IIC(unsigned char addr);
void Read_HMC5883L_Data(unsigned char *buf, unsigned char *tmp);

/*****************************************
//初始化与AMR传感器的相关部分IO端口
*****************************************/
int
hmc5983_init(void)
{
  uint8_t id[3];
#if USE_I2C
  uint16_t drate;

  P10SEL |= (BIT1 + BIT2);              // 初始化I2C相关IO

  drate = F_CPU / I2C_RATE;
  UCB3CTL1 = UCSWRST;                   // 禁用USCI
  UCB3CTL0 = UCMODE_3 + UCMST + UCSYNC; // 设置为I2C主同步模式
  UCB3CTL1 = UCSSEL__SMCLK + UCSWRST;   // 设置I2C时钟源
  UCB3BR0  = (drate & 0xff);            // 设置I2C速率
  UCB3BR1  = (drate >> 8);
  UCB3I2CSA = (SLA_W >> 1);             // 设置从设备地址
  UCB3CTL1 &= ~UCSWRST;                 // 启用I2C
#else
  SCL_PORT(DIR) |= BV(SCL_PIN); SCL_PORT(OUT) |= BV(SCL_PIN); // SCL
  SDA_PORT(DIR) |= BV(SDA_PIN); SDA_PORT(OUT) |= BV(SDA_PIN); // SDA
  //CS_PORT(DIR) |= BV(CS_PIN); CS_PORT(OUT) |= BV(CS_PIN); // CS
  //DRDY_PORT(DIR) &= ~BV(DRDY_PIN); // DRDY
#endif

  // read and check sensor identification to assure access ok
  id[0] = Read_IIC(REG_ID_A);
  id[1] = Read_IIC(REG_ID_B);
  id[2] = Read_IIC(REG_ID_C);
  if((id[0]!='H') || (id[1]!='4') || (id[2]!='3')) {
    while(1) { };
  }

  //HMC5883L init
  //Write_IIC(REG_CFG_A,0x10);     //number of samples averaged＝4； Data Output Rate 15Hz； Normal measurement configuration，
  //Write_IIC(REG_CFG_B,0xA0);     //Gain Configuration 1090 (default)

  //0:Continuous-Measurement Mode.
  //1:Single-Measurement Mode (Default).
  //2 or 3  Device is placed in idle mode.
  Write_IIC(REG_MODE,0x03);

  return 0;
};

void hmc5983_set_callback(hmc5983_callback_t cback)
{
  callback = cback;
}

void hmc5983_sample_read( uint8_t gain )
{
  disableInterrupts();
  hmc5983_sample(gain);

#if 0
  sampleT = clock_freq() / 50;
  sampleT = (F_CPU/4) / 50;
  while (DRDY() != 0 && --sampleT > 2) { }

  if (sampleT <= 2) {
    enableInterrupts();
    return; // sample timed out
  }
#else
  __delay_cycles((long)(F_CPU>>10)*7); // ~7ms
#endif
  enableInterrupts();

  hmc5983_interrupt();
}

/**
 * According to formula: Temperature = (MSB * 2^8 + LSB) / (2^4 * 8) + 25 in ℃
 * With rounding-off method. See page 21 of HMC5983 datasheet.
 */
int hmc5983_get_temperature(void)
{
  int8_t  tempH = ((TEMPERATURE_Buf[0] & 0x80) ? -128 : 0) + (TEMPERATURE_Buf[0] & 0x7F);
  uint8_t tempL = TEMPERATURE_Buf[1];
  return (tempH << 1) + ((tempL + 64) >> 7) + 25;
}

void
hmc5983_interrupt(void)
{
  if (in_sample) {
    in_sample = 0;

    // read sampled data and put sensor idle
    Read_HMC5883L_Data(HMC5883L_Buf,TEMPERATURE_Buf);
    hmc5983_idle();                      //将HC5883L设置为IDLE MODE
    LEDS_OFF(LEDS_YELLOW);

    // deliver to the callback
    if (callback != NULL)
      (callback)(HMC5883L_Buf, TEMPERATURE_Buf);
  }
#if USE_ISR
  DRDY_PORT(DIR) &= ~BV(DRDY_PIN); // DRDY as input
#endif
}

#if !USE_I2C
/*-----------------------------------------------------------------------------*/
void Delay(void)      //延时5us，I2C的速率设为100kbps
{
  unsigned char i;
  for(i=0;i<8;i++);
}

//当上电和总线空闲时，两根线均为高电平
void init_i2c(void)
{
  SDA_H();
  Delay();
  SCL_H();
  Delay();
}

//i2c 启动信号。SCL线为高电平期间，SDA线由高电平向低电平的变化表示起始信号
void i2c_start(void)
{
  SDA_OUT();
  SDA_H();
  Delay();
  SCL_H();
  Delay();
  SDA_L();
  Delay();
}

//i2c终止信号。SCL线为高电平期间，SDA线由低电平向高电平的变化表示终止信号
void i2c_stop(void)
{
  SDA_OUT();
  SDA_L();
  Delay();
  SCL_H();
  Delay();
  SDA_H();
  Delay();
}

void i2c_ack(void)
{
  unsigned char i=0;
  SDA_IN();
  SCL_H();
  Delay();
  while((SDA()==1)&&(i<250)) i++;    //check ack
  SCL_L();
  Delay();
}

void i2c_noack(void)
{
  SDA_OUT();
  SDA_H();
  Delay();
  SCL_H();
  Delay();
  SCL_L();
  Delay();
}

//I2C总线进行数据传送时，时钟信号为高电平期间，数据线上的数据必须保持稳定，只有在时钟线上的信号为低电平期间，数据线上的高电平或低电平状态才允许变化
void Write_Byte(unsigned char data)
{
  unsigned char i;
  SDA_OUT();
  for(i=0;i<8;i++)
  {
    SCL_L();
    Delay();
    if(data&0x80)
    {
      SDA_H();
    }
    else
    {
      SDA_L();
    }
    data=data<<1;
    SCL_H();
    Delay();
  }
  SCL_L();
  Delay();
  SDA_H();
  Delay();
}

unsigned char Read_Byte ( void )
{
  unsigned char i,data;
  SDA_IN();
  SCL_L();
  Delay();
  SDA_H();
  Delay();
  for(i=0;i<8;i++)
  {
    SCL_H();
    Delay();
    data=data<<1;
    if(SDA())
    {
      data++;
    }
    SCL_L();
    Delay();
  }
  return data;
}
/*-----------------------------------------------------------------------------*/
#endif /* USE_I2C */

void Write_IIC(unsigned char addr,unsigned char data)
{
#if USE_I2C
  while(UCB3CTL1& UCTXSTP);
  UCB3CTL1 |= UCTR;            // 写模式
  UCB3CTL1 |= UCTXSTT;         // 发送启动位

  UCB3TXBUF = addr;            // 发送地址字节
  while(!(UCB3IFG & UCTXIFG))  // 等待UCTXIFG=1与UCTXSTT=0 同时变化等待一个标志位即可
  {
    if(UCB3IFG & UCNACKIFG)    // 若无应答 UCNACKIFG=1
    {
      return; //return 1;
    }
  }

  UCB3TXBUF = data;            // 发送内容字节
  while(!(UCB3IFG & UCTXIFG)); // 等待UCTXIFG=1

  UCB3CTL1 |= UCTXSTP;
  while(UCB3CTL1 & UCTXSTP);   // 等待发送完成
  //return 0;
#else
  CS_L();
  i2c_start();
  Write_Byte(SLA_W);
  i2c_ack();
  Write_Byte(addr);
  i2c_ack();
  Write_Byte(data);
  i2c_ack();
  i2c_stop();
  CS_H();
#endif
}

unsigned char Read_IIC(unsigned char addr)
{
#if USE_I2C
  uint8_t val = 0;

  UCB3CTL1 |= UCTR;            // 写模式
  UCB3CTL1 |= UCTXSTT;         // 发送启动位和写控制字节

  UCB3TXBUF = addr;            // 发送地址字节，必须要先填充TXBUF
  while(!(UCB3IFG & UCTXIFG))  // 等待UCTXIFG=1与UCTXSTT=0 同时变化等待一个标志位即可
  {
    if(UCB3IFG & UCNACKIFG)    // 若无应答 UCNACKIFG=1
    {
      return 0xFF;
    }
  }

  UCB3CTL1 &= ~UCTR;           // 读模式
  UCB3CTL1 |= UCTXSTT;         // 发送启动位和读控制字节

  while(UCB3CTL1 & UCTXSTT);   // 等待UCTXSTT=0
  // 若无应答 UCNACKIFG = 1
  UCB3CTL1 |= UCTXSTP;         // 先发送停止位

  while(!(UCB3IFG & UCRXIFG)); // 读取内容字节
  val = UCB3RXBUF;             // 读取BUF寄存器在发送停止位之后
  while(UCB3CTL1 & UCTXSTP);

  return val;
#else
  unsigned char temp;
  CS_L();
  i2c_start();
  Write_Byte(SLA_W);
  i2c_ack();
  Write_Byte(addr);
  i2c_ack();
  i2c_start();
  Write_Byte(SLA_R);
  i2c_ack();
  temp=Read_Byte();
  i2c_noack();
  i2c_stop();
  CS_H();
  return temp;
#endif
}

void hmc5983_sample(uint8_t gain)
{
#if USE_ISR
#warning "no support 430"
#endif
  LEDS_ON(LEDS_YELLOW);

  //是否每次采样都需要开启温度传感器；
  //需要测试一下{开启/关闭}温度传感器的功耗；
  /*Register_A,0x98 -> 10011000
  * CRA7=1, enable temperature compensation
  * CRA6~5=00, 1 samples averaged (1 to 8) per measurement output.
  * CRA4~2=110,75 Data Output Rate (Hz)
  * CRA1~0=00,Normal measurement configuration (Default)
  */
  Write_IIC(REG_CFG_A,0x98);
  //Write_IIC(REG_CFG_A,0x1C);

   /*Register_B,0x80 -> 10000000
  *case 0: CRB7~5=000, 1370 gain, 0x00, 0.73mg/LSB
  *case 1: CRB7~5=001, 1090 gain, 0x20, 0.92mg/LSB
  *case 2: CRB7~5=010, 820 gain, 0x40, 1.22mg/LSB
  *case 3: CRB7~5=011, 660 gain, 0x60, 1.52mg/LSB
  *case 4: CRB7~5=100, 440 gain, 0x80, 2.27mg/LSB (milli-Gauss per count, LSB)
  *case 5: CRB7~5=101, 390 gain, 0xA0, 2.56mg/LSB
  *case 6: CRB7~5=110, 330 gain, 0xC0, 2.56mg/LSB
  *case 7: CRB7~5=111, 230 gain, 0xE0, 4.35mg/LSB
  *case 0: CRB4~0=00000,these bits must be cleared for correct operation.
  */
  switch(gain) {
  case 0:
    Write_IIC(REG_CFG_B,0x00);   //Gain Configuration 1370
    break;
  case 1:
    Write_IIC(REG_CFG_B,0x20);   //Gain Configuration 1090 (default)
    break;
  case 2:
    Write_IIC(REG_CFG_B,0x40);   //Gain Configuration 820
    break;
  case 3:
    Write_IIC(REG_CFG_B,0x60);   //Gain Configuration 660
    break;
  case 4:
    Write_IIC(REG_CFG_B,0x80);   //Gain Configuration 440
    break;
  case 5:
    Write_IIC(REG_CFG_B,0xA0);   //Gain Configuration 390
    break;
  case 6:
    Write_IIC(REG_CFG_B,0xC0);   //Gain Configuration 330
    break;
  case 7:
    Write_IIC(REG_CFG_B,0xE0);   //Gain Configuration 230
    break;
  default:
    break;
  }
  Write_IIC(REG_MODE,0x01);    //Single-Measurement Mode (Default).

  in_sample = 1;
}

void Read_HMC5883L_Data(unsigned char *buf, unsigned char *tmp)
{
  buf[0]=Read_IIC(REG_DATA_XH);       //Data Output X MSB Register
  buf[1]=Read_IIC(REG_DATA_XL);       //Data Output X LSB Register
  buf[2]=Read_IIC(REG_DATA_YH);       //Data Output y MSB Register
  buf[3]=Read_IIC(REG_DATA_YL);       //Data Output y LSB Register
  buf[4]=Read_IIC(REG_DATA_ZH);       //Data Output z MSB Register
  buf[5]=Read_IIC(REG_DATA_ZL);       //Data Output z LSB Register

  tmp[0]=Read_IIC(REG_TEMP_H);       //Temperature Output MSB Register
  tmp[1]=Read_IIC(REG_TEMP_L);       //Temperature Output LSB Register
}


void hmc5983_self_test(void)       //通过启动传感器self-test 的功能从而实现对于传感器好坏的判定.返回值为0 ，传感器是好的，返回为1，传感器是坏的；
{
  //Write_IIC(REG_CFG_A,0x71);    //8-average, 15 Hz default, positive self test measurement
  Write_IIC(REG_CFG_A,0x11);      //1-average, 15 Hz default, positive self test measurement
  /*Register_B,0x80 -> 10000000
  *case 0: CRB7~5=000, 1370 gain, 0x00, 0.73mg/LSB
  *case 1: CRB7~5=001, 1090 gain, 0x20, 0.92mg/LSB
  *case 2: CRB7~5=010, 820 gain, 0x40, 1.22mg/LSB
  *case 3: CRB7~5=011, 660 gain, 0x60, 1.52mg/LSB
  *case 4: CRB7~5=100, 440 gain, 0x80, 2.27mg/LSB (milli-Gauss per count, LSB)
  *case 5: CRB7~5=101, 390 gain, 0xA0, 2.56mg/LSB
  *case 6: CRB7~5=110, 330 gain, 0xC0, 2.56mg/LSB
  *case 7: CRB7~5=111, 230 gain, 0xE0, 4.35mg/LSB
  */
  Write_IIC(REG_CFG_B,0x80);      //Gain 5 Configuration 390
  Write_IIC(REG_MODE,0x01);       //0x01 Single-Measurement Mode
                                  //0x00 Continuous-measurement mode

  in_sample = 1;
  enableInterrupts();
  __delay_cycles((long)(F_CPU>>10)*7); // ~7ms
  hmc5983_interrupt();
}

void hmc5983_idle(void)
{
  Write_IIC(REG_MODE,0x03); //HMC5883L Device is placed in idle mode.
}
