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

// TO-DO: parse config.
// TO-DO: implement cooked interface.

#if !defined(__UHS_HIDBOOT_MOUSE_H__)
#define __UHS_HIDBOOT_MOUSE_H__

#if !defined(DEBUG_PRINTF_EXTRA_HUGE_USB_HIDBOOT_MOUSE)
#define DEBUG_PRINTF_EXTRA_HUGE_USB_HIDBOOT_MOUSE 0
#endif

#if DEBUG_PRINTF_EXTRA_HUGE
#if DEBUG_PRINTF_EXTRA_HUGE_USB_HIDBOOT_MOUSE
#define HIDBOOT_MOUSE_DUBUG(...) printf(__VA_ARGS__)
#else
#define HIDBOOT_MOUSE_DUBUG(...) VOID0
#endif
#else
#define HIDBOOT_MOUSE_DUBUG(...) VOID0
#endif

struct MOUSEINFO {
        uint8_t ReportID;
        struct {
                uint8_t bmLeftButton : 1;
                uint8_t bmRightButton : 1;
                uint8_t bmMiddleButton : 1;
                uint8_t bmButton4 : 1;
                uint8_t bmButton5 : 1;
                uint8_t bmDummy : 3;
        }__attribute__((packed));
        int8_t dX;
        int8_t dY;
        int8_t wheel1;
        int8_t wheel2;
        int8_t wheel3;
}__attribute__((packed));

class UHS_HIDBOOT_mouse : public UHS_HID_base {
public:
        MOUSEINFO mouse_data[2];

        UHS_HIDBOOT_mouse(UHS_HID *p) {
                parent = p;
                driver = UHS_HID_mouse;
        }

        ~UHS_HIDBOOT_mouse(){};
        void AJK_NI driverPoll() {
                uint8_t data[parent->epInfo[parent->epInterruptInIndex].maxPktSize];
                uint16_t length = parent->epInfo[parent->epInterruptInIndex].maxPktSize;
                uint8_t rv = parent->pUsb->inTransfer(parent->bAddress, parent->epInfo[parent->epInterruptInIndex].epAddr, &length, data);

                if(rv == 0 && length > 1) {
                        if(*data == 0x01U) {
                                // send only mouse events. Other IDs are "system controls"
                                parent->hidProcessor->onPoll(this, data, length);
                        }
                } else if(rv != UHS_HOST_ERROR_NAK) {
                        HIDBOOT_MOUSE_DUBUG("DP %02x A %02x EI %02x EA %02x\r\n", rv, parent->bAddress, parent->epInterruptInIndex, parent->epInfo[parent->epInterruptInIndex].epAddr);
                }
        }

        void AJK_NI driverStart() {
                printf("BOOT_MOUSE\r\n");
                {
                        uint16_t length = 128;
                        uint8_t buffer[length];
                        parent->ReportDescr(parent->bIface, length, buffer);
                }
                parent->SetProtocol(parent->bIface, 0);
                parent->SetIdle(parent->bIface, 0, 0);
                parent->hidProcessor->onStart(this);
        }

        void AJK_NI driverRelease() {
                parent->hidProcessor->onRelease(this);
        }
};

#endif // __UHS_HIDBOOT_MOUSE_H__
