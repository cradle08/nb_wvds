#ifndef __BC95_H__
#define __BC95_H__

#include "sys/etimer.h"
#include "lib/ringbuf.h"

#define NB_POWER_OFF    P5OUT |=BIT3;
#define NB_POWER_ON     P5OUT &= ~BIT3;

/* Connected Device Platform(CDP) server*/
#define DEFAULT_SERVER_IP    "58.250.57.68"
#define DEFAULT_SERVER_PORT  3075
#define DEFAULT_LOCAL_PORT   6677
#define BAND                  5

//#define AT_NRB     "AT+NRB" // reboot nb module
//#define AT_NBAND   "AT+NBAND" // get nb baud
//#define AT_CGATT   "AT+CGATT" // attach network
//#define AT_CSQ     "AT+CSQ" // query quaility
////#define AT_NSOCR   "AT+NSOCR=DGRAM,17,3005,1" // create socket
//#define AT+NSOCL   "AT+NSOCL"  // close socket
//#define AT_CEREG   "AT+CEREG" //current EPS Network Registration Status
//#define AT_NSOST   "AT+NSOST" // send msg AT cmd
//#define AT_NSORF   AT+NSORF // recv msg AT cmd

enum {
  AT_OK = 0,
  AT_ERROR
};

struct nb_param {
  uint8_t  seraddr[15]; // server ip address
  uint16_t serport; // server port
  uint8_t  localport; // local port
  uint8_t  nb_csq; // nb signal
  uint8_t  imsi[15];  // IMSI no
};
  
struct nb_at_exec {
  uint8_t retry; // re try times
  uint8_t revalue; // re value
  struct ringbuf* prb; // recv ring buf
  struct etimer nbet;  
};
  
uint8_t atcmd_exec_and_check(uint8_t* patcmd, uint8_t num, uint16_t delay, uint8_t* pcheck);
uint8_t check_atcmd_return(uint8_t* p);
uint8_t* find_at_return_value(uint8_t* p);

void nb_send(uint8_t* msg, uint16_t len);
void nb_module_init();
void nb_network_init();


#endif