/* ========================================
   功能描述: MMC3316驱动
   接口说明：见宏定义
   版本号  ：V1.0
   注意问题：
   修改记录：
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
  【名 称】void MMC3316_Port_Init(int devid)
  【功 能】端口初始化
  【备 注】根据具体硬件平台修改
  ================================================================*/
void MMC3316_Port_Init(int devid)
{
  cur_dev = devid;

  SDA_OUT();
  SCL_OUT();
}

/*================================================================
  【名 称】void I2C_Init(void)
  【功 能】I2C初始化，空闲状态
  【备 注】
  ================================================================*/
void MMC3316_I2C_Init(void)
{
  SDA_1();
  SCL_1();
}

/*================================================================
  【名 称】void I2C_Start(void)
  【功 能】I2C起始信号
  【备 注】SCL、SDA同为高，SDA跳变成低之后，SCL跳变成低
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
  【名 称】void I2C_Stop(void)
  【功 能】I2C停止信号
  【备 注】SCL、SDA同为低，SCL跳变成高之后，SDA跳变成高
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
  【名 称】unsigned char I2C_Write(unsigned char WRByte)
  【功 能】I2C写一个字节数据，返回ACK或者NACK
  【备 注】从高到低，依次发送
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
    SCL_1();			//输出SDA稳定后，拉高SCL给出上升沿，从机检测到后进行数据采样
    delay_us(2*TIME_DELAY);
    SCL_0();
    delay_us(TIME_DELAY);
    WRByte <<= 1;
  }

  SDA_1();
  SCL_1();
  delay_us(TIME_DELAY);
  SDA_IN();
  if(READ_SDA())			//SDA为高，没收到ACK
  {
    SCL_0();
    delay_us(TIME_DELAY);
    SDA_OUT();
    return NACK;
  }
  else 								//SDA为低，收到ACK
  {
    SCL_0();
    delay_us(TIME_DELAY);
    SDA_OUT();
    return ACK;
  }
}


/*================================================================
  【名 称】unsigned char I2C_Read(unsigned char AckValue)
  【功 能】I2C读一个字节数据，入口参数用于控制应答状态，ACK或者NACK
  【备 注】从高到低，依次接收
  ================================================================*/
static unsigned char I2C_Read(unsigned char AckValue)
{
  unsigned char i,RDByte=0;

  SCL_0();
  SDA_1();			//释放总线
  delay_us(TIME_DELAY);

  //当从传感器读取数据时, DATA 在 SCK 变低以后有效，且维持到下一个SCK 的下降沿。
  SDA_IN();
  for (i=0;i<8;i++)
  {
    RDByte <<= 1;					//移位
    SCL_1();			//给出上升沿
    delay_us(TIME_DELAY);	//延时等待信号稳定
    SDA_IN();
    if(READ_SDA()) 			//采样获取数据
    {
      RDByte |= 0x01;
    }
    else
    {
      RDByte &= 0xfe;
    }
    delay_us(TIME_DELAY);	//modify by yng
    SCL_0();   		//下降沿，从机给出下一位值
    delay_us(TIME_DELAY);
  }
  SDA_OUT();
  //SDA = AckValue;
  if(AckValue)
  {//应答状态
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
  【名 称】void MMC3316_Init(void)
  【功 能】初始化函数，主函数中调用
  【备 注】
  ================================================================*/
void MMC3316_Init(int devid)
{
  cur_dev = devid;

  MMC3316_I2C_Init();		//I2C初始化
  I2C_Start();	//启动I2C

  I2C_Write(MMC3316_ADDRW);  	//写地址
  I2C_Write(MMC3316_CTRL_0);  //写入指令
  I2C_Write(ACTION_PRE); 			//写入值

  delay_ms(50);

  //	I2C_Start();	//启动I2C
  //	I2C_Write(MMC3316_ADDRW);  	//写地址
  //	I2C_Write(MMC3316_CTRL_0);  //写入指令
  I2C_Write(ACTION_SET); 			//写入值
  //	I2C_Start();	//启动I2C
  //	I2C_Write(MMC3316_ADDRW);  	//写地址
  //	I2C_Write(MMC3316_CTRL_0);  //写入指令
  I2C_Write(ACTION_STOP); 		//写入值

  I2C_Stop();		//停I2C
}

/*================================================================
  【名 称】void MMC3316_Reset(void)
  【功 能】Reset
  【备 注】
  ================================================================*/
void MMC3316_Reset(int devid)
{
  cur_dev = devid;

  MMC3316_I2C_Init();		//I2C初始化
  I2C_Start();	//启动I2C

  I2C_Write(MMC3316_ADDRW);  	//写地址
  I2C_Write(MMC3316_CTRL_0);  //写入指令
  I2C_Write(ACTION_PRE); 			//写入值

  delay_ms(50);

  I2C_Write(ACTION_SET); 			//写入值
  I2C_Write(ACTION_STOP); 		//写入值

  delay_ms(50);

  I2C_Write(ACTION_RESET); 		//写入值
  I2C_Write(ACTION_STOP); 		//写入值

  I2C_Stop();		//停I2C

  delay_ms(1);
}

/*================================================================
  【名 称】void MMC3316_Check(void)
  【功 能】Check 检测能否通信
  【备 注】return true is succeed
  ================================================================*/
unsigned char MMC3316_Check(int devid)
{
  cur_dev = devid;

  MMC3316_I2C_Init();		//I2C初始化
  I2C_Start();	//启动I2C

  //The SET preparation action
  if(I2C_Write(MMC3316_ADDRW) == NACK)			//写地址
    return FALSE;
  if(I2C_Write(MMC3316_CTRL_0) == NACK)			//写入指令
    return FALSE;
  if(I2C_Write(ACTION_PRE) == NACK)					//写入值
    return FALSE;
  delay_ms(50);

  I2C_Start();	//启动I2C
  if(I2C_Write(MMC3316_ADDRW) == NACK)			//写地址
    return FALSE;
  if(I2C_Write(MMC3316_PID_1) == NACK)			//写入指令
    return FALSE;

  I2C_Start();	//启动I2C
  if(I2C_Write(MMC3316_ADDRR) == NACK)			//读地址
    return FALSE;
  if(I2C_Read(NACK) != 0x05)								//Product ID 1: is 0x05
    return FALSE;

  I2C_Stop();		//停I2C

  return TRUE;
}

/*================================================================
  【名 称】unsigned char MMC3316_Read(void)
  【功 能】MEASUREMENT
  【备 注】数据存在MMC3316_Struct中,return true is succeed
  ================================================================*/
unsigned char MMC3316_Read(int devid)
{
  uint32_t User_Timer = 500000;

  cur_dev = devid;

  MMC3316_I2C_Init();		//I2C初始化
  I2C_Start();	//启动I2C
  I2C_Write(MMC3316_ADDRW);  	//写地址
  I2C_Write(MMC3316_CTRL_0);  //写入指令
  I2C_Write(ACTION_PRE); 			//写入值
  I2C_Stop();		//停I2C

  //delay_ms(50);

  I2C_Start();	//启动I2C
  I2C_Write(MMC3316_ADDRW);  	//写地址
  I2C_Write(MMC3316_Status);  //写入指令

  I2C_Start();	//启动I2C
  I2C_Write(MMC3316_ADDRR);  	//读地址
  //Continuously read the Status Register until the Meas Done bit is set to 1
  while(((I2C_Read(NACK) & ACTION_READ) != ACTION_READ)&&(User_Timer--));
  if(User_Timer == 0)					//超时
    return FALSE;

  I2C_Start();	//启动I2C
  I2C_Write(MMC3316_ADDRW);  	//写地址
  I2C_Write(MMC3316_Xout_Low);//写入指令

  I2C_Start();	//启动I2C
  I2C_Write(MMC3316_ADDRR);  	//读地址

  One_Sample.x = I2C_Read(ACK);   //读取
  One_Sample.x += ((unsigned int)I2C_Read(ACK)<<8);   //读取

  One_Sample.y = I2C_Read(ACK);   //读取
  One_Sample.y += ((unsigned int)I2C_Read(ACK)<<8);   //读取

  One_Sample.z = I2C_Read(ACK);   //读取
  One_Sample.z += ((unsigned int)I2C_Read(NACK)<<8);   //读取	//ACK???
  
  
//  MMC3316[cur_dev].x = I2C_Read(ACK);   //读取
//  MMC3316[cur_dev].x += ((unsigned int)I2C_Read(ACK)<<8);   //读取
//
//  MMC3316[cur_dev].y = I2C_Read(ACK);   //读取
//  MMC3316[cur_dev].y += ((unsigned int)I2C_Read(ACK)<<8);   //读取
//
//  MMC3316[cur_dev].z = I2C_Read(ACK);   //读取
//  MMC3316[cur_dev].z += ((unsigned int)I2C_Read(NACK)<<8);   //读取	//ACK???

  I2C_Stop();		//停I2C

  return TRUE;
}

