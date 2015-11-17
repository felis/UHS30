/* Modified from newlib-2012.09 to support ISR safety. */

#include <limits.h>
#if defined(_NEWLIB_VERSION)
#include <Arduino.h>
#include <malloc.h>
#include <sys/lock.h>

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

//static volatile uint8_t irecover;

//#ifndef interruptsStatus
//#define interruptsStatus() __interruptsStatus()
//static inline unsigned char __interruptsStatus(void) __attribute__((always_inline, unused));
//
//static inline unsigned char __interruptsStatus(void) {
//        unsigned int primask;
//        asm volatile ("mrs %0, primask" : "=r" (primask));
//        if(primask) return 0;
//        return 1;
//}
//#endif

#endif

void
__malloc_lock(ptr)
struct _reent *ptr;
{
#ifdef __USE_ISR_SAFE_MALLOC__
//        uint8_t i = interruptsStatus();
        noInterrupts();
#if 0
        // debugging, pin 2 LOW
        digitalWrite(2, LOW);
#endif

//        if(!__isr_safety) {
//                irecover = i;
//        }
        __isr_safety++;
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
//                if(!__isr_safety && irecover) interrupts();
        }
        if(!__isr_safety) {
#if 0
        // debugging, pin 2 HIGH
        digitalWrite(2, HIGH);
#endif

                interrupts();
        }
#endif
}
#else
// This is needed to keep the compiler happy...
int mlock_null(void) {
  return 0;
}
#endif
