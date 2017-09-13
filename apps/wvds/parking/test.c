#include "contiki.h"
#include "mmc3316.h"
#include "hmc5983.h"
#include "qmc5883.h"
#include "net/rime/rimeaddr.h"
#include "net/rime/broadcast.h"
#include "at-cmd.h"
#include "dev/radio.h"
#include "dev/uart.h"
#include "dev/watchdog.h"
#include <stdio.h>
#include <string.h>

#ifndef APP_VER_MAJOR
#define APP_VER_MAJOR 1
#endif
#ifndef APP_VER_MINOR
#define APP_VER_MINOR 0
#endif
#ifndef APP_VER_PATCH
#define APP_VER_PATCH 0
#endif
/*------------------------------------------------------------------*/
#define FUSEXYZ 0

#if FUSEXYZ == 1
#include "xyzfuedetection.h"
#else
#include "VehicleDetection.h"
#endif

#if (WITH_MMC3316 + WITH_HMC5983 + WITH_QMC5883) != 1
#error "only one of WITH_MMC3316/HMC5983/QMC5883 can be defined to be 1"
#endif

static uint16_t seqno = 0;
static uint8_t status=0;
uint16_t sample_count=0;
uint8_t enter_leave_branch=0;

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
uint8_t gain_hmc5983 = 4;
uint8_t hist_gain=0;
uint16_t digital_resolution = 100; //外部变量，与gain_hmc5983相关，默认值为100
uint8_t the_node_id = 0x06;
bool reset_flag=false;

typedef struct {
  uint8_t nodeid;
  uint8_t status;
  uint16_t seqno;
  int16_t axis_x;
  int16_t axis_y;
  int16_t axis_z;
  int16_t temp; //temperature
} vehicle_sample_packet;

int16_t temperature;
vehicle_sample_packet ta_packet;

//static clock_time_t
int period = PARKINGAPP_SEND_PERIODIC_MSG_TIMEOUT;

static struct broadcast_conn broadcast;
/*--------------------------------------------------------*/
PROCESS(test_process, "Sensor Test");
AUTOSTART_PROCESSES(&test_process);

#if UNIT_MILLI_GAUSS==1
/*********************************************************************
* @fn      Float_Compute
*
* @brief   为了计算时不产生溢出，采用按位计算
*          x1--磁采样值
*          x2--数字分辨率
* @param   none
*
* @return  计算结果
*/
int16_t Float_Compute(int16_t x1, int16_t x2)
{
  int16_t t1=0, t2=0, t3=0, t4=0;
  t1 = x2/100;           //提取百位的值
  t2 = (x2-t1*100)/10;   //提取十位的值
  t3 = x2%10;            //提取个位的值
  t4 = t1*x1+t2*x1/10+t3*x1/100;
  return t4;
}

void dig_resolution_change()
{
  switch(gain_hmc5983) {
  case 0:
    digital_resolution=73; //Gain Configuration 1370
    break;
  case 1:
    digital_resolution=92;    //Gain Configuration 1090
    break;
  case 2:
    digital_resolution=122;   //Gain Configuration 820
    break;
  case 3:
    digital_resolution=152;   //Gain Configuration 660
    break;
  case 4:
    digital_resolution=227;  //Gain Configuration 440
    break;
  case 5:
    digital_resolution=256;  //Gain Configuration 440
    break;
  case 6:
    digital_resolution=303;   //Gain Configuration 330
    break;
  case 7:
    digital_resolution=435;  //Gain Configuration 230
    break;
  default:
    break;
  }
}

#endif

#if WITH_HMC5983 || WITH_QMC5883
void Sample_Record(unsigned char *buf, unsigned char *tmp)
{
  One_Sample.x=(int16_t)((buf[0]<<8) + buf[1]);
  One_Sample.y=(int16_t)((buf[2]<<8) + buf[3]);
  One_Sample.z=(int16_t)((buf[4]<<8) + buf[5]);

  temperature= ((tmp[0]<<8)+tmp[1])/128+25;//(MSB * 2^8 + LSB) / (2^4 * 8) + 25 in C

  //将采样数据转换为毫高斯
//#if UNIT_MILLI_GAUSS==1
//  One_Sample.x = Float_Compute(One_Sample.x, digital_resolution);
//  One_Sample.y = Float_Compute(One_Sample.y, digital_resolution);
//  One_Sample.z = Float_Compute(One_Sample.z, digital_resolution);
//#endif

}
#endif

/*--------------------------------------------------------*/
static void
radio_send(void)
{
#ifndef NO_RADIO
  //uint8_t i = 0;

  ta_packet.nodeid=the_node_id;
  ta_packet.seqno++;
  ta_packet.status=status;

  ta_packet.axis_x=One_Sample.x;
  ta_packet.axis_y=One_Sample.y;
  ta_packet.axis_z=One_Sample.z;

  ta_packet.temp = enter_leave_branch;//temperature;

//#if WITH_MMC3316
//#if SENSOR_PORT==0
//  ta_packet.axis_x=MMC3316[0].x;
//  ta_packet.axis_y=MMC3316[0].y;
//  ta_packet.axis_z=MMC3316[0].z;
//#else
//  ta_packet.axis_x=MMC3316[1].x;
//  ta_packet.axis_y=MMC3316[1].y;
//  ta_packet.axis_z=MMC3316[1].z;
//#endif
//#elif WITH_HMC5983
//  ta_packet.axis_x=HMC5983.x;
//  ta_packet.axis_y=HMC5983.y;
//  ta_packet.axis_z=HMC5983.z;
//#else
//#error "no support"
//#endif



  packetbuf_copyfrom(&ta_packet, sizeof(ta_packet));
  broadcast_send(&broadcast);
#endif
}

static void
app_uart_send(void)
{
  uint8_t i = 15;
  uint8_t buf[29] = {
    0x41,0x88,0x10,0xCD,0xAB,0xFF,0xFF,0x00,0x02,0x81,
    0x00,0x02,0x00,0x00,0x02,0x91,0x1F,0x0C,0x1F,0xF9,
    0x1F,0x6D,0x20,0x0C,0x1F,0xF2,0x20,0x00,0x00
  };
  buf[i++] = seqno;
#if WITH_MMC3316
  buf[i++] = MMC3316[0].x >> 8;
  buf[i++] = MMC3316[0].x & 0xff;
  buf[i++] = MMC3316[0].y >> 8;
  buf[i++] = MMC3316[0].y & 0xff;
  buf[i++] = MMC3316[0].z >> 8;
  buf[i++] = MMC3316[0].z & 0xff;
  buf[i++] = MMC3316[1].x >> 8;
  buf[i++] = MMC3316[1].x & 0xff;
  buf[i++] = MMC3316[1].y >> 8;
  buf[i++] = MMC3316[1].y & 0xff;
  buf[i++] = MMC3316[1].z >> 8;
  buf[i++] = MMC3316[1].z & 0xff;
#elif WITH_HMC5983 || WITH_QMC5883
  i += 6;
  //memcpy(buf + i, HMC5883L_Buf, 6);
  buf[i++] = (((uint16_t)One_Sample.x) >> 8);
  buf[i++] = (((uint16_t)One_Sample.x) & 0xff);
  buf[i++] = (((uint16_t)One_Sample.y) >> 8);
  buf[i++] = (((uint16_t)One_Sample.y) & 0xff);
  buf[i++] = (((uint16_t)One_Sample.z) >> 8);
  buf[i++] = (((uint16_t)One_Sample.z) & 0xff);
#else
#error "no support"
#endif
  buf[i++] = status;

  uart_send(buf, sizeof(buf));
}
/*--------------------------------------------------------*/
static void
broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *from)
{
#if NODEID == 1
  uint8_t buf[30] = {
    0x41,0x88,0x00,0xCD,0xAB,0xFF,0xFF,0x00,0x02,0x81,
    0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  };
  uint8_t len, i;

  buf[2]  = seqno++;
  buf[7]  = from->u8[1]; buf[8]  = from->u8[0];
  buf[11] = from->u8[0]; buf[12] = from->u8[1];
  memcpy(&buf[13], packetbuf_dataptr(), packetbuf_datalen());
  len = 13 + packetbuf_datalen();

  uart_send(buf, len);

#else
  uint8_t *data = (uint8_t *)packetbuf_dataptr();
  uint8_t len = packetbuf_datalen();
  uint8_t i;

  for (i = 0; i < len; i++) {
    atcmd_input(data[i]);
  }
#endif
}

static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
/*--------------------------------------------------------*/
int
reset(const char *arg, int len)
{
  watchdog_reboot();
  return 0;
}

int
setPeriod(const char *arg, int len)
{
#if NODEID == 1
  char buf[20] = "AT+setPeriod ";
  int i = 13;

  memcpy(buf + i, arg, len); i += len;
  buf[i++] = '\r';
  buf[i++] = '\n';

  packetbuf_copyfrom(buf, i);
  broadcast_send(&broadcast);

#else
  int p = 0;

  while (*arg != 0) {
    p = (p * 10) + (*arg - '0');
    ++arg;
  }
  period = p;
#endif

  return 0;
}
#if ACTIVATE_RF_BY_AMR==1
void Activate_RF()
{
}
#endif


void Re_Calibrate_Success(bool is_success)
{

}

/*--------------------------------------------------------*/
void Set_UP_HILLS(upload_wave * info)
{
}
/*--------------------------------------------------------*/
PROCESS_THREAD(test_process, ev, data)
{
  static struct etimer et;

  PROCESS_BEGIN();

#if NODEID > 1

#if WITH_MMC3316
#if SENSOR_PORT==0
  MMC3316_Port_Init(0); //端口初始化
  while(MMC3316_Check(0) == FALSE); //检测能否通信，否则卡死
  MMC3316_Reset(0); //Reset
  MMC3316_Init(0); //传感器初始化
#else
  MMC3316_Port_Init(1); //端口初始化
  while(MMC3316_Check(1) == FALSE); //检测能否通信，否则卡死
  MMC3316_Reset(1); //Reset
  MMC3316_Init(1); //传感器初始化
#endif

#elif WITH_HMC5983
  hmc5983_init();
  hmc5983_set_callback(Sample_Record);
  //dig_resolution_change();
  //hmc5983_self_test();

#elif WITH_QMC5883
  qmc5883_init();
  qmc5883_set_callback(Sample_Record);
  //dig_resolution_change();
  //hmc5983_self_test();

#else
#error "no support"
#endif

  Variant_Init();


#endif

  etimer_set(&et, PARKINGAPP_SEND_PERIODIC_MSG_TIMEOUT);

  printf("MMC3316 sensor:\n");

  radio_set_txpower(28); // 28设置射频功率
  broadcast_open(&broadcast, 129, &broadcast_call);

  atcmd_start();
  uart_set_input(atcmd_input);
  atcmd_set_output(uart_writeb);

  atcmd_register("reset", reset);
  atcmd_register("setPeriod", setPeriod);

  while (1) {
    PROCESS_WAIT_EVENT();

#if NODEID > 1
    etimer_set(&et, period);

#if WITH_MMC3316
#if SENSOR_PORT==0
    MMC3316_Read(0);
#else
    MMC3316_Read(1);
#endif

#elif WITH_HMC5983
    //test gain
    /*
    if(gain_hmc5983<7)
    {
      if(ta_packet.seqno >= (gain_hmc5983*30))
      {
        gain_hmc5983 +=1;
        dig_resolution_change();
      }
    }
    else
    {
      ta_packet.seqno=1;
      gain_hmc5983=1;
      dig_resolution_change();
    }
    */

    if(reset_flag)
    {
      hmc5983_self_test();
      reset_flag=false;
      continue;
    }
    else
    {
      hmc5983_sample_read(gain_hmc5983);
    }
    //

    //gain test
    if(hist_gain!=gain_hmc5983)
    {
      hist_gain=gain_hmc5983;
      continue;
    }

#elif WITH_QMC5883
    //test gain
    /*
    if(gain_hmc5983<7)
    {
      if(ta_packet.seqno >= (gain_hmc5983*30))
      {
        gain_hmc5983 +=1;
        dig_resolution_change();
      }
    }
    else
    {
      ta_packet.seqno=1;
      gain_hmc5983=1;
      dig_resolution_change();
    }
    */

    if(reset_flag)
    {
      qmc5883_self_test();
      reset_flag=false;
      continue;
    }
    else
    {
      qmc5883_sample_read(gain_hmc5983);
    }
    //

    //gain test
    if(hist_gain!=gain_hmc5983)
    {
      hist_gain=gain_hmc5983;
      continue;
    }


#else
#error "no support"
#endif

    status = Parking_Algorithm();

    ++seqno;

#if 0
    printf("now: %lu\n", clock_time());
    //printf("x1:%d, y1:%d, z1:%d\n", MMC3316[0].x, MMC3316[0].y, MMC3316[0].z);
    //printf("x2:%d, y2:%d, z2:%d\n", MMC3316[1].x, MMC3316[1].y, MMC3316[1].z);
#else
    app_uart_send();
#endif

    /*
    sample_count++;
    if( period==(CLOCK_SECOND/3))
    {
      if ( sample_count%3==0)
      {
        radio_send();
      }
    }
    else
    {
      radio_send();
    }*/

    radio_send();
#endif
  }

  PROCESS_END();
}

void Set_Gain_HMC5983(uint8_t gain)
{
  gain_hmc5983=gain;
}

uint16_t Get_Gain_HMC5983()
{
  return gain_hmc5983;
}

void Set_AMR_Period(int sample_period)
{
  if(period != sample_period)
  {
    //period = sample_period;
    period = (((uint32_t)sample_period * CLOCK_SECOND) / 1000);
    //process_poll(&test_process); 这个语句有问题
  }

}
