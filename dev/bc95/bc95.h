#ifndef _BC95_H_
#define _BC95_H_

#include "uart.h"

//-----------------------------------//
typedef u8 (*bc95_callback_t)(u8*);

#define BC95_OFF  P1OUT &= ~BIT0;
#define BC95_ON   P1DIR |= BIT0;  \
                  P1OUT |= BIT0;

#define ERR_NONE    ( 0)
#define ERR_OTHER   (0XFD)
#define ERR_REBOOT  (0XFE)
#define ERR_ACK     (0XFF)

/* Connected Device Platform(CDP) server*/
#define CDP_IP    "112.93.129.154"
#define CDP_PORT  "5683"
/*  */
#define BAND 5

#define AT_NCDP_SET		"AT+NCDP=112.93.129.154\r\n"

#define AT_NQMGR  "AT+NQMGR\r\n"
#define AT_NQMGS  "AT+NQMGS\r\n" 
#define AT_NMGR   "AT+NMGR\r\n"

//-----------------------------------//
typedef struct{
  u8 ACTIVE;
  u8 ABNORMAL_REBOOT;
  u8 AUTOCONNECT;
  u8 SCRAMBLING;
  u8 SI_AVOID;
  u8 NBAND;
  u8 CFUN;
  u8 ATTACH;
  u8 CEREG;
  u8 CSCON;   // 0 IDLE 1 CONNECT
  u8 CSQ;
  u8 IMSI[20]; //sim
  u8 IMEI[20]; //bc95
}bc95_info;

typedef struct{
  u8 cmd;       
  u8 res;          // OVERTIME\OK\ERR
  u8 ret;          // 重发次数 
  u16 idelay;
  u8(*callback)(void); // 
}bc95_stu;

typedef  bc95_stu* bc95_stu_t;

typedef struct{
  u8 rBuffered;
  u8 rReceived;
  u8 rDropped;
}bc95_smsg;

enum{  
  NONE = 0X00,
  NRB = 0x01,
  
  CFUN_REQ,  
  CFUN_SET,
  CFUN_CLR, 
  
  CGATT_REQ,  
  CGATT_SET,
  
  NBAND_REQ,
  NBAND_SET,
  
  CGSN,     
  CIMI,
  NCDP,        
  COPS,         
  CGDCONT,  
  CSQ,
  CCLK,
  
  CEREG_REQ,
  CSCON_REQ,
  
  NCONFIG_REQ,
  NCONFIG_SET,
  
  NQMGR,  
  NMGR,      
  NMGS,
};

//-----------------------------------//

u8 nb_hw_init(void);
u8 nb_sft_init(void);
u8 nb_att_net_req(void);
u8 nb_reg_net_req(void);
u8 nb_radio_on(void);
u8 nb_radio_off(void);
u8 nb_reboot(void);
u8 nb_module_off(void);
u8 nb_msg_tx(u16 datalen,u8* datastream);
u8 nb_dlbuf_req(void);
u8 nb_msg_rx(void);

//nb_cmd_tx(NCONFIG_REQ,"AT+NCONFIG?\r\n",hdl_nconfig_req);
//nb_cmd_tx(NBAND_SET,"AT+NBAND=5\r\n",hdl_cmd); 
//nb_cmd_tx(NBAND_REQ,"AT+NBAND?\r\n",hdl_nband_req);
//nb_cmd_tx(CFUN_REQ,"AT+CFUN?\r\n",hdl_cfun_req);
//nb_cmd_tx(CIMI,"AT+CIMI\r\n",hdl_imsi); 
//nb_cmd_tx(CSQ,"AT+CSQ\r\n",hdl_csq); 
//nb_cmd_tx(CGATT_REQ,"AT+CGATT?\r\n",hdl_cgatt_req); 
//nb_cmd_tx(CCLK,"AT+CCLK?\r\n",hdl_cclk); 
//nb_cmd_tx(CEREG_REQ,"AT+CEREG?\r\n",hdl_cereg_req); 
//nb_cmd_tx(CSCON_REQ,"AT+CSCON?\r\n",hdl_cscon_req); 
//
//
//
//nb_cmd_tx(NCONFIG_SET,"AT+NCONFIG=AUTOCONNECT,TRUE\r\n",hdl_cmd);
//nb_cmd_tx(NCONFIG_SET,"AT+NCONFIG=AUTOCONNECT,FALSE\r\n",hdl_cmd);
//nb_cmd_tx(NCONFIG_SET,"AT+NCONFIG=CR_0354_0338_SCRAMBLING,TRUE\r\n",hdl_cmd);
//nb_cmd_tx(NCONFIG_SET,"AT+NCONFIG=CR_0354_0338_SCRAMBLING,FALSE\r\n",hdl_cmd);    
//nb_cmd_tx(NCONFIG_SET,"AT+NCONFIG=CR_0859_SI_AVOID,TRUE\r\n",hdl_cmd);  
//nb_cmd_tx(NCONFIG_SET,"AT+NCONFIG=CR_0354_0338_SCRAMBLING,FALSE\r\n",hdl_cmd);    
//
//nb_cmd_tx(CFUN_SET,"AT+CFUN=1\r\n",hdl_cmd); 
//nb_cmd_tx(CFUN_CLR,"AT+CFUN=0\r\n",hdl_cmd); 
//
//nb_cmd_tx(CGATT_REQ,"AT+CGATT?\r\n",hdl_cmd); 
//nb_cmd_tx(CGATT_SET,"AT+CGATT=1\r\n",hdl_cmd); 
//
//nb_cmd_tx(COPS,"AT+COPS=1,2,\"46011\"\r\n",hdl_cmd); 
//nb_cmd_tx(CGDCONT,"AT+CGDCONT=1,\"IP\",\"ctnb\"\r\n",hdl_cmd); 

//-----------------------------------//
extern bc95_smsg bc95_s;
extern bc95_info bc95_i;
u8 hdl_cmd(void);
u8 nb_cmd_tx(u8 cmd,u8* str_at, u8 (*callback)());
#endif