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


/*
 * This file is here to prevent redundancy.
 */

#if !defined(__UHS_CDC_H__)
#define __UHS_CDC_H__
#include <UHS_host.h>

#define bmREQ_CDCOUT                    USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_INTERFACE
#define bmREQ_CDCIN                     USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_INTERFACE

// CDC Subclass Constants
#define               UHS_CDC_SUBCLASS_DLCM 0x01    // Direct Line Control Model
#define                UHS_CDC_SUBCLASS_ACM 0x02    // Abstract Control Model
#define                UHS_CDC_SUBCLASS_TCM 0x03    // Telephone Control Model
#define               UHS_CDC_SUBCLASS_MCCM 0x04    // Multi Channel Control Model
#define               UHS_CDC_SUBCLASS_CAPI 0x05    // CAPI Control Model
#define           UHS_CDC_SUBCLASS_ETHERNET 0x06    // Ethernet Network Control Model
#define                UHS_CDC_SUBCLASS_ATM 0x07    // ATM Network Control Model
#define   UHS_CDC_SUBCLASS_WIRELESS_HANDSET 0x08    // Wireless Handset Control Model
#define  UHS_CDC_SUBCLASS_DEVICE_MANAGEMENT 0x09    // Device Management
#define UHS_CDC_SUBCLASS_MOBILE_DIRECT_LINE 0x0A    // Mobile Direct Line Model
#define               UHS_CDC_SUBCLASS_OBEX 0x0B    // OBEX
#define       UHS_CDC_SUBCLASS_ETHERNET_EMU 0x0C    // Ethernet Emulation Model

// Communication Interface Class Control Protocol Codes
#define        UHS_CDC_PROTOCOL_ITU_T_V_250 0x01    // AT Commands defined by ITU-T V.250
#define           UHS_CDC_PROTOCOL_PCCA_101 0x02    // AT Commands defined by PCCA-101
#define         UHS_CDC_PROTOCOL_PCCA_101_O 0x03    // AT Commands defined by PCCA-101 & Annex O
#define           UHS_CDC_PROTOCOL_GSM_7_07 0x04    // AT Commands defined by GSM 7.07
#define         UHS_CDC_PROTOCOL_3GPP_27_07 0x05    // AT Commands defined by 3GPP 27.007
#define          UHS_CDC_PROTOCOL_C_S0017_0 0x06    // AT Commands defined by TIA for CDMA
#define            UHS_CDC_PROTOCOL_USB_EEM 0x07    // Ethernet Emulation Model

// CDC Commands defined by CDC 1.2
#define   UHS_CDC_SEND_ENCAPSULATED_COMMAND 0x00
#define   UHS_CDC_GET_ENCAPSULATED_RESPONSE 0x01

// CDC Commands defined by PSTN 1.2
#define            UHS_CDC_SET_COMM_FEATURE 0x02
#define            UHS_CDC_GET_COMM_FEATURE 0x03
#define          UHS_CDC_CLEAR_COMM_FEATURE 0x04
#define          UHS_CDC_SET_AUX_LINE_STATE 0x10
#define              UHS_CDC_SET_HOOK_STATE 0x11
#define                 UHS_CDC_PULSE_SETUP 0x12
#define                  UHS_CDC_SEND_PULSE 0x13
#define              UHS_CDC_SET_PULSE_TIME 0x14
#define               UHS_CDC_RING_AUX_JACK 0x15
#define             UHS_CDC_SET_LINE_CODING 0x20
#define             UHS_CDC_GET_LINE_CODING 0x21
#define      UHS_CDC_SET_CONTROL_LINE_STATE 0x22
#define                  UHS_CDC_SEND_BREAK 0x23
#define            UHS_CDC_SET_RINGER_PARMS 0x30
#define            UHS_CDC_GET_RINGER_PARMS 0x31
#define         UHS_CDC_SET_OPERATION_PARMS 0x32
#define         UHS_CDC_GET_OPERATION_PARMS 0x33
#define              UHS_CDC_SET_LINE_PARMS 0x34
#define              UHS_CDC_GET_LINE_PARMS 0x35
#define                 UHS_CDC_DIAL_DIGITS 0x36

// Class-Specific Notification Codes
#define          UHS_CDC_NETWORK_CONNECTION 0x00
#define          UHS_CDC_RESPONSE_AVAILABLE 0x01
#define         UHS_CDC_AUX_JACK_HOOK_STATE 0x08
#define                 UHS_CDC_RING_DETECT 0x09
#define                UHS_CDC_SERIAL_STATE 0x20
#define           UHS_CDC_CALL_STATE_CHANGE 0x28
#define           UHS_CDC_LINE_STATE_CHANGE 0x29
#define     UHS_CDC_CONNECTION_SPEED_CHANGE 0x2a

// CDC Functional Descriptor Structures

typedef struct {
        uint8_t bFunctionLength;
        uint8_t bDescriptorType;
        uint8_t bDescriptorSubtype;
        uint8_t bmCapabilities;
        uint8_t bDataInterface;
} UHS_CDC_CALL_MGMNT_FUNC_DESCR;

typedef struct {
        uint8_t bFunctionLength;
        uint8_t bDescriptorType;
        uint8_t bDescriptorSubtype;
        uint8_t bmCapabilities;
} UHS_CDC_ACM_FUNC_DESCR, UHS_CDC_DLM_FUNC_DESCR, UHS_CDC_TEL_OPER_MODES_FUNC_DESCR, UHS_CDC_TEL_CALL_STATE_REP_CPBL_FUNC_DESCR;

typedef struct {
        uint8_t bFunctionLength;
        uint8_t bDescriptorType;
        uint8_t bDescriptorSubtype;
        uint8_t bRingerVolSteps;
        uint8_t bNumRingerPatterns;
} UHS_CDC_TEL_RINGER_FUNC_DESCR;

typedef struct {
        uint32_t dwDTERate; // Data Terminal Rate in bits per second
        uint8_t bCharFormat; // 0 - 1 stop bit, 1 - 1.5 stop bits, 2 - 2 stop bits
        uint8_t bParityType; // 0 - None, 1 - Odd, 2 - Even, 3 - Mark, 4 - Space
        uint8_t bDataBits; // Data bits (5, 6, 7, 8 or 16)
} UHS_CDC_LINE_CODING;

typedef struct {
        uint8_t bmRequestType; // 0xa1 for class-specific notifications
        uint8_t bNotification;
        uint16_t wValue;
        uint16_t wIndex;
        uint16_t wLength;
        uint16_t bmState; //UART state bitmap for SERIAL_STATE, other notifications variable length
} UHS_CDC_CLASS_NOTIFICATION;

/**
 * This structure is used to report the extended capabilities of the connected device.
 * It is also used to report the current status.
 * Regular CDC-ACM reports all as false.
 */
typedef struct {

        union {
                uint8_t tty;

                struct {
                        bool enhanced : 1; // Do we have the ability to set/clear any features?
                        // Status and 8th bit in data stream.
                        // Presence only indicates feature is available, but this isn't used for CDC-ACM.
                        bool wide : 1;
                        bool autoflow_RTS : 1; // Has autoflow on RTS/CTS
                        bool autoflow_DSR : 1; // Has autoflow on DTR/DSR
                        bool autoflow_XON : 1; // Has autoflow  XON/XOFF
                        bool half_duplex : 1;  // Has half-duplex capability.
                } __attribute__((packed));
        };
} UHS_CDC_tty_features;

#if defined(LOAD_UHS_CDC_ACM_FTDI) && !defined(UHS_CDC_ACM_FTDI_LOADED)
#include "../UHS_CDC_ACM/UHS_CDC_ACM_FTDI.h"
#endif // CDC_ACM_FTDI loaded
#if defined(LOAD_UHS_CDC_ACM_PROLIFIC) && !defined(UHS_CDC_ACM_PROLIFIC_LOADED)
#include "../UHS_CDC_ACM/UHS_CDC_ACM_PROLIFIC.h"
#endif // CDC_ACM_PROLIFIC loaded
#if defined(LOAD_UHS_CDC_ACM_XR21B1411) && !defined(UHS_CDC_ACM_XR21B1411_LOADED)
#include "../UHS_CDC_ACM/UHS_CDC_ACM_XR21B1411.h"
#endif // CDC_ACM_XR21B1411 loaded
#if defined(LOAD_UHS_CDC_ACM) && !defined(UHS_CDC_ACM_LOADED)
#include "../UHS_CDC_ACM/UHS_CDC_ACM.h"
#endif // CDC_ACM loaded

#endif // __UHS_CDC_H__
