#include "contiki.h"
#include "platform-conf.h"
#include "app.h"
#include "VehicleDetection.h"
#include "qmc5883.h"


struct Sample_Struct One_Sample;
struct ALGO algo; 
process_event_t xyz_ready_event;

// app init function
void app_init()
{
  // init variable
  xyz_ready_event = process_alloc_event();
  
  // read *** form flash 
  
  
  // set read xyz magdata callback
  qmc5883_set_callback(app_get_magdata);
  // ready fault handle
 
}

// app init function
void app_get_magdata(unsigned char *data, unsigned char *temp)
{
  One_Sample.x=(int16_t)((data[0]<<8) + data[1]);
  One_Sample.y=(int16_t)((data[2]<<8) + data[3]);
  One_Sample.z=(int16_t)((data[4]<<8) + data[5]);
  //temperature= ((temp[0]<<8)+temp[1])/128+25;
   process_post(&NB_Device, xyz_ready_event, NULL);
}






