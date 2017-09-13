#include "contiki.h"

#ifndef _HMC5983_H
#define _HMC5983_H

#define  SLA_W   0x3c     //HMC5983L从机写地址，末位位0
#define  SLA_R   0x3d     //HMC5983L从机读地址，末位位1

#define  REG_CFG_A     0x00
#define  REG_CFG_B     0x01
#define  REG_MODE      0x02
#define  REG_DATA_XH   0x03 //Data Output X MSB Register
#define  REG_DATA_XL   0x04 //Data Output X LSB Register
#define  REG_DATA_ZH   0x05 //Data Output z MSB Register
#define  REG_DATA_ZL   0x06 //Data Output z LSB Register
#define  REG_DATA_YH   0x07 //Data Output y MSB Register
#define  REG_DATA_YL   0x08 //Data Output y LSB Register
#define  REG_STATUS    0x09
#define  REG_ID_A      0x0A
#define  REG_ID_B      0x0B
#define  REG_ID_C      0x0C
#define  REG_TEMP_H    0x31 //Temperature Output MSB Register
#define  REG_TEMP_L    0x32 //Temperature Output LSB Register

#define  GAIN_390      0x00
#define  GAIN_1090     0x20

#define  MODE_CONTI    0x00 //Continuous measurement
#define  MODE_SINGLE   0x01 //Single measurement
#define  MODE_IDLE     0x03 //Idle mode

typedef void (*hmc5983_callback_t)(unsigned char* buf, unsigned char* tmp);

extern unsigned char HMC5883L_Buf[6];

//typedef struct{
//  int16_t x;
//  int16_t y;
//  int16_t z;
//} HMC5983_Struct;
//
//extern HMC5983_Struct HMC5983;

/*****************************************
//函数声明
*****************************************/
extern int hmc5983_init(void);
extern void hmc5983_sample( uint8_t gain );
extern void hmc5983_self_test(void);
extern void hmc5983_idle(void);
extern void hmc5983_sample_read( uint8_t gain);
extern void hmc5983_set_callback(hmc5983_callback_t cback);
int hmc5983_get_temperature(void);
void hmc5983_interrupt(void);

#endif /* _HMC5983_H */
