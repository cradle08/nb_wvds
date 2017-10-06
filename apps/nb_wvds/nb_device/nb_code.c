#include "contiki.h"
#include "platform-conf.h"
#include "app.h"
#include "nb_code.h"

PROCESS(SendMSG_Process, "SNEDMSG_PROCESS");

extern struct BASE_ARG base_arg;



//**
struct RECV_MSG* get_available_recv_memb()
{
  if(list_length(recv_msg_list) < RECV_MSGMEM_NUM)
  {
    struct RECV_MSG* rm = memb_alloc(&recv_msg_mem);
    return rm;
  }
  return NULL;
}

//**
struct SNED_MSG* get_available_send_memb()
{
  if(list_length(send_msg_list) < SEND_MSGMEM_NUM)
  {
    struct SEND_MSG* sm = memb_alloc(&send_msg_mem);
    return sm;
  }
  return NULL;
}

//**
void msg_updata(struct RECV_MSG* pr)
{
  // ...
}

//**
void msg_app(struct RECV_MSG* pr)
{

}



//**
void msg_reboot(struct RECV_MSG* pr)
{
   // create reboot ack msg and then reboot device after delay
   struct SEND_MSG* pmem = memb_alloc(&send_msg_mem);
   if(pmem == NULL){
   
      // mem full,
   } else {
//     pmem->msg_head.startflag   = 0xAA;
//     pmem->msg_head.msglen      =
//     pmem->msg_head.cmd         = CMD_REBOOT_ACK;
//     pmem->msg_head.devno       = 
//     struct msg_reboot_ack* pra = (struct msg_reboot_ack*)pmem->playdata;
//     pra->tstamp                = 
//     pra->result                = 
//     pmem->msg_tail.crc         =
//     pmem->msg_tail.endflag     = 0xFF;
     // add to send list
     if(list_length(send_msg_list) < SEND_MSGMEM_NUM)
     {
        list_add(send_msg_list, pmem);
        process_poll(&SendMSG_Process); 
     }
     // start ctimer to reboot device after a while
     //...
}

//**
void msg_run_line(struct RECV_MSG* pr)
{
// 
}

//**
void msg_basic_param(struct RECV_MSG* pr)
{
  // uint8_t operation;  // 01-query, 02-set  
  // parse basic param 
  uint8_t len = pr->msg_head.msglen;
  struct msg_base_param* pbp = (struct msg_base_param*)pr->playdata;
  if(pbp->operation == 1) // query
  {
    // do it at next steps
  }else if(pbp->operation == 2){ // set
    if(pbp->hb_period != base_arg.hb_period){ // after next hb timer expired, new hb_period will available
       base_arg.hb_period = pbp->hb_period;    
     }else if(pbp->health_period != base_arg.health_period ){ // after next timer expired, new health_period will available
        base_arg.health_period = pbp->health_period; 
     }else if(pbp->batvoltage_thresh != base_arg.batvoltage_thresh){      
        base_arg.batvoltage_thresh = pbp->batvoltage_thresh;
     }else if(pbp->batquantity_thresh != base_arg.batquantity_thresh){
        base_arg.batquantity_thresh = pbp->batquantity_thresh;
     }
  }
  // create msg base param ack msg and add to send msg list
  struct SEND_MSG* ptemp = memb_alloc(&send_msg_mem);
  if(ptemp == NULL){
    // send mem is full
  }else{
    
  }
  
}

//**
void msg_algo_param(struct RECV_MSG* pr)
{


}

//**
void msg_hb_ack(struct RECV_MSG* pr)
{

}

//**
void msg_alarm_ack(struct RECV_MSG* pr)
{

}

//**
void msg_check_ack(struct RECV_MSG* pr)
{

}

//**
void msg_mag_change_ack(struct RECV_MSG* pr)
{

}

//**
void handle_recv_msg(struct RECV_MSG* p)
{
  struct RECV_MSG* pnext = p;
  if(pnext->msg_head.cmd == CMD_APP) {
    msg_app(pnext);
  } else if(pnext->msg_head.cmd == CMD_UPDATA){
    msg_updata(pnext);
  } else if(pnext->msg_head.cmd == CMD_REBOOT){
    msg_reboot(pnext);
  } else if(pnext->msg_head.cmd == CMD_RUN_LINE){
    msg_run_line(pnext);
  } else if(pnext->msg_head.cmd == CMD_ALGO_PARAM){
    msg_algo_param(pnext);
  } else if(pnext->msg_head.cmd == CMD_BASIC_PARAM){
    msg_basic_param(pnext);
  } else if(pnext->msg_head.cmd == CMD_HB_ACK){
    msg_hb_ack(pnext);
  } else if(pnext->msg_head.cmd == CMD_ALARM_ACK){
    msg_alarm_ack(pnext);
  } else if(pnext->msg_head.cmd == CMD_CHECK_ACK){
    msg_check_ack(pnext);
  } else if(pnext->msg_head.cmd == CMD_MAG_CHANGE_ACK){
    msg_mag_change_ack(pnext);
  }             
}





//**
void msg_app_ack(struct SEND_MSG* ps)
{

}

//**
void msg_updata_ack(struct SEND_MSG* ps)
{

}

//**
void msg_reboot_ack(struct SEND_MSG* ps)
{

}

//**
void msg_run_line_ack(struct SEND_MSG* ps)
{

}

//**
void msg_basic_param_ack(struct SEND_MSG* ps)
{


}

//**
void msg_algo_param_ack(struct SEND_MSG* ps)
{

}

//**
void msg_hb(struct SEND_MSG* ps)
{

}

//**
void msg_alarm(struct SEND_MSG* ps)
{

}

//**
void msg_check(struct SEND_MSG* ps)
{

}

//**
void msg_mag_change(struct SEND_MSG* ps)
{

}

//**
void handle_send_msg(struct SEND_MSG* p)
{  
  struct SEND_MSG* pnext = p;
  if(pnext->msg_head.cmd == CMD_HB){
    msg_hb(pnext);
  } else if (pnext->msg_head.cmd == CMD_CHECK){
    msg_check(pnext);
  } else if (pnext->msg_head.cmd == CMD_ALARM){
    msg_alarm(pnext);
  } else if (pnext->msg_head.cmd == CMD_MAG_CHANGE){
    msg_mag_change(pnext);
  } else if (pnext->msg_head.cmd == CMD_APP_ACK){
    msg_app_ack(pnext);
  } else if (pnext->msg_head.cmd == CMD_REBOOT_ACK){
    msg_reboot_ack(pnext);
  } else if (pnext->msg_head.cmd == CMD_UPDATA_ACK){
      msg_updata_ack(pnext);
  } else if (pnext->msg_head.cmd == CMD_RUN_LINE_ACK){
    msg_run_line_ack(pnext); 
  } else if (pnext->msg_head.cmd == CMD_BASE_PARAM_ACK){
    msg_basic_param_ack(pnext);
  } else if (pnext->msg_head.cmd == CMD_ALGO_PARAM_ACK){
    msg_algo_param_ack(pnext);
  }
}



//**
PROCESS_THREAD(SendMSG_Process, ev, data)
{
  PROCESS_BEGIN()
    
    
  while(1)
  {
    PROCESS_WAIT_EVENT();
    if(ev == PROCESS_EVENT_POLL)
    {
      // hande send msg form send_msg_list
      if(list_length(send_msg_list) > 0)
      {
        struct SEND_MSG* psm = list_pop(send_msg_list);
        handle_send_msg(psm);
      } else {
        // send msg list is empty, not need handle
      }
    }
  }
   



  PROCESS_END();
}


