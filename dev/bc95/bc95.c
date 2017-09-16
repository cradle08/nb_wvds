#include "system.h"
#include "bc95.h"
#include "timer.h"
#include "app.h"


//-------------------------------------------//
#define NB_AUTOCONNECT 0

#define REPEAT_MAX 10

#define BIT_SCRAMBLING  0x01
#define BIT_SI_AVOID    0x02
#define BIT_AUTOCONNECT 0x04

//-------------------------------------------//
u8 hdl_cmd(void);
u8 hdl_nconfig_req(void);
u8 hdl_nband_req(void);
u8 hdl_csq(void);
u8 hdl_cgsn(void);
u8 hdl_cfun_req(void);
u8 hdl_imsi(void);
u8 hdl_cscon_req(void);
u8 hdl_cereg_req(void);
u8 hdl_cgatt_req(void);
u8 hdl_cclk(void);
u8 hdl_nqmgr(void);
u8 hdl_nmgr(void);
//-------------------------------------------//
u8 bc95_nmgr_func(void);
u8 bc95_nqmgr_func(void);
u8 bc95_nmgs_func(void);
u8 bc95_nqmgs_func(void);

//-------------------------------------------//
bc95_info bc95_i;
bc95_smsg bc95_s;
bc95_stu bc95_t;

static bc95_callback_t bc95_rec_cb=NULL;
//-------------------------------------------//
void bc95_set_callback(bc95_callback_t cback)
{
  bc95_rec_cb = cback;
}

/*================================================================
���� �ơ� bc95_hw_init
���� �ܡ� ģ��Ӳ����ʼ��
���� ע�� 9600BAUD
================================================================*/
u8 nb_hw_init(void)
{
  uart1_init(9600);
  
#if (HW_VER==0)
  BC95_ON
#endif 
    
#if (HW_VER==1)
  P5DIR |= BIT3; //���
  P5SEL &= ~BIT3;

  P4DIR &= ~BIT7; //����
  P4SEL &= ~BIT7;
#endif
  delay_ms(500); //Module is powered on, wait for 3 seconds
  
  return TRUE;
}


/*================================================================
���� �ơ� nb_sft_init
���� �ܡ� NB����
���� ע��
================================================================*/
u8 nb_sft_init(void)
{
  u8 r;
  
  /* band 5 setting */
  nb_cmd_tx(NBAND_REQ,"AT+NBAND?\r\n",hdl_nband_req);
  if(BAND != bc95_i.NBAND)
  {
    nb_cmd_tx(NBAND_SET,"AT+NBAND=5\r\n",hdl_cmd);
    nb_reboot(); 
  }
  /* autoconnect off */
  nb_cmd_tx(NCONFIG_REQ,"AT+NCONFIG?\r\n",hdl_nconfig_req);
  if(1 != bc95_i.AUTOCONNECT || 1 != bc95_i.SCRAMBLING || 1 != bc95_i.SI_AVOID)
  {
    nb_cmd_tx(NCONFIG_SET,"AT+NCONFIG=AUTOCONNECT,TRUE\r\nAT+NCONFIG=CR_0354_0338_SCRAMBLING,TRUE\r\nAT+NCONFIG=CR_0859_SI_AVOID,TRUE\r\n",NULL);
    nb_reboot();
  }
 
  do{
    r = nb_cmd_tx(CSQ,"AT+CSQ\r\n",hdl_csq);  // signal strength
  }while(bc95_i.CSQ==99 || bc95_i.CSQ==0);
  
  /* attempt to attach net */
  do{
    nb_att_net_req(); // wait untill attach
  }while(!bc95_i.ATTACH);
  
  do{
    nb_reg_net_req(); // register base station 
  }while(!bc95_i.CEREG); 
  
  return r;
}

/*================================================================
���� �ơ� nb_att_net_req
���� �ܡ� ��ѯģ���Ƿ�������
���� ע�� 
================================================================*/
u8 nb_att_net_req(void)
{
  u8 r = ERR_NONE;
  r = nb_cmd_tx(CGATT_REQ,"AT+CGATT?\r\n",hdl_cgatt_req); 
  return r;
}

/*================================================================
���� �ơ� nb_att_net_req
���� �ܡ� ��ѯģ���Ƿ�ע������
���� ע�� 
================================================================*/
u8 nb_reg_net_req(void)
{
  u8 r = ERR_NONE;
  r = nb_cmd_tx(CEREG_REQ,"AT+CEREG?\r\n",hdl_cereg_req);  
  return r;
}

/*================================================================
���� �ơ� bc95_radio_on
���� �ܡ� ģ������
���� ע�� 
================================================================*/
u8 nb_radio_on(void)
{
  u8 r = ERR_NONE;
  
  nb_cmd_tx(CFUN_REQ,"AT+CFUN?\r\n",hdl_cfun_req);
  if(1 != bc95_i.CFUN)
  {
    delay_ms(1000);
    do{
    uart1_Tx(strlen((const char*)"AT+CFUN=1\r\n"),"AT+CFUN=1\r\n"); 
    while(uartRMsg.rxOK != TRUE);
    uartRMsg.rxOK = FALSE;
    r = hdl_cmd();
    }while(r==ERR_ACK);
    bc95_i.CFUN = 1;
  }
  delay_ms(4000);  
  return r;
}

/*================================================================
���� �ơ� bc95_radio_off
���� �ܡ� ģ������
���� ע�� ����״̬ connect\psm
================================================================*/
u8 nb_radio_off(void)
{
  u8 r = ERR_NONE;
  r = nb_cmd_tx(CFUN_CLR,"AT+CFUN=0\r\n",hdl_cmd); 
  return r;
}

/*================================================================
���� �ơ� nb_reboot
���� �ܡ� ģ������
���� ע�� ���������Ҫ2~3s
================================================================*/
u8 nb_reboot(void)
{
  uart1_Tx(strlen((const char*)"AT+NRB\r\n"),"AT+NRB\r\n"); 
  delay_ms(5000);
  uartRMsg.rxOK = FALSE;
  return ERR_NONE; 
}

/*================================================================
���� �ơ� nb_module_off
���� �ܡ� �ر�ģ��
���� ע�� 
================================================================*/
u8 nb_module_off(void)
{
  u8 r = ERR_NONE;
  r = nb_radio_off();
#if (HW_VER==0)  
  BC95_OFF
#endif    
#if (HW_VER==1)
  P5DIR |= BIT3; //���
  P5SEL &= ~BIT3;
#endif
  return r;
}

/*================================================================
���� �ơ� nb_dlbuf_req
���� �ܡ� �������ݻ�������
���� ע�� 
================================================================*/
u8 nb_dlbuf_req(void)
{
  u8 r = ERR_NONE;
  r = nb_cmd_tx(NQMGR,"AT+NQMGR\r\n",hdl_nqmgr); 
  return r;
}

/*================================================================
���� �ơ� nb_msg_tx
���� �ܡ� ��Ϣ����
���� ע�� datastreamΪ�������������������Ϣ����״̬
================================================================*/
u8 hex2char(u8 hex)
{
  u8 r;
  if(hex<0x0A)
  {
    r = hex+48;
  }
  else
  {
    r = (hex-0x0A)+65;
  }
  return r;
}

u8 nb_msg_tx(u16 datalen,u8* datastream)
{
  u16 len,i,j=0;
  u8 r = ERR_NONE;
  u8 charstream[BUF_SIZE] = {0};
  sprintf((char*)charstream,"AT+NMGS=%d,",datalen);
  i=strlen((char const*)charstream);
  len = i;
  for(j=0;j<=datalen;j++)
  {
    charstream[i++]=hex2char((datastream[j]>>4)&0x0F);
    charstream[i++]=hex2char(datastream[j]&0x0F);
  }
  charstream[i++] = '\r';
  charstream[i++] = '\n';  
//  r = nb_cmd_tx(NMGS,charstream,hdl_cmd);
  
  _DINT();
  uartRMsg.rxOK = FALSE;
  uart1_Tx(i,charstream);  
  // while((uartRMsg.rxOK != TRUE)&&(bc95_t.idelay--))
  while(uartRMsg.rxOK != TRUE)
  {
    get_UART1_data();
  }
  _EINT();
  r = hdl_cmd();
  
  return r;
}

/*================================================================
���� �ơ� nb_msg_rx
���� �ܡ� ��Ϣ����
���� ע�� datastreamΪ�������ݣ�������Ϣ����״̬
================================================================*/
u8 nb_msg_rx(void)
{
  u8 r = ERR_NONE;
  r = nb_cmd_tx(NMGR,"AT+NMGR\r\n",hdl_nmgr);
  return r;
}


//-------------------------------------------//

//-------------------------------------------//

/*================================================================
���� �ơ� nb_cmd_tx
���� �ܡ� ָ��Ͳ�����
���� ע�� 
================================================================*/
u8 nb_cmd_tx(u8 cmd,u8* str_at, u8 (*callback)())
{
  u8 r;

  bc95_t.cmd = cmd;
  bc95_t.res = ERR_NONE;
  bc95_t.ret = 0;
  bc95_t.idelay = 30000;
  bc95_t.callback = callback;
  
  do{
    
    _DINT();
    uartRMsg.rxOK = FALSE;
    uart1_Tx(strlen((const char*)str_at),str_at);  
   // while((uartRMsg.rxOK != TRUE)&&(bc95_t.idelay--))
    while(uartRMsg.rxOK != TRUE)
    {
      get_UART1_data();
    }
    _EINT();
    
    if(!bc95_t.idelay)
    {
      r = ERR_ACK;
    }
    
    if(uartRMsg.rxOK == TRUE)
    {
      r = bc95_t.callback();
    }
    
    if(r >= ERR_OTHER)
    {
      bc95_t.ret++;
      if(bc95_t.ret>12)
      {
        return FALSE;
      }
    }    
    else 
    {
      bc95_t.ret = 0;
    }
    
  }while(bc95_t.ret);
  
  
  delay_ms(1000);
  
  return TRUE; 
}

/*================================================================
���� �ơ� hdl_cmd
���� �ܡ� ��Ӧ������
���� ע�� ģ�鷵��OK\ERROR
================================================================*/
u8 hdl_cmd(void)
{
  if(uartRMsg.rxIndex>6)
    return ERR_ACK;
  if(uartRMsg.rxbuf[2] == 'O' && uartRMsg.rxbuf[3] == 'K')
    return ERR_NONE;
  else 
    return ERR_ACK;
}

/*================================================================
���� �ơ� hdl_nmgr
���� �ܡ� ��Ӧ������
���� ע�� [��ʼ�� ���ݳ���] [�豸���� ������ ��Ϣ����] [У���� ������]
================================================================*/
extern struct ALGO algo;
u8 hdl_nmgr(void)
{
  u16 i,j;
  u8 len=0,type=0;
  u8 *pt = NULL,datastream[1024];

  // ��ȡ���ջ���������
  pt = uartRMsg.rxbuf;
  for(i=0;(pt[i]!='\r')&&(pt[i+1]!='\n')&&(i<1024);i++)
  {
    datastream[i] = uartRMsg.rxbuf[i];
    datastream[i+1] = 0;
  }
  // ��ȡ���ݳ���
  pt = datastream;
  for(i=0;(pt[i]!=',')&&(i<1024);i++)
  {
    len = 10*len+(pt[i]-48);
  }  
  // ��ȡ����
  j=i+1;

  for(i=0;i<2*len;i++,j++)
  {
    if(pt[j]>'9')
    {
      pt[i] = pt[j] - 55;
    }
    else if(pt[j]<'A')
    {
      pt[i] = pt[j] - 48;      
    }
  }
  
  // �ж�֡ͷ
  if(pt[0]!=0x0A && pt[1]!=0x0A)
  {
    return ERR_ACK;
  }
  // �ж� [ack or crtl msg] 
  if(pt[2]==0x03&&pt[3]==0x07)
  {
    type = pt[4];
    //
    return ERR_NONE;
  }
  else if(pt[2]!=0x04&&pt[3]!=0x07)
  {
    algo.normalT = (uint16_t)(pt[4]<<4+pt[5])+(pt[6]+pt[7]);        
    algo.flunctT = (uint16_t)(pt[8]<<4+pt[9])+(pt[10]+pt[11]);;   
    algo.big_occ_thresh = (uint16_t)(pt[12]<<4+pt[13])+(pt[14]+pt[15]);   
    algo.mid_occ_thresh = (uint16_t)(pt[16]<<4+pt[17])+(pt[18]+pt[19]);   
    algo.litt_occ_thresh = (uint16_t)(pt[20]<<4+pt[21])+(pt[22]+pt[23]);;  

    return ERR_NONE;  
  }
  
  if(bc95_rec_cb) 
  {
    bc95_rec_cb(datastream);
  }

  return ERR_NONE; 
}

/*================================================================
���� �ơ� hdl_cclk
���� �ܡ� ��Ӧ������
���� ע�� ģ�鷵�ػ�վʱ��
================================================================*/
u8 tstamp[6];
u8 hdl_cclk(void)
{
  u8 *pt = NULL;
  u8 i;
  pt = (u8*)strstr((char*)uartRMsg.rxbuf,"+CCLK");
  if(!pt)
    return ERR_ACK;
  pt += strlen("+CCLK:");
  for(i=0;i<17;)
  {
    tstamp[i/3]=10*(*pt)+*(pt+1);
    i+=3;
    pt+=3;
  }
  return ERR_NONE;
}


/*================================================================
���� �ơ� hdl_nqmgr
���� �ܡ� ��Ӧ������
���� ע�� ģ�鷵�ؽ������
================================================================*/
u8 hdl_nqmgr(void)
{
  u8 *pt = NULL;
  u8 i;
  
  pt = (u8*)strstr((char*)uartRMsg.rxbuf,"BUFFERED=");
  if(!pt)
    return ERR_ACK;
  pt += strlen("BUFFERED=");
  for(i=0;*(pt+i)!=',';i++)
    bc95_s.rBuffered = 10*bc95_s.rBuffered+(*(pt+i)-48); 
  
  return ERR_NONE;
}


/*================================================================
���� �ơ� hdl_imsi
���� �ܡ� ��Ӧ������
���� ע�� ģ�鷵��sim����
================================================================*/
u8 hdl_imsi(void)
{
  u8 i,*pt = uartRMsg.rxbuf+2;
  
  for(i=0;*(pt+i)!='\r';i++){
    if(*(pt+i)>57 || *(pt+i)<48)  // ������
      return ERR_ACK;
    bc95_i.IMSI[i] = *(pt+i);
  }
  
  return ERR_NONE;
}

/*================================================================
���� �ơ� hdl_nband_req
���� �ܡ� ��Ӧ������
���� ע�� ģ�鷵�ص�ǰƵ��
================================================================*/
u8 hdl_nband_req(void)
{
  u8 *pt = NULL;
  u8 i,band = 0;
  
  pt = (u8*)strstr((char*)uartRMsg.rxbuf,"+NBAND:");
  if(pt == NULL)
    return ERR_OTHER;
  
  pt += strlen("+NBAND:");

  for(i=0;pt[i]!='\r';i++)
    band = band + (pt[i]-48);
  
  bc95_i.NBAND = band;
  
  return ERR_NONE;
 }

/*================================================================
���� �ơ� hdl_cgatt_req
���� �ܡ� ��Ӧ������
���� ע�� ģ�鷵�ظ���״̬
================================================================*/
u8 hdl_cgatt_req(void)
{
  u8 r, *pt = NULL;

  pt = (u8*)strstr((char*)uartRMsg.rxbuf,"+CGATT:");
  if(pt == NULL)
    return ERR_OTHER; 
  
  pt += strlen("+CGATT:");
    r = *pt-48;
 
  if(r!=1 && r!=0)
    r = ERR_ACK;
  
  bc95_i.ATTACH = r;
  
  return ERR_NONE;
}

/*================================================================
���� �ơ� hdl_cereg_req
���� �ܡ� ��Ӧ������
���� ע�� ģ�鷵��ע��״̬
================================================================*/
u8 hdl_cereg_req(void)
{
  u8 r, *pt = NULL;

  pt = (u8*)strstr((char*)uartRMsg.rxbuf,"+CEREG:");
  if(pt == NULL)
    return ERR_OTHER; 
  
  pt += strlen("+CEREG:0,");
  if(*pt<48 || *pt>57)
    r = ERR_ACK;
  else 
    r = *pt-48;
  
  bc95_i.CEREG = r;
  
  return ERR_NONE;
}

/*================================================================
���� �ơ� hdl_cscon_req
���� �ܡ� ��Ӧ������
���� ע�� ģ�鷵�ػ�վ����״̬
================================================================*/
u8 hdl_cscon_req(void)
{
  u8 r, *pt = NULL;

  pt = (u8*)strstr((char*)uartRMsg.rxbuf,"+CSCON:");
  if(pt == NULL)
    return ERR_OTHER; 
  
  pt += strlen("+CSCON:0,");
  if(*pt != 48 && *pt != 49)
    r = ERR_ACK;
  else 
    r = *pt-48;
  
  bc95_i.CSCON = r;  
  
  return ERR_NONE;
}

/*================================================================
���� �ơ� hdl_cfun_req
���� �ܡ� ��Ӧ������
���� ע�� �豸������Ƶ״̬
================================================================*/
u8 hdl_cfun_req(void)
{
  u8 r;
  u8 *pt = NULL;

  pt = (u8*)strstr((char*)uartRMsg.rxbuf,"+CFUN:");
  if(pt == NULL)
    return ERR_OTHER; 
  
  pt += strlen("+CFUN:");
  if(*pt != 48 && *pt != 49)
    r = ERR_ACK;
  else
    r = *pt-48;
  
  bc95_i.CFUN = r;
  
  return r;  
}

/*================================================================
���� �ơ� hdl_cgsn
���� �ܡ� ��Ӧ������
���� ע�� ģ�鷵��ģ��ʶ���
================================================================*/
u8 hdl_cgsn(void)
{
  u8 i,*pt = NULL;  

  pt = (u8*)strstr((char*)uartRMsg.rxbuf,"+CGSN:");
  if(pt == NULL)
    return ERR_ACK; 
  pt += strlen("+CGSN:");
  
  for(i=0;*(pt+i)!='\r';i++)
    bc95_i.IMEI[i]=*(pt+i);
  
  return ERR_NONE; 
}

/*================================================================
���� �ơ� hdl_csq
���� �ܡ� ��Ӧ������
���� ע�� ģ�鷵���ź�ֵ
================================================================*/
u8 hdl_csq(void)
{
  u8 i,r = 0,*pt = NULL;
  
  pt = (u8*)strstr((char*)uartRMsg.rxbuf,"+CSQ:");
  if(pt == NULL)
    return ERR_OTHER;    
  
  pt += strlen("+CSQ:");
  for(i=0;pt[i]!=',';i++)
    r = 10*r + (pt[i]-48);
  
  bc95_i.CSQ = r;
  
  return ERR_NONE;
}

/*================================================================
���� �ơ� hdl_nconfig_req
���� �ܡ� ��Ӧ������
���� ע�� ģ�鷵��ģ������
================================================================*/
u8 hdl_nconfig_req(void)
{
  u8 *pt = NULL;

  pt = (u8*)strstr((char*)uartRMsg.rxbuf,"AUTOCONNECT,");
  if(pt == NULL)
    return ERR_ACK; 
  
  pt += strlen("AUTOCONNECT,");
  if(*pt == 'T')
    bc95_i.AUTOCONNECT = 1;
  else if(*pt == 'F')
    bc95_i.AUTOCONNECT = 0;
  else 
    return ERR_ACK;
 
  pt = (u8*)strstr((char*)pt,"CR_0354_0338_SCRAMBLING,") + strlen("CR_0354_0338_SCRAMBLING,");
  if(pt == NULL)
    return ERR_ACK;
  
  if(*pt == 'T')
    bc95_i.SCRAMBLING = 1;
  else if(*pt == 'F')
    bc95_i.SCRAMBLING = 0;
  else 
    return ERR_ACK;
  
  
  pt = (u8*)strstr((char*)pt,"CR_0859_SI_AVOID,") + strlen("CR_0859_SI_AVOID,");
  if(pt == NULL)
    return ERR_ACK;    
  
  if(*pt == 'T')
    bc95_i.SI_AVOID = 1;
    else if(*pt == 'F')
    bc95_i.SI_AVOID = 0;
  else 
    return ERR_ACK;
  
  return ERR_NONE; 
}