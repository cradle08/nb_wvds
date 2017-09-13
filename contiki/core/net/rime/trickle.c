/**
 * \addtogroup trickle
 * @{
 */

/*
 * Copyright (c) 2007, Swedish Institute of Computer Science.
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

/**
 * \file
 *         Trickle (reliable single source flooding) for Rime
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "net/rime/trickle.h"
#include "lib/random.h"
#include "dev/radio.h"

#if CONTIKI_TARGET_NETSIM
#include "ether.h"
#endif

#define INTERVAL_MIN 1
#define INTERVAL_MAX 16

#define DUPLICATE_THRESHOLD 1

#define SEQNO_LT(a, b) ((signed char)((a) - (b)) < 0)

static const struct packetbuf_attrlist attributes[] =
  {
    TRICKLE_ATTRIBUTES PACKETBUF_ATTR_LAST
  };


#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static int run_trickle(struct trickle_conn *c);
/*---------------------------------------------------------------------------*/
#if WITH_LPM
static struct radio_res *trickle_rs = NULL;
#endif
/*---------------------------------------------------------------------------*/
#if WITH_LPM
static void
timedout(void *ptr)
{
  PRINTF("trickle off radio at timedout\n");
  radio_off(trickle_rs);
}
#endif
/*---------------------------------------------------------------------------*/
static void
send(void *ptr)
{
  struct trickle_conn *c = ptr;

  if(c->q != NULL) {
    PRINTF("trickle send\n");
#if WITH_LPM
    if (ctimer_expired(&c->timeout)) {
      PRINTF("trickle on radio\n");
      radio_on(trickle_rs, (PACKET_TIME << 3));
      ctimer_set(&c->timeout, (PACKET_TIME << 3), timedout, NULL);
    }
#endif
    queuebuf_to_packetbuf(c->q);
    broadcast_send(&c->c);
  } else {
    PRINTF("%d.%d: trickle send but c->q == NULL\n",
            rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1]);
  }
}
/*---------------------------------------------------------------------------*/
static void
timer_callback(void *ptr)
{
  struct trickle_conn *c = ptr;
  run_trickle(c);
}
/*---------------------------------------------------------------------------*/
#if TRICKLE_PERIODIC
static void
reset_interval(struct trickle_conn *c)
{
  PT_INIT(&c->pt);
  run_trickle(c);
}
#endif
/*---------------------------------------------------------------------------*/
static void
set_timer(struct trickle_conn *c, struct ctimer *t, clock_time_t i)
{
  ctimer_set(t, i, timer_callback, c);
}
/*---------------------------------------------------------------------------*/
static int
run_trickle(struct trickle_conn *c)
{
  clock_time_t interval;
  clock_time_t t;
  PT_BEGIN(&c->pt);

  while(1) {
    interval = c->interval << c->interval_scaling;
    t = interval / 2 + (random_rand() % (interval / 2));
    set_timer(c, &c->interval_timer, interval);
    set_timer(c, &c->t, t);
    PRINTF("trickle run %d/%d\n", interval, t);

    c->duplicates = 0;
    PT_YIELD(&c->pt); /* Wait until listen timeout */
    if(c->duplicates < DUPLICATE_THRESHOLD) {
      send(c);
    }
    PT_YIELD(&c->pt); /* Wait until interval timer expired. */
    if(c->interval_scaling < c->scaling_max) {
      c->interval_scaling++;
    }
  }

  PT_END(&c->pt);
}
/*---------------------------------------------------------------------------*/
static void
recv(struct broadcast_conn *bc, const rimeaddr_t *from)
{
  struct trickle_conn *c = (struct trickle_conn *)bc;
  uint16_t seqno = packetbuf_attr(PACKETBUF_ATTR_EPACKET_ID);

  PRINTF("trickle recv seqno %d from %d.%d our %d data len %d channel %d\n",
          seqno,
          from->u8[0], from->u8[1],
          c->seqno,
          packetbuf_datalen(),
          packetbuf_attr(PACKETBUF_ATTR_CHANNEL));

#if WITH_LPM
  if (!ctimer_expired(&c->timeout)) {
    PRINTF("trickle off radio at recv\n");
    ctimer_stop(&c->timeout);
    radio_off(trickle_rs);
  }
#endif

  if(seqno == c->seqno) {
    /*c->cb->recv(c);*/
    ++c->duplicates;

  } else if(SEQNO_LT(seqno, c->seqno)) {
#if TRICKLE_FORWARD
    c->interval_scaling = 0;
    send(c);
#endif

  } else { /* hdr->seqno > c->seqno */
#if CONTIKI_TARGET_NETSIM
    /*ether_set_line(from->u8[0], from->u8[1]);*/
#endif /* CONTIKI_TARGET_NETSIM */
    c->seqno = seqno;
    /* Store the incoming data in the queuebuf */
    if(c->q != NULL) {
      queuebuf_free(c->q);
    }
    c->q = queuebuf_new_from_packetbuf();
#if TRICKLE_FORWARD
    c->interval_scaling = 0;
    reset_interval(c);
    PRINTF("trickle will send in rand(%d)\n", c->interval);
    ctimer_set(&c->first_transmission_timer, random_rand() % c->interval,
            send, c);
#endif
    c->cb->recv(c);
  }
}
/*---------------------------------------------------------------------------*/
static CC_CONST_FUNCTION struct broadcast_callbacks bc = { recv };
/*---------------------------------------------------------------------------*/
void
trickle_open(struct trickle_conn *c, clock_time_t interval,
        uint16_t channel, const struct trickle_callbacks *cb)
{
  broadcast_open(&c->c, channel, &bc);
  c->cb = cb;
  c->q = NULL;
  c->interval = interval;
  c->interval_scaling = 0;
  c->scaling_max = INTERVAL_MAX;
  channel_set_attributes(channel, attributes);
#if WITH_LPM
  trickle_rs = radio_alloc("trickle");
#endif
}
/*---------------------------------------------------------------------------*/
void
trickle_close(struct trickle_conn *c)
{
  broadcast_close(&c->c);
  ctimer_stop(&c->t);
  ctimer_stop(&c->interval_timer);
}
/*---------------------------------------------------------------------------*/
void
trickle_send(struct trickle_conn *c)
{
  if(c->q != NULL) {
    queuebuf_free(c->q);
  }
  c->seqno++;
  packetbuf_set_attr(PACKETBUF_ATTR_EPACKET_ID, c->seqno);
  c->q = queuebuf_new_from_packetbuf();
  PRINTF("trickle send seqno %d\n",
          c->seqno);
#if TRICKLE_PERIODIC
  c->interval_scaling = 0;
  reset_interval(c);
#endif
  send(c);
}

void
trickle_resend(struct trickle_conn *c)
{
  if(c->q != NULL) {
    PRINTF("trickle resend\n");
    queuebuf_to_packetbuf(c->q);
    broadcast_send(&c->c);
  }
}
/*---------------------------------------------------------------------------*/
void
trickle_set_max_interval(struct trickle_conn *c, uint32_t interval)
{
  uint8_t max = 0;
  uint32_t maxint = c->interval;

  interval *= CLOCK_SECOND;
  while (maxint < interval) {
    maxint <<= 1;
    max += 1;
  }
  c->scaling_max = max;
  PRINTF("trickle set scale max %u for %lu\n", max, interval);
}
/*---------------------------------------------------------------------------*/
/** @} */
