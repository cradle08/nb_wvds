/**
 * \addtogroup rimemesh
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
 *         A mesh routing protocol
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "net/rime.h"
#include "net/rime/route.h"
#include "net/rime/mesh.h"

#include <stddef.h> /* For offsetof */

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#if MESH_BROADCAST
static struct mesh_conn *mesh_c = NULL;
#endif
/*---------------------------------------------------------------------------*/
#if MESH_BROADCAST
static void
mesh_no_fwd(void *ptr)
{
  struct mesh_conn *c = (struct mesh_conn*)ptr;

  if (--(c->retry)) {
    PRINTF("mesh rexmit broadcast, %d retry left\n", c->retry);
    queuebuf_to_packetbuf(c->queued_data);
    multihop_resend(&c->multihop, &rimeaddr_null);

    ctimer_set(&c->fwd_wait, (PACKET_TIME << 3), mesh_no_fwd, c);
  }
  else {
    PRINTF("warn mesh fail at max broadcasts\n");
    queuebuf_free(c->queued_data);
    c->queued_data = NULL;
    if(c->cb->timedout)
      c->cb->timedout(c);
  }
}

void
mesh_sniff_rcvd(void)
{
  struct mesh_conn *c = mesh_c;
  const rimeaddr_t *dest = packetbuf_addr(PACKETBUF_ADDR_ERECEIVER);
  const rimeaddr_t *orig = packetbuf_addr(PACKETBUF_ADDR_ESENDER);
  const rimeaddr_t *from = packetbuf_addr(PACKETBUF_ADDR_SENDER);

  PRINTF("mesh snooped orig %d.%d dest %d.%d\n", orig->u8[0], orig->u8[1], dest->u8[0], dest->u8[1]);
  if (rimeaddr_cmp(orig, &rimeaddr_node_addr) && rimeaddr_cmp(dest, &c->queued_data_dest)) {
    PRINTF("mesh snooped forward\n");
    ctimer_stop(&c->fwd_wait);
    queuebuf_free(c->queued_data);
    c->queued_data = NULL;
    if(c->cb->sent)
      c->cb->sent(c);
  }
}

void
mesh_sniff_sent(int mac_status)
{
}

RIME_SNIFFER(mesh_sniff, mesh_sniff_rcvd, mesh_sniff_sent);
#endif
/*---------------------------------------------------------------------------*/
static void
data_packet_received(struct multihop_conn *multihop,
    const rimeaddr_t *from,
    const rimeaddr_t *prevhop, uint8_t hops)
{
  struct mesh_conn *c = (struct mesh_conn *)
    ((char *)multihop - offsetof(struct mesh_conn, multihop));
  struct route_entry *rt;

  PRINTF("mesh rcvd orig %d.%d from %d.%d hop %d\n",
      from->u8[0], from->u8[1], prevhop->u8[0], prevhop->u8[1], hops);

  /* Refresh the route when we hear a packet from a neighbor. */
  rt = route_lookup(from);
  if(rt != NULL) {
#if ROUTE_FAIL
    rt->fail = 0;
#endif
    route_refresh(rt);
  }

  if(c->cb->recv) {
    c->cb->recv(c, from, hops);
  }
}
/*---------------------------------------------------------------------------*/
static rimeaddr_t *
data_packet_forward(struct multihop_conn *multihop,
    const rimeaddr_t *originator,
    const rimeaddr_t *dest,
    const rimeaddr_t *prevhop, uint8_t hops)
{
#if !MESH_BROADCAST
  struct route_entry *rt;
#endif
  struct mesh_conn *c = (struct mesh_conn *)
    ((char *)multihop - offsetof(struct mesh_conn, multihop));
  int fwd = 1;

#if MESH_INTERCEPT
  if (c->cb->intercept)
    fwd = c->cb->intercept(c);
#endif
  if (!fwd) {
    PRINTF("mesh skip forward\n");
    return NULL;
  }

#if MESH_BROADCAST
  if(c->queued_data != NULL) {
    queuebuf_free(c->queued_data);
  }
  c->queued_data = queuebuf_new_from_packetbuf();
  rimeaddr_copy(&c->queued_data_dest, dest);
  return (rimeaddr_t *)&rimeaddr_null;

#else
  rt = route_lookup(dest);
  if(rt == NULL) {
    if(c->queued_data != NULL) {
      queuebuf_free(c->queued_data);
    }

    PRINTF("data_packet_forward: queueing data, sending rreq\n");
    c->queued_data = queuebuf_new_from_packetbuf();
    rimeaddr_copy(&c->queued_data_dest, dest);
    route_discovery_discover(&c->route_discovery_conn, dest, MESH_PKT_TIMEOUT);

    return NULL;
  } else {
    route_refresh(rt);
  }

  return &rt->nexthop;
#endif
}

#if MESH_SENT_AFTER
void
multihop_packet_sent(struct multihop_conn *mc, int status, int num_tx)
{
  struct mesh_conn *c = (struct mesh_conn *)
    ((char *)mc - offsetof(struct mesh_conn, multihop));
  const rimeaddr_t *dest = packetbuf_addr(PACKETBUF_ADDR_ERECEIVER);
  const rimeaddr_t *to = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
#if !MESH_BROADCAST
  struct route_entry *rt = NULL;
#endif

  PRINTF("mesh sent dest %d.%d to %d.%d, %d, %d\n", dest->u8[0], dest->u8[1], to->u8[0], to->u8[1], status, num_tx);

#if MESH_BROADCAST
  ctimer_set(&c->fwd_wait, (PACKET_TIME << 3), mesh_no_fwd, c);
#else
  rt = route_lookup(dest);
  if (rt != NULL) {
    if(status == MAC_TX_OK) {
      rt->fail = 0;
    }
    else if(status == MAC_TX_NOACK) {
      ++rt->fail;
      PRINTF("mesh route to %d.%d fail %d\n", dest->u8[0], dest->u8[1], rt->fail);
      if (rt->fail >= ROUTE_MAX_FAIL) {
        PRINTF("mesh remove route to %d.%d\n", dest->u8[0], dest->u8[1]);
        route_remove(rt);
      }
      route_discovery_discover(&c->route_discovery_conn, dest, MESH_PKT_TIMEOUT);
    }
  }

  if(status == MAC_TX_OK) {
    if(c->cb->sent)
      c->cb->sent(c);
  } else {
    if(c->cb->timedout)
      c->cb->timedout(c);
  }
#endif
}
#endif
/*---------------------------------------------------------------------------*/
static void
found_route(struct route_discovery_conn *rdc, const rimeaddr_t *dest)
{
  struct route_entry *rt;
  struct mesh_conn *c = (struct mesh_conn *)
    ((char *)rdc - offsetof(struct mesh_conn, route_discovery_conn));

  PRINTF("mesh found route to %d.%d\n", dest->u8[0], dest->u8[1]);

  if(c->queued_data != NULL &&
     rimeaddr_cmp(dest, &c->queued_data_dest)) {
    queuebuf_to_packetbuf(c->queued_data);
    queuebuf_free(c->queued_data);
    c->queued_data = NULL;

    rt = route_lookup(dest);
    if(rt != NULL) {
      PRINTF("mesh resend to %d.%d\n", rt->nexthop.u8[0], rt->nexthop.u8[1]);
      multihop_resend(&c->multihop, &rt->nexthop);
#if !MESH_SENT_AFTER
      if(c->cb->sent != NULL) {
        c->cb->sent(c);
      }
#endif
    } else {
      if(c->cb->timedout != NULL) {
        c->cb->timedout(c);
      }
    }
  } else {
    if (c->queued_data == NULL) PRINTF("mesh no pkt to send\n");
    else if (!rimeaddr_cmp(dest, &c->queued_data_dest)) PRINTF("mesh not queued dest %d.%d\n", c->queued_data_dest.u8[0], c->queued_data_dest.u8[1]);
  }
}
/*---------------------------------------------------------------------------*/
static void
route_timed_out(struct route_discovery_conn *rdc)
{
  struct mesh_conn *c = (struct mesh_conn *)
    ((char *)rdc - offsetof(struct mesh_conn, route_discovery_conn));

  PRINTF("mesh route timedout\n");
  if(c->queued_data != NULL) {
    PRINTF("mesh free pkt\n");
    queuebuf_free(c->queued_data);
    c->queued_data = NULL;
  }

  if(c->cb->timedout) {
    c->cb->timedout(c);
  }
}
/*---------------------------------------------------------------------------*/
#if MESH_SENT_AFTER
static const struct multihop_callbacks data_callbacks = { data_packet_received,
  data_packet_forward, multihop_packet_sent };
#else
static const struct multihop_callbacks data_callbacks = { data_packet_received,
						    data_packet_forward };
#endif
static const struct route_discovery_callbacks route_discovery_callbacks =
  { found_route, route_timed_out };
/*---------------------------------------------------------------------------*/
void
mesh_open(struct mesh_conn *c, uint16_t channels,
    const struct mesh_callbacks *callbacks)
{
  route_init();
  multihop_open(&c->multihop, channels, &data_callbacks);
  route_discovery_open(&c->route_discovery_conn,
      CLOCK_SECOND * 2,
      channels + 1,
      &route_discovery_callbacks);
  c->cb = callbacks;
#if MESH_BROADCAST
  mesh_c = c;
  rime_sniffer_add(&mesh_sniff);
#endif
}
/*---------------------------------------------------------------------------*/
void
mesh_close(struct mesh_conn *c)
{
  multihop_close(&c->multihop);
  route_discovery_close(&c->route_discovery_conn);
#if MESH_BROADCAST
  mesh_c = NULL;
#endif
}
/*---------------------------------------------------------------------------*/
int
mesh_send(struct mesh_conn *c, const rimeaddr_t *to)
{
  int could_send;

  PRINTF("%d.%d: mesh_send to %d.%d\n",
      rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
      to->u8[0], to->u8[1]);

#if MESH_BROADCAST
  c->retry = MESH_MAX_REXMIT;
#endif
  could_send = multihop_send(&c->multihop, to);

  if(!could_send) {
    PRINTF("mesh_send: could not send\n");
    return 0;
  }
#if !MESH_SENT_AFTER
  if(c->cb->sent != NULL) {
    c->cb->sent(c);
  }
#endif
  return 1;
}

#if MESH_BROADCAST
int
mesh_cancel(struct mesh_conn *c)
{
  if (c->queued_data != NULL) {
    ctimer_stop(&c->fwd_wait);
    queuebuf_free(c->queued_data);
    c->queued_data = NULL;
    return 0;
  }
  return 1;
}
#endif
/*---------------------------------------------------------------------------*/
int
mesh_ready(struct mesh_conn *c)
{
  return (c->queued_data == NULL);
}


/** @} */
