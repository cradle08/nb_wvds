/**
 * \addtogroup rimeroute
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
 *         Rime route table
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include <stdio.h>
#include <string.h>

#include "lib/list.h"
#include "lib/memb.h"
#include "sys/ctimer.h"
#include "net/rime/route.h"
#include "contiki-conf.h"
#include "app.h"

#ifdef ROUTE_CONF_ENTRIES
#define NUM_RT_ENTRIES ROUTE_CONF_ENTRIES
#else /* ROUTE_CONF_ENTRIES */
#define NUM_RT_ENTRIES 8
#endif /* ROUTE_CONF_ENTRIES */

#ifdef ROUTE_CONF_DECAY_THRESHOLD
#define DECAY_THRESHOLD ROUTE_CONF_DECAY_THRESHOLD
#else /* ROUTE_CONF_DECAY_THRESHOLD */
#define DECAY_THRESHOLD 8
#endif /* ROUTE_CONF_DECAY_THRESHOLD */

#ifdef ROUTE_CONF_DEFAULT_LIFETIME
#define DEFAULT_LIFETIME ROUTE_CONF_DEFAULT_LIFETIME
#else /* ROUTE_CONF_DEFAULT_LIFETIME */
#define DEFAULT_LIFETIME 60
#endif /* ROUTE_CONF_DEFAULT_LIFETIME */

/*
 * List of route entries.
 */
LIST(route_table);
MEMB(route_mem, struct route_entry, NUM_RT_ENTRIES);

static struct ctimer t;

static int max_time = DEFAULT_LIFETIME;

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/*---------------------------------------------------------------------------*/
static void
periodic(void *ptr)
{
  struct route_entry *e;

  for(e = list_head(route_table); e != NULL; e = list_item_next(e)) {
    e->time++;
    if(e->time >= max_time) {
      PRINTF("route periodic: removing entry to %d.%d with nexthop %d.%d and cost %d\n",
          e->dest.u8[0], e->dest.u8[1],
          e->nexthop.u8[0], e->nexthop.u8[1],
          e->cost);
      route_remove(e);
    }
  }

  ctimer_set(&t, CLOCK_SECOND, periodic, NULL);
}
/*---------------------------------------------------------------------------*/
void
route_init(void)
{
  list_init(route_table);
  memb_init(&route_mem);
  PRINTF("route init %d\n", NUM_RT_ENTRIES);

  ctimer_set(&t, CLOCK_SECOND, periodic, NULL);

#if ROUTE_NV
  struct nv_route_table tbl;
  struct route_entry *e;
  struct nv_route_entry *ne;
  uint8_t buf[64];
  uint32_t addr;
  uint16_t left;
  uint8_t  empty[4] = {0xFF,0xFF,0xFF,0xFF};
  uint16_t nid;
  uint8_t  i;

  nv_read(NV_ROUTE_ADDR, (uint8_t*)&tbl, sizeof(struct nv_route_table));
  if (tbl.magic != NV_MAGIC) {
    tbl.magic = NV_MAGIC;
    tbl.count = 0;
    nv_erase(NV_ROUTE_ADDR, NV_ROUTE_SIZE);
    nv_write(NV_ROUTE_ADDR, (uint8_t*)&tbl, sizeof(struct nv_route_table));
  }

  addr = NV_ROUTE_ADDR;
  left = NV_ROUTE_SIZE;
  nid  = 0;
  while (left > 0) {
    nv_read(addr, buf, sizeof(buf));
    for (i = 0; i < sizeof(buf)/sizeof(struct nv_route_entry); i++) {
      if (nid > 0 && memcmp(buf + 4*i, empty, 4) != 0) {
        e = memb_alloc(&route_mem);
        if (e != NULL) {
          ne = (struct nv_route_entry *)(buf + 4*i);
          e->dest.u8[0] = (nid & 0xff);
          e->dest.u8[1] = (nid >> 8);
          rimeaddr_copy(&e->nexthop, &ne->nexthop);
          e->cost = ne->cost;
          e->seqno = 0;
          e->time = 0;
          e->decay = 0;
          list_add(route_table, e);
        } else {
          left = 0; // to quit outer while loop
          break; // quit inner for loop
        }
      }
      ++nid;
    }
    if (left == 0) {
      break;
    } else {
      addr += sizeof(buf);
      left -= sizeof(buf);
    }
  }
#endif
}
/*---------------------------------------------------------------------------*/
struct route_entry *
route_drop(void)
{
  struct route_entry *e;

  /* Remove oldest entry.  XXX */
  e = list_chop(route_table);
  if (e != NULL)
    PRINTF("route_drop: removing entry to %d.%d with nexthop %d.%d and cost %d\n",
      e->dest.u8[0], e->dest.u8[1],
      e->nexthop.u8[0], e->nexthop.u8[1],
      e->cost);

#if ROUTE_NV
  if (e != NULL) {
    struct nv_route_entry ne;
    uint16_t nid;
    uint16_t addr;

    nid = e->dest.u8[0] + (e->dest.u8[1] << 8);
    if (((nid + 1) * sizeof(struct nv_route_entry)) <= NV_ROUTE_SIZE) {
      nv_read(NV_ROUTE_ADDR + nid * sizeof(struct nv_route_entry),
          (uint8_t*)&ne, sizeof(struct nv_route_entry));
      if (!rimeaddr_cmp(&ne.nexthop, &e->nexthop)) {
        rimeaddr_copy(&ne.nexthop, &e->nexthop);
        ne.cost = e->cost;
        ne.reserved = 0;
        addr = NV_ROUTE_ADDR + nid * sizeof(struct nv_route_entry);
        PRINTF("route_drop: write %d at 0x%04X, %02X %02X %02X %02X\n", nid, addr, ne.nexthop.u8[0], ne.nexthop.u8[1], ne.cost, ne.reserved);
        nv_write(addr, (uint8_t*)&ne, sizeof(struct nv_route_entry));
      }
    }
  }
#endif

  return e;
}
/*---------------------------------------------------------------------------*/
int
route_add(const rimeaddr_t *dest, const rimeaddr_t *nexthop,
    uint8_t cost, uint8_t seqno)
{
  struct route_entry *e;

  /* Avoid inserting duplicate entries. */
  e = route_lookup(dest);
  if(e != NULL && rimeaddr_cmp(&e->nexthop, nexthop)) {
    list_remove(route_table, e);
  } else {
#if ROUTE_NV
    if (e == NULL) {
      struct nv_route_entry ne;
      uint16_t nid;
      uint16_t addr;

      nid = dest->u8[0] + (dest->u8[1] << 8);
      if (((nid + 1) * sizeof(struct nv_route_entry)) <= NV_ROUTE_SIZE) {
        rimeaddr_copy(&ne.nexthop, nexthop);
        ne.cost = cost;
        addr = NV_ROUTE_ADDR + nid * sizeof(struct nv_route_entry);
        PRINTF("route_add: write %d at 0x%04X, %02X %02X %02X %02X\n", nid, addr, ne.nexthop.u8[0], ne.nexthop.u8[1], ne.cost, ne.reserved);
        nv_write(addr, (uint8_t*)&ne, sizeof(struct nv_route_entry));
      }
    }
#endif
    /* Allocate a new entry or reuse the oldest entry with highest cost. */
    e = memb_alloc(&route_mem);
    if(e == NULL) {
      e = route_drop();
    }
  }

  if (e != NULL) {
    rimeaddr_copy(&e->dest, dest);
    rimeaddr_copy(&e->nexthop, nexthop);
    e->cost = cost;
    e->seqno = seqno;
    e->time = 0;
    e->decay = 0;

    /* New entry goes first. */
    list_push(route_table, e);

    PRINTF("route_add: new entry to %d.%d with nexthop %d.%d and cost %d\n",
        e->dest.u8[0], e->dest.u8[1],
        e->nexthop.u8[0], e->nexthop.u8[1],
        e->cost);
  } else {
    PRINTF("route_add: fail\n");
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
struct route_entry *
route_lookup(const rimeaddr_t *dest)
{
  struct route_entry *e;
  uint8_t lowest_cost;
  struct route_entry *best_entry;

  lowest_cost = -1;
  best_entry = NULL;

  /* Find the route with the lowest cost. */
  for(e = list_head(route_table); e != NULL; e = list_item_next(e)) {
    /*printf("route_lookup: comparing %d.%d.%d.%d with %d.%d.%d.%d\n",
      uip_ipaddr_to_quad(dest), uip_ipaddr_to_quad(&e->dest));*/

    if(rimeaddr_cmp(dest, &e->dest)) {
      //PRINTF("route_lookup: check %d.%d->%d.%d, cost %d\n",
      //    e->nexthop.u8[0], e->nexthop.u8[1], e->dest.u8[0], e->dest.u8[1], e->cost);
      if(e->cost < lowest_cost) {
        //PRINTF("route_lookup: replace\n");
        best_entry = e;
        lowest_cost = e->cost;
      }
    }
  }
  if (best_entry != NULL) {
    e = best_entry;
    PRINTF("route_lookup: got %d.%d->%d.%d, cost %d\n",
        e->nexthop.u8[0], e->nexthop.u8[1], e->dest.u8[0], e->dest.u8[1], e->cost);
  }

#if ROUTE_NV
  if (best_entry == NULL) {
    struct nv_route_entry ne;
    uint8_t empty[4] = {0xFF,0xFF,0xFF,0xFF};
    uint16_t nid;

    nid = dest->u8[0] + (dest->u8[1] << 8);
    if (((nid + 1) * sizeof(struct nv_route_entry)) <= NV_ROUTE_SIZE) {
      nv_read(NV_ROUTE_ADDR + nid * sizeof(struct nv_route_entry),
          (uint8_t*)&ne, sizeof(struct nv_route_entry));
      if (memcmp(&ne, empty, 4) != 0) {
        PRINTF("route_lookup: read %02X %02X %02X %02X\n",
            ne.nexthop.u8[0], ne.nexthop.u8[1], ne.cost, ne.reserved);
        e = memb_alloc(&route_mem);
        if(e == NULL) {
          e = route_drop();
        }
        if (e != NULL) {
          memset(e, 0, sizeof(struct route_entry));
          rimeaddr_copy(&e->dest, dest);
          rimeaddr_copy(&e->nexthop, &ne.nexthop);
          e->cost = ne.cost;
          list_push(route_table, e);
          best_entry = e;
          PRINTF("route_lookup: got %d.%d->%d.%d, cost %d\n",
              e->nexthop.u8[0], e->nexthop.u8[1], e->dest.u8[0], e->dest.u8[1], e->cost);
        }
      }
    }
  }
#endif

  return best_entry;
}
/*---------------------------------------------------------------------------*/
void
route_refresh(struct route_entry *e)
{
  if(e != NULL) {
    /* Refresh age of route so that used routes do not get thrown
       out. */
    e->time = 0;
    e->decay = 0;

    PRINTF("route_refresh: time %d last %d decay %d for entry to %d.%d with nexthop %d.%d and cost %d\n",
           e->time, e->time_last_decay, e->decay,
           e->dest.u8[0], e->dest.u8[1],
           e->nexthop.u8[0], e->nexthop.u8[1],
           e->cost);

  }
}
/*---------------------------------------------------------------------------*/
void
route_decay(struct route_entry *e)
{
  /* If routes are not refreshed, they decay over time. This function
     is called to decay a route. The route can only be decayed once
     per second. */
  PRINTF("route_decay: time %d last %d decay %d for entry to %d.%d with nexthop %d.%d and cost %d\n",
      e->time, e->time_last_decay, e->decay,
      e->dest.u8[0], e->dest.u8[1],
      e->nexthop.u8[0], e->nexthop.u8[1],
      e->cost);

  if(e->time != e->time_last_decay) {
    /* Do not decay a route too often - not more than once per second. */
    e->time_last_decay = e->time;
    e->decay++;

    if(e->decay >= DECAY_THRESHOLD) {
      PRINTF("route_decay: removing entry to %d.%d with nexthop %d.%d and cost %d\n",
          e->dest.u8[0], e->dest.u8[1],
          e->nexthop.u8[0], e->nexthop.u8[1],
          e->cost);
      route_remove(e);
    }
  }
}
/*---------------------------------------------------------------------------*/
void
route_remove(struct route_entry *e)
{
  PRINTF("route_remove: %d.%d->%d.%d,%d\n",
      e->nexthop.u8[0],e->nexthop.u8[1],e->dest.u8[0],e->dest.u8[1],e->cost);
  list_remove(route_table, e);
  memb_free(&route_mem, e);

#if ROUTE_NV
  uint8_t empty[4] = {0xFF,0xFF,0xFF,0xFF};
  uint16_t nid = 0;
  uint16_t addr = 0;

  nid = e->dest.u8[0] + (e->dest.u8[1] << 8);
  if (((nid + 1) * sizeof(struct nv_route_entry)) <= NV_ROUTE_SIZE) {
    addr = NV_ROUTE_ADDR + nid * sizeof(struct nv_route_entry);
    PRINTF("route_remove: clear %d at %04X\n", nid, addr);
    nv_write(addr, empty, sizeof(struct nv_route_entry));
  }
#endif
}
/*---------------------------------------------------------------------------*/
void
route_flush_all(void)
{
  struct route_entry *e;

  while(1) {
    e = list_pop(route_table);
    if(e != NULL) {
      memb_free(&route_mem, e);
    } else {
      break;
    }
  }

#if ROUTE_NV
  nv_erase(NV_ROUTE_ADDR, NV_ROUTE_SIZE);
#endif
}
/*---------------------------------------------------------------------------*/
void
route_set_lifetime(int seconds)
{
  max_time = seconds;
}
/*---------------------------------------------------------------------------*/
int
route_num(void)
{
  struct route_entry *e;
  int i = 0;

  for(e = list_head(route_table); e != NULL; e = list_item_next(e)) {
    i++;
  }
  return i;
}
/*---------------------------------------------------------------------------*/
struct route_entry *
route_get(int num)
{
  struct route_entry *e;
  int i = 0;

  for(e = list_head(route_table); e != NULL; e = list_item_next(e)) {
    if(i == num) {
      return e;
    }
    i++;
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
/** @} */
