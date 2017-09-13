#include "contiki.h"
#include "isr_compat.h"
#include "cc11xx.h"
#include "dev/button-sensor.h"

#define CC1120_GDO0_PORT(type) P1##type
#define CC1120_GDO0_PIN        7
/*------------------------------------------------------------------*/
extern int button1_interrupt(void);
extern int button2_interrupt(void);

/*------------------------------------------------------------------*/
ISR(PORT1, port1_interrupt)
{
  ENERGEST_ON(ENERGEST_TYPE_IRQ);

  if(CC1120_GDO0_PORT(IFG) & BV(CC1120_GDO0_PIN)) {
    ISR_BEG(ISR_RADIO);

    NODESTATS_ADD(radio);
    if(cc11xx_rx_interrupt()) {
      LPM4_EXIT;
    }

    /* Reset interrupt trigger */
    CC1120_GDO0_PORT(IFG) &= ~BV(CC1120_GDO0_PIN);
    //CC1120_GDO2_PORT(IFG) &= ~BV(CC1120_GDO2_PIN);
    //CC1120_GDO3_PORT(IFG) &= ~BV(CC1120_GDO3_PIN);
    ISR_END(ISR_RADIO);
  }

  if(BUTTON1_PORT(IFG) & BV(BUTTON1_PIN)) {
    if(button1_interrupt()) {
      LPM4_EXIT;
    }

    BUTTON1_PORT(IFG) &= ~BV(BUTTON1_PIN);
  }

  if(BUTTON2_PORT(IFG) & BV(BUTTON2_PIN)) {
    if(button2_interrupt()) {
      LPM4_EXIT;
    }

    BUTTON2_PORT(IFG) &= ~BV(BUTTON2_PIN);
  }

  ENERGEST_OFF(ENERGEST_TYPE_IRQ);
}
