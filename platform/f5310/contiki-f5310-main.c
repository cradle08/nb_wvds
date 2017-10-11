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

////#include "dev/flash.h"
//#include "dev/leds.h"
////#include "dev/uart.h"
//
//#include "dev/xmem.h"
//#include "lib/random.h"
//#include "lib/sensors.h"
//#include "net/rime.h"
//#include "sys/autostart.h"
//#include "sys/unixtime.h"

#include "contiki.h"
#include "sys/ctimer.h"
#include "platform-conf.h"
#include "uart1.h"
#include "qmc5883.h"
#include "watchdog.h"
#include "rtc.h"
#include "bc95.h"
   
   
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
static struct ctimer mytime;
static uint8_t unix_init_tstamp[6] = {16, 01, 01, 00, 00, 00}; // 160101 00:00:00

//** app init function
//void app_get_xyz(unsigned char *data, unsigned char *temp)
//{
//  One_Sample.x=(int16_t)((data[0]<<8) + data[1]);
//  One_Sample.y=(int16_t)((data[2]<<8) + data[3]);
//  One_Sample.z=(int16_t)((data[4]<<8) + data[5]);
//}   

int
main(int argc, char **argv)
{
  //Initalize hardware.
  msp430_cpu_init(F_CPU);
  clock_init();
  unixtime_init(unix_init_tstamp);
//  uart1_init(BAUD2UBR(9600));
  nb_module_init();
//  qmc5883_init();
    
  while(1)
  {
    //LMP4_EXIT;
  }
  
//  app_init(); // app data init
//  process_init();
//  process_start(&etimer_process, NULL);
//  ctimer_init();
//  process_start(&nb_device, NULL);
//  process_start(&sendmsg_process, NULL);
//  
//
//
//
//  while(1) {
//    uint8_t r;
//    do {
// //     watchdog_periodic();
//      r = process_run();
//    } while(r > 0);
//  }
//  LPM3;
  
  
//     P1DIR |= BIT2;  
//  while(1)
//  {    
//    uint16_t i = 0;
//    qmc5883_sample_read(1);
//    parking_s = Parking_Algorithm();
//    app_test_send_msg();
//    
//          uint16_t i = 0;
//          P1OUT ^=BIT2;
//          for(i = 0; i < 1000; i++)
//          {
//            __delay_cycles(8000);
//          }    
//  }

}






