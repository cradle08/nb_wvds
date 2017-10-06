
#include "app.h"
//#include "main_conf.h"
#include "VehicleDetection.h"


 struct Sample_Struct One_Sample;
 struct ALGO algo; 


void app_magdata_ready(unsigned char *data, unsigned char *temp)
{
  One_Sample.x=(int16_t)((data[0]<<8) + data[1]);
  One_Sample.y=(int16_t)((data[2]<<8) + data[3]);
  One_Sample.z=(int16_t)((data[4]<<8) + data[5]);
  //temperature= ((temp[0]<<8)+temp[1])/128+25;

}






