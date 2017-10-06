#ifndef _APP_H
#define _APP_H

//#include "main_conf.h"
#include "system.h"

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

void app_magdata_ready(unsigned char *data, unsigned char *temp);




#endif /* _APP_H */


