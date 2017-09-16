#include <msp430x54x.h>
#include <intrinsics.h>
#include "hal-pmm.h"
#include "platform.h"

static unsigned long dco_speed;

#define dint() do { } while(0)
#define eint() do { } while(0)

/*---------------------------------------------------------------------------*/
unsigned long
msp430_dco_speed(void)
{
  return dco_speed;
}
/*---------------------------------------------------------------------------*/
/* on fatal error, blink LEDs forever */
static void
panic(void)
{
  //P1DIR |= 0x03;
  while(1) {
    //__delay_cycles(10000);
    //P1OUT ^= 0x03;
  }
}

/*---------------------------------------------------------------------------*/
void
msp430_set_dco_speed(unsigned long mhz)
{
  int multiplier;

  dco_speed = mhz;

  dint();
  /* DCO multiplier m for x MHz:
     (m + 1) * FLLRef = Fdco
     (m + 1) * 32768 = x MHz
     m = x / 32768 - 1
     Set FLL Div = fDCOCLK/2
  */

  multiplier = mhz / 32768UL - 1;

  __bis_SR_register(SCG0);

  /* change the core voltage to enable higher clock speed */
  if(SetVCore(PMMCOREV_3) == PMM_STATUS_ERROR) {
    /* unable to set voltage, operation will be unpredictable */
    panic();
  }

  /* changed automatically by the FLL later on */
  UCSCTL0 = 0x0000;

  /* Select DCO range for (at worst) 6--23.7 MHz, (at best) 2.5--54 MHz */
  UCSCTL1 = DCORSEL_5;

  /* Set computed DCO multiplier */
  UCSCTL2 = FLLD_1 + multiplier;

  /* FLL sourced from external 32768 Hz crystal. */
  UCSCTL3 |= SELREF__XT1CLK;

  __bic_SR_register(SCG0);

  do {
    /* Clear XT2,XT1,DCO fault flags */
    UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + XT1HFOFFG + DCOFFG);
    /* Clear fault flags */
    SFRIFG1 &= ~OFIFG;

    __delay_cycles(10000);
    /* Test oscillator fault flag */
  } while(SFRIFG1 & OFIFG);

  /* Sources for the hardware clocks */
  UCSCTL4 |= SELA__XT1CLK | SELS__DCOCLKDIV | SELM__DCOCLK;

  eint();
}
/*---------------------------------------------------------------------------*/
void
msp430_quick_synch_dco(void)
{
  msp430_set_dco_speed(F_CPU);
}
/*---------------------------------------------------------------------------*/
static void
init_ports(void)
{
  /* Turn everything off, device drivers enable what is needed. */

  /* All configured for digital I/O */
#ifdef P1SEL
  P1SEL = 0;
#endif
#ifdef P2SEL
  P2SEL = 0;
#endif
#ifdef P3SEL
  P3SEL = 0;
#endif
#ifdef P4SEL
  P4SEL = 0;
#endif
#ifdef P5SEL
  P5SEL = 0;
#endif
#ifdef P6SEL
  P6SEL = 0;
#endif

  /* All available inputs */
#ifdef P1DIR
  P1DIR = 0;
  P1OUT = 0;
  P1IES = 0;
#endif
#ifdef P2DIR
  P2DIR = 1 << 6; /* output needed for the below config ? */
  P2OUT = 0;
  P2SEL = 1 << 6; /* test for setting the P2.6 to ACKL output */
  P2IES = 0;
#endif
#ifdef P3DIR
  P3DIR = 0;
  P3OUT = 0;
#endif
#ifdef P4DIR
  P4DIR = 0;
  P4OUT = 0;
#endif

#ifdef P5DIR
  P5DIR = 0;
  P5OUT = 0;
#endif

#ifdef P6DIR
  P6DIR = 0;
  P6OUT = 0;
#endif

#ifdef P7DIR
  P7DIR = 0;
  P7OUT = 0;
  P7SEL |= 0x03;     /* Configure for ext clock function on these pins */
#endif

#ifdef P8DIR
  P8DIR = 0;
  P8OUT = 0;
#endif

  P1IE = 0;
  P2IE = 0;
}
/*---------------------------------------------------------------------------*/
/* msp430-ld may align _end incorrectly. Workaround in cpu_init. */
#ifndef __IAR_SYSTEMS_ICC__
extern int _end;                /* Not in sys/unistd.h */
static char *cur_break = (char *)&_end;
#endif

void
msp430_cpu_init(void)
{
  dint();
  //watchdog_init();
  init_ports();
  dco_speed = 1048576; /* Default bootup DCO frequency */
  msp430_quick_synch_dco();
  eint();
#ifndef __IAR_SYSTEMS_ICC__
  if((uintptr_t)cur_break & 1) { /* Workaround for msp430-ld bug! */
    cur_break++;
  }
#endif
}
/*---------------------------------------------------------------------------*/
