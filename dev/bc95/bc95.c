#include "platform-conf.h"
#include "contiki.h"
#include "uart1.h"
#include "bc95.h"
#include "app.h"
#include "m25pe.h"


struct nb_at_exec atexec; // at exec struct
struct nb_param nbparam; // nb param
extern struct ringbuf recvringbuf; //recv ring buffer


/* check */
uint8_t check_atcmd_return(uint8_t* p)
{
   uint8_t* pvalue;
   uint8_t* ptemp  = recvringbuf.data;
   pvalue = strstr(ptemp, p);
   if(pvalue == NULL) return AT_ERROR;
   while(pvalue != recvringbuf.data+recvringbuf.head) {
      ringbuf_get(&recvringbuf);
   }
   return AT_OK;
}


/* exec at cmd and check its revalue , but not use it to send msg */
uint8_t atcmd_exec_and_check(uint8_t* patcmd, uint8_t num, uint16_t delay, uint8_t* pcheck)
{
  uint8_t cmdstr[30] = {0};
  uint8_t checkstr[15] = {0};
  uint8_t cmdlen, checklen, i = 0;
  
  strcpy(cmdstr, patcmd);
  strcpy(checkstr, pcheck);
  cmdlen   = strlen(cmdstr);
  checklen = strlen(checkstr);
  atexec.retry = num;
  
  nb_send(cmdstr, cmdlen);
  etimer_set(&atexec.nbet, MS2JIFFIES(delay));
  for(i = 0; i < atexec.retry; i++) {
    while(etimer_expired(&atexec.nbet)) {
      atexec.revalue = check_atcmd_return(checkstr);
      if(atexec.revalue == AT_OK) {
        i = 255;
        return AT_OK;
      }
      if(i < atexec.retry - 1) {
        etimer_set(&atexec.nbet, MS2JIFFIES(delay)); // try again      
      }
    }
    return AT_ERROR;
  }
}


/* nb network init */
void nb_network_init()
{
  uint8_t cmdlen, checklen;
  uint8_t cmdstr[30] = {0};
  uint8_t checkstr[15] = {0};
  

  atcmd_exec_and_check("AT\r\n" , 5, 150, "OK"); // AT try=5, delay=150
  atcmd_exec_and_check("AT+CGATT?\r\n", 30, 3000, "+CGATT:1"); //   // check CGATT (actach network)   try=30, delay=3000
  atcmd_exec_and_check("AT+CEREG?\r\n", 5, 150, "+CEREG:1,1"); // check CEREG (registrotion  status)  try=5, delay=150
  
  uint8_t buf[30] ={0}; 
  sprintf(buf, "AT+NSOCR=DGRAM,17,%d,1\r\n", nbparam.serport);
  atcmd_exec_and_check(buf, 5, 150, "+CEREG:"); // check CEREG (registrotion  status)  try=5, delay=150
   
}


/*  nb device init */
void nb_module_init()
{
  P5DIR |= BIT3; //out
  P5SEL &= ~BIT3;

  P4DIR &= ~BIT7; //int
  P4SEL &= ~BIT7;
  
  NB_POWER_ON
  
  nbparam.serport = DEFAULT_LOCAL_PORT;
  atexec.prb   = &recvringbuf;
}


/* nb sernd msg func*/
void nb_send(uint8_t* msg, uint16_t len)
{
  uint8_t* ptemp = msg;
  uart1_send(ptemp, len);
}





//
///*================================================================
//【名 称】 nb_syt_init
//【功 能】 NB入网
//【备 注】
//================================================================*/
//uint8_t nb_syt_init(void)
//{
///* 读取或初始化NB模块特定信息 */
// // m25pe_read(NV_NBIB_ADDR,(uint8_t*)&nbib, sizeof(struct NBIB));
//  if(nbib.magic != NV_MAGIC)
//  {
//    memset(&nbib, 0, sizeof(struct NBIB));
//    nbib.magic = NV_MAGIC;
//    nbib.reserved[0] = 0;
//    nbib.reserved[1] = 0;
//    strcpy((char *)nbib.apn,"CTNB");
//    strcpy((char *)nbib.server_ip,"58.250.57.68");
//    strcpy((char *)nbib.server_port,"3075");
//    strcpy((char *)nbib.local_port,"6008");
// //   m25pe_write(NV_NBIB_ADDR, (uint8_t*)&nbib, sizeof(struct NBIB));  
//  }
//  
//  uint8_t res = TRUE;
//  uint8_t ret = 0;
//  uint8_t step = 1;
//  // AT_sync
//  for(ret=0;ret<11;ret++)
//  {
//    res = nb_cmd_tx("AT\r\n", hdl_cmd);
//    if(ERR_NONE == res && 1 == step )
//    {
//      step++;
//      break;
//    }
//    if(ret == 10)
//    {
//      return step;
//    }
//    delay_ms(1000);
//  }
//
//  // query_CSQ
//  for(ret=0;ret<16;ret++)
//  {
//    res = nb_cmd_tx("AT+CSQ\r\n", hdl_csq);
//    if(ERR_NONE == res && 2 == step )
//    {
//      if(nb_i.csq<32||nb_i.csq>8)
//      {      
//        step++;
//        break;
//      }
//    }
//    if(ret == 15)
//    {
//      return FALSE;
//    }
//    delay_ms(2000);
//  }  
//  
//  // query_ATTCH
//  for(ret=0;ret<21;ret++)
//  {
//    res = nb_cmd_tx("AT+CGATT?\r\n", hdl_cgatt);
//    if(ERR_NONE == res && 3 == step )
//    {
//      if(1 == nb_i.attach)
//      {      
//        step++;
//        break;
//      }
//    }
//    if(ret == 20)
//    {
//      return FALSE;
//    }
//    delay_ms(4000);
//  }
//  
//  // query_CEREG
//  for(ret=0;ret<10;ret++)
//  {
//    res = nb_cmd_tx("AT+CEREG?\r\n", hdl_cereg);
//    if(ERR_NONE == res && 4 == step )
//    {
//      if(1 == nb_i.cereg)
//      {      
//        step++;
//        break;
//      }
//    }
//    if(ret == 9)
//    {
//      return FALSE;
//    }
//    delay_ms(1000);
//  }
//
//  //configuration PDP
//  uint8_t cmd[30];
//  sprintf((char*)cmd,"AT+CGDCONT=1,\"IP\",\"%s\"\r\n",nbib.apn);
//  for(ret=0;ret<5;ret++)
//  {
//    res = nb_cmd_tx(cmd, hdl_cmd);
//    if(ERR_NONE == res && 5 == step )
//    {   
//        step++;
//        break;
//    }
//    if(ret == 4)
//    {
//      return FALSE;
//    }
//    delay_ms(500);
//  }
//  
//  // creat socket
//  sprintf((char*)cmd,"AT+NSOCR=DGRAM,17,%s,0\r\n",nbib.local_port); // 忽略返回消息
//  for(ret=0;ret<5;ret++)
//  {
//    res = nb_cmd_tx("AT+NSOCL=0\r\n", hdl_cmd);
//    delay_ms(500);
//    res = nb_cmd_tx(cmd, hdl_nsocr);
//    if(ERR_NONE == res && 6 == step )
//    {
//        step++;
//        break;
//    }
//    if(ret == 4)
//    {
//      return FALSE;
//    }
//    delay_ms(500);
//  } 
//  
//  return step;
//}
//
//
///*================================================================
//【名 称】 bc95_radio_on
//【功 能】 模块上线
//【备 注】 
//================================================================*/
//uint8_t nb_radio_on(void)
//{
//  uint8_t r = ERR_NONE;
//  
//  nb_cmd_tx("AT+CFUN?\r\n",hdl_cfun);
//  if(1 != nb_i.cfun)
//  {
//    delay_ms(1000);
//    do{
//    uart1_Tx(strlen((const char*)"AT+CFUN=1\r\n"),"AT+CFUN=1\r\n"); 
//    while(uartRMsg.rxOK != TRUE);
//    uartRMsg.rxOK = FALSE;
//    r = hdl_cmd();
//    }while(r==ERR_ACK);
//    nb_i.cfun = 1;
//  }
//  delay_ms(4000);  
//  return r;
//}
//
///*================================================================
//【名 称】 bc95_radio_off
//【功 能】 模块上线
//【备 注】 返回状态 connect\psm
//================================================================*/
//uint8_t nb_radio_off(void)
//{
//  uint8_t r = ERR_NONE;
//  r = nb_cmd_tx("AT+CFUN=0\r\n",hdl_cmd); 
//  return r;
//}
//
///*================================================================
//【名 称】 nb_reboot
//【功 能】 模块重启
//【备 注】 重启大概需要2~3s
//================================================================*/
//uint8_t nb_reboot(void)
//{
//  uart1_Tx(strlen((const char*)"AT+NRB\r\n"),"AT+NRB\r\n"); 
//  delay_ms(5000);
//  uartRMsg.rxOK = FALSE;
//  return ERR_NONE; 
//}
//
///*================================================================
//【名 称】 nb_module_off
//【功 能】 关闭模块
//【备 注】 
//================================================================*/
//uint8_t nb_module_off(void)
//{
//  uint8_t r = ERR_NONE;
//  r = nb_radio_off();
//
//  P5DIR |= BIT3; //输出
//  P5SEL &= ~BIT3;
//
//  return r;
//}
//
//
///*================================================================
//【名 称】 nb_msg_tx
//【功 能】 消息发送
//【备 注】 datastream为处理后数据流，返回消息发送状态
//================================================================*/
//uint8_t dict[16][3]={
//  {0x00,0x33,0x30},{0x01,0x33,0x31},{0x02,0x33,0x32},{0x03,0x33,0x33},
//  {0x04,0x33,0x34},{0x05,0x33,0x35},{0x06,0x33,0x36},{0x07,0x33,0x37},
//  {0x08,0x33,0x38},{0x09,0x33,0x39},{0x0A,0x34,0x31},{0x0B,0x34,0x32},
//  {0x0C,0x34,0x33},{0x0D,0x34,0x34},{0x0E,0x34,0x35},{0x0F,0x34,0x36},
//};
//
////数据格式转换
//void dformat_transform(uint8_t* dest, uint8_t* scr, uint16_t datalen)
//{
//  uint16_t i;
//  uint8_t c;
//  for(i=0;i<datalen;i++)
//  {
//    c = (scr[i]>>4)&0x0F; // H
//    dest[4*i+0] = dict[c][1];
//    dest[4*i+1] = dict[c][2];      
//    c = scr[i]&0x0F; // L
//    dest[4*i+2] = dict[c][1];
//    dest[4*i+3] = dict[c][2];    
//  }
//  return ;
//}
//
//uint8_t nb_msg_tx(uint16_t datalen,uint8_t* datastream)
//{
//  uint16_t len = datalen;
//  uint8_t res = ERR_NONE;
//  uint32_t idelay = 0x3000;
//  
//  uint8_t charstream[BUF_SIZE];
//
//  _DINT();
//  uartRMsg.rxOK = FALSE; 
//  memset(charstream, 0, BUF_SIZE);
//  sprintf((char*)charstream,"AT+NSOST=%d,%s,%s,%d,",nb_i.socket,nbib.server_ip,nbib.server_port,2*len);
//  len=strlen((char const*)charstream);
//  uart1_Tx(len,charstream);
//  
//  dformat_transform(charstream,datastream,datalen);
//  charstream[4*datalen] = '\r';
//  charstream[4*datalen+1] = '\n'; 
//  uart1_Tx(4*datalen+2,charstream);
////  while(idelay--)
////  while(uartRMsg.rxOK != TRUE)
////  {
////    get_UART1_data();
////  }
//  _EINT();
//  if(!idelay)
//  {
//    res = ERR_ACK;  //超时
//  }
//  if(uartRMsg.rxOK == TRUE)
//  {
//    res = hdl_nsost(); // 响应处理
//  }
//  
//  return res;
//}
//
///*================================================================
//【名 称】 nb_msg_rx
//【功 能】 消息接收
//【备 注】 datastream为接收数据，返回消息接收状态
//================================================================*/
//uint8_t nb_msg_rx(void)
//{
//  uint8_t r = ERR_NONE;
//
//  return r;
//}
//
//
///*================================================================
//【名 称】 nb_cmd_tx
//【功 能】 指令发送并处理
//【备 注】 
//================================================================*/
//uint8_t nb_cmd_tx(uint8_t* str_at, uint8_t (*callback)())
//{
//  uint8_t res;
//  uint32_t idelay = 30000;
//  
//  // 发送指令
//  _DINT();
//  uartRMsg.rxOK = FALSE;
//  uart1_Tx(strlen((const char*)str_at),str_at);  
//  while(idelay--)
//  {
//    get_UART1_data();
//  }
//  _EINT();
//  if(!idelay)
//  {
//    res = ERR_ACK;  //超时
//  }
//  if(uartRMsg.rxOK == TRUE)
//  {
//    res = callback(); // 响应处理
//  }
//  
//  return res;
//}
//
///*================================================================
//【名 称】 hdl_cmd
//【功 能】 响应处理函数
//【备 注】 模块返回OK\ERROR
//================================================================*/
//uint8_t hdl_cmd(void)
//{
//  if(uartRMsg.rxIndex>6)
//    return ERR_ACK;
//  if(uartRMsg.rxbuf[2] == 'O' && uartRMsg.rxbuf[3] == 'K')
//    return ERR_NONE;
//  else 
//    return ERR_ACK;
//}
//
///*================================================================
//【名 称】 hdl_nsocr
//【功 能】 响应处理函数
//【备 注】 模块返回OK\ERROR
//================================================================*/
//uint8_t hdl_nsocr(void)
//{
//  if(uartRMsg.rxIndex!=11)
//    return ERR_ACK;
//  if(uartRMsg.rxbuf[7] == 'O' && uartRMsg.rxbuf[8] == 'K')
//  {
//    nb_i.socket = uartRMsg.rxbuf[2]-0x30;
//    return ERR_NONE;
//  }  
//  else 
//  {
//    return ERR_ACK;
//  }
//}
//
///*================================================================
//【名 称】 hdl_nsost
//【功 能】 响应处理函数
//【备 注】 模块返回OK\ERROR
//================================================================*/
//uint8_t hdl_nsost(void)
//{
//  uint8_t res,i,socket_t;
//  uint32_t idelay = 0x8000;
//  if(uartRMsg.rxIndex<12)
//    return ERR_ACK;
//  socket_t = uartRMsg.rxbuf[2];
//  for(i=4;uartRMsg.rxbuf[i]!='\r';i++);
//  if(uartRMsg.rxbuf[i+4] == 'O' && uartRMsg.rxbuf[i+5] == 'K')  // 成功发送数据
//  {
////      while(idelay--)
////    while(uartRMsg.rxOK != TRUE)
////    {
////      get_UART1_data(); // 等待接收
////    }
////    _EINT();
////    if(!idelay)
////    {
////      res = ERR_ACK;  // 超时重发
////    }
////    if(uartRMsg.rxOK == TRUE)
////    {
////      uint8_t* pt = NULL;
////      uint8_t buf[20]={0};
////      
////      if(pt = (uint8_t*)strstr((const char*)uartRMsg.rxbuf,"+NSONMI"))  // 获取下行消息 +NSONMI:0，6
////      {
////        pt+=8;
////        if(socket_t==*pt) // 同一socket
////        {
////          pt+=2;
////          sprintf((char*)buf,"AT+NSORF=%d,%d",socket_t);
////          do{
////            res = nb_cmd_tx(buf,hdl_nsorf); //解析响应，若获取为空则退出
////          }while(res);
////        }
////        else
////        {
////          res = ERR_ACK; // 非同一socket
////        }
////      
////      }
////      else
////      {
////        res = ERR_ACK;  // 错误响应重发
////      }
////    }
//    return ERR_NONE;
//  }  
//  else 
//  {
//    return ERR_ACK;
//  }
//}
//
///*================================================================
//【名 称】 hdl_nsorf
//【功 能】 响应处理函数
//【备 注】 模块返回OK\ERROR
//================================================================*/
//uint8_t hdl_nsorf(void)
//{
//  return 0;
//}
//
///*================================================================
//【名 称】 hdl_cclk
//【功 能】 响应处理函数
//【备 注】 模块返回基站时钟
//================================================================*/
//uint8_t tstamp[6];
//uint8_t hdl_cclk(void)
//{
//  uint8_t *pt = NULL;
//  uint8_t i;
//  pt = (uint8_t*)strstr((char*)uartRMsg.rxbuf,"+CCLK");
//  if(!pt)
//    return ERR_ACK;
//  pt += strlen("+CCLK:");
//  for(i=0;i<17;)
//  {
//    tstamp[i/3]=10*(*pt)+*(pt+1);
//    i+=3;
//    pt+=3;
//  }
//  return ERR_NONE;
//}
//
//
///*================================================================
//【名 称】 hdl_nband
//【功 能】 响应处理函数
//【备 注】 模块返回当前频段
//================================================================*/
//uint8_t hdl_nband(void)
//{
//  uint8_t *pt = NULL;
//  uint8_t i,band = 0;
//  
//  pt = (uint8_t*)strstr((char*)uartRMsg.rxbuf,"+NBAND:");
//  if(pt == NULL)
//    return ERR_OTHER;
//  
//  pt += strlen("+NBAND:");
//
//  for(i=0;pt[i]!='\r';i++)
//    band = band + (pt[i]-48);
//  
//  nb_i.nband = band;
//  
//  return ERR_NONE;
// }
//
///*================================================================
//【名 称】 hdl_cgatt
//【功 能】 响应处理函数
//【备 注】 模块返回附网状态
//================================================================*/
//uint8_t hdl_cgatt(void)
//{
//  uint8_t r, *pt = NULL;
//
//  pt = (uint8_t*)strstr((char*)uartRMsg.rxbuf,"+CGATT:");
//  if(pt == NULL)
//    return ERR_OTHER; 
//  
//  pt += strlen("+CGATT:");
//    r = *pt-48;
// 
//  if(r!=1 && r!=0)
//    r = ERR_ACK;
//  
//  nb_i.attach = r;
//  
//  return ERR_NONE;
//}
//
///*================================================================
//【名 称】 hdl_cereg
//【功 能】 响应处理函数
//【备 注】 模块返回注网状态
//================================================================*/
//uint8_t hdl_cereg(void)
//{
//  uint8_t r, *pt = NULL;
//
//  pt = (uint8_t*)strstr((char*)uartRMsg.rxbuf,"+CEREG:");
//  if(pt == NULL)
//    return ERR_OTHER; 
//  
//  pt += strlen("+CEREG:0,");
//  if(*pt<48 || *pt>57)
//    r = ERR_ACK;
//  else 
//    r = *pt-48;
//  
//  nb_i.cereg = r;
//  
//  return ERR_NONE;
//}
//
///*================================================================
//【名 称】 hdl_cfun
//【功 能】 响应处理函数
//【备 注】 设备返回射频状态
//================================================================*/
//uint8_t hdl_cfun(void)
//{
//  uint8_t r;
//  uint8_t *pt = NULL;
//
//  pt = (uint8_t*)strstr((char*)uartRMsg.rxbuf,"+CFUN:");
//  if(pt == NULL)
//    return ERR_OTHER; 
//  
//  pt += strlen("+CFUN:");
//  if(*pt != 48 && *pt != 49)
//    r = ERR_ACK;
//  else
//    r = *pt-48;
//  
//  nb_i.cfun = r;
//  
//  return r;  
//}
//
//
///*================================================================
//【名 称】 hdl_csq
//【功 能】 响应处理函数
//【备 注】 模块返回信号值
//================================================================*/
//uint8_t hdl_csq(void)
//{
//  uint8_t i,r = 0,*pt = NULL;
//  
//  pt = (uint8_t*)strstr((char*)uartRMsg.rxbuf,"+CSQ:");
//  if(pt == NULL)
//    return ERR_OTHER;    
//  
//  pt += strlen("+CSQ:");
//  for(i=0;pt[i]!=',';i++)
//    r = 10*r + (pt[i]-48);
//  
//  nb_i.csq = r;
//  
//  return ERR_NONE;
//}
