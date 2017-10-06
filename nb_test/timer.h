#ifndef _TIMER_H_
#define _TIMER_H_

#define CLOCK_CONF_SECOND 128

typedef void (*tmr_fnct_ptr) (void);
typedef u8 TMR_STATE;

typedef struct{// �ڶ��д��������ʱ��*****************************8
  u32           expire;         /* ��ʱ������ʱ�� */
  u32           period;         /* ��ʱʱ��       */
  TMR_STATE     state;          /* ��ʱ��״̬     */
  u32           opt;		        /* ��������       */
  tmr_fnct_ptr  callback;       /* �ص�����       */
  
  u8            timeup;         /* ��ʱ��־  */
}timer_t;


#define   TIMER_LIST_MAX        	5U      /* ��ʱ���б��С��֧��5�������ʱ�� */
 
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