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


#if !defined(__UHS_HID_H__)
#define __UHS_HID_H__
#include "../UHS_host/UHS_host.h"

class UHS_HID_DEVICE_base; // forward class declaration

#if defined(LOAD_UHS_BT)
void UHS_HID_SetBTInterface(UHS_USB_BT_BASE *host, BT_INFO *ei);
#endif
struct UHS_HID_interface {
        UHS_USB_HOST_BASE *USB_host;
#if defined(LOAD_UHS_BT)
        UHS_BT_HOST_BASE *BT_host;
#endif
        UHS_HID_DEVICE_base *driver
        uint16_t vid;
        uint16_t pid;
        uint8_t subklass;
        uint8_t protocol;
        uint8_t bMaxPacketSize0;
        uint8_t currentconfig;
        uint8_t parent;
        uint8_t port;
        uint8_t address;
        uint8_t bInterfaceNumber;
        uint8_t bAlternateSetting;
        uint8_t numep;
        ENDPOINT_INFO epInfo[16];
};



class UHS_HID_DEVICE_base {
        UHS_USB_HOST_BASE *USB_host;
#if defined(LOAD_UHS_BT)
        UHS_BT_HOST_BASE *BT_host;
#endif

        UHS_HID_DEVICE_base(void) {
                USB_host = NULL;
#if defined(LOAD_UHS_BT)
                BT_host = NULL;
#endif

        };
        virtual void Poll() {

        };
};




void UHS_HID_SetUSBInterface(UHS_USB_HOST_BASE *host, ENUMERATION_INFO *ei);
void UHS_HID_ScanUninitialized(void);
void UHS_HID_Poll(void);

#if defined(LOAD_UHS_HID) && !defined(UHS_HID_LOADED)
#include "UHS_HID_INLINE.h"
#endif
#endif // __UHS_HID_H__
