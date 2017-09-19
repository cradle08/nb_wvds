/*
 * Copyright (c) 2012, Thingsquare, http://www.thingsquare.com/.
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
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef CONTIKI_CONF_H
#define CONTIKI_CONF_H

#include "platform-conf.h"

//#include "mist-conf-const.h"

#ifndef MIST_CONF_NETSTACK
#define MIST_CONF_NETSTACK (MIST_CONF_DROWSIE)
#endif /* MIST_CONF_NETSTACK */



#define NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE 8
#define NULLRDC_CONF_802154_AUTOACK 1
#define NETSTACK_CONF_FRAMER  framer_802154
#if WITH_UIP6
#define NETSTACK_CONF_NETWORK sicslowpan_driver
#endif
#ifndef CC2420_CONF_AUTOACK
#define CC2420_CONF_AUTOACK              1
#endif /* CC2420_CONF_AUTOACK */

#ifndef CC2520_CONF_AUTOACK
#define CC2520_CONF_AUTOACK              1
#endif /* CC2520_CONF_AUTOACK */

#if WITH_UIP6
/* Network setup for IPv6 */
#define NETSTACK_CONF_NETWORK sicslowpan_driver

/* Specify a minimum packet size for 6lowpan compression to be
   enabled. This is needed for ContikiMAC, which needs packets to be
   larger than a specified size, if no ContikiMAC header should be
   used. */
#define SICSLOWPAN_CONF_COMPRESSION_THRESHOLD 0
//#define SICSLOWPAN_CONF_MAC_MAX_PAYLOAD        40
#define CONTIKIMAC_CONF_WITH_CONTIKIMAC_HEADER 0

#define CXMAC_CONF_ANNOUNCEMENTS         0
#define XMAC_CONF_ANNOUNCEMENTS          0

#ifndef QUEUEBUF_CONF_NUM
#define QUEUEBUF_CONF_NUM                8
#endif

#else /* WITH_UIP6 */

/* Network setup for non-IPv6 (rime). */

#define NETSTACK_CONF_NETWORK rime_driver

#define COLLECT_CONF_ANNOUNCEMENTS       1
#define CXMAC_CONF_ANNOUNCEMENTS         0
#define XMAC_CONF_ANNOUNCEMENTS          0
#define CONTIKIMAC_CONF_ANNOUNCEMENTS    0

#define CONTIKIMAC_CONF_COMPOWER         1
#define XMAC_CONF_COMPOWER               1
#define CXMAC_CONF_COMPOWER              1

#ifndef COLLECT_NEIGHBOR_CONF_MAX_COLLECT_NEIGHBORS
#define COLLECT_NEIGHBOR_CONF_MAX_COLLECT_NEIGHBORS     32
#endif /* COLLECT_NEIGHBOR_CONF_MAX_COLLECT_NEIGHBORS */

#ifndef QUEUEBUF_CONF_NUM
#define QUEUEBUF_CONF_NUM                16
#endif /* QUEUEBUF_CONF_NUM */

#ifndef TIMESYNCH_CONF_ENABLED
#define TIMESYNCH_CONF_ENABLED           0
#endif /* TIMESYNCH_CONF_ENABLED */

#if TIMESYNCH_CONF_ENABLED
/* CC2420 SDF timestamps must be on if timesynch is enabled. */
#undef CC2420_CONF_SFD_TIMESTAMPS
#define CC2420_CONF_SFD_TIMESTAMPS       1
#endif /* TIMESYNCH_CONF_ENABLED */

#endif /* WITH_UIP6 */

#define PACKETBUF_CONF_ATTRS_INLINE 1

#ifndef RF_CHANNEL
#define RF_CHANNEL              6 // 916.000MHz
#endif /* RF_CHANNEL */

#define CONTIKIMAC_CONF_BROADCAST_RATE_LIMIT 0

#define IEEE802154_CONF_PANID       0xABCD

#define SHELL_VARS_CONF_RAM_BEGIN 0x1100
#define SHELL_VARS_CONF_RAM_END 0x2000

#define PROFILE_CONF_ON 0
#ifndef ENERGEST_CONF_ON
#define ENERGEST_CONF_ON 1
#endif /* ENERGEST_CONF_ON */

#define ELFLOADER_CONF_TEXT_IN_ROM 0
#ifndef ELFLOADER_CONF_DATAMEMORY_SIZE
#define ELFLOADER_CONF_DATAMEMORY_SIZE 0x400
#endif /* ELFLOADER_CONF_DATAMEMORY_SIZE */
#ifndef ELFLOADER_CONF_TEXTMEMORY_SIZE
#define ELFLOADER_CONF_TEXTMEMORY_SIZE 0x800
#endif /* ELFLOADER_CONF_TEXTMEMORY_SIZE */


#define AODV_COMPLIANCE
#define AODV_NUM_RT_ENTRIES 32

#define WITH_ASCII 1

#define PROCESS_CONF_NUMEVENTS 8
#define PROCESS_CONF_STATS 1
/*#define PROCESS_CONF_FASTPOLL    4*/

#ifdef WITH_UIP6

#define RIMEADDR_CONF_SIZE              8

#define UIP_CONF_LL_802154              1
#define UIP_CONF_LLH_LEN                0

#define UIP_CONF_ROUTER                 1
#ifndef UIP_CONF_IPV6_RPL
#define UIP_CONF_IPV6_RPL               1
#endif /* UIP_CONF_IPV6_RPL */

/* configure number of neighbors and routes */
#ifndef NBR_TABLE_CONF_MAX_NEIGHBORS
#define NBR_TABLE_CONF_MAX_NEIGHBORS     30
#endif /* NBR_TABLE_CONF_MAX_NEIGHBORS */
#ifndef UIP_CONF_DS6_ROUTE_NBU
#define UIP_CONF_DS6_ROUTE_NBU   30
#endif /* UIP_CONF_DS6_ROUTE_NBU */

#define UIP_CONF_ND6_SEND_RA		0
#define UIP_CONF_ND6_REACHABLE_TIME     600000
#define UIP_CONF_ND6_RETRANS_TIMER      10000

#define UIP_CONF_IPV6                   1
#ifndef UIP_CONF_IPV6_QUEUE_PKT
#define UIP_CONF_IPV6_QUEUE_PKT         0
#endif /* UIP_CONF_IPV6_QUEUE_PKT */
#define UIP_CONF_IPV6_CHECKS            1
#define UIP_CONF_IPV6_REASSEMBLY        0
#define UIP_CONF_NETIF_MAX_ADDRESSES    3
#define UIP_CONF_ND6_MAX_PREFIXES       3
#define UIP_CONF_ND6_MAX_NEIGHBORS      4
#define UIP_CONF_ND6_MAX_DEFROUTERS     2
#define UIP_CONF_IP_FORWARD             0
#ifndef UIP_CONF_BUFFER_SIZE
#define UIP_CONF_BUFFER_SIZE		280
#endif

#define SICSLOWPAN_CONF_COMPRESSION_IPV6        0
#define SICSLOWPAN_CONF_COMPRESSION_HC1         1
#define SICSLOWPAN_CONF_COMPRESSION_HC01        2
#define SICSLOWPAN_CONF_COMPRESSION             SICSLOWPAN_COMPRESSION_HC06
#ifndef SICSLOWPAN_CONF_FRAG
#define SICSLOWPAN_CONF_FRAG                    1

/* Unit: 1/16th second. 4 => 0.25s timeout */
#ifndef SICSLOWPAN_CONF_MAXAGE
#define SICSLOWPAN_CONF_MAXAGE                  4
#endif /* SICSLOWPAN_CONF_MAXAGE */

#if (MIST_CONF_NETSTACK & MIST_CONF_DROWSIE_MULTICHANNEL)
/* We need to increase the fragmentation timeout, as the multichannel protocol may transmit
 * the same fragment on two channels, causing up to 0.6s delay inbetween fragments. */
#ifdef SICSLOWPAN_CONF_MAXAGE
#if SICSLOWPAN_CONF_MAXAGE < 12
#undef SICSLOWPAN_CONF_MAXAGE
#define SICSLOWPAN_CONF_MAXAGE                  12
#endif /* SICSLOWPAN_CONF_MAXAGE < 12 */
#endif /* SICSLOWPAN_CONF_MAXAGE */
#endif /* (MIST_CONF_NETSTACK & MIST_CONF_DROWSIE_MULTICHANNEL) */

#endif /* SICSLOWPAN_CONF_FRAG */
#define SICSLOWPAN_CONF_CONVENTIONAL_MAC	1
#define SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS       2
#ifndef SICSLOWPAN_CONF_MAX_MAC_TRANSMISSIONS
#define SICSLOWPAN_CONF_MAX_MAC_TRANSMISSIONS   5
#endif /* SICSLOWPAN_CONF_MAX_MAC_TRANSMISSIONS */
#else /* WITH_UIP6 */
#define UIP_CONF_IP_FORWARD      1
#ifndef UIP_CONF_BUFFER_SIZE
#define UIP_CONF_BUFFER_SIZE     108
#endif
#endif /* WITH_UIP6 */

#define UIP_CONF_ICMP_DEST_UNREACH 1

#define UIP_CONF_DHCP_LIGHT
#define UIP_CONF_LLH_LEN         0
#ifndef  UIP_CONF_RECEIVE_WINDOW
#define UIP_CONF_RECEIVE_WINDOW  100
#endif
#ifndef  UIP_CONF_TCP_MSS
#define UIP_CONF_TCP_MSS         100
#endif
#define UIP_CONF_MAX_CONNECTIONS 4
#define UIP_CONF_MAX_LISTENPORTS 8
#define UIP_CONF_UDP_CONNS       12
#define UIP_CONF_FWCACHE_SIZE    30
#define UIP_CONF_BROADCAST       1
#define UIP_ARCH_IPCHKSUM        0
#define UIP_CONF_UDP             1
#define UIP_CONF_UDP_CHECKSUMS   1
#define UIP_CONF_PINGADDRCONF    0
#define UIP_CONF_LOGGING         0

//#include "mist-default-conf.h"

/* include the project config */
/* PROJECT_CONF_H might be defined in the project Makefile */
#ifdef PROJECT_CONF_H
#include PROJECT_CONF_H
#endif /* PROJECT_CONF_H */

#if ((MIST_CONF_NETSTACK) & MIST_CONF_AES)
#ifndef NETSTACK_AES_KEY
#define NETSTACK_AES_KEY "thingsquare mist" /* 16 bytes */
#define NETSTACK_AES_KEY_DEFAULT 1
#endif /* NETSTACK_AES_KEY */
#endif /* ((MIST_CONF_NETSTACK) & MIST_CONF_AES) */

#define CONTIKI_TARGET_MIST_EXP5438 1

#define RIMESTATS_CONF_ON 1
#define RIMESTATS_CONF_ENABLED 1

#define HTONS(n) (((((uint16_t)(n))&0xff)<<8) | (((uint16_t)(n))>>8))
#define HTONL(n) ((((((uint32_t)(n)))&0xff)<<24) | (((((uint32_t)(n))>>8)&0xff)<<16) | (((((uint32_t)(n))>>16)&0xff)<<8) | (((((uint32_t)(n))>>24)&0xff)))
#define NTOHS(n) (((((uint16_t)(n))&0xff)<<8) | (((uint16_t)(n))>>8))
#define NTOHL(n) ((((((uint32_t)(n)))&0xff)<<24) | (((((uint32_t)(n))>>8)&0xff)<<16) | (((((uint32_t)(n))>>16)&0xff)<<8) | (((((uint32_t)(n))>>24)&0xff)))

#ifndef PACKET_TIME
#define PACKET_TIME (CLOCK_SECOND / 6) // 38.4kbps with TX delay
#endif

#ifndef WITH_LCD
#define WITH_LCD  0
#endif

#ifndef WITH_WDT
#define WITH_WDT  1
#endif

#ifndef WITH_OTA
#define WITH_OTA  0
#endif

#ifndef WITH_LPM
#define WITH_LPM  0
#endif

#ifndef WITH_MCU_LPM
#define WITH_MCU_LPM  1
#endif

#ifndef WITH_TASKMON
#define WITH_TASKMON  0
#endif

#ifndef WITH_DYNAMIC_TIMEBASE
#define WITH_DYNAMIC_TIMEBASE 0
#endif

#ifndef WITH_SERIAL_LINE
#define WITH_SERIAL_LINE  0
#endif

#ifndef NODEALARM
#define NODEALARM 1
#endif
#if NODEALARM
struct nodealarm {
  uint8_t radio;
  uint8_t sensor1;
  uint8_t sensor2;
  uint8_t flash1;
  uint8_t flash2;
  uint8_t rtc;
  uint8_t battery;
  uint8_t solarBat;
};
extern struct nodealarm nodealarm;
#endif

#ifndef NODESTATS
#define NODESTATS 0
#endif
#if NODESTATS
struct nodestats {
  uint32_t proc;
  uint32_t sleep;
  uint32_t wakeup;
  uint32_t radio;
  uint32_t timer;
  uint32_t uart1;
  uint32_t uart3;
  uint32_t epoll;
};
extern struct nodestats nodestats;
#define NODESTATS_ADD(x) nodestats.x++
#else
#define NODESTATS_ADD(x)
#endif

#ifndef ISRSTATS
#define ISRSTATS 0
#endif
#if ISRSTATS
enum {
  ISR_RADIO = 0,
  ISR_TIMER,
  ISR_UART1,
  ISR_UART3,
  ISR_MAX
};
typedef struct {
  unsigned short beg;
  unsigned short max;
  unsigned long  tot;
} isr_stat_t;
extern isr_stat_t isrStats[ISR_MAX];
extern uint8_t isrDepth;
extern uint8_t maxIsrDepth;
extern uint16_t isrEmbedN;
extern uint8_t isrQueue[ISR_MAX];
#define ISR_BEG(x) do { \
  isrStats[x].beg = RTIMER_NOW(); \
  isrQueue[isrDepth] = x; \
  ++isrDepth; \
  if (isrDepth > maxIsrDepth) { \
    maxIsrDepth = isrDepth; \
    if (maxIsrDepth > 1) \
      ++isrEmbedN; \
  } \
} while(0)
#define ISR_END(x) do { \
  rtimer_clock_t dt = 0; \
  --isrDepth; \
  if (RTIMER_NOW() > isrStats[x].beg) \
    dt = RTIMER_NOW() - isrStats[x].beg; \
  else \
    dt = RTIMER_ARCH_SECOND - isrStats[x].beg + RTIMER_NOW(); \
  if (dt > isrStats[x].max) \
    isrStats[x].max = dt; \
  isrStats[x].tot += dt; \
} while(0)
#else /* no ISRSTATS */
#define ISR_BEG(x)
#define ISR_END(x)
#endif /* ISRSTATS */

#endif /* CONTIKI_CONF_H */
