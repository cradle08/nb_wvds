/*
 * Copyright (c) 2006, Swedish Institute of Computer Science
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
 * @(#)$Id: contiki-z1-main.c,v 1.4 2010/08/26 22:08:11 nifi Exp $
 */

#include "contiki.h"
#include "platform-conf.h"
#include "app.h"
#include "qmc5883.h"
#include "uart1.h"
#include "vehicleDetection.h"


#include <stdio.h>
#include <string.h>
#include <stdarg.h>

//#include "dev/flash.h"
#include "dev/leds.h"
//#include "dev/uart.h"
#include "dev/watchdog.h"
#include "dev/xmem.h"
#include "lib/random.h"
#include "lib/sensors.h"
#include "net/rime.h"
#include "sys/autostart.h"
#include "sys/unixtime.h"

/*----------------------------------------------------------------------------*/
#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

   
extern struct Sample_Struct One_Sample;
extern uint8_t park_s;
static uint16_t seq = 0;
static uint8_t  parking_s = 3;

//**
void app_send_msg()
{
  uint8_t buf[40] = {0};
  if(seq >= 9999) seq = 1;
  sprintf(buf, "No=%d: x=%d,y=%d,z=%d,ps=%d", seq++, One_Sample.x, One_Sample.y, One_Sample.z, parking_s);
  uart1_send(buf, 40);
 // uart1_send("12345", 5);
}

//** app init function
void app_get_xyz(unsigned char *data, unsigned char *temp)
{
  One_Sample.x=(int16_t)((data[0]<<8) + data[1]);
  One_Sample.y=(int16_t)((data[2]<<8) + data[3]);
  One_Sample.z=(int16_t)((data[4]<<8) + data[5]);
}   

int
main(int argc, char **argv)
{
  
  //Initalize hardware.
  msp430_cpu_init(F_CPU);
  clock_init();

 // leds_init();
 // leds_on(LEDS_RED);
  uart1_init(BAUD2UBR(9600)); //Must come before first printf 
  //leds_on(LEDS_GREEN);
  /* xmem_init(); */
//  rtimer_init();

  qmc5883_init();
  qmc5883_set_callback(app_get_xyz);
  Variant_Init();
  uint16_t sample_period = 5;
  while(1)
  {
    uint16_t i = 0;
    qmc5883_sample_read(0);
    parking_s = Parking_Algorithm();
    app_send_msg();
    for(i = 0; i < sample_period; i++)
    {
      __delay_cycles(8000);
    }
  }
  
  /*
  //Initialize Contiki and our processes.
  process_init();
  process_start(&etimer_process, NULL);
 // process_start(&sensors_process, NULL);
#if UNIXTIME_PROCESS
  process_start(&unixtime_process, NULL);
#endif
  ctimer_init();
  //
  process_start(&NB_Device, NULL);
  
  while(1) {
    int r;
    do {
      // Reset watchdog.
//      watchdog_periodic();
      r = process_run();
    } while(r > 0);
  }
*/

}




