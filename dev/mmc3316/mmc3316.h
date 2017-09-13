/* ========================================
   ��������: MMC3316����
   �ӿ�˵�������궨��
   �汾��  ��V1.0
   ע�����⣺
   �޸ļ�¼��
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
  ���� �ơ�
  ���� �ܡ�ģ��IICʹ�õ����Ŷ��壬ע��Ҫ��������
  ���� ע���������ö�Ӧ�˿�Ϊ˫��״̬�����ݾ���Ӳ��ƽ̨�޸�
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

#define MMC3316_ADDRW 0x60		//MMC3316��I2C��ַWrite
#define MMC3316_ADDRR 0x61		//MMC3316��I2C��ַRead

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

#define ACK		0	//Ӧ��λ��ƽ����
#define NACK	1

#define FALSE		0	//Ӧ��λ��ƽ����
#define TRUE	1

#define TIME_DELAY	1	//Us��ʱ����

#define  FILTER_LENGTH   10   //�˲������г���

void MMC3316_Port_Init(int devid);

/*================================================================
  ���� �ơ�void MMC3316_Init(void)
  ���� �ܡ���ʼ���������������е���
  ���� ע��
  ================================================================*/
void MMC3316_Init(int devid);

/*================================================================
  ���� �ơ�void MMC3316_Reset(void)
  ���� �ܡ�Reset
  ���� ע��
  ================================================================*/
void MMC3316_Reset(int devid);

/*================================================================
  ���� �ơ�void MMC3316_Check(void)
  ���� �ܡ�Check ����ܷ�ͨ��
  ���� ע��return true is succeed
  ================================================================*/
unsigned char MMC3316_Check(int devid);

/*================================================================
  ���� �ơ�unsigned char MMC3316_Read(void)
  ���� �ܡ�MEASUREMENT
  ���� ע�����ݴ���MMC3316_Struct��,return true is succeed
  ================================================================*/
unsigned char MMC3316_Read(int devid);

#endif

