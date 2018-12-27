/*
 * File:   ISR_safe_memory.h
 * Author: root
 *
 * Created on December 26, 2018, 1:25 AM
 */

#ifndef ISR_SAFE_MEMORY_H
#define	ISR_SAFE_MEMORY_H

#ifdef	__cplusplus
extern "C" {
#endif
#include <limits.h>

#if defined(__AVR__)
#define ISR_MEMORY_IS_SAFE
#endif

#if defined(_NEWLIB_VERSION)
#define ISR_MEMORY_IS_SAFE
#endif



#ifdef	__cplusplus
}
#endif

#if !defined(ISR_MEMORY_IS_SAFE)
#error MEMORY is not ISR SAFE!
#endif

#endif	/* ISR_SAFE_MEMORY_H */

