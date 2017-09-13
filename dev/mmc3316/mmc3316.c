/* ========================================
   ��������: MMC3316����
   �ӿ�˵�������궨��
   �汾��  ��V1.0
   ע�����⣺
   �޸ļ�¼��
   BY THF20151118
 * ========================================
 */

#include "MMC3316.H"
#include <stdint.h>
#include "platform-conf.h"
#include "VehicleDetection.h"

// sensor 1: SCL/P5.4, SDA/P3.7
#define SCL1(reg)  P5##reg
#define SCL1_PIN   BIT4
#define SDA1(reg)  P3##reg
#define SDA1_PIN   BIT7

// sensor 2: SCL/P10.2, SDA/P10.1
#define SCL2(reg)  P10##reg
#define SCL2_PIN   BIT2
#define SDA2(reg)  P10##reg
#define SDA2_PIN   BIT1

#define delay_us(x) __delay_cycles((long)(F_CPU*(double)x/1000000.0))
#define delay_ms(x) __delay_cycles((long)(F_CPU*(double)x/1000.0))

//MMC3316_Struct MMC3316[2];
static int cur_dev = -1;

#if 1
void SCL_OUT(void)
{
  switch(cur_dev) {
    case 0:
      SCL1(DIR) |=  SCL1_PIN;
      SCL1(SEL) &= ~SCL1_PIN;
      break;
    case 1:
      SCL2(DIR) |=  SCL2_PIN;
      SCL2(SEL) &= ~SCL2_PIN;
      break;
    default:
      break;
  }
}

void SCL_1(void)
{
  switch(cur_dev) {
    case 0:
      SCL1(OUT) |= SCL1_PIN; break;
    case 1:
      SCL2(OUT) |= SCL2_PIN; break;
    default:
      break;
  }
}

void SCL_0(void)
{
  switch(cur_dev) {
    case 0:
      SCL1(OUT) &= ~SCL1_PIN; break;
    case 1:
      SCL2(OUT) &= ~SCL2_PIN; break;
    default:
      break;
  }
}

void SDA_OUT(void)
{
  switch(cur_dev) {
    case 0:
      SDA1(DIR) |=  SDA1_PIN;
      SDA1(SEL) &= ~SDA1_PIN;
      break;
    case 1:
      SDA2(DIR) |=  SDA2_PIN;
      SDA2(SEL) &= ~SDA2_PIN;
      break;
  }
}

void SDA_IN(void)
{
  switch(cur_dev) {
    case 0:
      SDA1(DIR) &= ~SDA1_PIN;
      SDA1(SEL) &= ~SDA1_PIN;
      break;
    case 1:
      SDA2(DIR) &= ~SDA2_PIN;
      SDA2(SEL) &= ~SDA2_PIN;
      break;
  }
}

void SDA_1(void)
{
  switch(cur_dev) {
    case 0:
      SDA1(OUT) |= SDA1_PIN; break;
    case 1:
      SDA2(OUT) |= SDA2_PIN; break;
    default:
      break;
  }
}

void SDA_0(void)
{
  switch(cur_dev) {
    case 0:
      SDA1(OUT) &= ~SDA1_PIN; break;
    case 1:
      SDA2(OUT) &= ~SDA2_PIN; break;
    default:
      break;
  }
}

#else
#define SCL_OUT() do { \
  switch(cur_dev) { \
    case 0: \
      SCL1(DIR) |=  SCL1_PIN; \
      SCL1(SEL) &= ~SCL1_PIN; \
      break; \
    case 1: \
      SCL2(DIR) |=  SCL2_PIN; \
      SCL2(SEL) &= ~SCL2_PIN; \
      break; \
    default: \
      break; \
  } \
} while(0)

#define SCL_1() do { \
  switch(cur_dev) { \
    case 0: \
      SCL1(OUT) |= SCL1_PIN; break; \
    case 1: \
      SCL2(OUT) |= SCL2_PIN; break; \
    default: \
      break; \
  } \
} while(0)

#define SCL_0() do { \
  switch(cur_dev) { \
    case 0: \
      SCL1(OUT) &= ~SCL1_PIN; break; \
    case 1: \
      SCL2(OUT) &= ~SCL2_PIN; break; \
    default: \
      break; \
  } \
} while(0)

#define SDA_OUT() do { \
  switch(cur_dev) { \
    case 0: \
      SDA1(DIR) |=  SDA1_PIN; \
      SDA1(SEL) &= ~SDA1_PIN; \
      break; \
    case 1: \
      SDA2(DIR) |=  SDA2_PIN; \
      SDA2(SEL) &= ~SDA2_PIN; \
      break; \
  } \
} while(0)

#define SDA_IN() do { \
  switch(cur_dev) { \
    case 0: \
      SDA1(DIR) &= ~SDA1_PIN; \
      SDA1(SEL) &= ~SDA1_PIN; \
      break; \
    case 1: \
      SDA2(DIR) &= ~SDA2_PIN; \
      SDA2(SEL) &= ~SDA2_PIN; \
      break; \
  } \
} while(0)


#define SDA_1() do { \
  switch(cur_dev) { \
    case 0: \
      SDA1(OUT) |= SDA1_PIN; break; \
    case 1: \
      SDA2(OUT) |= SDA2_PIN; break; \
    default: \
      break; \
  } \
} while(0)

#define SDA_0() do { \
  switch(cur_dev) { \
    case 0: \
      SDA1(OUT) &= ~SDA1_PIN; break; \
    case 1: \
      SDA2(OUT) &= ~SDA2_PIN; break; \
    default: \
      break; \
  } \
} while(0)

#endif

int READ_SDA(void)
{
  int r = -1;

  switch(cur_dev) {
    case 0:
      r = (SDA1(IN) & SDA1_PIN); break;
    case 1:
      r = (SDA2(IN) & SDA2_PIN); break;
    default:
      break;
  }

  return r;
}

/*================================================================
  ���� �ơ�void MMC3316_Port_Init(int devid)
  ���� �ܡ��˿ڳ�ʼ��
  ���� ע�����ݾ���Ӳ��ƽ̨�޸�
  ================================================================*/
void MMC3316_Port_Init(int devid)
{
  cur_dev = devid;

  SDA_OUT();
  SCL_OUT();
}

/*================================================================
  ���� �ơ�void I2C_Init(void)
  ���� �ܡ�I2C��ʼ��������״̬
  ���� ע��
  ================================================================*/
void MMC3316_I2C_Init(void)
{
  SDA_1();
  SCL_1();
}

/*================================================================
  ���� �ơ�void I2C_Start(void)
  ���� �ܡ�I2C��ʼ�ź�
  ���� ע��SCL��SDAͬΪ�ߣ�SDA����ɵ�֮��SCL����ɵ�
  ================================================================*/
static void I2C_Start(void)
{
  SDA_1();
  SCL_1();
  delay_us(TIME_DELAY);
  SDA_0();
  delay_us(TIME_DELAY);
  SCL_0();
  delay_us(TIME_DELAY);
}

/*================================================================
  ���� �ơ�void I2C_Stop(void)
  ���� �ܡ�I2Cֹͣ�ź�
  ���� ע��SCL��SDAͬΪ�ͣ�SCL����ɸ�֮��SDA����ɸ�
  ================================================================*/
static void I2C_Stop(void)
{
  SDA_0();
  SCL_0();
  delay_us(TIME_DELAY);
  SCL_1();
  delay_us(TIME_DELAY);
  SDA_1();
  delay_us(TIME_DELAY);
}

/*================================================================
  ���� �ơ�unsigned char I2C_Write(unsigned char WRByte)
  ���� �ܡ�I2Cдһ���ֽ����ݣ�����ACK����NACK
  ���� ע���Ӹߵ��ͣ����η���
  ================================================================*/
static unsigned char I2C_Write(unsigned char WRByte)
{
  unsigned char i;

  //SCL_0();
  for(i=0;i<8;i++)
  {
    if(WRByte&0x80)
    {
      SDA_1();
    }
    else
    {
      SDA_0();
    }
    delay_us(TIME_DELAY);
    SCL_1();			//���SDA�ȶ�������SCL���������أ��ӻ���⵽��������ݲ���
    delay_us(2*TIME_DELAY);
    SCL_0();
    delay_us(TIME_DELAY);
    WRByte <<= 1;
  }

  SDA_1();
  SCL_1();
  delay_us(TIME_DELAY);
  SDA_IN();
  if(READ_SDA())			//SDAΪ�ߣ�û�յ�ACK
  {
    SCL_0();
    delay_us(TIME_DELAY);
    SDA_OUT();
    return NACK;
  }
  else 								//SDAΪ�ͣ��յ�ACK
  {
    SCL_0();
    delay_us(TIME_DELAY);
    SDA_OUT();
    return ACK;
  }
}


/*================================================================
  ���� �ơ�unsigned char I2C_Read(unsigned char AckValue)
  ���� �ܡ�I2C��һ���ֽ����ݣ���ڲ������ڿ���Ӧ��״̬��ACK����NACK
  ���� ע���Ӹߵ��ͣ����ν���
  ================================================================*/
static unsigned char I2C_Read(unsigned char AckValue)
{
  unsigned char i,RDByte=0;

  SCL_0();
  SDA_1();			//�ͷ�����
  delay_us(TIME_DELAY);

  //���Ӵ�������ȡ����ʱ, DATA �� SCK ����Ժ���Ч����ά�ֵ���һ��SCK ���½��ء�
  SDA_IN();
  for (i=0;i<8;i++)
  {
    RDByte <<= 1;					//��λ
    SCL_1();			//����������
    delay_us(TIME_DELAY);	//��ʱ�ȴ��ź��ȶ�
    SDA_IN();
    if(READ_SDA()) 			//������ȡ����
    {
      RDByte |= 0x01;
    }
    else
    {
      RDByte &= 0xfe;
    }
    delay_us(TIME_DELAY);	//modify by yng
    SCL_0();   		//�½��أ��ӻ�������һλֵ
    delay_us(TIME_DELAY);
  }
  SDA_OUT();
  //SDA = AckValue;
  if(AckValue)
  {//Ӧ��״̬
    SDA_1();		//NACK
  }
  else
  {
    SDA_0();		//ACK
  }
  delay_us(TIME_DELAY);
  SCL_1();
  delay_us(TIME_DELAY);
  SCL_0();
  SDA_1();
  delay_us(TIME_DELAY);

  return RDByte;
}

/*================================================================
  ���� �ơ�void MMC3316_Init(void)
  ���� �ܡ���ʼ���������������е���
  ���� ע��
  ================================================================*/
void MMC3316_Init(int devid)
{
  cur_dev = devid;

  MMC3316_I2C_Init();		//I2C��ʼ��
  I2C_Start();	//����I2C

  I2C_Write(MMC3316_ADDRW);  	//д��ַ
  I2C_Write(MMC3316_CTRL_0);  //д��ָ��
  I2C_Write(ACTION_PRE); 			//д��ֵ

  delay_ms(50);

  //	I2C_Start();	//����I2C
  //	I2C_Write(MMC3316_ADDRW);  	//д��ַ
  //	I2C_Write(MMC3316_CTRL_0);  //д��ָ��
  I2C_Write(ACTION_SET); 			//д��ֵ
  //	I2C_Start();	//����I2C
  //	I2C_Write(MMC3316_ADDRW);  	//д��ַ
  //	I2C_Write(MMC3316_CTRL_0);  //д��ָ��
  I2C_Write(ACTION_STOP); 		//д��ֵ

  I2C_Stop();		//ͣI2C
}

/*================================================================
  ���� �ơ�void MMC3316_Reset(void)
  ���� �ܡ�Reset
  ���� ע��
  ================================================================*/
void MMC3316_Reset(int devid)
{
  cur_dev = devid;

  MMC3316_I2C_Init();		//I2C��ʼ��
  I2C_Start();	//����I2C

  I2C_Write(MMC3316_ADDRW);  	//д��ַ
  I2C_Write(MMC3316_CTRL_0);  //д��ָ��
  I2C_Write(ACTION_PRE); 			//д��ֵ

  delay_ms(50);

  I2C_Write(ACTION_SET); 			//д��ֵ
  I2C_Write(ACTION_STOP); 		//д��ֵ

  delay_ms(50);

  I2C_Write(ACTION_RESET); 		//д��ֵ
  I2C_Write(ACTION_STOP); 		//д��ֵ

  I2C_Stop();		//ͣI2C

  delay_ms(1);
}

/*================================================================
  ���� �ơ�void MMC3316_Check(void)
  ���� �ܡ�Check ����ܷ�ͨ��
  ���� ע��return true is succeed
  ================================================================*/
unsigned char MMC3316_Check(int devid)
{
  cur_dev = devid;

  MMC3316_I2C_Init();		//I2C��ʼ��
  I2C_Start();	//����I2C

  //The SET preparation action
  if(I2C_Write(MMC3316_ADDRW) == NACK)			//д��ַ
    return FALSE;
  if(I2C_Write(MMC3316_CTRL_0) == NACK)			//д��ָ��
    return FALSE;
  if(I2C_Write(ACTION_PRE) == NACK)					//д��ֵ
    return FALSE;
  delay_ms(50);

  I2C_Start();	//����I2C
  if(I2C_Write(MMC3316_ADDRW) == NACK)			//д��ַ
    return FALSE;
  if(I2C_Write(MMC3316_PID_1) == NACK)			//д��ָ��
    return FALSE;

  I2C_Start();	//����I2C
  if(I2C_Write(MMC3316_ADDRR) == NACK)			//����ַ
    return FALSE;
  if(I2C_Read(NACK) != 0x05)								//Product ID 1: is 0x05
    return FALSE;

  I2C_Stop();		//ͣI2C

  return TRUE;
}

/*================================================================
  ���� �ơ�unsigned char MMC3316_Read(void)
  ���� �ܡ�MEASUREMENT
  ���� ע�����ݴ���MMC3316_Struct��,return true is succeed
  ================================================================*/
unsigned char MMC3316_Read(int devid)
{
  uint32_t User_Timer = 500000;

  cur_dev = devid;

  MMC3316_I2C_Init();		//I2C��ʼ��
  I2C_Start();	//����I2C
  I2C_Write(MMC3316_ADDRW);  	//д��ַ
  I2C_Write(MMC3316_CTRL_0);  //д��ָ��
  I2C_Write(ACTION_PRE); 			//д��ֵ
  I2C_Stop();		//ͣI2C

  //delay_ms(50);

  I2C_Start();	//����I2C
  I2C_Write(MMC3316_ADDRW);  	//д��ַ
  I2C_Write(MMC3316_Status);  //д��ָ��

  I2C_Start();	//����I2C
  I2C_Write(MMC3316_ADDRR);  	//����ַ
  //Continuously read the Status Register until the Meas Done bit is set to 1
  while(((I2C_Read(NACK) & ACTION_READ) != ACTION_READ)&&(User_Timer--));
  if(User_Timer == 0)					//��ʱ
    return FALSE;

  I2C_Start();	//����I2C
  I2C_Write(MMC3316_ADDRW);  	//д��ַ
  I2C_Write(MMC3316_Xout_Low);//д��ָ��

  I2C_Start();	//����I2C
  I2C_Write(MMC3316_ADDRR);  	//����ַ

  One_Sample.x = I2C_Read(ACK);   //��ȡ
  One_Sample.x += ((unsigned int)I2C_Read(ACK)<<8);   //��ȡ

  One_Sample.y = I2C_Read(ACK);   //��ȡ
  One_Sample.y += ((unsigned int)I2C_Read(ACK)<<8);   //��ȡ

  One_Sample.z = I2C_Read(ACK);   //��ȡ
  One_Sample.z += ((unsigned int)I2C_Read(NACK)<<8);   //��ȡ	//ACK???
  
  
//  MMC3316[cur_dev].x = I2C_Read(ACK);   //��ȡ
//  MMC3316[cur_dev].x += ((unsigned int)I2C_Read(ACK)<<8);   //��ȡ
//
//  MMC3316[cur_dev].y = I2C_Read(ACK);   //��ȡ
//  MMC3316[cur_dev].y += ((unsigned int)I2C_Read(ACK)<<8);   //��ȡ
//
//  MMC3316[cur_dev].z = I2C_Read(ACK);   //��ȡ
//  MMC3316[cur_dev].z += ((unsigned int)I2C_Read(NACK)<<8);   //��ȡ	//ACK???

  I2C_Stop();		//ͣI2C

  return TRUE;
}

