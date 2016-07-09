/*
 * File:   fcntl.h
 * Author: xxxajk@gmail.com
 *
 * Created on March 5, 2014, 1:15 AM
 */

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
#ifndef XXFCNTL_H
#define	XXFCNTL_H

#if !defined(AVR) && !defined (__AVR)
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

