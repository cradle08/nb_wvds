#include "contiki.h"
#include "platform-conf.h"
#include "app.h"
#include "uart1.h"
#include "app.h"
#include "vehicleDetection.h"
#include "qmc5883.h"

struct Sample_Struct One_Sample; // zyx mag data
struct ALGO algo;  // algorithm parameters


#define RECV_MSG_BUF_LEN 512
#define SEND_MSG_BUF_LEN 256

///////
PROCESS_NAME(NB_Device);
PROCESS(NB_Device, "NBDEV");
AUTOSTART_PROCESSES(&NB_Device);



//struct Sample_Struct One_Sample; // zyx mag data
//struct ALGO algo;  // algorithm parameters
static struct MSG msg; // msg struct
static process_event_t xyz_ready_event; // read mag data event
static process_event_t recv_msg_event; // recv a msg form nb
static struct ctimer hb_ct;  // heart beat ctimer
static struct ctimer evt_ct; // park event ctimer
static struct ctimer mag_ct; // mag change ctimer
static struct etimer sa_et;  // sample timer
static uint8_t park_s = 3;   // parking status
static uint16_t sample_period = 0; // sample period 

struct ringbuf RecvRingBuf;
static uint16_t send_index = 0; // send msg context index
static uint16_t recv_index = 0; // recv msg context index
int8_t sendmsgbuf[SEND_MSG_BUF_LEN] = {0}; //  send msg buffer 
int8_t recvmsgbuf[RECV_MSG_BUF_LEN] = {0}; // recv msg buffer
int8_t rt_index = 0; // recv temp buffer index
#define RT_BUF_LEN 2 // recv temp buffer len
int8_t rtbuf[RT_BUF_LEN] = {0}; // recv temp buffer
uint8_t recv_status = 0;


int8_t OctCharToHex[10] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09}; // 0=48,9=57
int8_t BigCharToHex[6] = {0x0A, 0x0B, 0x0C, 0x0D, 0x0F}; //A=65,F=70
int8_t HexToOctChar[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'}; 
int8_t HexToBigChar[6] ={'A', 'B', 'C', 'D', 'E', 'F'};

//** app init function
void app_init()
{
  // init variable
  xyz_ready_event = process_alloc_event();
  recv_msg_event = process_alloc_event();

  // read *** form flash 
  
  // set read xyz magdata callback
  qmc5883_set_callback(app_get_magdata);
  // ready fault handle
  // init recv buf ...
  recv_init();
  uart1_set_input(uart1_recv_callback); // set uart1 recv callback func when recv byte form nb module
 
}

//** app init function
void app_get_magdata(unsigned char *data, unsigned char *temp)
{
  One_Sample.x=(int16_t)((data[0]<<8) + data[1]);
  One_Sample.y=(int16_t)((data[2]<<8) + data[3]);
  One_Sample.z=(int16_t)((data[4]<<8) + data[5]);
  //temperature= ((temp[0]<<8)+temp[1])/128+25;
  process_post(&NB_Device, xyz_ready_event, NULL);
}

//** recv init operation
void recv_init()
{
    // init recv ring buf
    ringbuf_init(&RecvRingBuf, recvmsgbuf, RECV_MSG_BUF_LEN);
    memset(recvmsgbuf, 0, RECV_MSG_BUF_LEN);
    memset(rtbuf, 0, RT_BUF_LEN);
    recv_index = 0;
    rt_index = 0;
    recv_status = RECV_STARTFLAG;
}

//** change char to hex and add to recv buffer 
int8_t add2recvbuf()
{
  int8_t ch, low, hig, i = 0;
  for(i = 0; i < RT_BUF_LEN; i++)
  {
    if(i == 0)
    {
      if('0' <= rtbuf[i] <= '9')
      {
        hig = OctCharToHex[rtbuf[i] - '0'];
      }else if('A' <= rtbuf[i] <= 'F') {
        hig = BigCharToHex[rtbuf[i] - 'A'];
      } else{
        //err: messy code
      }
    }else {
      if('0' <= rtbuf[i] <= '9')
      {
        low = OctCharToHex[rtbuf[i] - '0'];
      }else if('A' <= rtbuf[i] <= 'F') {
        low = BigCharToHex[rtbuf[i] - 'A'];
      }else{
        //err: messy code
      }
    }
  }
  ch = ((hig << 8) | low);
  return ringbuf_put(&RecvRingBuf, ch);
}


//** uart1 rxd interrupt handle callback function
int uart1_recv_callback(int8_t c)
{
/*
  rtbuf[rt_index++] = c;
  //reset rt_index(recv temp index) everytime after change char to hex
  if(rt_index >= 2) 
  {
    add2recvbuf();
    rt_index = 0; 
  }
}
// parse recv msg form
uint8_t parse_recv_msg()
{*/
  // change recv status at last three byte
  if(msg.msg_head.msglen == recv_index) 
  {
    recv_status == RECV_CRC;
  } else if(msg.msg_head.msglen + 2 == recv_index){
  {
    recv_status == RECV_ENDFLAG;
  }
  
  // recv msg and add to recvmsgbuf
  if(recv_status == RECV_PLAYDATA){ // recv play data, most cast at this status
    rtbuf[rt_index++] = c;
    if(rt_index == RT_BUF_LEN)
    {
      add2recvbuf();
      recv_status = RECV_PLAYDATA;
    }
  } else if(recv_status == RECV_STARTFLAG){ // start to recv start flag
    if(c == 'A') 
    {
      rtbuf[rt_index++] = 'A';
      if(rt_index == RT_BUF_LEN)
      {
        recv_status = RECV_LEN;
      }
    }else{
      // start flag error: init
      recv_init();
    }
  } else if(recv_status == RECV_LEN){ // start to recv msg len
    rtbuf[rt_index++] = c;
    if(rt_index == RT_BUF_LEN) // 2 recv right
    {
     msg.msg_head.msglen = (uint8_t)add2recvbuf();
      recv_status = RECV_PLAYDATA;
    }
  } else if(recv_status == RECV_CRC){ // start to recv crc
    rtbuf[rt_index++] = c;
    if(rt_index == RT_BUF_LEN) // 2
    {
      add2recvbuf();
      recv_status = RECV_CRC;
    }
  } else if(recv_status == RECV_ENDFLAG){
    if(c == 'F')
    {
      rt_index++;
      if(rt_index == RT_BUF_LEN) // 2
      {
        // recv msg over and right, post a event 
        //precess_post(&NB_Device, recv_msg_event, NULL);
      }
    } else{
      // the endflag error ,init and drop this msg
      recv_init();
    }
  }
  
  //reset rt_index(recv temp index) everytime after change char to hex
  if(rt_index >= 2) rt_index = 0; 
  }
}

//** app send parking msg
void app_send_parking_msg() 
{
;
}

//** app send leaving msg
//void app_send_leaving_msg
//{
//
//}

//** app send strong mag msg
void app_send_strongmag_msg()
{
  ;
}



//** app send msg func
void app_send_msg(uint8_t status)
{
  if(status == PARKING){ // send parking msg
    app_send_parking_msg();
  }else if(status == LEAVING){ // send leaving msg
    app_send_leaving_msg();
  }else if(status == STRONG_MAG){ // send strong mag msg
    app_send_strongmag_msg();
  }else if(status == INIT_MAG){ // device at init status
    //
  }
}

//**
PROCESS_THREAD(NB_Device , ev, data)
{
  PROCESS_BEGIN();
  etimer_set(&sa_et, CLOCK_SECOND); // at first and init sample freqency is 1s
  Variant_Init();
  app_init();
  process_poll(&NB_Device);
  
  
  while(1)
  {
    PROCESS_WAIT_EVENT();
    if(ev == PROCESS_EVENT_POLL){
      // 
    }else if(ev == PROCESS_EVENT_TIMER){ // etimer expired 
      //
      if(etimer_expired(&sa_et)) // sa_et expired
      {
        sample_period = Get_AMR_Period();
        etimer_set(&sa_et, JIFFIES_NUM(sample_period));
        qmc5883_sample_read(0);
      }      
      // ...     
    }else if(ev == xyz_ready_event){ // app had ready xyz mag data and save them at One_Sample   
      uint8_t park_status;
      dint();
      park_status  = Parking_Algorithm();
      eint();
      if(park_s != park_status)
      {
        app_send_msg(park_status);
      }
      //
    }else if(ev == recv_msg_event){ // had recv a msg form nb module
      //parse and handle this msg 
      
    }
  
  
  
  }
  PROCESS_END();
}