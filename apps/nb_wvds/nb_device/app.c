#include "contiki.h"
#include "platform-conf.h"
#include "app.h"
#include "uart1.h"
#include "app.h"
#include "vehicleDetection.h"
#include "qmc5883.h"

PROCESS(NB_Device, "NBDEV");
AUTOSTART_PROCESSES(&NB_Device);



struct Sample_Struct One_Sample; // zyx mag data
struct ALGO algo;  // algorithm parameters
static process_event_t xyz_ready_event; // read mag data event
static struct ctimer hb_ct;  // heart beat ctimer
static struct ctimer evt_ct; // park event ctimer
static struct ctimer mag_ct; // mag change ctimer
static struct etimer sa_et;  // sample timer
static uint8_t park_s = 3;   // parking status
static uint16_t sample_period = 0; // sample period 


//** app init function
void app_init()
{
  // init variable
  xyz_ready_event = process_alloc_event();
  
  // read *** form flash 
  
  // set read xyz magdata callback
  qmc5883_set_callback(app_get_magdata);
  // ready fault handle
 
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

//**
void app_send_msg()
{
  static uint32_t seq;
  uint8_t buf[40] = {0};
//  if(seq >= 9999) seq = 1;
  sprintf(buf, "No=%d: x=%d,y=%d,z=%d,ps=%d", seq++, One_Sample.x, One_Sample.y, One_Sample.z, park_s);
  uart1_Tx(40, buf);
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
    if(ev == PROCESS_EVENT_POLL)
    {
      
    
    } 
    else if(ev == PROCESS_EVENT_TIMER) // etimer expired 
    {
      //
      if(etimer_expired(&sa_et)) // sa_et expired
      {
        sample_period = Get_AMR_Period();
        etimer_set(&sa_et, JIFFIES_NUM(sample_period));
        qmc5883_sample_read(0);
      }      
      // ...
      
    }
    else if(ev == xyz_ready_event) // app had ready xyz mag data and save them at One_Sample
    {
      dint();
      park_s = Parking_Algorithm();
      eint();
      //
    }
  
  
  
  }
  PROCESS_END();
}



