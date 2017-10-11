/** \addtogroup lib
 * @{ */

/**
 * \defgroup ringbuf Ring buffer library
 * @{
 *
 * The ring buffer library implements ring (circular) buffer where
 * bytes can be read and written independently. A ring buffer is
 * particularly useful in device drivers where data can come in
 * through interrupts.
 *
 */
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
 *         Header file for the ring buffer library
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#ifndef __RINGBUF_H__
#define __RINGBUF_H__

#include "contiki-conf.h"

/**
 * \brief      Structure that holds the state of a ring buffer.
 *
 *             This structure holds the state of a ring buffer. The
 *             actual buffer needs to be defined separately. This
 *             struct is an opaque structure with no user-visible
 *             elements.
 *
 */
struct ringbuf {
  uint8_t *data;
  int8_t flag; // //0: read or write, 1:only read, 2: only write
  uint16_t size;
  /* XXX these must be 8-bit quantities to avoid race conditions. */
  uint16_t head, tail;
};

void      ringbuf_init(struct ringbuf *r, uint8_t *dataptr, uint16_t size);
int8_t    ringbuf_put(struct ringbuf *r, uint8_t c);
uint8_t    ringbuf_get(struct ringbuf *r);
bool      ringbuf_is_full(struct ringbuf *r);
bool      ringbuf_is_empty(struct ringbuf *r);
uint16_t   ringbuf_size(struct ringbuf *r);
uint16_t  ringbuf_elements(struct ringbuf *r);

#endif /* __RINGBUF_H__ */
