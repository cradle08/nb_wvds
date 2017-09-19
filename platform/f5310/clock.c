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
 * @(#)$Id: clock.c,v 1.25 2010/04/04 12:29:50 adamdunkels Exp $
 */

#include "contiki-conf.h"
#include "isr_compat.h"
//#include "sys/energest.h"
#include "sys/clock.h"
#include "sys/etimer.h"
//#include "rtimer-arch.h"

//#if !UNIXTIME_ARCH
#include "sys/unixtime.h"
//#endif

//#include "dev/watchdog.h"
//#include "dev/leds.h"

//#define MAX_INTERVAL (RTIMER_ARCH_SECOND >> 1)
//#define INTERVAL (RTIMER_ARCH_SECOND / CLOCK_SECOND)

//#define MAX_TICKS (~((clock_time_t)0) / 2)

   
static volatile unsigned long seconds = 0;
static volatile clock_time_t jiffies = 0;
#define MAX_TICKS (~((clock_time_t)0) / 2)

   
//
ISR(TIMER1_A1, timera1)
{
  if(TA1IV == 2) {
    /* HW timer bug fix: Interrupt handler called before TR==CCR.
     * Occurrs when timer state is toggled between STOP and CONT. */
    unsigned short v1;
    unsigned short v2;
    do {
      v1 = (TA1CTL & MC1);
      v2 = TA1CCR1;
      v2 -= TA1R;
    } while (v1 && v2 == 1);
    
    /* Make sure interrupt time is future */
    unsigned short dt;
    do {
      /*TACTL &= ~MC1;*/
      TA1CCR1 += INTERVAL;
      /*TACTL |= MC1;*/
      ++jiffies;

      /* Make sure the CLOCK_CONF_SECOND is a power of two, to ensure
         that the modulo operation below becomes a logical and and not
         an expensive divide. Algorithm from Wikipedia:
         http://en.wikipedia.org/wiki/Power_of_two */

      if(jiffies % CLOCK_SECOND == 0) {
        ++seconds;
      }

      dt  = TA1CCR1;
      dt -= TA1R;
    } while(dt > INTERVAL);

 //   if(etimer_pending() && (etimer_next_expiration_time() - jiffies - 1) > MAX_TICKS) {
 //     etimer_request_poll();
  //    LPM4_EXIT;
 //   }
  }
}

//
clock_time_t
clock_time(void)
{
  clock_time_t t1, t2;
  do {
    t1 = jiffies;
    t2 = jiffies;
  } while(t1 != t2);
  return t1;
}

//
void
clock_set(clock_time_t clock, clock_time_t fclock)
{
  TA1R = fclock;
  TA1CCR1 = fclock + INTERVAL;
  jiffies = clock;
}

//
void
clock_init(void)
{
  // disable interrupe
  dint();
  // clear TA1R
  TA1CTL |= TACLR;
  // select the signal soure, coutinus inc mode for TA1
  TA1CTL |= TASSEL0 + MC1;
  // enable cctl1 interrupt
  TA1CCTL1 |= CCIE;
  // set ccr1, interrupt after 1/128 s
  TA1CCR1 = INTERVAL;
  // jiffies init 0
  jiffies = 0;
  // enable interrupt
  eint();
}


//
unsigned long
clock_seconds(void)
{
  unsigned long t1, t2;
  do {
    t1 = seconds;
    t2 = seconds;
  } while(t1 != t2);
  return t1;
}

//
rtimer_clock_t
clock_counter(void)
{
  return TA1R;
}




