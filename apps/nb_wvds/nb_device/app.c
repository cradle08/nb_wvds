#include "contiki.h"
#include "platform-conf.h"
#include "app.h"
#include "uart1.h"
#include "app.h"
#include "vehicleDetection.h"
#include "qmc5883.h"
#include "nb_code.h"

struct Sample_Struct One_Sample; // zyx mag data
struct ALGO algo;  // algorithm parameters
struct BASE_ARG base_arg; // base arg init
int8_t device_no[DEVICE_NO_LEN] = {0};



PROCESS(nb_device, "NBDEV");
//AUTOSTART_PROCESSES(&NB_Device);



//struct Sample_Struct One_Sample; // zyx mag data
//struct ALGO algo;  // algorithm parameters

process_event_t handle_msg_event; // handle a msg form recvlist
process_event_t magdata_ready_event; // get mag data form qmc5883
static struct ctimer hb_ct;  // heart beat ctimer
static struct ctimer alarm_ct; // alarm(device self check) ctimer
static struct etimer sa_et;  // sample timer
static uint8_t park_s = 3;   // parking status
static uint16_t sample_period = 0; // sample period 
static uint8_t parse_index = 0; //index use at parse_recv_msg
//static struct SEND_MSG sendmsg; // send msg struct
static struct RECV_MSG recvmsg; // send msg structstruct ringbuf recvringbuf;
uint8_t recv_status = 0;
struct ringbuf recvringbuf; //recv ring buffer
int8_t rt_index = 0; // recv temp buffer index
#define RT_BUF_LEN 2 // recv temp buffer len
uint8_t rtbuf[RT_BUF_LEN] = {0}; // recv temp buffer
uint8_t sendmsgbuf[SEND_MSG_BUF_LEN] = {0}; //  send msg buffer  165
uint8_t recvmsgbuf[RECV_MSG_BUF_LEN] = {0}; // recv msg buffer   512
uint8_t OctCharToHex[10] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09}; // 0=48,9=57
uint8_t BigCharToHex[6] = {0x0A, 0x0B, 0x0C, 0x0D, 0x0F}; //A=65,F=70
uint8_t HexToChar[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9','A', 'B', 'C', 'D', 'E', 'F'}; 





// for test
uint16_t seq = 0;





/* recv var init operation */
void recv_var_init()
{
    // init recv ring buf
    ringbuf_init(&recvringbuf, recvmsgbuf, RECV_MSG_BUF_LEN);
    memset(recvmsgbuf, 0, RECV_MSG_BUF_LEN);
    memb_init(&recv_msg_mem);
    list_init(recv_msg_list);
    memset(rtbuf, 0, RT_BUF_LEN);
    rt_index = 0;
    recv_status = MSG_STARTFLAG;
}

/* change char to hex and add to recv buffer */
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
  return ringbuf_put(&recvringbuf, ch);
}

/* send var init  */
void send_var_init()
{
  memset(sendmsgbuf, 0, SEND_MSG_BUF_LEN);
  memset(recvmsgbuf, 0, RECV_MSG_BUF_LEN);
  memb_init(&send_msg_mem);
  list_init(send_msg_list);
}




/* send msg via bc95 */
void app_send_msg(struct SEND_MSG* p){
  struct SEND_MSG* ptemp = p;
  uint8_t playdata_len = ptemp->msg_head.msglen - 7; // 7 =device no len and cmd len
  uint16_t index = 0;
  uint16_t i = 0;
  sendmsgbuf[index++] = 'A';
  sendmsgbuf[index++] = 'A';
  sendmsgbuf[index++] = HexToChar[ptemp->msg_head.msglen & 0x0F];
  sendmsgbuf[index++] = HexToChar[(ptemp->msg_head.msglen >> 8)& 0x0F];
  for(i = 0; i < playdata_len; i++)
  {
    sendmsgbuf[index++] = HexToChar[ptemp->playdata[i] & 0x0F];
    sendmsgbuf[index++] = HexToChar[(ptemp->playdata[i] >> 4)& 0x0F];
  }
  for(i= 0; i < 2; i++)
  {
    sendmsgbuf[index++] = HexToChar[ptemp->msg_tail.crc[i] & 0x0F];
    sendmsgbuf[index++] = HexToChar[(ptemp->msg_tail.crc[i] >> 4) & 0x0F];
  }
  sendmsgbuf[index++] = 'F';
  sendmsgbuf[index++] = 'F';
  
  dint();
  // watchdog_periodic()
  //... bc95send(sendmsgbuf, playdata_len + 5);
  // watchdog_periodic()
  eint();
  memset(sendmsgbuf, 0, SEND_MSG_BUF_LEN);
}


/* app init function */
void app_get_magdata(unsigned char *data, unsigned char *temp)
{
  One_Sample.x=(int16_t)((data[0]<<8) + data[1]);
  One_Sample.y=(int16_t)((data[2]<<8) + data[3]);
  One_Sample.z=(int16_t)((data[4]<<8) + data[5]);
  //temperature= ((temp[0]<<8)+temp[1])/128+25;
  process_post(&nb_device, magdata_ready_event, NULL);
}


/* app init function */
void app_init()
{
  Variant_Init(); // algo var init
  // read *** form flash 
  recv_var_init(); // recv buf init
  send_var_init(); // send buf init
  base_arg_init(); // 
  magdata_ready_event = process_alloc_event(); // init sample event number
  handle_msg_event = process_alloc_event(); // init handle msg event number

  qmc5883_set_callback(app_get_magdata); // set read xyz magdata callback
  uart1_set_input(uart1_recv_callback); // set uart1 recv callback func when recv byte form nb module
}

/*   */
void base_arg_init()
{
//.... like nb_signal, battary, ...
// need create a new struct
   base_arg.hb_period          = 1800; //1800s
   base_arg.health_period      = 3600; //3600s
   base_arg.batvoltage_thresh  = 30; //3.0v
   base_arg.batquantity_thresh = 20; //20%
}

/* uart1 rxd interrupt handle callback function */
uint16_t uart1_recv_callback(uint8_t c)
{
  rtbuf[rt_index++] = c;
  //reset rt_index(recv temp index) everytime after change char to hex
  if(rt_index >= 2) 
  {
    add2recvbuf();
    rt_index = 0; 
  }
  return 0;
}

/*  */
void create_hb_msg() // create heart beat  msg
{
  struct SEND_MSG* psm = memb_alloc(&send_msg_mem);
  if(psm == NULL){
    //fuill
  } else{    
    psm->msg_head.startflag   = 0xAA;
    psm->msg_head.cmd         = CMD_HB;
    struct msg_hb* ptemp      = (struct msg_hb*) psm->playdata;
//    ptemp->tstamp       = 
    ptemp->status       = 11;
    ptemp->nb_signal    = 11;
    ptemp->temperature  = 11;
//    ptemp->mag          = 11;
    ptemp->bat_voltage  = 11;
    ptemp->bat_quantity = 11;
//    psm->msg_tail.crc     = 11;
    psm->msg_tail.endflag     = 0xFF;
  }   
  if(list_length(send_msg_list) < SEND_MSGMEM_NUM)
  {
    list_push(send_msg_list, psm);
    process_poll(&sendmsg_process);
  }
}

/* create alarm msg */
void create_alarm_msg()
{
  struct SEND_MSG* psm = (struct SEND_MSG*) memb_alloc(&send_msg_mem);
  if(psm == NULL){
    //fuill
  } else{  
    psm->msg_head.startflag   = 0xAA;
    psm->msg_head.cmd         = CMD_ALARM;
    struct msg_alarm* ptemp   = (struct msg_alarm*) psm->playdata;
    ptemp->nb_signal          = 11;
    ptemp->mag_sersor         = 11;
    ptemp->flash              = 11;
    ptemp->rtc                = 11;
    ptemp->battery            = 11;
//    psm->msg_tail.crc         = 11;
    psm->msg_tail.endflag = 0xFF;
  }
  
  if(list_length(send_msg_list) < SEND_MSGMEM_NUM)
  {
    list_push(send_msg_list, psm);
    process_poll(&sendmsg_process);
  }
}
  
/* create chcek(park) msg and add to send msg list */
void create_check_msg(uint8_t status)
{
  struct SEND_MSG* psm = memb_alloc(&send_msg_mem);
  if(psm == NULL){
    //fuill
  } else{  
    psm->msg_head.startflag   = 0xAA;
    psm->msg_head.cmd         = CMD_CHECK;
    struct msg_check* ptemp = (struct msg_check*) psm->playdata;
    ptemp->bat_quantity = 11;
    ptemp->bat_voltage  = 11;
//    ptemp->mag          = 11;
    ptemp->nb_signal    = 11;
    ptemp->status       = 11;
    ptemp->temperature  = 11;
//    ptemp->tstamp       = 11;
//    psm->msg_tail.crc     = 11;
    psm->msg_tail.endflag   = 0xFF;
  }
  if(list_length(send_msg_list) < SEND_MSGMEM_NUM)
  {
    list_push(send_msg_list, psm);
    process_poll(&sendmsg_process);
  }  
}


/* create mag change msg and add to send msg list*/
void create_mag_change_msg()
{
  struct SEND_MSG* psm = (struct SEND_MSG*)memb_alloc(&send_msg_mem);
  if(psm == NULL){
    //fuill
  } else{  
    psm->msg_head.startflag   = 0xAA;
    psm->msg_head.cmd         = CMD_MAG_CHANGE;
    struct msg_mag_change* ptemp = (struct msg_mag_change*) psm->playdata;
 //   ptemp->tstamp       = 11;
 //   ptemp->hillvalleys  = 11;
 //   ptemp->baseline     = 11;
    ptemp->judge_branch = 11;
    ptemp->status       = 11;
 //   psm->msg_tail.crc   = 11;
    psm->msg_tail.endflag   = 0xFF;
  }
  if(list_length(send_msg_list) < SEND_MSGMEM_NUM)
  {
    list_push(send_msg_list, psm);
    process_poll(&sendmsg_process);
  }    
}

/* add to recv msg list */
void into_recv_msglist()
{
  struct RECV_MSG* intomsg = memb_alloc(&send_msg_mem);
  if(intomsg == NULL)
  {
    // recv msg mem is full, send logger msg and drop this msg
    // ...
  } else {
    memmove(intomsg, &recvmsg, sizeof(struct RECV_MSG));
    list_add(recv_msg_list, intomsg);
    memset(&recvmsg, 0, sizeof(struct RECV_MSG));
    process_post(&nb_device, handle_msg_event, NULL);
  }
}


/* parse recv msg form */
int8_t parse_recv_msg()
{
  uint8_t rc;
  rc = ringbuf_get(&recvringbuf);  
  if(recv_status == MSG_STARTFLAG){ // start to recv start flag
    if(rc == 0xAA) 
    {
      recv_status = MSG_LEN;
    } else{
 //     continue; // pass this vailed field
    }
  }else if(recv_status == MSG_PLAYDATA) {

    //...   recvmsg.playdata[parse_index++] = rc;
    if( parse_index >= recvmsg.msg_head.msglen - 7) // 7(device no and cmd)
    {
      recv_status = MSG_CRC;
      parse_index = 0;
    }
  
  }else if(recv_status == MSG_DEVICE_NO){ // recv play data, most cast at this status
    recvmsg.msg_head.devno[parse_index++] = rc; 
    if(parse_index >= 6)
    {
      // check the device no , if it is right, or drop this msg and alarm
      // ...
      parse_index = 0;
      recv_status = MSG_CMD;
    }
  }else if(recv_status == MSG_CMD) {
    
    recvmsg.msg_head.cmd = rc;
    if(recvmsg.msg_head.cmd == CMD_UPDATA) // if it is updata, save packet to flash directly, don't need add to list
    {
      // ...
    }
    recv_status = MSG_PLAYDATA;  
    parse_index++;
    
  } else if(recv_status == MSG_LEN){ // start to recv msg len
    
    recvmsg.msg_head.msglen = (uint8_t)rc;
    recv_status = MSG_DEVICE_NO;    
     
  } else if(recv_status == MSG_CRC){ // start to recv crc
    recvmsg.msg_tail.crc[parse_index++] = rc;
    if(parse_index >= 2)
    {
      recv_status = MSG_ENDFLAG;
      parse_index = 0;
    }
  } else if(recv_status == MSG_ENDFLAG){
    if(rc == 0xFF)
    {
      into_recv_msglist();// after parse msg ,after then send to list, and handle it later
      recv_status = MSG_STARTFLAG;
      parse_index = 0;
      memset(&recvmsg, 0, sizeof(struct RECV_MSG));
      return 0; // parse msg success     
    } else { // recv msg fail ,drop this msg
       memset(&recvmsg, 0, sizeof(recvmsg));
       recv_status = MSG_STARTFLAG;
       parse_index = 0;
       return 2; // parse msg fail, msg is not full
    }
  }
  return 1; // parsing
}

/*  */
void app_test_send_msg()
{
  uint8_t buf[40] = {0};
  if(seq >= 9999) seq = 1;
  sprintf(buf, "No=%d: x=%d,y=%d,z=%d,ps=%d", seq++, One_Sample.x, One_Sample.y, One_Sample.z, park_s);
  uart1_send(buf, 40);
 // uart1_send("12345", 5);
 // ctimer_restart(&mytime);
}




/* nb device main process  */
PROCESS_THREAD(nb_device , ev, data)
{
  PROCESS_BEGIN();
  etimer_set(&sa_et, CLOCK_SECOND); // at first and init sample freqency is 1s
  Variant_Init();
  app_init();
  process_poll(&nb_device);
  
  
  while(1)
  {
    PROCESS_WAIT_EVENT();
    if(ev == PROCESS_EVENT_POLL){ // handle parse and save msg to recvlist
      // 
      int8_t value;
      parse_index = 0;
      while(!ringbuf_is_empty(&recvringbuf))
      {
        value = parse_recv_msg();
        if( value == 0 || value == 2) // 0:when recv a msg success, go to next step, 2:fail, go on next step
          break; 
      }      
    }else if(ev == PROCESS_EVENT_TIMER){ // etimer expired 
      //
      if (etimer_expired(&sa_et)) { // sa_et expired    
        sample_period = Get_AMR_Period();
        etimer_set(&sa_et, MS2JIFFIES(sample_period));
        qmc5883_sample_read(0);
      }      
    } else if(ev == magdata_ready_event) { // app had ready xyz mag data and save them at One_Sample   
      uint8_t park_status;
      dint();
      park_status  = Parking_Algorithm();
      eint();
     // app_test_send_msg();
      if(park_s != park_status)
      {
        create_check_msg(park_status);
        park_s = park_status;
        create_mag_change_msg();
        //uart_send(); // for test
      }
      //
    } else if(ev == handle_msg_event) { // handle msg at recv_msg_list
      if(0 < list_length(recv_msg_list))
      {
        struct RECV_MSG* ptemp;
        ptemp = list_pop(recv_msg_list); // first msg at recv_msg_list
        handle_recv_msg(ptemp);
        memb_free(&recv_msg_mem, ptemp); // free memb
        // ...
      }
    }
    
 
    //....    
    
      
  }
  
  PROCESS_END();
}