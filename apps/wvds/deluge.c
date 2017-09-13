/*
 * Copyright (c) 2007, Swedish Institute of Computer Science
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
 *  An implementation of the Deluge protocol.
 *      (Hui and Culler: The dynamic behavior of a data
 *      dissemination protocol for network programming at scale,
 *      ACM SenSys 2004)
 * \author
 *  Nicolas Tsiftes <nvt@sics.se>
 */

#include "contiki.h"
#include "net/rime.h"
#include "cfs/cfs.h"
#include "loader/elfloader.h"
#include "lib/crc16.h"
#include "lib/random.h"
#include "dev/radio.h"
#include "dev/leds.h"
#include "node-id.h"
#include "deluge.h"
#include "app.h"

#if NETSIM
#include "ether.h"
#include <stdio.h>
#endif

#include <stdlib.h>
#include <string.h>

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#ifndef MAC_LEN
#define MAC_LEN 8
#endif

#ifndef DELUGE_NV
#define DELUGE_NV 1
#endif

#if DELUGE_NV
extern struct OTA ota;
#endif

// Must be 2^n, i.e. 2,4,8,16,...
#define DELUGE_RAND_N 4
#define DELUGE_RAND_MASK (DELUGE_RAND_N - 1)

/* Implementation-specific variables. */
static struct broadcast_conn deluge_broadcast;
static struct unicast_conn deluge_uc;
static struct deluge_object current_object = {
  .object_id = 0xff,
  .size = 0,
  .version = 0,
  .update_version = 0,
  .avail_pages = 0,
  .current_rx_page = 0,
  .current_tx_page = -1,
  .nrequests = 0,
  .tx_set = 0,
  .cfs_fd = -1
};
//static process_event_t deluge_event;

static struct deluge_object_meta objects[OBJ_COUNT];
static uint8_t deluge_mode = 0;
static uint8_t deluge_target[MAC_LEN] = {0};
const uint8_t deluge_all[MAC_LEN] = {0};

/* Deluge variables. */
static int deluge_state;
static int deluge_paused = 0;
//static int old_summary;
static int neighbor_inconsistency;
static unsigned r_interval;
static unsigned recv_adv;
static int broadcast_profile;

/* Deluge timers. */
static struct ctimer rx_timer;
static struct ctimer tx_timer;
static struct ctimer summary_timer;
static struct ctimer profile_timer;
static struct ctimer timeout;
static uint8_t timeout_r = 0;

/* Deluge objects will get an ID that defaults to the current value of
   the next_object_id parameter. */
//static deluge_object_id_t next_object_id;

const struct deluge_callbacks *deluge_callback;

#if WITH_LPM
static struct radio_res *deluge_rs = NULL;
#endif
/*-----------------------------------------------------------------------------*/
void send_profile(void *ptr);

/* Rime callbacks. */
static void broadcast_recv(struct broadcast_conn *, const rimeaddr_t *);
static void unicast_recv(struct unicast_conn *, const rimeaddr_t *);

static const struct broadcast_callbacks broadcast_call = {broadcast_recv, NULL};
static const struct unicast_callbacks unicast_call = {unicast_recv, NULL};

/*-----------------------------------------------------------------------------*/
/* The Deluge process manages the main Deluge timer. */
PROCESS(deluge_process, "Deluge");
/*-----------------------------------------------------------------------------*/
int
deluge_start(const struct deluge_callbacks *cb)
{
  deluge_callback = cb;
  broadcast_open(&deluge_broadcast, DELUGE_BROADCAST_CHANNEL, &broadcast_call);
  unicast_open(&deluge_uc, DELUGE_UNICAST_CHANNEL, &unicast_call);
#if WITH_LPM
  deluge_rs = radio_alloc("deluge");
#endif
  return 0;
}

int
deluge_pause(void)
{
  deluge_paused = 1;
  return 0;
}

int
deluge_resume(void)
{
  deluge_paused = 0;
  return 0;
}

static void
transition(int state)
{
  if(state != deluge_state) {
    switch(deluge_state) {
    case DELUGE_STATE_MAINTAIN:
      ctimer_stop(&summary_timer);
      ctimer_stop(&profile_timer);
      break;
    case DELUGE_STATE_RX:
      ctimer_stop(&rx_timer);
      break;
    case DELUGE_STATE_TX:
      ctimer_stop(&tx_timer);
      break;
    }
    PRINTF("deluge s%d->s%d\n", deluge_state, state);
    deluge_state = state;
  }
}

static int
write_page(struct deluge_object *obj, unsigned pagenum, unsigned char *data)
{
  cfs_offset_t offset;

  offset = (uint32_t)pagenum * S_PAGE;

  PRINTF("deluge write page %d at %d:0x%04X\n", pagenum, obj->cfs_fd, offset);
  if(cfs_seek(obj->cfs_fd, offset, CFS_SEEK_SET) != offset) {
    return -1;
  }
  return cfs_write(obj->cfs_fd, (char *)data, S_PAGE);
}

static int
read_page(struct deluge_object *obj, unsigned pagenum, unsigned char *buf)
{
  cfs_offset_t offset;

  offset = (uint32_t)pagenum * S_PAGE;

  PRINTF("deluge read page %d at %d:0x%04X\n", pagenum, obj->cfs_fd, offset);
  if(cfs_seek(obj->cfs_fd, offset, CFS_SEEK_SET) != offset) {
    return -1;
  }
  return cfs_read(obj->cfs_fd, (char *)buf, S_PAGE);
}

#if DELUGE_PAGES
static void
init_page(struct deluge_object *obj, int pagenum, int have)
{
  struct deluge_page *page;
  unsigned char buf[S_PAGE];

  page = &obj->pages[pagenum];

  page->flags = 0;
  page->last_request = 0;
  page->last_data = 0;

  if(have) {
    page->version = obj->version;
    page->packet_set = ALL_PACKETS;
    page->flags |= PAGE_COMPLETE;
    read_page(obj, pagenum, buf);
    page->crc = crc16_data(buf, S_PAGE, 0);
  } else {
    page->version = 0;
    page->packet_set = 0;
  }
}
#endif

#if 0
static cfs_offset_t
file_size(const char *file)
{
  int fd;
  cfs_offset_t size;

  fd = cfs_open(file, CFS_READ);
  if(fd < 0) {
    return (cfs_offset_t)-1;
  }

  size = cfs_seek(fd, 0, CFS_SEEK_END);
  cfs_close(fd);

  return size;
}
#endif

static int
init_object(uint8_t objid, struct deluge_object *obj, char *filename, unsigned version)
{
#if DELUGE_PAGES
  static struct deluge_page *page;
  int i;
#endif

  if (obj->cfs_fd >= 0) {
    cfs_close(obj->cfs_fd);
    obj->cfs_fd = -1;
  }

  obj->cfs_fd = cfs_open(filename, CFS_READ | CFS_WRITE);
  if(obj->cfs_fd < 0) {
    PRINTF("warn deluge fail open %s\n", filename);
    return -1;
  }

#if DELUGE_NV
  obj->filename = filename;
  obj->object_id = objid;
  obj->size = ota.images[objid].size;
  obj->version = obj->update_version = version;
  obj->current_rx_page = ota.images[objid].rcvd;
  obj->nrequests = 0;
  obj->tx_set = 0;
#else
  obj->filename = filename;
  obj->object_id = objid;
  //obj->size = file_size(filename);
  obj->size = cfs_seek(obj->cfs_fd, 0, CFS_SEEK_END);
  obj->version = obj->update_version = version;
  obj->current_rx_page = 0;
  obj->nrequests = 0;
  obj->tx_set = 0;
  PRINTF("deluge init object %s, fd %d, ver %d\n", obj->filename, obj->cfs_fd, obj->version);
#endif

#if DELUGE_PAGES
#if DELUGE_MALLOC
  obj->pages = malloc(OBJECT_PAGE_COUNT(*obj) * sizeof(*obj->pages));
  if(obj->pages == NULL) {
    cfs_close(obj->cfs_fd);
    return -1;
  }
#endif /* DELUGE_MALLOC */

  for(i = 0; i < OBJECT_PAGE_COUNT(*obj); i++) {
    page = &obj->pages[i];
    init_page(obj, i, 1);
  }
#else /* not DELUGE_PAGES */
  memset(&obj->cur_page, 0, sizeof(struct deluge_page));
#if DELUGE_NV
  obj->cur_page.version = ota.images[objid].ver;
  obj->avail_pages = ota.images[objid].rcvd;
#else
  obj->avail_pages = OBJECT_PAGE_COUNT(*obj);
#endif
#endif /* DELUGE_PAGES */
  memset(obj->current_page, 0, sizeof(obj->current_page));

  return 0;
}

static int
load_object(uint8_t objid)
{
  char fname[11] = {0};
  uint8_t verno = 0;

  if (objid == OBJ_GATEWAY) { strncpy(fname, "AP", 2); }
  else if (objid == OBJ_ROUTER) { strncpy(fname, "RP", 2); }
  else if (objid == OBJ_DEVICE) { strncpy(fname, "VD", 2); }
  verno = ota.images[objid].ver;

  PRINTF("deluge load object %d, %s, v%d\n", objid, fname, verno);
  return init_object(objid, &current_object, fname, verno);
}

static int
highest_available_page(struct deluge_object *obj)
{
  int i = 0;

#if DELUGE_PAGES
  for(i = 0; i < OBJECT_PAGE_COUNT(*obj); i++) {
    if(!(obj->pages[i].flags & PAGE_COMPLETE)) {
      break;
    }
  }
#else
  i = current_object.avail_pages;
#endif

  return i;
}

static void
send_request(void *arg)
{
  struct deluge_object *obj;
  struct deluge_msg_request request;

  obj = (struct deluge_object *)arg;

  request.cmd = DELUGE_CMD_REQUEST;
  request.mode = deluge_mode;
  memcpy(request.target, deluge_target, MAC_LEN);
  request.pagenum[0] = (obj->current_rx_page >> 8);
  request.pagenum[1] = (obj->current_rx_page & 0xff);
#if DELUGE_PAGES
  request.version = obj->pages[request.pagenum].version;
  request.request_set = ~obj->pages[obj->current_rx_page].packet_set;
#else
  request.version = obj->update_version;
  request.request_set = (ALL_PACKETS - current_object.cur_page.packet_set);
#endif
  request.object_id = obj->object_id;
  memcpy(request.mac, node_mac, MAC_LEN);

  PRINTF("deluge sending request to %d.%d for page %d, version %u, request_set 0x%02X\n",
         obj->summary_from.u8[0], obj->summary_from.u8[1],
         request.pagenum, request.version, request.request_set);
  packetbuf_copyfrom(&request, sizeof(request));
  unicast_send(&deluge_uc, &obj->summary_from);

#define MIN(a,b) (((a)<(b))?((a)):((b)))
  /* Deluge R.2 */
  if (deluge_mode == DELUGE_MODE_ONENODE) {
    clock_time_t t = 0;
    t = (PACKET_TIME * (N_PKT + 2)) << MIN((obj->nrequests >> 1), 4);
    if (obj->nrequests < 255) obj->nrequests++;
    ctimer_set(&rx_timer, t, send_request, &current_object);
  }
  else if (deluge_mode == DELUGE_MODE_ONEHOP) {
    clock_time_t t = 0;
    t = PACKET_TIME * (DELUGE_RAND_N + N_PKT + (random_rand() & DELUGE_RAND_MASK));
    ctimer_set(&rx_timer, t, send_request, &current_object);
  }
  else {
    if(++obj->nrequests == CONST_LAMBDA) {
      /* XXX check rate here too. */
      obj->nrequests = 0;
      transition(DELUGE_STATE_MAINTAIN);
    } else {
      ctimer_reset(&rx_timer);
    }
  }
}

#if WITH_LPM
void
deluge_noupdate(void *ptr)
{
  PRINTF("deluge off radio as no update\n");
  radio_off(deluge_rs); // 无更新，返回休眠
}
#endif

void
advertise_summary(void *ptr)
{
  struct deluge_object *obj = (struct deluge_object *)ptr;
  struct deluge_msg_summary summary;
  int avail = 0;

  if(recv_adv >= CONST_K) {
    ctimer_stop(&summary_timer);
    return;
  }

  avail = highest_available_page(obj);
  summary.cmd = DELUGE_CMD_SUMMARY;
  summary.mode = deluge_mode;
  memcpy(summary.target, deluge_target, MAC_LEN);
  summary.version = obj->update_version;
  summary.highest_available[0] = (avail >> 8);
  summary.highest_available[1] = (avail & 0xff);
  summary.object_id = obj->object_id;
  memcpy(summary.mac, node_mac, MAC_LEN);

  PRINTF("deluge advertising summary for object id %u: version=%u, available=%u\n",
      (unsigned)obj->object_id, summary.version, summary.highest_available);

  packetbuf_copyfrom(&summary, sizeof(summary));
  broadcast_send(&deluge_broadcast);
}

static void
handle_summary(struct deluge_msg_summary *msg, const rimeaddr_t *sender)
{
  int highest_available, i;
  clock_time_t oldest_request, oldest_data, now;
  struct deluge_page *page;
  int msg_highest_available = (msg->highest_available[0]<<8) + msg->highest_available[1];

  PRINTF("handle_summary: %d,%d\n", msg->version, current_object.update_version);
  if (msg->mode == DELUGE_MODE_ONEHOP && msg->target[2] != node_mac[2]) {
    PRINTF("deluge ignore SUM not for us type\n");
    return;
  }

  if (memcmp(msg->target+3, deluge_all+3, MAC_LEN-3) != 0 && memcmp(msg->target, node_mac, MAC_LEN) != 0) {
    PRINTF("deluge ignore SUM not for us\n");
    return;
  }

  highest_available = highest_available_page(&current_object);

  if ((msg->version == objects[msg->object_id].version)
      && (msg_highest_available <= highest_available)) {
    PRINTF("deluge ignore SUM object id %d version %d\n", msg->object_id, msg->version);
    return;
  }

  if (msg->object_id != current_object.object_id) {
    load_object(msg->object_id);
  }

  PRINTF("handle_summary: ver %d/%d, avail %d/%d\n", msg->version, current_object.version,
         msg->highest_available, highest_available);
  if(msg->version != current_object.version ||
     msg_highest_available != highest_available) {
    neighbor_inconsistency = 1;
  } else {
    recv_adv++;
  }

  if(msg->version < current_object.version) {
    //old_summary = 1;
    broadcast_profile = 1;
    send_profile(&current_object);
  }

  /* Deluge M.5 */
  if(msg->version == current_object.update_version &&
     msg_highest_available > highest_available) {
    //if(msg_highest_available > OBJECT_PAGE_COUNT(current_object)) {
    //  PRINTF("Error: highest available is above object page count!\n");
    // return;
    //}

    oldest_request = oldest_data = now = clock_time();
    for(i = 0; i < msg_highest_available; i++) {
#if DELUGE_PAGES
      page = &current_object.pages[i];
#else
      page = &current_object.cur_page;
#endif
      if(page->last_request < oldest_request) {
        oldest_request = page->last_request;
      }
      if(page->last_request < oldest_data) {
        oldest_data = page->last_data;
      }
    }

    if(((now - oldest_request) / CLOCK_SECOND) <= 2 * r_interval ||
       ((now - oldest_data) / CLOCK_SECOND) <= r_interval) {
      PRINTF("ignore as now %d, %d, %d, %d\n", now, oldest_request, oldest_data, r_interval);
      return;
    }

    rimeaddr_copy(&current_object.summary_from, sender);
    transition(DELUGE_STATE_RX);

    if(ctimer_expired(&rx_timer)) {
      clock_time_t t = 0;
      if (deluge_mode == DELUGE_MODE_ONENODE) {
        t = (CLOCK_SECOND >> 4);
      } else if (deluge_mode == DELUGE_MODE_ONEHOP) {
        t = PACKET_TIME * (random_rand() & DELUGE_RAND_MASK);
      } else {
        t = CONST_OMEGA * ESTIMATED_TX_TIME + ((unsigned)random_rand() % T_R);
      }
      PRINTF("request after %lu\n", t);
      ctimer_set(&rx_timer, t, send_request, &current_object);
    }
  }
}

static void
send_page(struct deluge_object *obj, unsigned int pagenum)
{
  unsigned char buf[S_PAGE] = {0};
  struct deluge_msg_packet pkt;
  unsigned char *cp;
  unsigned int ccrc;

  pkt.cmd = DELUGE_CMD_PACKET;
  pkt.mode = deluge_mode;
  memcpy(pkt.target, deluge_target, MAC_LEN);
  pkt.pagenum[0] = (pagenum >> 8);
  pkt.pagenum[1] = (pagenum & 0xff);
#if DELUGE_PAGES
  pkt.version = obj->pages[pagenum].version;
#else
  pkt.version = obj->version;
#endif
  pkt.packetnum = 0;
  pkt.object_id = obj->object_id;

  read_page(obj, pagenum, buf);

  /* Divide the page into packets and send them one at a time. */
  for(cp = buf; cp + S_PKT <= (unsigned char *)&buf[S_PAGE]; cp += S_PKT) {
    if(obj->tx_set & (1 << pkt.packetnum)) {
      ccrc = crc16_data(cp, S_PKT, 0);
      pkt.crc[0] = (ccrc >> 8);
      pkt.crc[1] = (ccrc & 0xff);
      memcpy(pkt.payload, cp, S_PKT);
      packetbuf_copyfrom(&pkt, sizeof(pkt));
      PRINTF("deluge send packet for object id %d, version %d, page %d, packet num %d\n", pkt.object_id, pkt.version, pkt.pagenum, pkt.packetnum);
      broadcast_send(&deluge_broadcast);
    }
    pkt.packetnum++;
  }
  obj->tx_set = 0;
}

static void
tx_callback(void *arg)
{
  struct deluge_object *obj;

  obj = (struct deluge_object *)arg;
  if(obj->current_tx_page >= 0 && obj->tx_set) {
    send_page(obj, obj->current_tx_page);
    /* Deluge T.2. */
    if(obj->tx_set) {
      packetbuf_set_attr(PACKETBUF_ATTR_PACKET_TYPE,
          PACKETBUF_ATTR_PACKET_TYPE_STREAM);
      ctimer_reset(&tx_timer);
    } else {
      packetbuf_set_attr(PACKETBUF_ATTR_PACKET_TYPE,
          PACKETBUF_ATTR_PACKET_TYPE_STREAM_END);
      obj->current_tx_page = -1;
      transition(DELUGE_STATE_MAINTAIN);
    }
  }
}

static void
handle_request(struct deluge_msg_request *msg)
{
  int highest_available;
  int msg_pagenum = (msg->pagenum[0] << 8) + msg->pagenum[1];

  if (deluge_paused) {
    return;
  }

  if (node_mac[2] == NODE_VD) {
    PRINTF("deluge ignore REQ as device\n");
    return;
  }

  if (msg->object_id < current_object.object_id) {
    PRINTF("deluge ignore REQ as object id %d<%d\n", msg->object_id, current_object.object_id);
    return;
  }

  if(msg_pagenum >= OBJECT_PAGE_COUNT(current_object)) {
    PRINTF("deluge ignore REQ pagenum %d>%d\n", pagenum, OBJECT_PAGE_COUNT(current_object));
    return;
  }

  if(msg->version != current_object.version) {
    //PRINTF("deluge nbr inconsist %d!=%d\n", msg->version, current_object.version);
    neighbor_inconsistency = 1;
  }

  highest_available = highest_available_page(&current_object);

  /* Deluge M.6 */
  if(msg->version == current_object.version &&
      msg_pagenum <= highest_available) {
#if DELUGE_PAGES
    current_object.pages[msg_pagenum].last_request = clock_time();
#else
    current_object.cur_page.last_request = clock_time();
#endif
    /* Deluge T.1 */
    if(msg_pagenum == current_object.current_tx_page) {
      current_object.tx_set |= msg->request_set;
    } else {
      current_object.current_tx_page = msg_pagenum;
      current_object.tx_set = msg->request_set;
    }

    if (deluge_state != DELUGE_STATE_TX) {
      transition(DELUGE_STATE_TX);
      if (deluge_mode == DELUGE_MODE_ONENODE) {
        ctimer_set(&tx_timer, (CLOCK_SECOND >> 6), tx_callback, &current_object);
      } else if (deluge_mode == DELUGE_MODE_ONEHOP) {
        ctimer_set(&tx_timer, (PACKET_TIME * DELUGE_RAND_N), tx_callback, &current_object);
      } else {
        ctimer_set(&tx_timer, CLOCK_SECOND, tx_callback, &current_object);
      }
    }
  } else {
    if (msg->version != current_object.version) PRINTF("deluge not current version %d!=%d\n", msg->version, current_object.version);
    else if (msg_pagenum > highest_available) PRINTF("deluge pagenum invalid %d>%d\n", msg_pagenum, highest_available);
  }
}

static void
handle_packet(struct deluge_msg_packet *msg)
{
  struct deluge_page *page;
  uint16_t crc;
  struct deluge_msg_packet packet;
  uint16_t packet_pagenum;
  uint16_t packet_crc;
#if DELUGE_NV
  uint8_t idx = msg->object_id;
#endif

  if (!((memcmp(msg->target, node_mac, MAC_LEN) == 0)
        || ((msg->target[2] == node_mac[2])
            && (memcmp(msg->target+3, deluge_all+3, MAC_LEN-3) == 0)))) {
    PRINTF("deluge ignore PKT not for us\n");
    return;
  }

  if (msg->object_id < current_object.object_id) {
    PRINTF("deluge ignore PKT as object id %d<%d\n", msg->object_id, current_object.object_id);
    return;
  }

  memcpy(&packet, msg, sizeof(packet));
  packet_pagenum = (packet.pagenum[0]<<8) + packet.pagenum[1];
  packet_crc = (packet.crc[0]<<8) + packet.crc[1];

  PRINTF("deluge incoming packet for object id %u, version %u, page %u, packet num %u!\n",
         (unsigned)packet.object_id, (unsigned)packet.version,
         (unsigned)packet.pagenum, (unsigned)packet.packetnum);

  if(packet_pagenum != current_object.current_rx_page) {
    PRINTF("deluge ignore page not current rx %d!=%d\n", packet.pagenum, current_object.current_rx_page);
    return;
  }

  if(packet.version != current_object.version) {
    //PRINTF("deluge nbr inconsist %d!=%d\n", packet.version, current_object.version);
    neighbor_inconsistency = 1;
  }

  if (timeout_r == 1) {
    PRINTF("deluge keep radio on for update\n");
    ctimer_stop(&timeout); timeout_r = 0;
  }

#if DELUGE_PAGES
  page = &current_object.pages[packet.pagenum];
#else
  page = &current_object.cur_page;
#endif
  if(packet.version == page->version && !(page->flags & PAGE_COMPLETE)) {
    memcpy(&current_object.current_page[S_PKT * packet.packetnum],
           packet.payload, S_PKT);

    crc = crc16_data(packet.payload, S_PKT, 0);
    if(packet_crc != crc) {
      PRINTF("packet crc: %hu, calculated crc: %hu\n", packet.crc, crc);
      return;
    }

    page->last_data = clock_time();
    page->packet_set |= (1 << packet.packetnum);

    if(page->packet_set == ALL_PACKETS) {
      /* This is the last packet of the requested page; stop streaming. */
      packetbuf_set_attr(PACKETBUF_ATTR_PACKET_TYPE,
                         PACKETBUF_ATTR_PACKET_TYPE_STREAM_END);

      write_page(&current_object, packet_pagenum, current_object.current_page);
      page->version = packet.version;
      page->flags = PAGE_COMPLETE;
      PRINTF("deluge page %u completed\n", packet.pagenum);

      current_object.current_rx_page++;
      current_object.nrequests = 0;
#if !DELUGE_PAGES
      current_object.avail_pages++;
      memset(&current_object.cur_page, 0, sizeof(struct deluge_page));
      current_object.cur_page.version = packet.version;
#endif
#if DELUGE_NV
      ota.images[idx].rcvd++;
      ota.crc = crc16_data((uint8_t*)&ota, sizeof(struct OTA)-2, CRC_INIT);
      nv_write(NV_OTA_ADDR, (uint8_t*)&ota, sizeof(struct OTA));
#endif

      if(packet_pagenum == OBJECT_PAGE_COUNT(current_object) - 1) {
        current_object.version = current_object.update_version;
        objects[current_object.object_id].version = current_object.update_version;
        //leds_on(LEDS_RED);
        PRINTF("deluge update completed for object %u, version %u\n",
               (unsigned)current_object.object_id, packet.version);

        cfs_close(current_object.cfs_fd); // close to write file metadata
        current_object.cfs_fd = cfs_open(current_object.filename, CFS_READ|CFS_WRITE);
        transition(DELUGE_STATE_MAINTAIN);
#if DELUGE_NV
        ota.images[idx].rcvd = packet_pagenum + 1;
        ota.images[idx].pend = 0;
        ota.crc = crc16_data((uint8_t*)&ota, sizeof(struct OTA)-2, CRC_INIT);
        nv_write(NV_OTA_ADDR, (uint8_t*)&ota, sizeof(struct OTA));
#endif

#if WITH_LPM
        PRINTF("deluge off radio as update complete\n");
        radio_off(deluge_rs); // 更新完成，返回休眠
#endif
        if (deluge_callback) {
          deluge_callback->completed(current_object.object_id, current_object.version, current_object.size);
        }

      } else if(current_object.current_rx_page < OBJECT_PAGE_COUNT(current_object)) {
        if (deluge_mode == DELUGE_MODE_ONENODE) {
          clock_time_t t = (CLOCK_SECOND >> 4);
          PRINTF("deluge request next after %d\n", t);
          ctimer_set(&rx_timer, t, send_request, &current_object);
        }
        else if (deluge_mode == DELUGE_MODE_ONEHOP) {
          clock_time_t t = PACKET_TIME * (random_rand() & DELUGE_RAND_MASK);
          PRINTF("deluge request next after %d\n", t);
          ctimer_set(&rx_timer, t, send_request, &current_object);
        }
        else {
          if(ctimer_expired(&rx_timer)) {
            clock_time_t t = CONST_OMEGA * ESTIMATED_TX_TIME + (random_rand() % T_R);
            PRINTF("deluge request next after %d\n", t);
            ctimer_set(&rx_timer, t, send_request, &current_object);
          }
        }
      }
      /* Deluge R.3 */
      if ((deluge_mode == DELUGE_MODE_ONENODE) || (deluge_mode == DELUGE_MODE_ONEHOP)) {
        /* Keep RX state to request subsequent pages at rx_timer expired */
      } else {
        transition(DELUGE_STATE_MAINTAIN);
      }
    } else {
      /* More packets to come. Put lower layers in streaming mode. */
      packetbuf_set_attr(PACKETBUF_ATTR_PACKET_TYPE,
                         PACKETBUF_ATTR_PACKET_TYPE_STREAM);
    }
  } else {
    if (packet.version != page->version) PRINTF("deluge ignore packet as version %d!=%d\n", packet.version, page->version);
    else if (page->flags & PAGE_COMPLETE) PRINTF("deluge ignore packet as page complete\n");
  }
}

void
send_profile(void *ptr)
{
  struct deluge_object *obj = (struct deluge_object *)ptr;
  struct deluge_msg_profile *msg;
  unsigned char buf[sizeof(*msg)];
  int npages = 0;
  //unsigned char buf[sizeof(*msg) + OBJECT_PAGE_COUNT(*obj)];
  //int i;

  if(broadcast_profile && recv_adv < CONST_K) {
    broadcast_profile = 0;

    npages = OBJECT_PAGE_COUNT(*obj);
    msg = (struct deluge_msg_profile *)buf;
    msg->cmd = DELUGE_CMD_PROFILE;
    msg->mode = deluge_mode;
    memcpy(msg->target, deluge_target, MAC_LEN);
    msg->version = obj->version;
    msg->npages[0] = (npages >> 8);
    msg->npages[1] = (npages & 0xff);
    msg->object_id = obj->object_id;
    //for(i = 0; i < msg->npages; i++) {
    //  msg->version_vector[i] = obj->pages[i].version;
    //}
    memcpy(msg->mac, node_mac, MAC_LEN);

    packetbuf_copyfrom(buf, sizeof(buf));
    broadcast_send(&deluge_broadcast);
  }
}

static void
handle_profile(struct deluge_msg_profile *msg, const rimeaddr_t *sender)
{
  struct deluge_object *obj;
#if DELUGE_PAGES
  int i;
  int npages;
  char *p;
#endif
  int msg_npages = (msg->npages[0]<<8) + msg->npages[1];

  PRINTF("handle_profile: %d,%d\n", msg->version, current_object.update_version);
  if (msg->mode == DELUGE_MODE_ONEHOP && msg->target[2] != node_mac[2]) {
    PRINTF("deluge ignore PROF not for us type\n");
    return;
  }

  if (memcmp(msg->target+3, deluge_all+3, MAC_LEN-3) != 0 && memcmp(msg->target, node_mac, MAC_LEN) != 0) {
    PRINTF("deluge ignore PROF not for us\n");
    return;
  }

  if (msg->object_id < current_object.object_id) {
    PRINTF("deluge ignore PROF as object id %d<%d\n", msg->object_id, current_object.object_id);
    return;
  }

  obj = &current_object;
  if(msg->version <= current_object.update_version) {
    PRINTF("deluge ignore PROF as version %d<=%d\n", msg->version, current_object.update_version);
    return;
  }

  if (!ctimer_expired(&timeout)) {
    PRINTF("deluge keep radio on for update\n");
    ctimer_stop(&timeout); timeout_r = 0;
  }

  PRINTF("Received profile of version %u with a vector of %u pages.\n",
         msg->version, msg->npages);

  if (msg->mode == DELUGE_MODE_ONENODE || msg->mode == DELUGE_MODE_ONEHOP) {
    deluge_mode = msg->mode;
    memcpy(deluge_target, msg->mac, MAC_LEN);
  }

  //leds_off(LEDS_RED);
  current_object.tx_set = 0;

  obj->size = (uint32_t)msg_npages * S_PAGE;

#if DELUGE_PAGES
  npages = OBJECT_PAGE_COUNT(*obj);
#if DELUGE_MALLOC
  p = malloc(OBJECT_PAGE_COUNT(*obj) * sizeof(*obj->pages));
  if(p == NULL) {
    PRINTF("Failed to reallocate memory for pages!\n");
    return;
  }

  memcpy(p, obj->pages, npages * sizeof(*obj->pages));
  free(obj->pages);
  obj->pages = (struct deluge_page *)p;
#endif /* DELUGE_MALLOC */

  if(msg->npages < npages) {
    npages = msg->npages;
  }

  for(i = 0; i < npages; i++) {
    //if(msg->version_vector[i] > obj->pages[i].version) {
      obj->pages[i].packet_set = 0;
      obj->pages[i].flags &= ~PAGE_COMPLETE;
      obj->pages[i].version = msg->version;
      //obj->pages[i].version = msg->version_vector[i];
    //}
  }

  for(; i < msg->npages; i++) {
    init_page(obj, i, 0);
    obj->pages[i].packet_set = 0;
    obj->pages[i].flags &= ~PAGE_COMPLETE;
    obj->pages[i].version = msg->version;
  }
#else /* no DELUGE_PAGES */
#if DELUGE_NV
  obj->avail_pages = 0;
  if (ota.images[obj->object_id].pend != 0)
    obj->avail_pages = ota.images[obj->object_id].rcvd;
#else
  obj->avail_pages = 0;
#endif
  memset(&obj->cur_page, 0, sizeof(struct deluge_page));
  obj->cur_page.version = msg->version;
#endif /* DELUGE_PAGES */

  obj->current_rx_page = highest_available_page(obj);
  obj->update_version = msg->version;

  transition(DELUGE_STATE_RX);

#if DELUGE_NV
  uint8_t idx = obj->object_id;
  if ((ota.images[idx].pend == 0) || (msg->version != ota.images[idx].ver)) {
    ota.images[idx].ver = msg->version;
    ota.images[idx].rcvd = 0;
    ota.images[idx].addr = 0;
    ota.images[idx].size = obj->size;
    ota.images[idx].crc = 0x0000;
    ota.images[idx].pend = 1;
    ota.images[idx].mode = msg->mode;
    rimeaddr_copy(&(ota.images[idx].from), sender);
    ota.crc = crc16_data((uint8_t*)&ota, sizeof(struct OTA)-2, CRC_INIT);
    nv_write(NV_OTA_ADDR, (uint8_t*)&ota, sizeof(struct OTA));
  }
#endif

#if WITH_LPM
  radio_off(deluge_rs); // 结束无更新超时
  radio_on(deluge_rs, ((clock_time_t)CLOCK_SECOND * 600)); // 重新开始超时以防更新未正常完成
#endif

  clock_time_t t = 0;
  if (deluge_mode == DELUGE_MODE_ONENODE) {
    t = (CLOCK_SECOND >> 4);
  } else if (deluge_mode == DELUGE_MODE_ONEHOP) {
    t = PACKET_TIME * (random_rand() & DELUGE_RAND_MASK);
  } else {
    t = CONST_OMEGA * ESTIMATED_TX_TIME + ((unsigned)random_rand() % T_R);
  }
  PRINTF("request after %d\n", t);
  rimeaddr_copy(&obj->summary_from, sender);
  ctimer_set(&rx_timer, t, send_request, obj);
}

static void
command_dispatcher(const rimeaddr_t *sender)
{
  char *msg;
  int len;
  struct deluge_msg_profile *profile;
#if DEBUG
  const char *MSGS[5] = { "", "SUM", "REQ", "PKT", "PROF" };
#endif

  msg = packetbuf_dataptr();
  len = packetbuf_datalen();
  if(len < 1)
    return;

  PRINTF("deluge dispatch msg %s from %d.%d\n", MSGS[msg[0]], sender->u8[0], sender->u8[1]);
  switch(msg[0]) {
  case DELUGE_CMD_SUMMARY:
    if(len >= sizeof(struct deluge_msg_summary))
      handle_summary((struct deluge_msg_summary *)msg, sender);
    break;
  case DELUGE_CMD_REQUEST:
    if(len >= sizeof(struct deluge_msg_request))
      handle_request((struct deluge_msg_request *)msg);
    break;
  case DELUGE_CMD_PACKET:
    if(len >= sizeof(struct deluge_msg_packet))
      handle_packet((struct deluge_msg_packet *)msg);
    break;
  case DELUGE_CMD_PROFILE:
    profile = (struct deluge_msg_profile *)msg;
    if(len >= sizeof(*profile) /*&&
       len >= sizeof(*profile) + profile->npages * sizeof(profile->version_vector[0])*/) {
      handle_profile(profile, sender);
    } else {
      PRINTF("ignore profile %d,%d,%d\n", len, sizeof(*profile), profile->npages);
    }
    break;
  default:
    PRINTF("Incoming packet with unknown command: %d\n", msg[0]);
  }
}

static void
unicast_recv(struct unicast_conn *c, const rimeaddr_t *sender)
{
  command_dispatcher(sender);
}

static void
broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *sender)
{
  command_dispatcher(sender);
}

int
deluge_disseminate(uint8_t objid, char *file, unsigned version, uint8_t mode, uint8_t *target)
{
#if DELUGE_NV
  objects[OBJ_GATEWAY].object_id = OBJ_GATEWAY;
  objects[OBJ_GATEWAY].version = ota.images[OBJ_GATEWAY].ver;
  objects[OBJ_ROUTER].object_id = OBJ_ROUTER;
  objects[OBJ_ROUTER].version = ota.images[OBJ_ROUTER].ver;
  objects[OBJ_DEVICE].object_id = OBJ_DEVICE;
  objects[OBJ_DEVICE].version = ota.images[OBJ_DEVICE].ver;
#else
  objects[OBJ_GATEWAY].object_id = OBJ_GATEWAY;
  objects[OBJ_GATEWAY].version = 0x10;
  objects[OBJ_ROUTER].object_id = OBJ_ROUTER;
  objects[OBJ_ROUTER].version = 0x10;
  objects[OBJ_DEVICE].object_id = OBJ_DEVICE;
  objects[OBJ_DEVICE].version = 0x10;
  objects[objid].version = version;
#endif

#if WITH_LPM
  radio_on(deluge_rs, (CLOCK_SECOND << 3)); // 开始超时以防无更新
  ctimer_set(&timeout, (CLOCK_SECOND << 3), deluge_noupdate, NULL); timeout_r = 1;
#endif

  deluge_mode = mode;
  memcpy(deluge_target, target, MAC_LEN);

  if(init_object(objid, &current_object, file, version) < 0) {
    return -1;
  }
  process_start(&deluge_process, file);

#if DELUGE_NV
  if (ota.images[objid].pend != 0) {
    clock_time_t t = 0;
    if (deluge_mode == DELUGE_MODE_ONENODE) {
      t = (CLOCK_SECOND >> 4);
    } else if (deluge_mode == DELUGE_MODE_ONEHOP) {
      t = PACKET_TIME * (random_rand() & DELUGE_RAND_MASK);
    } else {
      t = CONST_OMEGA * ESTIMATED_TX_TIME + ((unsigned)random_rand() % T_R);
    }
    PRINTF("request after %d\n", t);
    rimeaddr_copy(&current_object.summary_from, &(ota.images[objid].from));
    ctimer_set(&rx_timer, t, send_request, &current_object);
  }
#endif

  return 0;
}

int
deluge_stop(void)
{
  process_exit(&deluge_process);
  return 0;
}

static void
deluge_exit(void)
{
  ctimer_stop(&rx_timer);
  ctimer_stop(&tx_timer);
  ctimer_stop(&summary_timer);
  ctimer_stop(&profile_timer);
  ctimer_stop(&timeout); timeout_r = 0;

  unicast_close(&deluge_uc);
  broadcast_close(&deluge_broadcast);
  if(current_object.cfs_fd >= 0) {
    cfs_close(current_object.cfs_fd);
    current_object.cfs_fd = -1;
  }
#if DELUGE_PAGES
#if DELUGE_MALLOC
  if(current_object.pages != NULL) {
    free(current_object.pages);
  }
#endif
#endif
}

PROCESS_THREAD(deluge_process, ev, data)
{
  static struct etimer et;
  static unsigned time_counter;
  static unsigned r_rand;

  PROCESS_EXITHANDLER(deluge_exit());
  PROCESS_BEGIN();

  //deluge_event = process_alloc_event();

  //broadcast_open(&deluge_broadcast, DELUGE_BROADCAST_CHANNEL, &broadcast_call);
  //unicast_open(&deluge_uc, DELUGE_UNICAST_CHANNEL, &unicast_call);
  r_interval = T_LOW;

  PRINTF("Maintaining state for object %s of %d pages\n",
      current_object.filename, OBJECT_PAGE_COUNT(current_object));

  deluge_state = DELUGE_STATE_MAINTAIN;

  for(r_interval = T_LOW;;) {
    if(neighbor_inconsistency) {
      /* Deluge M.2 */
      r_interval = T_LOW;
      neighbor_inconsistency = 0;
    } else {
      /* Deluge M.3 */
      r_interval = (2 * r_interval >= T_HIGH) ? T_HIGH : 2 * r_interval;
    }

    r_rand = r_interval / 2 + ((unsigned)random_rand() % (r_interval / 2));
    recv_adv = 0;
    //old_summary = 0;

    /* Deluge M.1 */
    ctimer_set(&summary_timer, r_rand * CLOCK_SECOND,
               advertise_summary, &current_object);

    /* Deluge M.4 */
    ctimer_set(&profile_timer, r_rand * CLOCK_SECOND,
               send_profile, &current_object);

    LONG_TIMER(et, time_counter, r_interval);
  }

  PROCESS_END();
}
