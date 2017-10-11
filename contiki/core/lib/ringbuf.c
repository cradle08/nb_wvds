/*
 * Copyright (c) 2008, Swedish Institute of Computer Science.
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
 *         Ring buffer library implementation
 * \author
 *         Adam Dunkels <adam@sics.se>
 */
#include "contiki.h"
#include "platform-conf.h"
#include "lib/ringbuf.h"
#include "app.h"



/*---------------------------------------------------------------------------*/
void
ringbuf_init(struct ringbuf *r, uint8_t *dataptr, uint16_t size)
{
  r->data = dataptr;
  r->flag = 0; //0: read or write, 1:only read, 2: only write
  r->size = size - 1;
  r->head = 0;
  r->tail = 0;

}
/*---------------------------------------------------------------------------*/
int8_t
ringbuf_put(struct ringbuf *r, uint8_t c)
{
  /* Check if buffer is full. If it is full, return 0 to indicate that
     the element was not inserted into the buffer.

     XXX: there is a potential risk for a race condition here, because
     the ->get_ptr field may be written concurrently by the
     ringbuf_get() function. To avoid this, access to ->get_ptr must
     be atomic. We use an uint8_t type, which makes access atomic on
     most platforms, but C does not guarantee this.
  */
  
  if(ringbuf_is_full(r))
  {
    r->flag = 1; //0: read or write, 1:only read, 2: only write
    return -1;
  } else {
    r->tail++;
    r->data[r->tail] = c;
    return 0;
  }
}

/*---------------------------------------------------------------------------*/
bool ringbuf_is_full(struct ringbuf *r)
{
  uint16_t index = r->tail + 1;
  if(r->tail >= r->size)
     index = 0;
  if(index == r->head)
    return true;
  else
    return false;
}

/*---------------------------------------------------------------------------*/
uint8_t
ringbuf_get(struct ringbuf *r)
{
  if(ringbuf_is_empty(r))
  {
    r->flag = 2; //0: read or write, 1:only read, 2: only write
    return -1;
  } else {
    r->head++;
    return r->data[r->head];
  }
}
  
/*---------------------------------------------------------------------------*/
bool ringbuf_is_empty(struct ringbuf *r)
{
  uint16_t index = r->head + 1;
  if(r->head >= r->size)
     index = 0;
  if(index == r->tail)
    return true;
  else
    return false;
}

/*---------------------------------------------------------------------------*/
uint16_t ringbuf_size(struct ringbuf *r)
{
  return r->size + 1;
}
//**---------------------------------------------------------------------------*/
uint16_t ringbuf_elements(struct ringbuf *r)
{
  if(ringbuf_is_full(r))
  {
    return r->size + 1;
  } else if(ringbuf_is_empty(r)){
    return 0;
  }else if(r->head < r->tail)
    return r->tail - r->head;
  else
    return r->tail + 1 + r->size - r->head;
}
/*---------------------------------------------------------------------------*/
