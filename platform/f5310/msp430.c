/*
 * Copyright (c) 2005, Swedish Institute of Computer Science
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 * @(#)$Id: msp430.c,v 1.1 2010/08/24 16:26:38 joxe Exp $
 */
#include "contiki.h"
#include "platform-conf.h"

//#include "dev/watchdog.h"
//#include "dev/leds.h"
//#include "hal-pmm.h"


//** port init as normal input port
static void port_init(void)
{	
  /* Turn everything off, device drivers enable what is needed. */
  /* All configured for digital I/O */
  P1SEL = 0;
  P2SEL = 0;
  P4SEL = 0;
  P5SEL = 0;
  P6SEL = 0;
  /* All available inputs */
  P1DIR = 0;
  P1OUT = 0;
  P1REN = 0XFF;
  P1IES = 0;
  
  P2DIR = 0;
  P2REN = 0XFF;
  P2OUT = 0;
  
  P4DIR = 0;                            // P4.0 - P4.7 output
  P4REN = 0XFF;
  P4OUT = 0;                            // P4.0 - P4.6 Port Map functions
  
  P5DIR = 0;
  P5REN = 0XFF;
  P5OUT = 0;
  
  P6DIR = 0;
  P6REN = 0XFF;
  P6OUT = 0;
  
  PJDIR = 0XFF;
  PJREN = 0XFF;
  PJOUT = 0;

  P1IE = 0;
  P2IE = 0;
}

//**
static void port_mapping(void)
{ 
  dint();

  P1DIR |= BIT0;                            // ACLK set out to pins
  P1SEL |= BIT0;                            
//  P2DIR |= BIT2;                            // SMCLK set out to pins
//  P2SEL |= BIT2;                            
  
  P4SEL |= BIT7;  
  P4DIR |= BIT7;      // MCLK set out to pins

  PMAPPWD = 0x02D52;      
  PMAPCTL = PMAPRECFG;    
  //
  P4MAP7 = PM_MCLK;// output mclk
  //I2C
  P4MAP1 = PM_UCB1SDA;//PM_UCB1SDA; 
  P4MAP2 = PM_UCB1SCL;//PM_UCB1SCL;
  // UART	
  P4MAP4 = PM_UCA1TXD;//PM_UCA1TXD;
  P4MAP5 = PM_UCA1RXD;//PM_UCA1RXD;

  PMAPPWD = 0;                   
}
   
  
// init clock and signal soure
void msp430_dco_init(uint32_t sped) // sped=8M
{
#if OS_DEBUG
  uint16_t flln = (sped /32768) - 1; //243  (8M)
  // set p5.4 p5.5 as xtclk port
  P5SEL |= BIT4 + BIT5;
  // use x1in x1out(LF) as clock source
  UCSCTL6 &= ~XT1OFF;
  UCSCTL6 &= ~XTS;
  UCSCTL6 |= XCAP_3;
  //  set fll
  __bis_SR_register(SCG0);
  UCSCTL0 = 0x0000;
  UCSCTL1 |= DCORSEL_5; // 3.2--12.3
  UCSCTL2 |= FLLD_1 + flln;
  UCSCTL3 |= SELREF__XT1CLK;
  __bic_SR_register(SCG0);
  
  // wait untill dco stable
  __delay_cycles(250000);
  do {
    // clear xt1/2 dco fault flag
    UCSCTL7 &= ~(XT1LFOFFG + XT2OFFG+ DCOFFG);
    // clear fault flag
    SFRIFG1 &= ~OFIFG;
    // delay wait dco stale 
    __delay_cycles(1000);
    // check oscillator fault flag
  } while(SFRIFG1 & OFIFG);
  // set aclk smclk mclk clock source
  UCSCTL4 |= SELA__XT1CLK + SELS__DCOCLK + SELS__DCOCLK; // SEL_3:DCOCLK, SEL_4:DCOCLKDIV,32.768k 8m 8m
  eint();
  
 
#else
  
  P5SEL |=0x30;
  // SET XT1 AND XT2 ON
  UCSCTL6 &= ~XT1OFF;   
  //XT1 WORKING UNDER LOW FREQUENCY
  UCSCTL6 &=~XTS;
  //CAPCITOR CHOOSE
  UCSCTL6 |=XCAP_3;    
  // WAIT UNTIL STABLIZATION
  do
  {
    UCSCTL7 &= ~(XT1LFOFFG  + DCOFFG);
    UCSCTL7 &=~(XT1LFOFFG);
    _NOP();
                                                                                                                                                                     // Clear XT2,XT1,DCO fault flags
    SFRIFG1 &= ~OFIFG;                       // Clear fault flags
  }
  while (SFRIFG1&OFIFG);                   // Test oscillator fault flag
  //CHOOSE ACK SCLK MCLK CLOCK SOURCE :ACK=32K; SCLK=AROUND 1 M; MCLK=ROUND 8M                
  UCSCTL4 |= SELA_0+SELS_5+SELM_5;          //xt1 for aclk, xt2 for sclk and mclk
  UCSCTL5 |=0;                         //0 divided  frequency   
  UCSCTL1 |=DCORSEL_0;                      //DCO operating frequency in the lowest frequency 

#endif
}


//** cpu port and dco init
void msp430_cpu_init(uint32_t sped)
{
  dint();
  watchdog_init();
  watchdog_stop(); //...
  //watchdog_start();
  port_init();
  port_mapping();
  msp430_dco_init(sped);
  eint();
}



