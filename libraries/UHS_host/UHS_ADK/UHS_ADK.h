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

/* Google ADK interface support header */

#if !defined(__UHS_ADK_H__)
#define __UHS_ADK_H__

#include <UHS_host.h>

#define ADK_MAX_ENDPOINTS 3 // endpoint 0, bulk_IN, bulk_OUT

#if !defined(UHS_ADK_MANUFACTURER)
#define UHS_ADK_MANUFACTURER "Circuits at Home"
#endif
#if !defined(UHS_ADK_MODEL)
#define UHS_ADK_MODEL "Arduino"
#endif
#if !defined(UHS_ADK_DESCRIPTION)
#define UHS_ADK_DESCRIPTION "Example ADK device"
#endif
#if !defined(UHS_ADK_VERSION)
#define UHS_ADK_VERSION "0.1"
#endif
#if !defined(UHS_ADK_URI)
#define UHS_ADK_URI "http://www.circuitsathome.com"
#endif
#if !defined(UHS_ADK_SERIAL)
#define UHS_ADK_SERIAL "0001"
#endif

#define UHS_PID_GOOGLE_ADK   0x2D00
#define UHS_PID_GOOGLE_ADB   0x2D01

/* requests */
#define UHS_ADK_GETPROTO      51  // check USB accessory protocol version
#define UHS_ADK_SENDSTR       52  // send identifying string
#define UHS_ADK_ACCSTART      53  // start device in accessory mode

#define UHS_ADK_bmREQ_GET     USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_VENDOR|USB_SETUP_RECIPIENT_DEVICE
#define UHS_ADK_bmREQ_SEND    USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_VENDOR|USB_SETUP_RECIPIENT_DEVICE

#define UHS_ADK_ID_MANUFACTURER   0
#define UHS_ADK_ID_MODEL          1
#define UHS_ADK_ID_DESCRIPTION    2
#define UHS_ADK_ID_VERSION        3
#define UHS_ADK_ID_URI            4
#define UHS_ADK_ID_SERIAL         5

#if DEBUG_PRINTF_EXTRA_HUGE
#ifdef DEBUG_PRINTF_EXTRA_HUGE_ADK_HOST
#define ADK_HOST_DEBUG(...) printf(__VA_ARGS__)
#else
#define ADK_HOST_DEBUG(...) VOID0
#endif
#else
#define ADK_HOST_DEBUG(...) VOID0
#endif

class UHS_ADK_Enabler : public UHS_USBInterface {
public:
        volatile UHS_EpInfo epInfo[1]; // we only need the control endpoint

        bool OKtoEnumerate(ENUMERATION_INFO *ei);
        uint8_t SetInterface(ENUMERATION_INFO *ei);
        uint8_t OnStart(void);

        /* ADK proprietary requests */
        uint8_t getProto(uint8_t *adkproto);
        uint8_t sendStr(uint8_t index, const char *str);
        uint8_t switchAcc(void);
};


class UHS_ADK : public UHS_USBInterface {
public:
        static const uint8_t epDataInIndex = 1; // DataIn endpoint index
        static const uint8_t epDataOutIndex = 2; // DataOUT endpoint index
        volatile UHS_EpInfo epInfo[ADK_MAX_ENDPOINTS];
        volatile bool ready; // device ready indicator
        UHS_ADK_Enabler enabler;

        UHS_ADK(UHS_USB_HOST_BASE *p);

        // Methods for receiving and sending data
        uint8_t Read(uint16_t *nbytesptr, uint8_t *dataptr);
        uint8_t Write(uint16_t nbytes, uint8_t *dataptr);

        uint8_t OnStart(void);
        bool OKtoEnumerate(ENUMERATION_INFO *ei);
        uint8_t SetInterface(ENUMERATION_INFO *ei);
        void DriverDefaults(void);

        uint8_t GetAddress() {
                return bAddress;
        };

        bool isReady() {
                pUsb->DisablePoll();
                bool rv = ready;
                pUsb->EnablePoll();
                return rv;
        };
};

#if defined(LOAD_UHS_ADK) && !defined(UHS_ADK_LOADED)
#include "UHS_ADK_INLINE.h"
#endif
#endif // __UHS_ADK_H__
