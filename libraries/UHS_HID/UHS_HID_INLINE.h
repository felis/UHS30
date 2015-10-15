/* Copyright (C) 2011 Circuits At Home, LTD. All rights reserved.

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

#if defined(LOAD_UHS_HID) && defined(__UHS_HID_H__) && !defined(UHS_HID_LOADED)
#define UHS_HID_LOADED

#if DEBUG_PRINTF_EXTRA_HUGE
#ifdef DEBUG_PRINTF_EXTRA_HUGE_USB_HID
#define HID_DUBUG(...) printf(__VA_ARGS__)
#else
#define HID_DUBUG(...) VOID0
#endif
#else
#define HID_DUBUG(...) VOID0
#endif


#ifndef AJK_NI
#define AJK_NI __attribute__((noinline))
#endif

#if defined(LOAD_UHS_BT)
void AJK_NI UHS_HID_SetBTInterface(UHS_USB_BT_BASE *host, BT_INFO *ei) {

}
#endif

/**
 * Add host and ei to USB interface list
 */
void AJK_NI UHS_HID_SetUSBInterface(UHS_USB_HOST_BASE *host, ENUMERATION_INFO *ei) {

}

/**
 * Scan all unpaired interfaces and pair them with a driver, then start the driver
 */
void AJK_NI UHS_HID_ScanUninitialized(void) {

}

/**
 * Poll all paired interfaces
 */
void AJK_NI UHS_HID_Poll(void) {
}

#else
#error "Never include USB_HID_INLINE.h, include USB_HID.h instead"
#endif
