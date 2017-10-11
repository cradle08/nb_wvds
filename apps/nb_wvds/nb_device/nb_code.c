#include "contiki.h"
#include "platform-conf.h"
#include "app.h"
#include "nb_code.h"



PROCESS(sendmsg_process, "SNEDMSG");

extern struct BASE_ARG base_arg;
extern struct ALGO algo;  // algorithm parameters


/* handle updata*/
void handle_updata(struct RECV_MSG* pr)
{
 
}

/* */
void handle_app(struct RECV_MSG* pr)
{

}



/* */
void handle_reboot(struct RECV_MSG* pr)
{
   // create reboot ack msg and then reboot device after delay
   struct SEND_MSG* pmem = memb_alloc(&send_msg_mem);
   if(pmem == NULL){
   
      // mem full,
   } else {
     pmem->msg_head.startflag   = 0xAA;
     pmem->msg_head.msglen      = 11;
     pmem->msg_head.cmd         = CMD_REBOOT_ACK;
 //    pmem->msg_head.devno       = 11;
     struct msg_reboot_ack* pra = (struct msg_reboot_ack*) pmem->playdata;
//     pra->tstamp                = 11;
     pra->result                = 11;
//     pmem->msg_tail.crc         = 11;
     pmem->msg_tail.endflag     = 0xFF;
     // add to send list
     if(list_length(send_msg_list) < SEND_MSGMEM_NUM)
     {
        list_add(send_msg_list, pmem);
        process_poll(&sendmsg_process); 
     }
     // start ctimer to reboot device after a while
     
   }
}



/* */
void handle_run_line(struct RECV_MSG* pr)
{
// 
  
}


///* base param */
void handle_base_param(struct RECV_MSG* pr)
{
//  uint8_t len                = pr->msg_head.msglen;
//  struct msg_base_param* pbp = (struct msg_base_param*) pr->playdata;
//  uint8_t oper               = pbp->operation;
//  if(oper == 1) // query
//  {
//    // do it at next steps
//  }else if(oper == 2){ // set
//    base_arg.hb_period = pbp->hb_period;    
//    base_arg.health_period = pbp->health_period; 
//    base_arg.batvoltage_thresh = pbp->batvoltage_thresh;
//    base_arg.batquantity_thresh = pbp->batquantity_thresh;     
//  }
//  
//  // create msg base param ack msg and add to send msg list 
//  struct SEND_MSG* ptemp = (struct SEND_MSG*)memb_alloc(&send_msg_mem);
//  if(ptemp == NULL){
//
//  }else{
//    ptemp->msg_head.startflag   = 0xAA;
////    ptemp->msg_head.msglen      = 11;
//    struct msg_base_param* pbp2 = (struct msg_base_param*)(ptemp->playdata);
////    pbp2->operation             = oper;
//    pbp2->health_period         = base_arg.health_period;
//    pbp2->batvoltage_thresh     = base_arg.health_period;
//    pbp2->batquantity_thresh    = base_arg.batquantity_thresh;
////    ptemp->msg_tail.crc         = 11;
//    ptemp->msg_tail.endflag     = 0xFF;
//    // add to send list and up a event
//    if(list_length(send_msg_list) < SEND_MSGMEM_NUM){
//      list_push(send_msg_list, ptemp);
//      process_poll(&sendmsg_process);  
//    }
//  }
}
//
//
//
///* */
void handle_algo_param(struct RECV_MSG* pr)
{
//    uint8_t len                = pr->msg_head.msglen;
//    struct msg_algo_param* pap =  (struct msg_algo_param*)pr->playdata;
//    uint8_t oper               = pap->operation;
//    if(oper == 1){ // query
//    
//    } else if(oper == 2){ // set
//      algo.normalT = pap->normal_period;
//      algo.flunctT  = pap->flunc_period;
//      algo.big_occ_thresh = pap->big_occthresh;
//      algo.mid_occ_thresh = pap->mid_occthresh;
//      algo.litt_occ_thresh = pap->lit_occthresh;
//      uint8_t i = 0;
//      for(i = 0; i < AXIS_NUM; i++) // 3
//      {
//        algo.base_line[i] = pap->baseline[i];
//      }
//    }
//    // create algo param ack msg and add to send list, up a event
//    struct SEND_MSG* ptemp = memb_alloc(&send_msg_mem);
//    if(ptemp == NULL){
//      // send mem is full
//    }else{
//      ptemp->msg_head.startflag   = 0xAA;
//      ptemp->msg_head.msglen      = 11;
//      struct msg_algo_param* pap2 = (struct msg_algo_param*) ptemp->playdata;           
//      pap2->normal_period         = algo.normalT;
//      pap2->flunc_period          = algo.flunctT;
//      pap2->big_occthresh         = algo.big_occ_thresh;
//      pap2->mid_occthresh         = algo.mid_occ_thresh;
//      pap2->lit_occthresh         = algo.litt_occ_thresh;
////      ptemp->crc                  = 11;
//      ptemp->msg_tail.endflag              = 0xFF;
//      if(list_length(send_msg_list) < SEND_MSGMEM_NUM){
//        list_push(send_msg_list, ptemp);
//        process_poll(&sendmsg_process);
//      }
//    }
}

/* */
void handle_hb_ack(struct RECV_MSG* pr)
{
  
}

/* */
void handle_alarm_ack(struct RECV_MSG* pr)
{

}

/* */
void handle_check_ack(struct RECV_MSG* pr)
{

}


/*  handle mag change ack */
void handle_mag_change_ack(struct RECV_MSG* pr)
{


}

/* */
void handle_recv_msg(struct RECV_MSG* p)
{
  struct RECV_MSG* pnext = p;
  if (pnext->msg_head.cmd == CMD_APP) {
    handle_app(pnext);
  } else if (pnext->msg_head.cmd == CMD_UPDATA) {
    handle_updata(pnext);
  } else if (pnext->msg_head.cmd == CMD_REBOOT) {
    handle_reboot(pnext);
  } else if (pnext->msg_head.cmd == CMD_RUN_LINE) {
    handle_run_line(pnext);
  } else if (pnext->msg_head.cmd == CMD_ALGO_PARAM) {
    handle_algo_param(pnext);
  } else if (pnext->msg_head.cmd == CMD_BASIC_PARAM) {
    handle_base_param(pnext);
  } else if (pnext->msg_head.cmd == CMD_HB_ACK) {
    handle_hb_ack(pnext);
  } else if (pnext->msg_head.cmd == CMD_ALARM_ACK) {
    handle_alarm_ack(pnext);
  } else if (pnext->msg_head.cmd == CMD_CHECK_ACK) {
    handle_check_ack(pnext);
  } else if (pnext->msg_head.cmd == CMD_MAG_CHANGE_ACK) {
//     handle_mag_change_ack(pnext);
  }
}


/*
part 
*/

/* */
//void handle_app_ack(struct SEND_MSG* ps)
//{
//  
//}
//
/* */
//void handle_updata_ack(struct SEND_MSG* ps)
//{
//
//}
//
////**
//void handle_reboot_ack(struct SEND_MSG* ps)
//{
//
//}
//
/* */
//void handle_run_line_ack(struct SEND_MSG* ps)
//{
//
//}
//
///* */
//void handle_basic_param_ack(struct SEND_MSG* ps)
//{
//
//
//}
//
/////* */
//void handle_algo_param_ack(struct SEND_MSG* ps)
//{
//
//}
//
/////* */
//void handle_hb(struct SEND_MSG* ps)
//{
//
//}
//
////**
//void handle_alarm(struct SEND_MSG* ps)
//{
//
//}
//
////**
//void handle_check(struct SEND_MSG* ps)
//{
//
//}
//
////**
//void handle_mag_change(struct SEND_MSG* ps)
//{
//
//}

///* */
void handle_send_msg(struct SEND_MSG* p)
{  
  app_send_msg(p);
  
//  struct SEND_MSG* pnext = p;
//  if(pnext->msg_head.cmd == CMD_HB){
//    handle_hb(pnext);
//  } else if (pnext->msg_head.cmd == CMD_CHECK){
//    handle_check(pnext);
//  } else if (pnext->msg_head.cmd == CMD_ALARM){
//    handle_alarm(pnext);
//  } else if (pnext->msg_head.cmd == CMD_MAG_CHANGE){
//    handle_mag_change(pnext);
//  } else if (pnext->msg_head.cmd == CMD_APP_ACK){
//    handle_app_ack(pnext);
//  } else if (pnext->msg_head.cmd == CMD_REBOOT_ACK){
//    handle_reboot_ack(pnext);
//  } else if (pnext->msg_head.cmd == CMD_UPDATA_ACK){
//      handle_updata_ack(pnext);
//  } else if (pnext->msg_head.cmd == CMD_RUN_LINE_ACK){
//    handle_run_line_ack(pnext); 
//  } else if (pnext->msg_head.cmd == CMD_BASE_PARAM_ACK){
//    handle_basic_param_ack(pnext);
//  } else if (pnext->msg_head.cmd == CMD_ALGO_PARAM_ACK){
//    handle_algo_param_ack(pnext);
//  }

}



/* */
PROCESS_THREAD(sendmsg_process, ev, data)
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
        memset(psm, 0, sizeof(struct SEND_MSG));
        
      } else {
        // send msg list is empty, not need handle
      }
    }
  }
   


  PROCESS_END();
}