#ifndef _APP_H
#define _APP_H

#include "contiki.h"
#include "net/rime/rimeaddr.h"
#include "cfs-flash.h"

#ifndef APP_COMMIT
#define APP_COMMIT "9dcf96e0"
#endif
#ifndef APP_BUILD
#define APP_BUILD "20170718"
#endif
/*------------------------------------------------------------------*/
#define WITH_MMC3316  0
#define WITH_HMC5983  0
#define WITH_QMC5883  1
#if (WITH_MMC3316 + WITH_HMC5983 + WITH_QMC5883) != 1
#error "only one of WITH_MMC3316/HMC5983/QMC5883 can be defined to be 1"
#endif

#ifndef WITH_GPRS
#define WITH_GPRS 0
#endif

#if WITH_GPRS
#ifndef GPRS_AUTH
#define GPRS_AUTH 1
#endif
#else /* not WITH_GPRS */
#define GPRS_AUTH 0
#endif

#ifndef WITH_NETWORK_LOCK
#define WITH_NETWORK_LOCK  0
#endif

#ifndef WITH_BLACKLIST
#define WITH_BLACKLIST  0
#endif

#ifndef WITH_TSTAMP
#define WITH_TSTAMP  1
#endif

#ifndef WITH_OFFLINE_MONITOR
#define WITH_OFFLINE_MONITOR  1
#endif

/*------------------------------------------------------------------*/
#define APP_MESH_CHANNEL     0x84
#define APP_NETCMD_CHANNEL   0x87
#define APP_TRICKLE_CHANNEL  0x88
#define APP_UNICAST_CHANNEL  0x89

#define APHBEATT_MIN  10  // AP心跳周期最小值，以秒为单位
#define RPHBEATT_MIN  30  // RP心跳周期最小值，以秒为单位
#define VDHBEATT_MIN  30  // VD心跳周期最小值，以秒为单位
#define HEALTHT_MIN   60  // 自检周期最小值，以秒为单位
#define COMMANDT_MIN  10  // 下行命令轮询周期最小值，以秒为单位
#define PARAMT_MIN    60  // 全网参数轮询周期最小值，以秒为单位

#define CRC_INIT  0x0000  // CRC16校验和的初始值

/*------------------------------------------------------------------*/
#define NV_MAGIC  0xCDAB

// 各数据存储段的起始地址
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

#define NV_ROUTE_FILE "route"
#define NV_ROUTE_SIZE 1024

#define NV_NODES_FILE "nodes"
#define NV_NODES_SIZE 2048

#define NV_BLIST_FILE "blacklist"
#define NV_BLIST_SIZE 512

#define NV_MSGS_FILE "messages"
#if !CONTIKI_TARGET_COOJA
#if BOARD_CADRE1120_VD
#define NV_MSGS_SIZE 30720 // 30kB
#define NV_MSGS_MAXLEN 128
#define NV_MSGS_MAXNUM 239
#elif BOARD_CADRE1120_AP
#define NV_MSGS_SIZE 256000UL // 250kB
#define NV_MSGS_MAXLEN 128
#define NV_MSGS_MAXNUM 1999
#else
#error "no support"
#endif
#else /* CONTIKI_TARGET_COOJA */
#define NV_MSGS_SIZE 1024
#define NV_MSGS_MAXLEN 128
#define NV_MSGS_MAXNUM 8
#endif

#define NV_FILE "NV"
#define NV_SIZE (4096 + NV_MSGS_SIZE)

// 网络容量，即可管理的VD/RP节点总个数
#define NETWORK_CAPACITY (NV_NODES_SIZE / 8 - 1)

#define FLASH_BOOT_ADDR  0x1800
struct BOOT {
  uint16_t magic;   // 初始化标志
   uint8_t role;    // 节点类型
   uint8_t version; // 固件版本
   uint8_t update;  // 更新来源
   uint8_t flash;   // Flash编号
  uint32_t size;    // 新固件大小
  uint32_t addr;    // 新固件地址
  uint16_t crc;     // 新固件校验和
   uint8_t updver;  // 新固件版本
};

#define AP_IMG_SIZE ((uint32_t)94<<10)
#define RP_IMG_SIZE ((uint32_t)64<<10)
#define VD_IMG_SIZE ((uint32_t)90<<10)

// 无线升级信息(OTA Information)
struct OTA {
  uint16_t magic;
  struct {
    uint8_t id;    // 固件适用节点类型
    uint8_t ver;   // 固件版本号
    uint16_t rcvd; // 已接收的固件页数
    uint32_t addr; // 固件在Flash中起始地址
    uint32_t size; // 固件大小
    uint16_t crc;  // 固件CRC
    rimeaddr_t from; // 发送固件的源节点
     uint8_t pend; // 固件是否接收完成，0:已完成，1:未完成
     uint8_t mode; // 更新模式
     uint8_t reserv[2];
  } images[3];
  uint16_t crc;    // OTA的CRC16校验和
};

// 全网参数信息(PAN Information Bank)
struct PIB {
  uint16_t magic;
  uint16_t apHbeatT;        // AP心跳周期，以秒为单位
  uint16_t rpHbeatT;        // RP心跳周期，以秒为单位
  uint16_t vdHbeatT;        // VD心跳周期，以秒为单位
  uint16_t healthT;         // 自检周期，以秒为单位
  uint16_t commandT;        // 下行命令轮询周期，以秒为单位
  uint16_t paramT;          // 全网参数轮询周期，以秒为单位
  uint8_t batVoltageThr;    // 电池电压报警阈值，以.1V为单位
  uint8_t batQuantityThr;   // 电池电量报警阈值，以1%为单位
  uint8_t solarVoltageThr;  // 太阳能电池电压报警阈值，以.1V为单位
  uint8_t radioChan;        // 射频信道(1~10)
  int8_t radioPower;        // 射频功率(17dBm~28dBm)
  uint16_t crc;             // PIB的CRC16校验和
};

// 节点特定信息(Node Information Bank)
struct NIB {
  uint16_t magic;
  uint8_t  devno[6];        // 设备编码
  uint16_t addr;            // 网内短地址
  uint8_t  hwver;           // 硬件版本号
  uint8_t  fwver;           // 固件版本号
  uint8_t  activated;       // VD激活标志(仅VD使用)
  uint8_t  tricseq;         // 全网参数序号
  uint8_t  locked;          // 网络锁定标记(仅AP使用)
  uint8_t  reserved[1];
  uint8_t  aeskey[16];      // AES加密密钥
  uint8_t  aesiv[16];       // AES加密向量
  char     host[16];        // 服务器IP
  uint16_t port;            // 服务器端口
  uint8_t  reservedb[12];
  uint16_t crc;             // NIB的CRC16检验和
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
  uint8_t gain_hmc5983;     // 磁传感器增益
  uint8_t status;           // 车位状态
  int16_t base_line[3];     // 背景磁场基线值
  uint16_t crc;             // ALGO的CRC16校验和
};

// 存储消息环形队列
struct MSGS {
  uint16_t magic;
  uint16_t count;           // 消息个数
  uint16_t readptr;         // 读取指针
  uint16_t writeptr;        // 写入指针
  uint8_t  reserv[1];
  uint8_t  changed;         // 是否已改变标志
  uint32_t wcount;          // 写入次数
  uint16_t crc;             // MSGS的CRC16校验和
};

// 存储消息的元数据
struct msgmeta {
  uint8_t id;              // 消息命令字
  uint8_t len;             // 消息长度
  uint16_t crc;            // 消息检验和
  uint32_t ts;             // 消息的时间戳
  uint8_t synced;          // 消息时间戳的同步标志
  uint8_t fixed;           // 消息时间戳的修正标志
  uint8_t erased;          // 擦除标志
  uint8_t sent;            // 已发送标志
  uint8_t history;         // 是否历史数据标志, 0:不是, 1:是
  uint8_t reserv[1];
  uint16_t mcrc;           // 元数据自身的CRC
};
/*------------------------------------------------------------------*/
// 主线程状态枚举值
enum {
  S_START = 0,              // 初始状态
  S_PREACT,                 // 预激活
  S_ACTED,                  // 已激活
  S_JOIN,                   // 入网
  S_SYNC,                   // 同步
  S_OFFLINE,                // 掉线
  S_READY,                  // 已入网
  S_SLEEP,                  // 休眠
  S_CONN,                   // 连接GPRS
};

// 入网响应枚举值
enum {
  APP_ACCEPT = 1,           // 同意入网
  APP_REJECT = 2,           // 拒绝入网
  APP_ASSIGN = 3,           // 分配新地址
  APP_RECONN = 4            // 重新入网
};

// 拒绝入网原因枚举值
enum {
  APP_REJECT_NOAUTH    = 1, // 因非授权设备拒绝其入网
  APP_REJECT_NETFULL   = 2, // 因网络已满拒绝其入网
  APP_REJECT_BLACKLIST = 3, // 因在黑名单中拒绝其入网
  APP_REJECT_LOCKED    = 4  // 因网络被锁定拒绝其入网
};

// 应用层发送结果枚举值
enum {
  APP_TX_OK    = 0,         // 发送成功(无需ACK或收到ACK)
  APP_TX_FAIL  = 1,         // 发送失败
  APP_TX_NOACK = 2          // 发送成功但未收到ACK
};

enum {
  APP_EVT_ONLINE  = 1,      // 节点上线
  APP_EVT_OFFLINE = 2       // 节点下线
};

// 子操作枚举值
enum {
  APP_GET = 1,              // 查询
  APP_SET = 2,              // 配置
  APP_ADD = 3,              // 增加
  APP_DEL = 4               // 删除
};

enum {
  APP_OK  = 0,
  APP_ERR = 1
};

enum {
  NODE_VD = 0x01,           // 车位检测器节点
  NODE_RP = 0x02,           // 中继节点
  NODE_AP = 0x04            // 网关节点
};

/*------------------------------------------------------------------*/
// 错误/警告/信息代码枚举值
enum {
  /* 错误码: 0x00-0x0F */
  E_FAIL            = 0x01,
  E_NULL            = 0x02,
  E_BUSY            = 0x03,
  E_INVAL           = 0x04,
  E_STATE           = 0x05,
  E_SIZE            = 0x06,
  E_CRCX            = 0x07,
  E_EMPTY           = 0x08,
  E_DROP            = 0x09,
  E_FULL            = 0x0A,
  E_IDENTITY_ERR    = 0x10,
  E_SENSOR_FAIL     = 0x11,

  /* 警告码: 0x40-0x7F */
  W_JOIN_REJECTED   = 0x41,
  W_SEND_BUSY,
  W_DROP_SEND_MSG,
  W_DROP_RCVD_MSG,
  W_FWD_QUEUE_NULL,
  W_RECV_QUEUE_FAIL,
  W_QUEUEBUF_EMPTY,
  W_MSG_NOT_HANDLE,
  W_MSG_NO_ACK,
  W_ACK_NOT_MATCH,
  W_RECV_AT_WRONG_STATE,
  W_READ_MSG_CRCX,
  W_INVALID_PIB,
  W_INVALID_OTA_EXEC,
  W_UNKNOWN_TARGET,
  W_VD_LIST_FULL,
  W_VD_LIST_FAIL,

  W_AP_ACCEPT_LIST_FULL = 0x61,
  W_AP_MESH_SEND_NULL,
  W_AP_GPRS_SEND_NULL,
  W_AP_GPRS_FWD_FAIL,

  W_GPRS_ERROR         = 0x71,
  W_GPRS_NO_ACK,
  W_GPRS_RCVD_CRCX,
  W_GPRS_DROP_RCVD,
  W_GPRS_RECV_QUEUE_FAIL,
  W_GPRS_SEND_NOT_READY,
  W_GPRS_SEND_BUSY,
  W_GPRS_SEND_FAIL,
  W_GPRS_SENT_FAIL,

  /* 运行信息码: 0x80-0xFF */
  I_REBOOT             = 0x81,
  I_FACTORY_RESET,
  I_NETWORK_JOINED,
  I_UPDATE_LOCAL_TIME,
  I_NEW_PIB,
  I_FWD_COMMAND,
  I_SAVED_MSG_COUNT,
  I_OTA_START,
  I_OTA_FINISH,
  I_OTA_PAUSED,
  I_OTA_RESUME,

  /** VD运行信息码 **/
  I_VD_RADIO_OFF       = 0x91,
  I_VD_WAIT_ACT,
  I_VD_BACK_SLEEP,
  I_VD_PREACT,
  I_VD_ACT_CONNED,
  I_VD_NO_FAECMD,
  I_VD_SAVE_MSG,
  I_VD_SEND_SAVE,
  I_VD_ERASE_MSG,

  /** RP运行信息码 **/

  /** AP运行信息码 **/

  /** GPRS信息码 **/
  I_GPRS_CONNED        = 0xD1,
  I_GPRS_CLOSED,
  I_GPRS_RSSI,
  I_GPRS_ACKED,
  I_GPRS_CHANGE,
  I_GPRS_CHANGE_CLOSE,
  I_GPRS_CHANGE_REOPEN
};

/*------------------------------------------------------------------*/
#define APP_WVDS  0x01

#define APP_AES_KEY  "aeskey-cadre2016"
#define APP_AES_IV   "0123456789ABCDEF"

#ifndef APP_ACK_WAIT
#define APP_ACK_WAIT (CLOCK_SECOND << 0)
#endif

#ifndef APP_FAECMD_WAIT
#define APP_FAECMD_WAIT (CLOCK_SECOND * 120)
#endif

#ifndef VD_REINIT_WAIT
#define VD_REINIT_WAIT (CLOCK_SECOND * 60)
#endif

#ifndef APP_GPRS_ACKWAIT
#define APP_GPRS_ACKWAIT (CLOCK_SECOND << 2)
#endif

#define DEVNO_LEN   6         // 设备编码字节数
#define MAC_LEN     8         // MAC地址字节数
#define TSTAMP_LEN  6         // 时间戳字节数

/*------------------------------------------------------------------*/
enum {
  /* Shenzhen Standard */
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
  APPDATA_STATS_DATA  = 0x27, /* 统计数据查询消息 */

  APPDATA_RESET       = 0x31, /* 重启节点消息 */
  APPDATA_FACT_RESET  = 0x32, /* 恢复出厂设置消息 */
  APPDATA_VD_ACTIVATE = 0x33, /* VD激活消息 */
  APPDATA_VD_FAECONN  = 0x34, /* VD现场连接消息 */
  APPDATA_VD_DISCONN  = 0x35, /* VD现场断开连接消息 */
  APPDATA_VD_REINIT   = 0x36, /* VD重新标定 */

  APPDATA_NODE_INFO   = 0x3C, /* 节点信息查询消息 */
  APPDATA_OTA_PROG    = 0x3D, /* 无线升级进度消息 */
  APPDATA_LED_MODE    = 0x3E, /* Led指示设置消息 */
  APPDATA_DBG_MODE    = 0x3F  /* 调试模式设置消息 */
};

#ifndef APP_DATA_MAXLEN
#define APP_DATA_MAXLEN 89
#endif

#ifndef APPMSG_MAXLEN
#define APPMSG_MAXLEN (APPMSG_HEADER_LEN + APP_DATA_MAXLEN + APPMSG_FOOTER_LEN)
#endif

#ifndef APPMSG_AES_MAXLEN
#define APPMSG_AES_MAXLEN (APP_DATA_MAXLEN + 7 + 16 + 5)
#endif

#ifndef APPMSG_B64_MAXLEN
#define APPMSG_B64_MAXLEN ((APP_DATA_MAXLEN + 7 + 16) * 4 / 3 + 1 + 5)
#endif

#define APPMSG_BEG 0xAA
#define APPMSG_END 0xFF

#define APPMSG_UP   0
#define APPMSG_DOWN 1

#define APPMSG_HEADER_LEN 9
struct app_msg_header {
  uint8_t beg;
  uint8_t len;
  uint8_t devno[6];
  uint8_t cmdop:6;
  uint8_t rep:1;
  uint8_t dir:1;
};

#define APPMSG_FOOTER_LEN 3
struct app_msg_footer {
  uint8_t crc[2];
  uint8_t end;
};

struct app_msg {
  struct app_msg_header header;
  uint8_t data[APP_DATA_MAXLEN];
  struct app_msg_footer footer;
};

/*------------------------------------------------------------------*/
struct app_data_ack {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t result;
};

// APPDATA_PARK_EVT
struct app_data_parkevt {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t status;
   int8_t temperature;
  uint8_t seqno;
  uint8_t magnetic[6];
  uint8_t batVoltage;
  uint8_t batQuantity;
};

// APPDATA_SYNC_TIME
struct app_data_synctime {
  uint8_t tstamp[TSTAMP_LEN];
};

struct app_data_syncresp {
  uint8_t tstamp[TSTAMP_LEN];
};

// APPDATA_MAG_DATA
#define APPDATA_MAGDATA_NUM 10
struct app_data_magdata {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t seqno;
  uint8_t hillvalleys[APPDATA_MAGDATA_NUM][6];
  uint8_t baselines[6];
  uint8_t smooths[6];
  uint8_t judgebranch;
  uint8_t status;
};

// APPDATA_HIST_DATA
#define APPDATA_HISTDATA_NUM 5
struct app_data_histdata {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t count;
  uint8_t reserv;
  struct {
    uint8_t devno[DEVNO_LEN];
    uint8_t tstamp[TSTAMP_LEN];
    uint8_t status;
  } entries[APPDATA_HISTDATA_NUM];
};

// APPDATA_AP_CONN
struct app_data_apconn {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t HwVer;
  uint8_t FwVer;
};

struct app_data_apconnack {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t res;
};

// APPDATA_NODETABLE
struct app_data_nodetbl {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t subop;
  uint8_t mac[MAC_LEN];
};

struct app_data_nodetbl_resp {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t subop;
  uint8_t result;
};

// APPDATA_BLACKLIST
struct app_data_blackl {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t subop;
  uint8_t mac[MAC_LEN];
};

struct app_data_blackl_resp {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t subop;
  uint8_t res;
  uint8_t total;
  uint8_t index;
  uint8_t count;
  uint8_t macs[MAC_LEN * 5];
};

// APPDATA_OTA_EXEC
struct app_data_ota_exec {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t type;
  uint8_t fwver;
  uint8_t target[MAC_LEN];
  uint8_t mode;
  uint8_t count;
  uint8_t autop;
  uint8_t commit[8];
};

struct app_data_ota_execack {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t result;
  uint8_t version;
};

// APPDATA_OTA_DATA
#define APP_OTA_DATALEN 128
struct app_data_ota_data {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t type; // target node role: VD,RP,AP
  uint8_t ver;
  uint8_t beg:1; // first packet
  uint8_t reserva:6;
  uint8_t fin:1; // finish packet
  uint8_t reservb;
  uint8_t addr[4]; // start address
  uint8_t len[2]; // length of data
  uint8_t data[APP_OTA_DATALEN];
  uint8_t crc[2]; // crc of type to data
};

struct app_data_ota_dataack {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t res;
  uint8_t addr[4];
  uint8_t arg[4];
};

// APPDATA_VD_CONN
struct app_data_vdconn {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t HwVer;
  uint8_t FwVer;
  uint8_t rpMAC[MAC_LEN];
  uint8_t apMAC[MAC_LEN];
};

struct app_data_vdconnack {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t res;
  uint8_t argd[14];
};

// APPDATA_VD_HBEAT
struct app_data_vdhbeat {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t status;
   int8_t temperature;
  uint8_t seqno;
  uint8_t magnetic[6];
  uint8_t batVoltage;
  uint8_t batQuantity;
  uint8_t parent[MAC_LEN];
   int8_t recvRSSI;
   int8_t recvLQI;
   int8_t sendRSSI;
   int8_t sendLQI;
};

struct app_data_vdhback {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t seqno;
};

// APPDATA_RP_HBEAT
struct app_data_rphbeat {
  uint8_t tstamp[TSTAMP_LEN];
   int8_t temperature;
  uint8_t solarBatVoltage;
  uint8_t batVoltage;
  uint8_t batQuantity;
  uint8_t parent[MAC_LEN];
   int8_t recvRSSI;
   int8_t recvLQI;
   int8_t sendRSSI;
   int8_t sendLQI;
};

struct app_data_rphback {
  uint8_t tstamp[TSTAMP_LEN];
};

// APPDATA_AP_HBEAT
struct app_data_aphbeat {
  uint8_t tstamp[TSTAMP_LEN];
   int8_t temperature;
  uint8_t solarBatVoltage;
  uint8_t batVoltage;
  uint8_t batQuantity;
   int8_t gprsRSSI;
};

struct app_data_aphback {
  uint8_t tstamp[TSTAMP_LEN];
};

// APPDATA_GPRS_PARAM
struct app_data_gprs {
  uint8_t tstamp[TSTAMP_LEN];
     char host[16];
  uint8_t port[2];
};

struct app_data_gprs_ack {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t res;
};

// APPDATA_RADIO_PARAM
struct app_data_radio {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t subop;
  uint8_t chan;
  uint8_t freq[4];
   int8_t power;
};

struct app_data_radio_ack {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t subop;
  uint8_t res;
  uint8_t chan;
  uint8_t freq[4];
   int8_t power;
};

// APPDATA_BASIC_PARAM
struct app_data_basic {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t subop;
  uint8_t param[sizeof(struct PIB) - 6]; // 排除PIB.magic/radioChan/radioPower/crc
};

struct app_data_basic_ack {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t subop;
  uint8_t res;
  uint8_t param[sizeof(struct PIB) - 6]; // 排除PIB.magic/radioChan/radioPower/crc
};

// APPDATA_ALGO_PARAM
struct app_data_algo {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t subop;
  uint8_t param[sizeof(struct ALGO) - 5]; // 排除ALGO.magic/status/crc
};

struct app_data_algo_ack {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t subop;
  uint8_t res;
  uint8_t param[sizeof(struct ALGO) - 5]; // 排除ALGO.magic/status/crc
};

// APPDATA_VD_REINIT
struct app_data_reinit {
  uint8_t tstamp[TSTAMP_LEN];
};

struct app_data_reinit_resp {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t result;
  uint8_t param[sizeof(struct ALGO) - 5]; // 排除ALGO.magic/status/crc
};

// APPDATA_ALARM
struct app_data_alarm {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t radio;
  uint8_t sensor1;
  uint8_t sensor2;
  uint8_t flash1;
  uint8_t flash2;
  uint8_t rtc;
  uint8_t battery;
  uint8_t solarBat;
};

// APPDATA_ONOFFLINE
struct app_data_offline {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t event;
  uint8_t MAC[MAC_LEN];
};

// APPDATA_LOCKORUN
struct app_data_lockor {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t subop;
};

struct app_data_lockor_ack {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t res;
};

// APPDATA_RESET: 0x31
struct app_data_reset {
  uint8_t tstamp[TSTAMP_LEN];
};

// APPDATA_FACT_RESET: 0x32
struct app_data_freset {
  uint8_t tstamp[TSTAMP_LEN];
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

// APPDATA_NODE_INFO: 0x3C
struct app_data_nodeinfo {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t role;
};

struct app_data_nodeinfo_resp {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t HwVer;
  uint8_t FwVer;
};
/*------------------------------------------------------------------*/
struct node {
  uint8_t    mac[MAC_LEN];  // 节点MAC地址
  rimeaddr_t addr;          // 节点短地址
};

struct nodes_table {
  uint16_t magic;
  uint16_t count;
   uint8_t reserved[4];
   uint8_t nodes[0];
};

struct blacklist {
  uint16_t magic;
  uint16_t count;
   uint8_t reserved[4];
   uint8_t nodes[0];
};

/*------------------------------------------------------------------*/
extern const int OTA_IMGID[];
extern const char* OTA_FILE[];
extern const uint8_t OTA_ROLE[];
extern const uint32_t OTA_ADDR[];

/*------------------------------------------------------------------*/
int nv_init(void);
int nv_read(uint32_t addr, uint8_t *buf, uint8_t len);
int nv_write(uint32_t addr, uint8_t *buf, uint8_t len);
int nv_erase(uint32_t addr, uint32_t len);

void EraseFlash(unsigned long waddr);
unsigned char WriteFlash(unsigned long addr,unsigned char *pdata, unsigned int length);
int ReadFlash(unsigned long addr, unsigned char *buf, unsigned int len);
unsigned char ReadFlashByte(unsigned long waddr);

void app_show_reboot(void);
void app_show_inact(void);
void app_show_chan(uint8_t chan);
void app_fatal(void);
/*------------------------------------------------------------------*/
#if DEVICE
#if WITH_MMC3316
#include "mmc3316.h"
#elif WITH_HMC5983
#include "hmc5983.h"
#elif WITH_QMC5883
#include "qmc5883.h"
#else
#error "no support"
#endif
#endif

#endif /* _APP_H */
