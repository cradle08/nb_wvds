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
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "dev/button-sensor.h"
#if CC11xx_CC1101 || CC11xx_CC1120
#include "cc11xx.h"
#endif /* CC11xx_CC1101 || CC11xx_CC1120 */
#include "dev/flash.h"
#include "dev/leds.h"
#if WITH_SERIAL_LINE
#include "dev/serial-line.h"
#endif
#include "dev/slip.h"
#include "dev/uart.h"
#include "dev/watchdog.h"
#include "dev/xmem.h"
#include "lib/random.h"
#include "lib/sensors.h"
#include "net/mac/frame802154.h"
#include "net/netstack.h"
#include "net/rime.h"
#include "sys/autostart.h"
#include "sys/profile.h"
#include "sys/unixtime.h"

#include "node-id.h"
#include "lcd.h"
#include "duty-cycle-scroller.h"

#include "dev/battery-sensor.h"
#include "dev/solarbat-sensor.h"

#if WITH_UIP6
#include "net/uip-ds6.h"
#endif /* WITH_UIP6 */

/*----------------------------------------------------------------------------*/
//#if NETSTACK_AES_KEY_DEFAULT
//#warning Using default AES key "thingsquare mist", change it in project-conf.h like this:
//#warning #define NETSTACK_AES_KEY {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}
//#endif /* NETSTACK_AES_KEY */

#if BOARD_CADRE1120_AP
//#warning "building for board Cadre1120 AP"
#elif BOARD_CADRE1120_VD
//#warning "building for board Cadre1120 VD"
#else
#error "no valid board definition, define it in project-conf.h like this:"
#error "#define BOARD_CADRE1120_XX 1"
#endif

/*----------------------------------------------------------------------------*/
#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#if BOARD_CADRE1120_AP
SENSORS(&battery_sensor, &solarbat_sensor, &button1_sensor, &button2_sensor);
#elif BOARD_CADRE1120_VD
SENSORS(&battery_sensor);
#else
#warning "no sensor support for this board"
#endif

#if NODEALARM
struct nodealarm nodealarm;
#endif
#if NODESTATS
struct nodestats nodestats;
#endif
#if ISRSTATS
uint8_t isrDepth = 0;
uint8_t maxIsrDepth = 0;
isr_stat_t isrStats[ISR_MAX] = {{0}};
uint16_t isrEmbedN = 0;
uint8_t isrQueue[ISR_MAX] = {0};
#endif
/*---------------------------------------------------------------------------*/
#if WITH_LPM
static struct {
  uint8_t sleep;
  uint8_t led_on;
} lpm_state;

extern uint8_t radio_is_off;
static void lpm_enter(void);
static void lpm_quit(void);
#endif
/*---------------------------------------------------------------------------*/
static void
set_rime_addr(void)
{
  rimeaddr_t addr;
  int i;

  memset(&addr, 0, sizeof(rimeaddr_t));
#if UIP_CONF_IPV6
  memcpy(addr.u8, node_mac, sizeof(addr.u8));
#else
  if(node_id == 0) {
    for(i = 0; i < sizeof(rimeaddr_t); ++i) {
      addr.u8[i] = node_mac[7 - i];
    }
  } else {
    addr.u8[0] = node_id & 0xff;
    addr.u8[1] = node_id >> 8;
  }
#endif
  rimeaddr_set_node_addr(&addr);
  PRINTF("Rime addr ");
  for(i = 0; i < sizeof(addr.u8) - 1; i++) {
    PRINTF("%u.", addr.u8[i]);
  }
  PRINTF("%u\n", addr.u8[i]);
}
/*---------------------------------------------------------------------------*/
static void
print_processes(struct process * const processes[])
{
  /*  const struct process * const * p = processes;*/
  PRINTF("Starting");
  while(*processes != NULL) {
    PRINTF(" %s", (*processes)->name);
    processes++;
  }
#if DEBUG
  putchar('\n');
#endif
}
/*--------------------------------------------------------------------------*/
int
main(int argc, char **argv)
{
  /*
   * Initalize hardware.
   */
  msp430_cpu_init();
  clock_init();

  leds_init();
  leds_on(LEDS_RED);

  uart_init(BAUD2UBR(115200)); /* Must come before first printf */
  //uart_send("booted\n", 7);

  //leds_on(LEDS_GREEN);
  /* xmem_init(); */

  rtimer_init();

#if WITH_LCD
  lcd_init();
#endif

  //watchdog_init();

  PRINTF(CONTIKI_VERSION_STRING "\n");
  /*  PRINTF("Compiled at %s, %s\n", __TIME__, __DATE__);*/

  /*
   * Hardware initialization done!
   */

  //leds_on(LEDS_RED);

  /* Restore node id if such has been stored in external mem */
#ifdef NODEID
  node_id = NODEID;

#ifdef BURN_NODEID
  node_id_burn(node_id);
  node_id_restore(); /* also configures node_mac[] */
#endif /* BURN_NODEID */
#else
  node_id_restore(); /* also configures node_mac[] */
#endif /* NODE_ID */

  /* for setting "hardcoded" IEEE 802.15.4 MAC addresses */
#ifdef MAC_1
  {
    uint8_t ieee[] = { MAC_1, MAC_2, MAC_3, MAC_4, MAC_5, MAC_6, MAC_7, MAC_8 };
    memcpy(node_mac, ieee, sizeof(uip_lladdr.addr));
  }
#endif

   /*
   * Initialize Contiki and our processes.
   */
  process_init();
  process_start(&etimer_process, NULL);
  process_start(&sensors_process, NULL);
#if UNIXTIME_PROCESS
  process_start(&unixtime_process, NULL);
#endif

  ctimer_init();

  set_rime_addr();

  random_init(node_id);

  NETSTACK_RADIO.init();
  leds_on(LEDS_GREEN);
#if CC11xx_CC1101 || CC11xx_CC1120
  PRINTF("Starting up cc11xx radio at channel %d\n", RF_CHANNEL);
  cc11xx_channel_set(RF_CHANNEL);
#endif /* CC11xx_CC1101 || CC11xx_CC1120 */
#if CONFIGURE_CC2420 || CONFIGURE_CC2520
  {
    uint8_t longaddr[8];
    uint16_t shortaddr;

    shortaddr = (rimeaddr_node_addr.u8[0] << 8) + rimeaddr_node_addr.u8[1];
    memset(longaddr, 0, sizeof(longaddr));
    rimeaddr_copy((rimeaddr_t *)&longaddr, &rimeaddr_node_addr);
    PRINTF("MAC %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n", longaddr[0],
           longaddr[1], longaddr[2], longaddr[3], longaddr[4], longaddr[5],
           longaddr[6], longaddr[7]);

#if CONFIGURE_CC2420
    cc2420_set_pan_addr(IEEE802154_PANID, shortaddr, longaddr);
#endif /* CONFIGURE_CC2420 */
#if CONFIGURE_CC2520
    cc2520_set_pan_addr(IEEE802154_PANID, shortaddr, longaddr);
#endif /* CONFIGURE_CC2520 */
  }
#if CONFIGURE_CC2420
  cc2420_set_channel(RF_CHANNEL);
#endif /* CONFIGURE_CC2420 */
#if CONFIGURE_CC2520
  cc2520_set_channel(RF_CHANNEL);
#endif /* CONFIGURE_CC2520 */
#endif /* CONFIGURE_CC2420 || CONFIGURE_CC2520 */

  NETSTACK_RADIO.on();

  //leds_off(LEDS_ALL);

  if(node_id > 0) {
    PRINTF("Node id %u.\n", node_id);
  } else {
    PRINTF("Node id not set.\n");
  }

#if WITH_UIP6
  memcpy(&uip_lladdr.addr, node_mac, sizeof(uip_lladdr.addr));
  /* Setup nullmac-like MAC for 802.15.4 */

  queuebuf_init();

  netstack_init();

  PRINTF("%s/%s %lu %u\n",
         NETSTACK_RDC.name,
         NETSTACK_MAC.name,
         CLOCK_SECOND / (NETSTACK_RDC.channel_check_interval() == 0 ? 1:
                         NETSTACK_RDC.channel_check_interval()),
         RF_CHANNEL);

  process_start(&tcpip_process, NULL);

  PRINTF("IPv6 ");
  {
    uip_ds6_addr_t *lladdr;
    int i;
    lladdr = uip_ds6_get_link_local(-1);
    for(i = 0; i < 7; ++i) {
      PRINTF("%02x%02x:", lladdr->ipaddr.u8[i * 2],
             lladdr->ipaddr.u8[i * 2 + 1]);
    }
    PRINTF("%02x%02x\n", lladdr->ipaddr.u8[14], lladdr->ipaddr.u8[15]);
  }

  if(1) {
    uip_ipaddr_t ipaddr;
    int i;
    uip_ip6addr(&ipaddr, 0xfc00, 0, 0, 0, 0, 0, 0, 0);
    uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
    uip_ds6_addr_add(&ipaddr, 0, ADDR_TENTATIVE);
    PRINTF("Tentative global IPv6 address ");
    for(i = 0; i < 7; ++i) {
      PRINTF("%02x%02x:",
             ipaddr.u8[i * 2], ipaddr.u8[i * 2 + 1]);
    }
    PRINTF("%02x%02x\n",
           ipaddr.u8[7 * 2], ipaddr.u8[7 * 2 + 1]);
  }

#else /* WITH_UIP6 */

  netstack_init();

  PRINTF("%s %lu %u\n",
         NETSTACK_RDC.name,
         CLOCK_SECOND / (NETSTACK_RDC.channel_check_interval() == 0? 1:
                         NETSTACK_RDC.channel_check_interval()),
         RF_CHANNEL);
#endif /* WITH_UIP6 */

#if !WITH_UIP6
#if WITH_SERIAL_LINE
  uart_set_input(serial_line_input_byte);
  serial_line_init();
#endif
#endif

#ifdef NETSTACK_AES_H
#ifndef NETSTACK_AES_KEY
#error Please define NETSTACK_AES_KEY!
#endif /* NETSTACK_AES_KEY */
  {
    const uint8_t key[] = NETSTACK_AES_KEY;
    netstack_aes_set_key(key);
  }
  /*printf("AES encryption is enabled: '%s'\n", NETSTACK_AES_KEY);*/
  PRINTF("AES encryption is enabled\n");
#else /* NETSTACK_AES_H */
  PRINTF("Warning: AES encryption is disabled\n");
#endif /* NETSTACK_AES_H */

#if TIMESYNCH_CONF_ENABLED
  timesynch_init();
  timesynch_set_authority_level(rimeaddr_node_addr.u8[0]);
#endif /* TIMESYNCH_CONF_ENABLED */


#if CC11xx_CC1101 || CC11xx_CC1120
  PRINTF("cc11xx radio at channel %d\n", RF_CHANNEL);
  cc11xx_channel_set(RF_CHANNEL);
#endif /* CC11xx_CC1101 || CC11xx_CC1120 */
#if CONFIGURE_CC2420
  {
    uint8_t longaddr[8];
    uint16_t shortaddr;

    shortaddr = (rimeaddr_node_addr.u8[0] << 8) +
      rimeaddr_node_addr.u8[1];
    memset(longaddr, 0, sizeof(longaddr));
    rimeaddr_copy((rimeaddr_t *)&longaddr, &rimeaddr_node_addr);
    PRINTF("MAC %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
           longaddr[0], longaddr[1], longaddr[2], longaddr[3],
           longaddr[4], longaddr[5], longaddr[6], longaddr[7]);

    cc2420_set_pan_addr(IEEE802154_PANID, shortaddr, longaddr);
  }
  cc2420_set_channel(RF_CHANNEL);
#endif /* CONFIGURE_CC2420 */
  NETSTACK_RADIO.on();

  /*  process_start(&sensors_process, NULL);
      SENSORS_ACTIVATE(button_sensor);*/

  energest_init();
  ENERGEST_ON(ENERGEST_TYPE_CPU);

#if WITH_UIP6
  simple_rpl_init();
#endif

  //watchdog_start();
  leds_off(LEDS_ALL);

  print_processes(autostart_processes);
  autostart_start(autostart_processes);

#if WITH_LCD
  duty_cycle_scroller_start(CLOCK_SECOND * 2);
#endif

#if IP64_CONF_UIP_FALLBACK_INTERFACE_SLIP && WITH_SLIP
  /* Start the SLIP */
  PRINTF("Initiating SLIP: my IP is 172.16.0.2...\n");
  slip_arch_init(0);
  {
    uip_ip4addr_t ipv4addr, netmask;

    uip_ipaddr(&ipv4addr, 172, 16, 0, 2);
    uip_ipaddr(&netmask, 255, 255, 255, 0);
    ip64_set_ipv4_address(&ipv4addr, &netmask);
  }
  uart_set_input(slip_input_byte);
#endif /* IP64_CONF_UIP_FALLBACK_INTERFACE_SLIP */

  /*
   * This is the scheduler loop.
   */
#if !WITH_MCU_LPM
  eint();
#endif
  while(1) {
    int r;
    do {
      /* Reset watchdog. */
      watchdog_periodic();
      NODESTATS_ADD(proc);
      r = process_run();
    } while(r > 0);

#if WITH_MCU_LPM
    /*
     * Idle processing.
     */
    int s = splhigh();          /* Disable interrupts. */
    /* uart3_active is for avoiding LPM3 when still sending or receiving */
    if(process_nevents() != 0 || uart_active()) {
      splx(s);                  /* Re-enable interrupts. */
    } else {
      static unsigned long irq_energest = 0;

      /* Re-enable interrupts and go to sleep atomically. */
      ENERGEST_OFF(ENERGEST_TYPE_CPU);
      ENERGEST_ON(ENERGEST_TYPE_LPM);
      /* We only want to measure the processing done in IRQs when we
         are asleep, so we discard the processing time done when we
         were awake. */
      energest_type_set(ENERGEST_TYPE_IRQ, irq_energest);
#if WITH_LPM
      lpm_enter();
#endif
      //watchdog_stop();
      NODESTATS_ADD(sleep);
      _BIS_SR(GIE | SCG0 | SCG1 | CPUOFF); /* LPM3 sleep. This
                                              statement will block
                                              until the CPU is
                                              woken up by an
                                              interrupt that sets
                                              the wake up flag. */

      /* We get the current processing time for interrupts that was
         done during the LPM and store it for next time around.  */
      dint();
      irq_energest = energest_type_time(ENERGEST_TYPE_IRQ);
      eint();
      NODESTATS_ADD(wakeup);
      //watchdog_start();
#if WITH_LPM
      lpm_quit();
#endif
      ENERGEST_OFF(ENERGEST_TYPE_LPM);
      ENERGEST_ON(ENERGEST_TYPE_CPU);
    }
#endif
  }
}
/*---------------------------------------------------------------------------*/
#if WITH_LPM
static void
lpm_enter(void)
{
  // disable ADC
  ADC12CTL0&=~ADC12ENC;
  ADC12CTL0=0;
#if BOARD_CADRE1120_VD
  P6SEL&=~BIT4;
  P6DIR&=~BIT4;
  P6REN|= BIT4;
  P6OUT|= BIT4;
#endif

  // turn off led
  //if ((P2OUT & BIT0) == 0)
  //  lpm_state.led_on = 1;
  //P2OUT |= BIT0;
  P2SEL &= ~BIT0;
  P2DIR &= ~BIT0;
  P2REN |=  BIT0;
  P2OUT |=  BIT0;

  // REF
  P5SEL &=~(BIT1+BIT0);
  P5DIR &=~(BIT1+BIT0);
  P5REN |=(BIT1+BIT0);
  P5OUT |=(BIT1+BIT0);

  // UART
  P10DIR &= ~0x30;
  P10OUT |=  0x30;

  // P10.3
  P10DIR &= ~BIT3;
  P10REN |=  BIT3;
  P10OUT |=  BIT3;

  lpm_state.sleep = 1;
  if (radio_is_off)
    __no_operation();
}

static void
lpm_quit(void)
{
  // LED
  P2DIR |=  BIT0;
  P2REN &= ~BIT0;
  P2OUT |=  BIT0;
#if LED_LPM
  // led on for active indication
  P2OUT &= ~BIT0;
#endif
  // restore led state
  //if (lpm_state.led_on)
  //  P2OUT &= ~BIT0;

  lpm_state.sleep = 0;
}
#endif
/*---------------------------------------------------------------------------*/
