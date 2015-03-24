/*
 * File:   fcntl.h
 * Author: root
 *
 * Created on March 5, 2014, 1:15 AM
 */

#ifndef XXFCNTL_H
#define	XXFCNTL_H

#if !defined(AVR)
#include_next <fcntl.h>
#endif

#define O_RDONLY 0x01
#define O_WRONLY 0x02
#define O_RDWR   (O_RDONLY | O_WRONLY)
#define O_CREAT  0x04
#define O_APPEND 0x08
#define O_TRUNC  0x10

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* XXFCNTL_H */

