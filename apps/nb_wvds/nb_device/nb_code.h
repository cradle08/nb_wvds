#ifndef __NB_CODE__
#define __NB_CODE__




// msg cmd 
enum {
  CMD_APP = 0xF1,         // updata app(ser 2 device)
  CMD_APP_ACK = 0xF2,     // updata ack(device 2 ser)
  CMD_UPDATA = 0x98,      // updata packet data(s2d)
  CMD_UPDATA_ACK = 0x18,  // updata packet data(d2s)
  CMD_RUN_LINE = 0xF5,    // query which segment cpu runing(s2d) 
  CMD_RUN_LINE_ACK = 0xF6, // ack(d2s)
  CMD_CHECK = 0x01,        // park status check(d2s)
  CMD_CHECK_ACK = 0x81,    // ack (s2d)
  CMD_HB = 0x04,           // heart beat(d2s)
  CMD_HB_ACK = 0x84,       // heart beat(s2d)
  CMD_ALARM = 0x23,        // alarm cmd(d2s)
  CMD_ALARM_ACK = 0xA3,    // alarm ack(s2d)
  CMD_BASIC_PARAM = 0xA5,   // base param set(s2d)
  CMD_BASE_PARAM_ACK = 0x25, // base param set ack(d2s)
  CMD_ALGO_PARAM = 0xA5,     // algorithm param set(s2d)
  CMD_ALGO_PARAM_ACK = 0X25, // algorith param set ack(d2s)
  CMD_REBOOT = 0xB1,         // reboot msg(s2d)
  CMD_REBOOT_ACK = 0x31,     // reboot ack(d2s)
  CMD_MAG_CHANGE = 0x24,            // mag change msg(d2s)
  CMD_MAG_CHANGE_ACK = 0xA4         // mag change msg ack(s2d)
};
  

// msg header struct
struct Msg_Head{
  int8_t startflag; // AA
  uint8_t msglen;   // messge len
  uint8_t devno[6];     // device number
  uint8_t cmd;       // command id   
};

// msg tail
struct Msg_Tail{
  uint8_t crc[2];    // crc of type to data
  int8_t  endflag;   //FF
};


// send msg 
struct SEND_MSG{
  struct SEND_MSG* next;
  struct Msg_Head msg_head;
  int8_t playdata[SEND_PLAYDATA_LEN];
  struct Msg_Tail msg_tail;
};

// recv msg
struct RECV_MSG{
  struct RECV_MSG* next;
  struct Msg_Head msg_head;
//  int8_t playata[RECV_PLAYDATA_LEN];
  struct Msg_Tail msg_tail;
};

MEMB(recv_msg_mem, struct RECV_MSG, RECV_MSGMEM_NUM); // 3
LIST(recv_msg_list);
MEMB(send_msg_mem, struct SEND_MSG, SEND_MSGMEM_NUM); // 5
LIST(send_msg_list);


// most msg ack form
struct msg_ack{
  uint8_t tstamp[TSTAMP_LEN];
};


// update  msg form(s2d)
struct msg_data_app {
  uint8_t sfver; // soft version
  uint8_t hdver; // hard version
  uint8_t size;  // the packet size
  int8_t data;   // ???
  uint8_t crc;   // crc
};

// update packet data msg form(s2d)
struct msg_update {
  uint8_t size;  // the packet size
  uint8_t no;    // the number of packet which is download
  int8_t*  data; // the packet data 
};

// updata packet data msg form ack(d2s)
struct msg_updata_ack {
  uint8_t tstamp[TSTAMP_LEN];  // time stamp
  uint8_t no;                  // 
  uint8_t nb_signal;           // nb module signal strength
};

// query which segment device runing 
struct msg_segment {
  int8_t data[2]; //???
};

//
struct msg_segment_ack {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t sfver;
  uint8_t hdver;
  uint8_t nb_signal;
};

//  device check msg(d2s)
struct msg_check {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t status;      // device checking status: 0-leaving,1-parking, 2-strong mag, 3-init
  uint8_t nb_signal;   //
  uint8_t temperature; // temperature
  int16_t mag[3];      // xyz magnetic
  uint8_t bat_voltage; // bat voltage
  uint8_t bat_quantity; // bat quantity
};
   
//  heart beat msg(d2s)
struct msg_hb {
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t status;
  uint8_t nb_signal;
  uint8_t temperature;
  int16_t mag[3];
  uint8_t bat_voltage;
  uint8_t bat_quantity;
};

//  alarm msg(d2s)
struct msg_alarm {
  uint8_t nb_signal;
  uint8_t mag_sersor;   //qmc5883 status: 00-normal,01-can't sample, 02- value more than int16_t
  uint8_t flash;    // flash(m25pe) status: 00-narmal,01-can't ready or wirte, 02-full  
  uint8_t rtc;         // rtc status: 00-normal, 01-stop, 02-wrong value
  uint8_t battery;  // battery status: 00-normal, 01-low voltage
};

//  device base param set(s2d)
struct msg_base_param {
  uint8_t operation;  // 01-query, 02-set
  uint16_t hb_period;  // heart beat period
  uint16_t health_period; // device self check period
  uint16_t batvoltage_thresh; // battery voltage threshold
  uint8_t  batquantity_thresh; // battery quantity threshold
};

// device base param set ack(d2s)
struct msg_base_param_ack {
  uint8_t operation;  // 01-query, 02-set
  uint16_t hb_period;  // heart beat period
  uint16_t health_period; // device self check period
  uint16_t batvoltage_thresh; // battery voltage threshold
  uint8_t  batquantity_thresh; // battery quantity threshold
};

// algorithm param set(s2d)
struct msg_algo_param {
    uint8_t operation;  // 01-query, 02-set
    uint16_t normal_period;  // normal sample period
    uint16_t flunc_period;   // flunct sample period
    uint16_t big_occthresh;  // quickly check threshold
    uint8_t  mid_occthresh;  // normal check threshold
    uint8_t  lit_occthresh;  // little check threshold
    int16_t  baseline[3];    // xyz baseline value
    uint8_t  sensor_gain;    // magnetic sensor gain
};

// algorithm param set ack(d2s)
struct msg_algo_param_ack {
    uint8_t operation;  // 01-query, 02-set
    uint16_t normal_period;  // normal sample period
    uint16_t flunc_period;   // flunct sample period
    uint16_t big_occthresh;  // quickly check threshold
    uint8_t  mid_occthresh;  // normal check threshold
    uint8_t  lit_occthresh;  // little check threshold
    int16_t  baseline[3];    // xyz baseline value
    uint8_t  sensor_gain;    // magnetic sensor gain
};
   
// magnetic change msg(d2s)
struct msg_mag_change {
    uint8_t tstamp[TSTAMP_LEN];
    int16_t hillvalleys[UPLOAD_MAGDATA_NUM][3];  // magnetic hill and valley value 
    int16_t  baseline[3];    // xyz baseline value
    int16_t  smooth[3];    // xyz baseline value
    uint8_t  judge_branch; // judge branch
    uint8_t  status;
};

// magnetic change msg ack(d2s)
struct msg_mag_change_ack{
    uint8_t tstamp[TSTAMP_LEN];
    uint8_t status; // 01-sucess, 00-fail //?????
};

// device reboot msg
struct msg_reboot{
    uint8_t tstamp[TSTAMP_LEN];
};

// devicce reboot msg ack
struct msg_reboot_ack{
  uint8_t tstamp[TSTAMP_LEN];
  uint8_t result;  // 0-reboot success, 1-reboot fail
};





struct RECV_MSG* get_available_recv_memb();
struct SNED_MSG* get_available_send_memb();
void handle_recv_msg(struct RECV_MSG* p);
void handle_send_msg(struct SEND_MSG* p);

// handle each msg func
void msg_app(struct RECV_MSG* pr);
void msg_app_ack(struct SEND_MSG* ps);
void msg_updata(struct RECV_MSG* pr);
void msg_updata_ack(struct SEND_MSG* ps);
void msg_reboot(struct RECV_MSG* pr);
void msg_reboot_ack(struct SEND_MSG* ps);
void msg_run_line(struct RECV_MSG* pr);
void msg_run_line_ack(struct SEND_MSG* ps);
void msg_basic_param(struct RECV_MSG* pr);
void msg_basic_param_ack(struct SEND_MSG* ps);
void msg_algo_param(struct RECV_MSG* pr);
void msg_algo_param_ack(struct SEND_MSG* ps);
void msg_hb(struct SEND_MSG* ps);
void msg_hb_ack(struct RECV_MSG* pr);
void msg_alarm(struct SEND_MSG* ps);
void msg_alarm_ack(struct RECV_MSG* pr);
void msg_check(struct SEND_MSG* ps);
void msg_check_ack(struct RECV_MSG* pr);
void msg_mag_change(struct SEND_MSG* ps);
void msg_mag_change_ack(struct RECV_MSG* pr);

PROCESS_NAME(SendMSG_Process);
#endif // __NB_CODE__