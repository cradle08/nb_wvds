#include "contiki.h"
#include "platform-conf.h"
#include "qmc5883.h"
#include "isr_compat.h"
#include <string.h>

#define REG_DATA_XL  0x00
#define REG_DATA_XH  0x01
#define REG_DATA_YL  0x02
#define REG_DATA_YH  0x03
#define REG_DATA_ZL  0x04
#define REG_DATA_ZH  0x05
#define REG_STATUS   0x06
#define REG_TEMPE_L  0x07
#define REG_TEMPE_H  0x08
#define REG_MODE     0x09
#define REG_CTRL     0x0A
#define REG_SET_RES  0x0B

#define USE_ISR      0
#define USE_CTIMER   0

#define QMC5883_LPM  0

#define QMC5883_NEW  1

#ifndef QMC5883_RANGE
#define QMC5883_RANGE  8 // 8 or 2
#endif
#if QMC5883_RANGE == 8
/*
 *0x91 OSR:128, 8G, 10Hz, Continuous
 *0x51 OSR:256, 8G, 10Hz, Continuous
 *0x11 OSR:512, 8G, 10Hz, Continuous
 *0x19 OSR:512, 8G, 100Hz, Continuous
*/
#define QMC5883_MODE  0x19
#define LSB_PER_MG    3
//#warning "using range ±8 Gauss"

#elif QMC5883_RANGE == 2
/*
 *0x81 OSR:128, 2G, 10Hz, Continuous
 *0x41 OSR:256, 2G, 10Hz, Continuous
 *0x01 OSR:512, 2G, 10Hz, Continuous
 *0x09 OSR:512, 2G, 100Hz, Continuous
*/
#define QMC5883_MODE  0x81
#define LSB_PER_MG    12
//#warning "using range ±2 Gauss"

#else
#error "QMC5883_RANGE must defined to be 2 or 8"
#endif




static uint8_t qmc5883_reg = 0;
static uint8_t qmc5883_XYZ[6] = {0};
static uint8_t qmc5883_temp[2] = {0};

static uint8_t qmc5883_is_on = 0;
static uint8_t qmc5883_pend = 0;
static qmc5883_callback_t qmc5883_cb;

#if USE_CTIMER
static struct ctimer qmc5883_ct;
#endif

#define I2C_WAIT 128 // ~4ms //32768>>8  =128
#define BUSYWAIT_UNTIL(cond, max_time) do { \
  rtimer_clock_t t0; \
  t0 = RTIMER_NOW(); \
  while(!(cond) && RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + (max_time))) { } \
} while(0)
/*------------------------------------------------------------------*/
void qmc5883_read(int16_t *x, int16_t *y, int16_t *z, int16_t *t);
void qmc5883_standby(void);
void qmc5883_interrupt(void *ptr);

int qmc5883_write_reg(uint8_t reg, uint8_t val);
uint8_t qmc5883_read_reg(uint8_t reg);
int qmc5883_read_regs(uint8_t reg, uint8_t len, uint8_t *buf);

void qmc5883_arch_init(void);
void qmc5883_arch_on(void);
void qmc5883_arch_off(void);
int qmc5883_arch_read(uint8_t reg, uint8_t len, uint8_t *buf);
int qmc5883_arch_write(uint8_t *buf, uint8_t len);

/*------------------------------------------------------------------*/

int qmc5883_init(void)
{
  uint8_t *reg = &qmc5883_reg;
  static uint8_t fail = 0;

  qmc5883_arch_init();
  

  
  qmc5883_write_reg(REG_CTRL, 0x80);
  *reg = qmc5883_read_reg(REG_MODE);

  do {
    qmc5883_write_reg(REG_MODE, QMC5883_MODE);
    *reg = qmc5883_read_reg(REG_MODE);
  } while((*reg != QMC5883_MODE) && (++fail < 4));
  
  if (*reg != QMC5883_MODE) {
    return 1;
  }

#if QMC5883_NEW
  qmc5883_write_reg(0x0B, 0x01);
  qmc5883_write_reg(0x20, 0x40);
  qmc5883_write_reg(0x21, 0x01);
#endif

#if QMC5883_LPM
  qmc5883_standby();
#endif
  return 0;
}

void qmc5883_set_callback(qmc5883_callback_t callback)
{
  qmc5883_cb = callback;
}

void qmc5883_self_test(void)
{
}

void qmc5883_sample_read(uint8_t gain)
{
  qmc5883_sample();
#if !USE_ISR
#if USE_CTIMER
  ctimer_set(&qmc5883_ct, (CLOCK_SECOND>>7), qmc5883_interrupt, NULL);
#else
  qmc5883_interrupt(NULL);
#endif
#endif
}

int qmc5883_get_temperature(void)
{
  return 26;
}

/*------------------------------------------------------------------*/
void qmc5883_sample(void)
{
#if QMC5883_LPM
  if (qmc5883_is_on == 0)
    qmc5883_arch_on();
#endif

#if QMC5883_NEW
  qmc5883_write_reg(0x09, 0x1D);//00011101
  qmc5883_write_reg(0x09, 0x1C);//00011100
#else
  //qmc5883_write_reg(REG_CTRL, 0x80);
  qmc5883_write_reg(REG_SET_RES, 0x01);
  qmc5883_write_reg(0x20, 0x40);
  qmc5883_write_reg(0x21, 0x01);
  qmc5883_write_reg(REG_MODE, QMC5883_MODE);
#endif
  qmc5883_pend = 1;

#if !USE_ISR && !USE_CTIMER
  //while ((qmc5883_read_reg(REG_STATUS) & 0x01) == 0) {
  __delay_cycles((long)F_CPU >> 11); // wait ~2ms
  //}
#endif
}

void
qmc5883_read(int16_t *x, int16_t *y, int16_t *z, int16_t *t)
{
  uint8_t buf[9] = {0} ;

  qmc5883_read_regs(REG_DATA_XL, 9, buf);

  *x = ((int16_t)(buf[0] + (buf[1]<<8))) / LSB_PER_MG;
  *y = ((int16_t)(buf[2] + (buf[3]<<8))) / LSB_PER_MG;
  *z = ((int16_t)(buf[4] + (buf[5]<<8))) / LSB_PER_MG;
  *t = (int16_t)(buf[7] + (buf[8]<<8));
}

void
qmc5883_standby(void)
{
  qmc5883_write_reg(REG_MODE, 0x00);
  qmc5883_arch_off();
}

void qmc5883_interrupt(void *ptr)
{
  int16_t xx, yy, zz, tt;
  uint8_t *xyz = qmc5883_XYZ;
  uint8_t *tem = qmc5883_temp;

  if (qmc5883_pend == 0) {
    return;
  }

  if ((qmc5883_read_reg(REG_STATUS) & 0x01) == 0) {
    memset(xyz, 0xFF, 6);
    memset(tem, 0xFF, 2);
    return;
  }

  qmc5883_pend = 0;
  qmc5883_read(&xx, &yy, &zz, &tt);
#if QMC5883_LPM
  qmc5883_standby();
#endif

  *xyz++ = (((uint16_t)xx) >> 8);
  *xyz++ = (((uint16_t)xx) & 0xff);
  *xyz++ = (((uint16_t)yy) >> 8);
  *xyz++ = (((uint16_t)yy) & 0xff);
  *xyz++ = (((uint16_t)zz) >> 8);
  *xyz++ = (((uint16_t)zz) & 0xff);
  *tem++ = (((uint16_t)tt) >> 8);
  *tem++ = (((uint16_t)tt) & 0xff);
  if(qmc5883_cb) {
    qmc5883_cb(qmc5883_XYZ, qmc5883_temp);
  }
}


/*------------------------------------------------------------------*/
int qmc5883_write_reg(uint8_t reg, uint8_t val)
{
  uint8_t buf[2];
  buf[0] = reg;
  buf[1] = val;
  return qmc5883_arch_write(buf, 2);
}

uint8_t qmc5883_read_reg(uint8_t reg)
{
  uint8_t buf[1];
  int r = 0;
  r = qmc5883_arch_read(reg, 1, buf);
  return (r == 0 ? buf[0] : 0xFF);
}

int qmc5883_read_regs(uint8_t reg, uint8_t len, uint8_t *buf)
{
#if 1
  return qmc5883_arch_read(reg, len, buf);
#else
  uint8_t i;
  for (i = 0; i < len; i++)
    buf[i] = qmc5883_read_reg(reg+i);
  return 0;
#endif
}

/*------------------------------------------------------------------*/
// -----------------------
//  QMC5883  MSP430F5438a
// -----------------------
//  SCL      P10.1
//  SDA      P10.2
//  CS       Px.x
//  DRDY     P2.4
// -----------------------
void
qmc5883_arch_init(void)
{
  qmc5883_arch_on();
}


void qmc5883_arch_on(void)
{
  // p4.1,p4.2 SDA/SCL
  P4SEL |= BIT1+BIT2;
  P4DIR &= ~(BIT1 + BIT2);
  
  UCB1CTL1 |= UCSWRST;
  UCB1CTL0 |= UCMODE_3 + UCMST + UCSYNC;
  UCB1CTL1 |= UCSSEL__SMCLK+UCSWRST;
  UCB1BR0 = 80;
  UCB1BR1 = 0x00;
  UCB1I2CSA = 0x0D;

  UCB1CTL1 &= ~UCSWRST;
  
#if USE_ISR
  P2SEL &= ~BIT0;
  P2DIR &= ~BIT0;
  P2IES &= ~BIT0; // rising edge
  P2IFG &= ~BIT0;
  P2IE  |=  BIT0;
#endif

  qmc5883_is_on = 1;
  
}


void qmc5883_arch_off(void)
{
  // P4.1,p4.2 SDA/SCL
  P4SEL &= ~(BIT1 + BIT2);
  P4DIR &= ~(BIT1 + BIT2);
  P4REN |=  (BIT1 + BIT2);
  P4OUT |=  (BIT1 + BIT2);

  qmc5883_is_on = 0;
}
  



int qmc5883_arch_read(uint8_t reg, uint8_t len, uint8_t *buf)

{
  uint8_t idx = 0;
  uint8_t end = 0xFF; // DO NOT remove
  int r = 0;

  UCB1CTL1 |= UCTR;
  UCB1CTL1 |= UCTXSTT;

  UCB1TXBUF = reg;
  BUSYWAIT_UNTIL((r = (UCB1IFG & UCTXIFG)), I2C_WAIT);
  if (r == 0)
    return 1;
  
  UCB1CTL1 &= ~UCTR;
  UCB1CTL1 |= UCTXSTT;
  BUSYWAIT_UNTIL((r = !(UCB1CTL1 & UCTXSTT)),I2C_WAIT);
  if (r == 0)
    return 2;
         
  idx = 0;
  while(len) {
    BUSYWAIT_UNTIL((r = UCB1IFG & UCRXIFG),I2C_WAIT);    
    if (r == 0)
      return 3;
      
    if (len == 1) {
      UCB1CTL1 |= UCTXSTP;
    }
    buf[idx++] = UCB1RXBUF;
    len--;
    UCB1IFG &= ~UCRXIFG;
  }

  BUSYWAIT_UNTIL((r = !(UCB1CTL1&UCTXSTP)),I2C_WAIT);
  if (r == 0)
    return 4;

  BUSYWAIT_UNTIL((r = (UCB1IFG & UCRXIFG)),I2C_WAIT);
  if (r == 0)
    return 5;
    
  end = UCB1RXBUF;
  
  return 0;
}

//  After STT is clear, must check whether UCNACKIFG has been set
//  In this case you should send a stop condition and bail out
  
//u8 r =0;

int qmc5883_arch_write(uint8_t *buf, uint8_t len)
{
  int r = 0;

  UCB1CTL1 |= UCTXSTT+UCTR;

  while(len){
  UCB1TXBUF=*buf++;
  len--;
  BUSYWAIT_UNTIL((r = UCB1IFG&UCTXIFG),I2C_WAIT);
  if (r == 0)
    return 1;
  }  
  
  UCB1CTL1|=UCTXSTP;
  BUSYWAIT_UNTIL((r = UCB1IFG&UCTXIFG),I2C_WAIT);
    if (r == 0)
    return 2;
    
  return 0;
}

/*


不同I2C slave，有不同的command gap time的，datasheet 里应该会有这些。

做Start / Stop 的时候要加些延时进去就OK了。

另外while的时候加个timeout机制啊，超时报错，增加些健壮性，。

while (UCB0CTL1 & UCTXSTP && timeout){

timeout--;

}


*/
#if USE_ISR
ISR(USCI_B1, USCI_B1_interrupt)
{
  P2IFG &= ~BIT4;
  qmc5883_interrupt(NULL);
  LPM3_EXIT;
}
#endif