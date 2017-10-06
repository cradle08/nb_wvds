#include "system.h"
#include "timer.h"

#define INTERVAL  128          /* 1/128(s) */
//64*CLOCK_CONF_SECOND 1s

u32 seconds;
static u32 timer_tick;      /* �����ʱ��ʱ����1 timer_tick = 1(s) */
static volatile u32 timer_count = 0; /* �Ѿ�ʹ�õ������ʱ������ */
static timer_t *timer_list[TIMER_LIST_MAX] = {NULL};


static void timer_cleanup(timer_t *ptimer);
static u8 timer_link(timer_t *ptimer);
static u8 timer_unlink(timer_t *ptimer);

/*================================================================
���� �ơ�u8 timer_create()
���� �ܡ������ʱ������
���� ע��period-��ʱʱ��(s) opt-���ڲ��� callback-�ص����� 
================================================================*/
u8 timer_create(timer_t *ptimer,	
                      u32 period,
                      u32 opt,
                      tmr_fnct_ptr  callback)
{
  if(!ptimer)
    return  0;
  ptimer->expire = 0;
  ptimer->period = period;
  ptimer->opt = opt;
  ptimer->state = TMR_STATE_STOPPED;
  ptimer->callback = callback;
  ptimer->timeup = 0;
  return 1;
}

/*================================================================
���� �ơ�u8 timer_set()
���� �ܡ������ʱ������
���� ע��period-��ʱʱ��(s
================================================================*/
u8 timer_set(timer_t *ptimer, u32 period)                   
{
  if(!ptimer)
    return  0;
  ptimer->period = period;
  ptimer->timeup = 0;
  return 1;
}			 

/*================================================================
���� �ơ�u8 timer_del()
���� �ܡ������ʱ��ɾ��
���� ע��ɾ��һ����ʱ�����ͷż�ʱ���ṹռ�ݵ��ڴ�
================================================================*/
u8 timer_del(timer_t *ptimer)
{
  if(!ptimer)
    return 0;
  timer_cleanup(ptimer);
	//free();
  ptimer = NULL;
  return 1;
}


 /*================================================================
���� �ơ�u8 timer_start()
���� �ܡ������ʱ������
���� ע����ʱ���ڴ˲�����ɺ�ʼ����
================================================================*/
u8 timer_start(timer_t *ptimer)
{
  if(!ptimer)
    return 0;

  switch(ptimer->state)
  {
    case TMR_STATE_RUNNING:
      timer_unlink(ptimer);
      timer_link(ptimer);
      return 1;
    case TMR_STATE_STOPPED:
    case TMR_STATE_COMPLETED:
      timer_link(ptimer);
      return 1;
    case TMR_STATE_UNUSED:
      return 0;
    default:
      return 0;
  }
}


/*================================================================
���� �ơ�u8 timer_cancel()
���� �ܡ�ȡ����ǰ���еļ�ʱ
���� ע��ȡ��һ����ʱ��,��timer_idָ����б�������ΪNULL
================================================================*/
u8 timer_cancel(timer_t *ptimer)
{
  if(!ptimer)
    return 0;
  return timer_unlink(ptimer);
}


/*================================================================
���� �ơ�u8 timer_poll()
���� �ܡ���ѯ��ʱ��
���� ע�������¼�ִ����ִ���¼�
================================================================*/
void timer_ev_poll(void)
{
  u8 i;
  timer_t *ptmr = NULL;
  for(i=0;i<TIMER_LIST_MAX;i++)
  {
    if(!timer_list[i])
      continue;
    ptmr = timer_list[i];
    if(ptmr->timeup == 1)
    {  
      ptmr->callback();
      ptmr->timeup = 0;
    }
  }
}

/* static function implement **************************************/

static void timer_cleanup(timer_t *ptimer)
{
  if(!ptimer)
    return;
  ptimer->state = TMR_STATE_UNUSED;
  ptimer->expire = 0;
  ptimer->opt = 0;
}


static u8 timer_link(timer_t *ptimer)
{
  u8 i;
  if(!ptimer)
    return 0;
  if(timer_count >= TIMER_LIST_MAX)
    return 0;
  for(i=0;i<TIMER_LIST_MAX;i++){
    if(timer_list[i] != NULL)
			continue;
    ptimer->state  = TMR_STATE_RUNNING;
    ptimer->expire = ptimer->period + timer_tick;
    timer_list[i]  = ptimer;
    timer_count++;
    return 1;
  }
  return 0;
}


static u8 timer_unlink(timer_t *ptimer)
{
	u8 i;
	if(!ptimer)
		return 0;
	for(i=0;i<TIMER_LIST_MAX;i++){
		if(timer_list[i] != ptimer)
			continue;
		timer_list[i] = NULL;
		ptimer->state = TMR_STATE_STOPPED;
		timer_count--;
		return 1;
	}
	return 0;
}


static volatile u32  hal_jiffies;  /* �ܽ����� */
u8 tick_inited = 0;    /* ��ʼ����־λ */

void TA0_init(void)
{
  TA0CTL = TASSEL0 | TACLR | ID_1 ;  // 16384Hz = 128*128
  TA0CCTL0 = CCIE;
  TA0CCR0 = INTERVAL; 
  TA0CTL |= (MC1 + TACLR);

  hal_jiffies = 0;
  tick_inited = 1;
}


#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void)
{
  timer_t *ptmr = NULL;
  /* HW timer bug fix: Interrupt handler called before TR==CCR.
   * Occurrs when timer state is toggled between STOP and CONT. */
  while((TA0CTL & MC1) && TA0CCR0 - TA0R == 1);
  /* Make sure interrupt time is future */
  do {
    TA0CCR0 += INTERVAL;
    timer_tick++;
  } while((TA0CCR0 - TA0R) > INTERVAL);

  if(hal_jiffies % CLOCK_CONF_SECOND == 0)
  {
    seconds++;
  }
  _DINT();
  
  for(u8 i=0;i<TIMER_LIST_MAX;i++)
  {
    if(!timer_list[i])
      continue;
    ptmr = timer_list[i];
    if(timer_tick >= ptmr->expire)
    {  
      ptmr->timeup = 1;
      if(ptmr->opt == OPT_TMR_PERIODIC)
      {
        //timer_link(timer_list[i]);
        ptmr->expire = timer_tick + ptmr->period;
      }
      else
      {
        ptmr->state = TMR_STATE_COMPLETED;
        timer_unlink(timer_list[i]);
      }
    }
  }
  _EINT();	
  _BIC_SR_IRQ(LPM4_bits);
}