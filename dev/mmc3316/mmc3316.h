/* ========================================
   功能描述: MMC3316驱动
   接口说明：见宏定义
   版本号  ：V1.0
   注意问题：
   修改记录：
   BY THF20151118
   */

#ifndef _MMC3316_H
#define _MMC3316_H

#include <stdio.h>
#include <msp430x54xA.h>

//typedef struct{
//  unsigned int x;
//  unsigned int y;
//  unsigned int z;
//} MMC3316_Struct;
//
//extern MMC3316_Struct MMC3316[];

/*================================================================
  【名 称】
  【功 能】模拟IIC使用的引脚定义，注意要上拉电阻
  【备 注】需已配置对应端口为双向状态，根据具体硬件平台修改
  ================================================================*/
#if 0
#define MMC3316_SCL_PORT(reg) P5##reg
#define MMC3316_SCL_PIN  BIT4
#define MMC3316_SDA_PORT(reg) P3##reg
#define MMC3316_SDA_PIN  BIT7
#endif

#if 0
#define MMC3316_SCL_PORT(reg) P10##reg
#define MMC3316_SCL_PIN  BIT2
#define MMC3316_SDA_PORT(reg) P10##reg
#define MMC3316_SDA_PIN  BIT1
#endif

//#define MMC3316_SDA_1()	MMC3316_SDA_PORT(OUT) |= MMC3316_SDA_PIN;
//#define MMC3316_SDA_0()	MMC3316_SDA_PORT(OUT) &= ~MMC3316_SDA_PIN;
//
//#define MMC3316_SCL_1()	MMC3316_SCL_PORT(OUT) |= MMC3316_SCL_PIN;
//#define MMC3316_SCL_0()	MMC3316_SCL_PORT(OUT) &= ~MMC3316_SCL_PIN;

//#define READ_SDA  (MMC3316_SDA_PORT(IN) & MMC3316_SDA_PIN)

#define MMC3316_ADDRW 0x60		//MMC3316的I2C地址Write
#define MMC3316_ADDRR 0x61		//MMC3316的I2C地址Read

#define MMC3316_Xout_Low	0x00	//
#define MMC3316_Xout High	0x01	//
#define MMC3316_Yout_Low	0x02	//
#define MMC3316_Yout High	0x03	//
#define MMC3316_Zout_Low	0x04	//
#define MMC3316_Zout High	0x05	//
#define MMC3316_Status		0x06	//
#define MMC3316_CTRL_0		0x07	//
#define MMC3316_CTRL_1		0x08	//

#define MMC3316_PID_0			0x10	//
#define MMC3316_R0				0x1C	//
#define MMC3316_R1				0x1D	//
#define MMC3316_R2				0x1E	//
#define MMC3316_R3				0x1F	//
#define MMC3316_PID_1			0x20	//

#define ACTION_STOP					0x00	//
#define ACTION_PRE					0x01	//prepare
#define ACTION_SET					0x20
#define ACTION_RESET				0x40

#define ACTION_READ					0x01

#define ACK		0	//应答位电平定义
#define NACK	1

#define FALSE		0	//应答位电平定义
#define TRUE	1

#define TIME_DELAY	1	//Us延时调用

#define  FILTER_LENGTH   10   //滤波器队列长度

void MMC3316_Port_Init(int devid);

/*================================================================
  【名 称】void MMC3316_Init(void)
  【功 能】初始化函数，主函数中调用
  【备 注】
  ================================================================*/
void MMC3316_Init(int devid);

/*================================================================
  【名 称】void MMC3316_Reset(void)
  【功 能】Reset
  【备 注】
  ================================================================*/
void MMC3316_Reset(int devid);

/*================================================================
  【名 称】void MMC3316_Check(void)
  【功 能】Check 检测能否通信
  【备 注】return true is succeed
  ================================================================*/
unsigned char MMC3316_Check(int devid);

/*================================================================
  【名 称】unsigned char MMC3316_Read(void)
  【功 能】MEASUREMENT
  【备 注】数据存在MMC3316_Struct中,return true is succeed
  ================================================================*/
unsigned char MMC3316_Read(int devid);

#endif

