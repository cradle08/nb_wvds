#ifndef _APP_H
#define _APP_H

#include "platform-conf.h"
#include "contiki.h"

#define NV_MAGIC  0xCDAB
#define CRC_INIT  0x0000  // CRC16校验和的初始值
#define WITH_QMC5883  1


struct Sample_Struct {
  int16_t x;
  int16_t y;
  int16_t z;
} ;

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

// msg header struct
struct Msg_Head{
  int8_t startflag; // AA
  uint8_t msglen;   // messge len
};

// device number struct
typedef struct{
  uint8_t  devtype;  // dev type, 05:nb device
  uint8_t  producer; // which producer
  uint8_t  year;     // product time: year
  uint8_t  month;    //product time: month
  uint16_t sn;       // serial number
}devno_t;

// msg body struct
struct Msg_PlayData{
  uint8_t devno[6];     // device number
  uint8_t cmd;       // command id   
  int8_t  *message; //play data
};

// msg tail
struct Msg_Tail{
  uint8_t crc[2];    // crc of type to data
  int8_t  endflag;   //FF
};

// msg struct
struct MSG{
    struct Msg_Head  msg_head;
    struct Msg_PlayData msg_playdata;
    struct Msg_Tail msg_tail;
};



// enum recv stats
enum{
  RECV_STARTFLAG = 1,
  RECV_LEN,
  RECV_PLAYDATA,
  RECV_CRC,
  RECV_ENDFLAG
};

// park status
enum{
   LEAVING = 0,
   PARKING = 1,
   STRONG_MAG = 2,
   INIT_MAG = 3
};




void app_init(); // app init func
void app_get_magdata(unsigned char *data, unsigned char *temp); // get xyz mag data callback func
int uart1_recv_callback(int8_t c); // uart1 rxd interrupt handle callback function
int8_t add2recvbuf(); // add byte to recv msg  buffer
void app_send_msg(uint8_t status); // app send msg func
void app_send_parking_msg(); // app send parking msg
void app_send_leaving_msg(); // app send leaving msg
void app_send_strongmag_msg(); // app send strong mag msg

void recv_init();



#endif /* _APP_H */


