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
#if !defined(__UHS_HUB_H__)
#define __UHS_HUB_H__

#include <UHS_host.h>


// Hub Requests
#define UHS_HUB_bmREQ_CLEAR_HUB_FEATURE         (USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_DEVICE)
#define UHS_HUB_bmREQ_CLEAR_PORT_FEATURE        (USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_OTHER)
#define UHS_HUB_bmREQ_CLEAR_TT_BUFFER           (USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_OTHER)
#define UHS_HUB_bmREQ_GET_HUB_DESCRIPTOR        (USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_DEVICE)
#define UHS_HUB_bmREQ_GET_HUB_STATUS            (USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_DEVICE)
#define UHS_HUB_bmREQ_GET_PORT_STATUS           (USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_OTHER)
#define UHS_HUB_bmREQ_RESET_TT                  (USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_OTHER)
#define UHS_HUB_bmREQ_SET_DESCRIPTOR            (USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_DEVICE)
#define UHS_HUB_bmREQ_SET_FEATURE               (USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_DEVICE)
#define UHS_HUB_bmREQ_SET_PORT_FEATURE          (USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_OTHER)
#define UHS_HUB_bmREQ_GET_TT_STATE              (USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_OTHER)
#define UHS_HUB_bmREQ_STOP_TT                   (USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_OTHER)

// Hub Class Requests
#define UHS_HUB_REQUEST_CLEAR_TT_BUFFER         8
#define UHS_HUB_REQUEST_RESET_TT                9
#define UHS_HUB_REQUEST_GET_TT_STATE            10
#define UHS_HUB_REQUEST_STOP_TT                 11

// Hub Features
#define UHS_HUB_FEATURE_C_HUB_LOCAL_POWER       0
#define UHS_HUB_FEATURE_C_HUB_OVER_CURRENT      1
#define UHS_HUB_FEATURE_PORT_CONNECTION         0
#define UHS_HUB_FEATURE_PORT_ENABLE             1
#define UHS_HUB_FEATURE_PORT_SUSPEND            2
#define UHS_HUB_FEATURE_PORT_OVER_CURRENT       3
#define UHS_HUB_FEATURE_PORT_RESET              4
#define UHS_HUB_FEATURE_PORT_POWER              8
#define UHS_HUB_FEATURE_PORT_LOW_SPEED          9
#define UHS_HUB_FEATURE_C_PORT_CONNECTION       16
#define UHS_HUB_FEATURE_C_PORT_ENABLE           17
#define UHS_HUB_FEATURE_C_PORT_SUSPEND          18
#define UHS_HUB_FEATURE_C_PORT_OVER_CURRENT     19
#define UHS_HUB_FEATURE_C_PORT_RESET            20
#define UHS_HUB_FEATURE_PORT_TEST               21
#define UHS_HUB_FEATURE_PORT_INDICATOR          22

// Hub Port Test Modes
#define UHS_HUB_PORT_TEST_MODE_J                1
#define UHS_HUB_PORT_TEST_MODE_K                2
#define UHS_HUB_PORT_TEST_MODE_SE0_NAK          3
#define UHS_HUB_PORT_TEST_MODE_PACKET           4
#define UHS_HUB_PORT_TEST_MODE_FORCE_ENABLE     5

// Hub Port Indicator Color
#define UHS_HUB_PORT_INDICATOR_AUTO             0
#define UHS_HUB_PORT_INDICATOR_AMBER            1
#define UHS_HUB_PORT_INDICATOR_GREEN            2
#define UHS_HUB_PORT_INDICATOR_OFF              3

// Hub Port Status Bitmasks
#define UHS_HUB_bmPORT_STATUS_PORT_CONNECTION   0x0001U
#define UHS_HUB_bmPORT_STATUS_PORT_ENABLE       0x0002U
#define UHS_HUB_bmPORT_STATUS_PORT_SUSPEND      0x0004U
#define UHS_HUB_bmPORT_STATUS_PORT_OVER_CURRENT 0x0008U
#define UHS_HUB_bmPORT_STATUS_PORT_RESET        0x0010U
#define UHS_HUB_bmPORT_STATUS_PORT_POWER        0x0100U
#define UHS_HUB_bmPORT_STATUS_PORT_LOW_SPEED    0x0200U
#define UHS_HUB_bmPORT_STATUS_PORT_HIGH_SPEED   0x0400U
#define UHS_HUB_bmPORT_STATUS_PORT_TEST         0x0800U
#define UHS_HUB_bmPORT_STATUS_PORT_INDICATOR    0x1000U

// Hub Status Bitmasks (used one byte instead of two)
#define UHS_HUB_bmSTATUS_LOCAL_POWER_SOURCE     0x01U
#define UHS_HUB_bmSTATUS_OVER_CURRENT           0x12U

// Hub Port Configuring Substates
#define UHS_HUB_STATE_PORT_CONFIGURING          0xb0U
#define UHS_HUB_STATE_PORT_POWERED_OFF          0xb1U
#define UHS_HUB_STATE_PORT_WAIT_FOR_POWER_GOOD  0xb2U
#define UHS_HUB_STATE_PORT_DISCONNECTED         0xb3U
#define UHS_HUB_STATE_PORT_DISABLED             0xb4U
#define UHS_HUB_STATE_PORT_RESETTING            0xb5U
#define UHS_HUB_STATE_PORT_ENABLED              0xb6U

// Additional Error Codes
#define UHS_HUB_ERROR_PORT_HAS_BEEN_RESET       0xb1U

// The bit mask to check for all necessary state bits
#define UHS_HUB_bmPORT_STATUS_ALL_MAIN          ((0UL  | UHS_HUB_bmPORT_STATUS_PORT_CONNECTION  | UHS_HUB_bmPORT_STATUS_PORT_ENABLE  | UHS_HUB_bmPORT_STATUS_PORT_SUSPEND  | UHS_HUB_bmPORT_STATUS_PORT_RESET) << 16) | UHS_HUB_bmPORT_STATUS_PORT_POWER | UHS_HUB_bmPORT_STATUS_PORT_ENABLE | UHS_HUB_bmPORT_STATUS_PORT_CONNECTION | UHS_HUB_bmPORT_STATUS_PORT_SUSPEND)

// Bit mask to check for DISABLED state in HubEvent::bmStatus field
#define UHS_HUB_bmPORT_STATE_CHECK_DISABLED     (0x0000U | UHS_HUB_bmPORT_STATUS_PORT_POWER | UHS_HUB_bmPORT_STATUS_PORT_ENABLE | UHS_HUB_bmPORT_STATUS_PORT_CONNECTION | UHS_HUB_bmPORT_STATUS_PORT_SUSPEND)

// Hub Port States
#define UHS_HUB_bmPORT_STATE_DISABLED           (0x0000U | UHS_HUB_bmPORT_STATUS_PORT_POWER | UHS_HUB_bmPORT_STATUS_PORT_CONNECTION)

// Hub Port Events
#define UHS_HUB_bmPORT_EVENT_CONNECT            (((0UL | UHS_HUB_bmPORT_STATUS_PORT_CONNECTION) << 16) | UHS_HUB_bmPORT_STATUS_PORT_POWER | UHS_HUB_bmPORT_STATUS_PORT_CONNECTION)
#define UHS_HUB_bmPORT_EVENT_DISCONNECT         (((0UL | UHS_HUB_bmPORT_STATUS_PORT_CONNECTION) << 16) | UHS_HUB_bmPORT_STATUS_PORT_POWER)
#define UHS_HUB_bmPORT_EVENT_RESET_COMPLETE     (((0UL | UHS_HUB_bmPORT_STATUS_PORT_RESET) << 16) | UHS_HUB_bmPORT_STATUS_PORT_POWER | UHS_HUB_bmPORT_STATUS_PORT_ENABLE | UHS_HUB_bmPORT_STATUS_PORT_CONNECTION)
#define UHS_HUB_bmPORT_EVENT_LS_CONNECT         (((0UL | UHS_HUB_bmPORT_STATUS_PORT_CONNECTION) << 16) | UHS_HUB_bmPORT_STATUS_PORT_POWER | UHS_HUB_bmPORT_STATUS_PORT_CONNECTION | UHS_HUB_bmPORT_STATUS_PORT_LOW_SPEED)
#define UHS_HUB_bmPORT_EVENT_LS_RESET_COMPLETE  (((0UL | UHS_HUB_bmPORT_STATUS_PORT_RESET) << 16) | UHS_HUB_bmPORT_STATUS_PORT_POWER | UHS_HUB_bmPORT_STATUS_PORT_ENABLE | UHS_HUB_bmPORT_STATUS_PORT_CONNECTION | UHS_HUB_bmPORT_STATUS_PORT_LOW_SPEED)
#define UHS_HUB_bmPORT_EVENT_LS_PORT_ENABLED    (((0UL | UHS_HUB_bmPORT_STATUS_PORT_CONNECTION | UHS_HUB_bmPORT_STATUS_PORT_ENABLE) << 16) | UHS_HUB_bmPORT_STATUS_PORT_POWER | UHS_HUB_bmPORT_STATUS_PORT_ENABLE | UHS_HUB_bmPORT_STATUS_PORT_CONNECTION | UHS_HUB_bmPORT_STATUS_PORT_LOW_SPEED)

struct UHS_HubDescriptor {
        uint8_t bDescLength; // descriptor length
        uint8_t bDescriptorType; // descriptor type
        uint8_t bNbrPorts; // number of ports a hub equiped with

        struct {
                uint16_t LogPwrSwitchMode : 2;
                uint16_t CompoundDevice : 1;
                uint16_t OverCurrentProtectMode : 2;
                uint16_t TTThinkTime : 2;
                uint16_t PortIndicatorsSupported : 1;
                uint16_t Reserved : 8;
        } __attribute__((packed));

        uint8_t bPwrOn2PwrGood;
        uint8_t bHubContrCurrent;
} __attribute__((packed));

struct UHS_HubEvent {

        union {

                struct {
                        uint16_t bmStatus; // port status bits
                        uint16_t bmChange; // port status change bits
                } __attribute__((packed));
                uint32_t bmEvent;
                uint8_t evtBuff[4];
        };
} __attribute__((packed));

class UHS_USBHub :  public UHS_USBInterface {
        bool bResetInitiated; // True when reset is triggered


        uint8_t bNbrPorts; // number of ports

        void CheckHubStatus(void);
        uint8_t PortStatusChange(uint8_t port, UHS_HubEvent &evt);
        void DriverDefaults(void);

public:
        UHS_EpInfo epInfo[2]; // interrupt endpoint info structure

        UHS_USBHub(UHS_USB_HOST_BASE *p);

        uint8_t ClearHubFeature(uint8_t fid);
        uint8_t ClearPortFeature(uint8_t fid, uint8_t port, uint8_t sel = 0);
        uint8_t GetHubDescriptor(uint8_t index, uint16_t nbytes, uint8_t *dataptr);
        uint8_t GetHubStatus(uint16_t nbytes, uint8_t* dataptr);
        uint8_t GetPortStatus(uint8_t port, uint16_t nbytes, uint8_t* dataptr);
        uint8_t SetHubDescriptor(uint8_t port, uint16_t nbytes, uint8_t* dataptr);
        uint8_t SetHubFeature(uint8_t fid);
        uint8_t SetPortFeature(uint8_t fid, uint8_t port, uint8_t sel = 0);

        void PrintHubStatus(void);

        virtual bool OKtoEnumerate(ENUMERATION_INFO *ei);
        virtual uint8_t SetInterface(ENUMERATION_INFO *ei);
        virtual uint8_t Start(void);
        virtual void Release(void);
        virtual void Poll(void);
        virtual void ResetHubPort(uint8_t port);
        virtual uint8_t Finalize(void);

        virtual uint8_t UHS_NI GetAddress(void) {
                return bAddress;
        };
};

// Clear Hub Feature

inline uint8_t UHS_USBHub::ClearHubFeature(uint8_t fid) {
        return ( pUsb->ctrlReq(bAddress, UHS_HUB_bmREQ_CLEAR_HUB_FEATURE, USB_REQUEST_CLEAR_FEATURE, fid, 0, 0, 0, 0, NULL));
}
// Clear Port Feature

inline uint8_t UHS_USBHub::ClearPortFeature(uint8_t fid, uint8_t port, uint8_t sel) {
        return ( pUsb->ctrlReq(bAddress, UHS_HUB_bmREQ_CLEAR_PORT_FEATURE, USB_REQUEST_CLEAR_FEATURE, fid, 0, ((0x0000 | port) | (sel << 8)), 0, 0, NULL));
}
// Get Hub Descriptor

inline uint8_t UHS_USBHub::GetHubDescriptor(uint8_t index, uint16_t nbytes, uint8_t *dataptr) {
        return ( pUsb->ctrlReq(bAddress, UHS_HUB_bmREQ_GET_HUB_DESCRIPTOR, USB_REQUEST_GET_DESCRIPTOR, index, 0x29, 0, nbytes, nbytes, dataptr));
}
// Get Hub Status

inline uint8_t UHS_USBHub::GetHubStatus(uint16_t nbytes, uint8_t* dataptr) {
        return ( pUsb->ctrlReq(bAddress, UHS_HUB_bmREQ_GET_HUB_STATUS, USB_REQUEST_GET_STATUS, 0, 0, 0x0000, nbytes, nbytes, dataptr));
}
// Get Port Status

inline uint8_t UHS_USBHub::GetPortStatus(uint8_t port, uint16_t nbytes, uint8_t* dataptr) {
        return ( pUsb->ctrlReq(bAddress, UHS_HUB_bmREQ_GET_PORT_STATUS, USB_REQUEST_GET_STATUS, 0, 0, port, nbytes, nbytes, dataptr));
}

// Set Hub Descriptor
//inline uint8_t UHS_USBHub::SetHubDescriptor(uint8_t port, uint16_t nbytes, uint8_t* dataptr) {
//        return ( pUsb->ctrlReq(bAddress, UHS_HUB_bmREQ_SET_DESCRIPTOR, USB_REQUEST_SET_DESCRIPTOR, 0, 0, port, nbytes, nbytes, dataptr));
//}
// Set Hub Feature

inline uint8_t UHS_USBHub::SetHubFeature(uint8_t fid) {
        return ( pUsb->ctrlReq(bAddress, UHS_HUB_bmREQ_SET_FEATURE, USB_REQUEST_SET_FEATURE, fid, 0, 0, 0, 0, NULL));
}
// Set Port Feature

inline uint8_t UHS_USBHub::SetPortFeature(uint8_t fid, uint8_t port, uint8_t sel) {
        return ( pUsb->ctrlReq(bAddress, UHS_HUB_bmREQ_SET_PORT_FEATURE, USB_REQUEST_SET_FEATURE, fid, 0, (((0x0000 | sel) << 8) | port), 0, 0, NULL));
}

void UHS_NI PrintHubPortStatus(UHS_USB_HOST_BASE *usbptr, uint8_t addr, uint8_t port, bool print_changes = false);
#if defined(LOAD_UHS_HUB) && !defined(UHS_HUB_LOADED)
#include "UHS_HUB_INLINE.h"
#endif
#endif // __UHS_USBHUB_H__
