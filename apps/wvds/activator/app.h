#ifndef _APP_H
#define _APP_H

#define DEVMAC_LEN  6         // 设备编码字节数
#define MAC_LEN     8         // MAC地址字节数
#define TSTAMP_LEN  6         // 时间戳字节数

#define APP_WVDS  0x01

#define APP_NETCMD_CHANNEL   0x87
#define APP_UNICAST_CHANNEL  0x89

enum {
  NODE_VD = 0x01,
  NODE_RP = 0x02,
  NODE_AP = 0x04
};

// 子操作枚举值
enum {
  APP_GET = 1,              // 查询
  APP_SET = 2,              // 配置
  APP_ADD = 3,              // 增加
  APP_DEL = 4               // 删除
};

// 错误代码枚举值
enum {
  E_FAIL  = 0x01,
  E_NULL  = 0x02,
  E_BUSY  = 0x03,
  E_INVAL = 0x04,
  E_STATE = 0x05,
  E_SIZE  = 0x06,
  E_CRCX  = 0x07,
  E_EMPTY = 0x08,
  E_DROP  = 0x09,
  E_FULL  = 0x0A
};
/*------------------------------------------------------------------*/
/*  应用层消息定义                                                  */
/*------------------------------------------------------------------*/
#define APPMSG_BEG 0xAA
#define APPMSG_END 0xFF

#define APPMSG_UP   0
#define APPMSG_DOWN 1

#define APPMSG_HEADER_LEN 9
struct app_msg_header {
  uint8_t beg;
  uint8_t len;
  uint8_t devmac[DEVMAC_LEN];
  uint8_t cmdop:6;
  uint8_t rep:1;
  uint8_t dir:1;
};

#define APPMSG_FOOTER_LEN 3
struct app_msg_footer {
  uint8_t crc[2];
  uint8_t end;
};

#define APP_DATA_MAXLEN  48
#define APPMSG_MAXLEN    (APPMSG_HEADER_LEN + APP_DATA_MAXLEN + APPMSG_FOOTER_LEN)
struct app_msg {
  struct app_msg_header header;
  uint8_t data[APP_DATA_MAXLEN];
  struct app_msg_footer footer;
};

/*------------------------------------------------------------------*/
/*  应用层消息有效载荷定义                                          */
/*------------------------------------------------------------------*/
enum {
  APPDATA_PARK_EVT    = 0x01, /* 泊位事件消息 */
  APPDATA_VD_HBEAT    = 0x02, /* VD心跳消息 */
  APPDATA_RP_HBEAT    = 0x03, /* RP心跳消息 */
  APPDATA_AP_HBEAT    = 0x04, /* AP心跳消息 */
  APPDATA_SYNC_TIME   = 0x05, /* 时间同步消息 */
  /* Between AP and FES */
  APPDATA_AP_CONN     = 0x11, /* AP入网消息 */
  APPDATA_NODETABLE   = 0x12, /* 节点列表管理消息 */
  APPDATA_BLACKLIST   = 0x13, /* 黑名单管理消息 */
  APPDATA_GPRS_PARAM  = 0x14, /* GPRS参数查询配置消息 */
  APPDATA_RADIO_PARAM = 0x15, /* 射频参数查询配置消息 */
  APPDATA_ONOFFLINE   = 0x16, /* 节点上线掉线报告消息 */
  APPDATA_LOCKORUN    = 0x17, /* 子网锁定解锁消息 */
  APPDATA_OTA_DATA    = 0x18, /* 无线升级数据消息 */
  APPDATA_OTA_EXEC    = 0x19, /* 无线升级执行消息 */
  /* Between VD/RP/AP and FES */
  APPDATA_VD_CONN     = 0x21, /* VD/RP入网消息 */
  APPDATA_ALARM       = 0x22, /* 故障报警消息 */
  APPDATA_HIST_DATA   = 0x23, /* 历史数据消息 */
  APPDATA_MAG_DATA    = 0x24, /* 磁场波动数据消息 */
  APPDATA_BASIC_PARAM = 0x25, /* 全网参数设置消息 */
  APPDATA_ALGO_PARAM  = 0x26, /* 算法参数设置消息 */
  APPDATA_RESET       = 0x31, /* 重启节点消息 */
  APPDATA_FACT_RESET  = 0x32, /* 恢复出厂设置消息 */
  APPDATA_VD_ACTIVATE = 0x33, /* VD激活消息 */
  APPDATA_VD_FAECONN  = 0x34, /* VD现场连接消息 */
  APPDATA_VD_DISCONN  = 0x35, /* VD现场断开连接消息 */
  APPDATA_VD_REINIT   = 0x36, /* VD重新标定消息 */
};

// APPDATA_ALGO_PARAM: 0x26
struct app_data_algo {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t subop;
  uint8_t normalT[2];
  uint8_t flunctT[2];
  uint8_t bigOccThr;
  uint8_t midOccThr;
  uint8_t litOccThr;
  uint8_t unOccThr;
  uint8_t baseLineX[2];
  uint8_t baseLineY[2];
  uint8_t baseLineZ[2];
  uint8_t sensorGain;
};

struct app_data_algo_resp {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t subop;
  uint8_t result;
  uint8_t normalT[2];
  uint8_t flunctT[2];
  uint8_t bigOccThr;
  uint8_t midOccThr;
  uint8_t litOccThr;
  uint8_t unOccThr;
  uint8_t baseLineX[2];
  uint8_t baseLineY[2];
  uint8_t baseLineZ[2];
  uint8_t sensorGain;
};

// APPDATA_VD_ACTIVATE: 0x33
struct app_data_vd_activate {
  uint8_t tstamp[TSTAMP_LEN];
   int8_t radio;
  uint8_t sensor;
  uint8_t flash;
  uint8_t batV;
  uint8_t batQ;
};

struct app_data_vd_activate_resp {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t result;
};

// APPDATA_VD_FAECONN: 0x34
struct app_data_vd_faeconn {
  uint8_t tstamp[TSTAMP_LEN];
   int8_t radio;
  uint8_t sensor;
  uint8_t flash;
  uint8_t batV;
  uint8_t batQ;
};

struct app_data_vd_faeconn_resp {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t result;
};

// APPDATA_VD_DISCONN: 0x35
struct app_data_vd_disconn {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t subop;
  uint8_t channel;
};

struct app_data_vd_disconn_resp {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t result;
};

// APPDATA_VD_REINIT: 0x36
struct app_data_reinit {
  uint8_t tstamp[TSTAMP_LEN];
};

struct app_data_reinit_resp {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t result;
  uint8_t normalT[2];
  uint8_t flunctT[2];
  uint8_t bigOccThr;
  uint8_t midOccThr;
  uint8_t litOccThr;
  uint8_t unOccThr;
  uint8_t baseLineX[2];
  uint8_t baseLineY[2];
  uint8_t baseLineZ[2];
  uint8_t sensorGain;
};

/*------------------------------------------------------------------*/
/*  网络层消息定义                                                  */
/*------------------------------------------------------------------*/
struct netcmd_req {
  uint8_t  type;
  uint8_t  mac[MAC_LEN];
  uint8_t  fwver;
  uint8_t  data[APPMSG_MAXLEN];
};

struct netcmd_cmd {
  uint8_t  type;
  uint8_t  opt:7;
  uint8_t  more:1;
  uint16_t seqno;
  rimeaddr_t dest;
  uint8_t  subid;
  uint8_t  len;
  uint8_t  data[APPMSG_MAXLEN];
};

#endif
