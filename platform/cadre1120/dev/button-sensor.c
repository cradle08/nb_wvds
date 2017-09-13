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
 */
#include "contiki.h"
#include "lib/sensors.h"
#include "dev/button-sensor.h"
#include "isr_compat.h"

#define HWCONF_PIN(name, port, bit)                                 \
static void name##_SELECT() {P##port##SEL &= ~(1 << bit);}          \
static int  name##_READ() {return (P##port##IN & (1 << bit));}      \
static void name##_MAKE_INPUT() {P##port##DIR &= ~(1 << bit);}

#define HWCONF_IRQ(name, port, bit)                                 \
static void name##_ENABLE_IRQ() {P##port##IE |= 1 << bit;}          \
static void name##_DISABLE_IRQ() {P##port##IE &= ~(1 << bit);}      \
static int  name##_IRQ_ENABLED() {return P##port##IE & (1 << bit);} \
static void name##_IRQ_EDGE_SELECTD() {P##port##IES |= 1 << bit;}   \
static void name##_CLEAR_IRQ() {P##port##IFG &= ~(1 << bit);}

const struct sensors_sensor button1_sensor;
const struct sensors_sensor button2_sensor;

static struct timer debouncetimer1;
static int status_b1(int type);
static struct timer debouncetimer2;
static int status_b2(int type);

HWCONF_PIN(BUTTON1, 1, 0);
HWCONF_IRQ(BUTTON1, 1, 0);
HWCONF_PIN(BUTTON2, 1, 1);
HWCONF_IRQ(BUTTON2, 1, 1);
/*---------------------------------------------------------------------------*/
#if 0
ISR(PORT1, irq_p1)
{
  ENERGEST_ON(ENERGEST_TYPE_IRQ);

  if(BUTTON_CHECK_IRQ()) {
    if(timer_expired(&debouncetimer1)) {
      timer_set(&debouncetimer1, CLOCK_SECOND / 8);
      sensors_changed(&button1_sensor);
      LPM4_EXIT;
    }
  }
  P1IFG = 0x00;
  ENERGEST_OFF(ENERGEST_TYPE_IRQ);
}

#else
int
button1_interrupt(void)
{
  if(timer_expired(&debouncetimer1)) {
    timer_set(&debouncetimer1, CLOCK_SECOND / 8);
    sensors_changed(&button1_sensor);
    return 1;
  }
  return 0;
}

int
button2_interrupt(void)
{
  if(timer_expired(&debouncetimer2)) {
    timer_set(&debouncetimer2, CLOCK_SECOND / 8);
    sensors_changed(&button2_sensor);
    return 1;
  }
  return 0;
}
#endif
/*---------------------------------------------------------------------------*/
static int
value_b1(int type)
{
  return BUTTON1_READ() || !timer_expired(&debouncetimer1);
}

static int
value_b2(int type)
{
  return BUTTON2_READ() || !timer_expired(&debouncetimer2);
}
/*---------------------------------------------------------------------------*/
static int
configure_b1(int type, int c)
{
  switch (type) {
  case SENSORS_ACTIVE:
    if (c) {
      if(!status_b1(SENSORS_ACTIVE)) {
        timer_set(&debouncetimer1, 0);
        BUTTON1_SELECT();
        BUTTON1_MAKE_INPUT();
        BUTTON1_IRQ_EDGE_SELECTD();
        BUTTON1_CLEAR_IRQ();
        BUTTON1_ENABLE_IRQ();
      }
    } else {
      BUTTON1_DISABLE_IRQ();
    }
    return 1;
  }
  return 0;
}

static int
configure_b2(int type, int c)
{
  switch (type) {
  case SENSORS_ACTIVE:
    if (c) {
      if(!status_b2(SENSORS_ACTIVE)) {
        timer_set(&debouncetimer2, 0);
        BUTTON2_SELECT();
        BUTTON2_MAKE_INPUT();
        BUTTON2_IRQ_EDGE_SELECTD();
        BUTTON2_CLEAR_IRQ();
        BUTTON2_ENABLE_IRQ();
      }
    } else {
      BUTTON2_DISABLE_IRQ();
    }
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
status_b1(int type)
{
  switch (type) {
  case SENSORS_ACTIVE:
  case SENSORS_READY:
    return BUTTON1_IRQ_ENABLED();
  }
  return 0;
}

static int
status_b2(int type)
{
  switch (type) {
  case SENSORS_ACTIVE:
  case SENSORS_READY:
    return BUTTON2_IRQ_ENABLED();
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(button1_sensor, "Button1", value_b1, configure_b1, status_b1);
SENSORS_SENSOR(button2_sensor, "Button2", value_b2, configure_b2, status_b2);
