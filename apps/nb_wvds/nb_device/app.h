#ifndef __APP_H__
#define __APP_H__

#include "platform-conf.h"
#include "contiki.h"
#include "nb_code.h"

#define NV_MAGIC  0xCDAB
#define CRC_INIT  0x0000  // CRC16校验和的初始值
#define WITH_QMC5883  1


struct Sample_Struct {
  int16_t x;
  int16_t y;
  int16_t z;
};

// 检测算法参数(Algorithm Paramters) 
struct ALGO {
  uint16_t magic;
  uint16_t normalT;         // 平稳采样周期，以毫秒为单位
  uint16_t flunctT;         // 波动采样周期，以毫秒为单位
  uint8_t big_occ_thresh;   // 快速有车判决阈值
  uint8_t mid_occ_thresh;   // 平稳后有车判决阈值
  uint8_t litt_occ_thresh;  // 低磁车判决阈值
  uint8_t unocc_thresh;     // 无车判决阈值
  uint8_t axis_stable_threshold;  // xyz stable thresh ...
//  uint8_t gain_hmc5983;     // 磁传感器增益
  uint8_t status;           // 车位状态
  int16_t base_line[3];     // 背景磁场基线值
  uint16_t crc;             // ALGO的CRC16校验和
};

// device base param
struct BASE_ARG{
  uint16_t hb_period;  // heart beat period
  uint16_t health_period; // device self check period
  uint16_t batvoltage_thresh; // battery voltage threshold
  uint8_t  batquantity_thresh; // battery quantity threshold
  // ...
};

// start address to save data for app
enum {
  NV_OTA_ADDR   = 0x0000, // -0x0040: 64B
  NV_PIB_ADDR   = 0x0040, // -0x0080: 64B
  NV_NIB_ADDR   = 0x0080, // -0x0100: 128B
  NV_STATS_ADDR = 0x0100, // -0x0180: 128B
  NV_ALGO_ADDR  = 0x0180, // -0x0200: 128B
  NV_ROUTE_ADDR = 0x0200, // -0x0600: 1024B
  NV_NODES_ADDR = 0x0600, // -0x0E00: 2048B
  NV_BLIST_ADDR = 0x0E00, // -0x1000: 512B
  NV_MSGS_ADDR  = 0x1000, // -0x40000: 252kB
};






// park status
enum{
   LEAVING = 0,
   PARKING = 1,
   STRONG_MAG = 2,
   INIT_MAG = 3
};


// enum recv stats
enum{
  MSG_STARTFLAG = 1,
  MSG_LEN,
  MSG_DEVICE_NO,
  MSG_CMD,
  MSG_PLAYDATA,
  MSG_CRC,
  MSG_ENDFLAG
};


void recv_var_init(); // recv var init
void send_var_init(); // send var inti
int8_t add2recvbuf(); // add byte to recv msg  buffer
//int8_t add2sendbuf(); // add byte to send msg buffer
void base_arg_init(); // other var init
void app_init(); // app init func
void app_get_magdata(unsigned char *data, unsigned char *temp); // get xyz mag data callback func
uint16_t  uart1_recv_callback(uint8_t c); // uart1 rxd interrupt handle callback function

void create_check_msg(uint8_t status); // create check msg(0:leaving,1:parking, 2:strong magnetic,3:init)
void create_mag_change_msg(); // create mag change msg
void create_hb_msg(); // create heart beat  msg
void create_alarm_msg(); // create alarm(device self check) msg
void app_send_msg(struct SEND_MSG* p); // 


//void handle_recv_msg(); // after parse recv msg , we need to handle it
void into_recv_msglist(); // send recv msg into recv list,and handle it later
int8_t parse_recv_msg(); // parse recv msg, 0:parse a msg success, 1:parsing, 2:parse fail


    

PROCESS_NAME(nb_device);
#endif /* __APP_H__ */