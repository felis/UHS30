/* Modified from newlib-2012.09 to support ISR safety. */
/* Copyright (C) 2015-2016 Andrew J. Kroll
   and
Copyright (C) 2011 Circuits At Home, LTD. All rights reserved.

This software may be distributed and modified under the terms of the GNU
General Public License version 2 (GPL2) as published by the Free Software
Foundation and appearing in the file GPL2.TXT included in the packaging of
this file. Please note that GPL2 Section 2[b] requires that all works based
on this software must also be made publicly available under the terms of
the GPL2 ("Copyleft").

Contact information
-------------------

Circuits At Home, LTD
Web      :  http://www.circuitsathome.com
e-mail   :  support@circuitsathome.com
 */

#include <limits.h>
#if defined(_NEWLIB_VERSION)
#include <Arduino.h>
#include <malloc.h>
#include <sys/lock.h>
#if !defined(DDSB)
#if defined(__arm__)
__attribute__((always_inline)) static inline void A__DSB(void) {
        __asm__ volatile ("dsb");
}
#endif // defined(__USE_CMSIS_VECTORS__)
#elif defined(__mips__)
__attribute__((always_inline)) static inline void A__DSB(void) {
        __asm__ volatile ("sync" : : : "memory");
}
#else
#error "No Barrier implementation"
#endif // defined(__arm__)

#define DDSB() A__DSB()

/* Indicate that we are to use ISR safety. */
#define __USE_ISR_SAFE_MALLOC__ 1

/*
FUNCTION
<<__malloc_lock>>, <<__malloc_unlock>>---lock malloc pool

INDEX
        __malloc_lock
INDEX
        __malloc_unlock

ANSI_SYNOPSIS
        #include <malloc.h>
        void __malloc_lock (struct _reent *<[reent]>);
        void __malloc_unlock (struct _reent *<[reent]>);

TRAD_SYNOPSIS
        void __malloc_lock(<[reent]>)
        struct _reent *<[reent]>;

        void __malloc_unlock(<[reent]>)
        struct _reent *<[reent]>;

DESCRIPTION
The <<malloc>> family of routines call these functions when they need to lock
the memory pool.  The version of these routines supplied in the library use
the lock API defined in sys/lock.h.  If multiple threads of execution can
call <<malloc>>, or if <<malloc>> can be called reentrantly, then you need to
define your own versions of these functions in order to safely lock the
memory pool during a call.  If you do not, the memory pool may become
corrupted.

A call to <<malloc>> may call <<__malloc_lock>> recursively; that is,
the sequence of calls may go <<__malloc_lock>>, <<__malloc_lock>>,
<<__malloc_unlock>>, <<__malloc_unlock>>.  Any implementation of these
routines must be careful to avoid causing a thread to wait for a lock
that it already holds.
 */


#ifndef __SINGLE_THREAD__
__attribute__((unused))
__LOCK_INIT_RECURSIVE(static, __malloc_lock_object);
#endif

#ifdef __USE_ISR_SAFE_MALLOC__
static volatile unsigned long __isr_safety = 0;


#if defined(__arm__)
static volatile uint8_t irecover;
#define dont_interrupt() __dont_interrupt()
#define can_interrupt(x)  __can_interrupt(x)

static inline void __can_interrupt(uint8_t status)  __attribute__((always_inline, unused));

static inline  void __can_interrupt(uint8_t status) {
        if(status) interrupts();
}

static inline unsigned char __dont_interrupt(void) __attribute__((always_inline, unused));

static inline unsigned char __dont_interrupt(void) {
        unsigned int primask, faultmask;
        asm volatile ("mrs %0, primask" : "=r" (primask));
        asm volatile ("mrs %0, faultmask" : "=r" (faultmask));
        noInterrupts();
        if(primask || faultmask) return 0;
        return 1;
}

#elif defined(ARDUINO_ARCH_PIC32)
static volatile uint32_t irecover;
#define dont_interrupt() disableInterrupts()
#define can_interrupt(x)  restoreInterrupts(x)
#else
#error "No ISR safety, sorry."
#endif

void
__malloc_lock(ptr)
struct _reent *ptr;
{
#ifdef __USE_ISR_SAFE_MALLOC__
#if defined(__arm__)
static volatile uint8_t i;
#elif defined(ARDUINO_ARCH_PIC32)
static volatile uint32_t i;
#endif

        i = dont_interrupt();
#if 0
        // debugging, pin 2 LOW
        digitalWrite(2, LOW);
#endif

        if(!__isr_safety) {
                irecover = i;
        }
        __isr_safety++;
        DDSB();
#endif
#ifndef __SINGLE_THREAD__
        __lock_acquire_recursive(__malloc_lock_object);
#endif
}

void
__malloc_unlock(ptr)
struct _reent *ptr;
{
#ifndef __SINGLE_THREAD__
        __lock_release_recursive(__malloc_lock_object);
#endif
#ifdef __USE_ISR_SAFE_MALLOC__
        if(__isr_safety) {
                __isr_safety--;
        }
        DDSB();
        if(!__isr_safety) {
#if 0
        // debugging, pin 2 HIGH
        digitalWrite(2, HIGH);
#endif
                can_interrupt(irecover);
        }
#endif
}
#else
// This is needed to keep the compiler happy...
int mlock_null(void) {
  return 0;
}
#endif
#endif
