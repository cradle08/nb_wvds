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
 */

#ifndef MSP430DEF_H
#define MSP430DEF_H

//#define F_CPU  8000000ul
#define MSP430_CPU_SPEED F_CPU

#define MSP430_REQUIRE_CPUON 0
#define MSP430_REQUIRE_LPM1 1
#define MSP430_REQUIRE_LPM2 2
#define MSP430_REQUIRE_LPM3 3



#ifdef __IAR_SYSTEMS_ICC__
#include <intrinsics.h>
#include <in430.h>
#include <msp430.h>
#define dint() __disable_interrupt()
#define eint() __enable_interrupt()
#define __MSP430__ 1
#define CC_CONF_INLINE

#else /* __IAR_SYSTEMS_ICC__ */

#ifdef __MSPGCC__
#include <msp430.h>
#include <legacymsp430.h>
#else /* __MSPGCC__ */
#include <io.h>
#include <signal.h>
#if !defined(MSP430_MEMCPY_WORKAROUND) && (__GNUC__ < 4)
#define MSP430_MEMCPY_WORKAROUND 1
#endif
#endif /* __MSPGCC__ */

#define CC_CONF_INLINE inline

#endif /* __IAR_SYSTEMS_ICC__ */

#ifndef BV
#define BV(x) (1 << x)
#endif

   /*
#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
#ifndef uint8_t
typedef unsigned char   uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long  uint32_t;
typedef   signed char    int8_t;
typedef          short  int16_t;
typedef          long   int32_t;
#endif
#endif // !HAVE_STDINT_H 

// These names are deprecated, use C99 names. 
typedef  uint8_t    u8_t;
typedef uint16_t   u16_t;
typedef uint32_t   u32_t;
typedef  int32_t   s32_t;

// default DCOSYNCH Period is 30 seconds 
#ifdef DCOSYNCH_CONF_PERIOD
#define DCOSYNCH_PERIOD DCOSYNCH_CONF_PERIOD
#else
#define DCOSYNCH_PERIOD 30
#endif

*/


#endif /* MSP430DEF_H */
