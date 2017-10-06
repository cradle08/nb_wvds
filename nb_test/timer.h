#ifndef _TIMER_H_
#define _TIMER_H_

#define CLOCK_CONF_SECOND 128

typedef void (*tmr_fnct_ptr) (void);
typedef u8 TMR_STATE;

typedef struct{// 在堆中创建软件定时器*****************************8
  u32           expire;         /* 定时器到期时间 */
  u32           period;         /* 定时时间       */
  TMR_STATE     state;          /* 定时器状态     */
  u32           opt;		        /* 操作类型       */
  tmr_fnct_ptr  callback;       /* 回调函数       */
  
  u8            timeup;         /* 到时标志  */
}timer_t;


#define   TIMER_LIST_MAX        	5U      /* 计时器列表大小，支持5个软件定时器 */
 
/* timer state */
#define   TMR_STATE_UNUSED        (TMR_STATE)(0u)
#define   TMR_STATE_STOPPED       (TMR_STATE)(1u)
#define   TMR_STATE_RUNNING       (TMR_STATE)(2u)
#define   TMR_STATE_COMPLETED     (TMR_STATE)(3u)

#define  OPT_TMR_NONE             (0u)  /* No option selected */
#define  OPT_TMR_ONE_SHOT         (1u)  /* Timer will not auto restart when it expires */
#define  OPT_TMR_PERIODIC         (2u)  /* Timer will auto restart when it expires */


void TA0_init(void);
u8 timer_create(timer_t  *ptimer,u32 period,u32 opt,tmr_fnct_ptr callback);
u8 timer_set(timer_t *ptimer, u32 period);
u8 timer_del(timer_t *ptimer);
u8 timer_start(timer_t *ptimer);
u8 timer_cancel(timer_t *ptimer);
void timer_ev_poll(void);



extern timer_t *timer_list[];


#endif /* _TIMER_H_ */