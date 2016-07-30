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

#if !defined(_UHS_host_h_) || defined(USBCORE_H)
#error "Never include UHS_UsbCore.h directly; include UHS_Host.h instead"
#else
#define USBCORE_H

#ifndef UHS_HOST_MAX_INTERFACE_DRIVERS
#define                  UHS_HOST_MAX_INTERFACE_DRIVERS 16      // Maximum number of USB interface drivers
#endif


// As we make extensions to a target interface add to UHS_HOST_MAX_INTERFACE_DRIVERS
// This offset gets calculated for supporting wide subclasses, such as HID, BT, etc.
#define UHS_HID_INDEX (UHS_HOST_MAX_INTERFACE_DRIVERS + 1)

/* Common setup data constant combinations  */
//get descriptor request type
#define UHS_bmREQ_GET_DESCR \
        (USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_STANDARD|USB_SETUP_RECIPIENT_DEVICE)

//set request type for all but 'set feature' and 'set interface'
#define UHS_bmREQ_SET \
        (USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_STANDARD|USB_SETUP_RECIPIENT_DEVICE)
//get interface request type
#define UHS_bmREQ_CL_GET_INTF \
        (USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_INTERFACE)

// D7           data transfer direction (0 - host-to-device, 1 - device-to-host)
// D6-5         Type (0- standard, 1 - class, 2 - vendor, 3 - reserved)
// D4-0         Recipient (0 - device, 1 - interface, 2 - endpoint, 3 - other, 4..31 - reserved)

// TO-DO: Use the python script to generate these.
// TO-DO: Add _all_ subclasses here.
// USB Device Classes, Subclasses and Protocols
////////////////////////////////////////////////////////////////////////////////
// Use Class Info in the Interface Descriptors
#define                    UHS_USB_CLASS_USE_CLASS_INFO 0x00U

////////////////////////////////////////////////////////////////////////////////
// Audio
#define                             UHS_USB_CLASS_AUDIO 0x01U
// Subclasses
#define                   UHS_USB_SUBCLASS_AUDIOCONTROL 0x01U
#define                 UHS_USB_SUBCLASS_AUDIOSTREAMING 0x02U
#define                  UHS_USB_SUBCLASS_MIDISTREAMING 0x03U

////////////////////////////////////////////////////////////////////////////////
// Communications and CDC Control
#define                  UHS_USB_CLASS_COM_AND_CDC_CTRL 0x02U

////////////////////////////////////////////////////////////////////////////////
// HID
#define                               UHS_USB_CLASS_HID 0x03U

////////////////////////////////////////////////////////////////////////////////
// Physical
#define                          UHS_USB_CLASS_PHYSICAL 0x05U

////////////////////////////////////////////////////////////////////////////////
// Image
#define                             UHS_USB_CLASS_IMAGE 0x06U

////////////////////////////////////////////////////////////////////////////////
// Printer
#define                           UHS_USB_CLASS_PRINTER 0x07U

////////////////////////////////////////////////////////////////////////////////
// Mass Storage
#define                      UHS_USB_CLASS_MASS_STORAGE 0x08
// Subclasses
#define             UHS_BULK_SUBCLASS_SCSI_NOT_REPORTED 0x00U   // De facto use
#define                           UHS_BULK_SUBCLASS_RBC 0x01U
#define                         UHS_BULK_SUBCLASS_ATAPI 0x02U   // MMC-5 (ATAPI)
#define                     UHS_BULK_SUBCLASS_OBSOLETE1 0x03U   // Was QIC-157
#define                           UHS_BULK_SUBCLASS_UFI 0x04U   // Specifies how to interface Floppy Disk Drives to USB
#define                     UHS_BULK_SUBCLASS_OBSOLETE2 0x05U   // Was SFF-8070i
#define                          UHS_BULK_SUBCLASS_SCSI 0x06U   // SCSI Transparent Command Set
#define                         UHS_BULK_SUBCLASS_LSDFS 0x07U   // Specifies how host has to negotiate access before trying SCSI
#define                      UHS_BULK_SUBCLASS_IEEE1667 0x08U
// Protocols
#define                              UHS_STOR_PROTO_CBI 0x00U   // CBI (with command completion interrupt)
#define                       UHS_STOR_PROTO_CBI_NO_INT 0x01U   // CBI (without command completion interrupt)
#define                         UHS_STOR_PROTO_OBSOLETE 0x02U
#define                              UHS_STOR_PROTO_BBB 0x50U   // Bulk Only Transport
#define                              UHS_STOR_PROTO_UAS 0x62U

////////////////////////////////////////////////////////////////////////////////
// Hub
#define                               UHS_USB_CLASS_HUB 0x09U

////////////////////////////////////////////////////////////////////////////////
// CDC-Data
#define                          UHS_USB_CLASS_CDC_DATA 0x0AU

////////////////////////////////////////////////////////////////////////////////
// Smart-Card
#define                        UHS_USB_CLASS_SMART_CARD 0x0BU

////////////////////////////////////////////////////////////////////////////////
// Content Security
#define                  UHS_USB_CLASS_CONTENT_SECURITY 0x0DU

////////////////////////////////////////////////////////////////////////////////
// Video
#define                             UHS_USB_CLASS_VIDEO 0x0EU

////////////////////////////////////////////////////////////////////////////////
// Personal Healthcare
#define                   UHS_USB_CLASS_PERSONAL_HEALTH 0x0FU

////////////////////////////////////////////////////////////////////////////////
// Diagnostic Device
#define                 UHS_USB_CLASS_DIAGNOSTIC_DEVICE 0xDCU

////////////////////////////////////////////////////////////////////////////////
// Wireless Controller
#define                     UHS_USB_CLASS_WIRELESS_CTRL 0xE0U

////////////////////////////////////////////////////////////////////////////////
// Miscellaneous
#define                              UHS_USB_CLASS_MISC 0xEFU

////////////////////////////////////////////////////////////////////////////////
// Application Specific
#define                      UHS_USB_CLASS_APP_SPECIFIC 0xFEU

////////////////////////////////////////////////////////////////////////////////
// Vendor Specific
#define                   UHS_USB_CLASS_VENDOR_SPECIFIC 0xFFU

////////////////////////////////////////////////////////////////////////////////


/* USB state machine states */
#define UHS_USB_HOST_STATE_MASK                         0xF0U

// Configure states, MSN == 0
#define                     UHS_USB_HOST_STATE_DETACHED 0x00U
#define                     UHS_USB_HOST_STATE_DEBOUNCE 0x01U
#define        UHS_USB_HOST_STATE_DEBOUNCE_NOT_COMPLETE 0x02U
#define           UHS_USB_HOST_STATE_RESET_NOT_COMPLETE 0x03U
#define                     UHS_USB_HOST_STATE_WAIT_SOF 0x04U
#define               UHS_USB_HOST_STATE_WAIT_BUS_READY 0x05U
#define                 UHS_USB_HOST_STATE_RESET_DEVICE 0x0AU
#define                  UHS_USB_HOST_STATE_CONFIGURING 0x0CU // Looks like "CO"nfig (backwards)
#define             UHS_USB_HOST_STATE_CONFIGURING_DONE 0x0DU // Looks like "DO"one (backwards)
#define                        UHS_USB_HOST_STATE_CHECK 0x0EU
#define                      UHS_USB_HOST_STATE_ILLEGAL 0x0FU // Foo

// Run states, MSN != 0
#define                      UHS_USB_HOST_STATE_RUNNING 0x60U // Looks like "GO"
#define                         UHS_USB_HOST_STATE_IDLE 0x1DU // Looks like "ID"le
#define                        UHS_USB_HOST_STATE_ERROR 0xF0U // Looks like "FO"o
#define                   UHS_USB_HOST_STATE_INITIALIZE 0x10U // Looks like "I"nit

/* Host error result codes */
#define                                       hrSUCCESS 0x00U // No Error
#define                                          hrBUSY 0x01U // Hardware is busy, transfer pending
#define                                        hrBADREQ 0x02U // Transfer Launch Request was bad
#define                                           hrDMA 0x03U // DMA was too short, or too long
#define                                           hrNAK 0x04U // Device sent NAK
#define                                         hrSTALL 0x05U // Device sent STALL
#define                                        hrTOGERR 0x06U // toggle bit incorrect
#define                                      hrWRONGPID 0x07U // Received wrong Packet ID
#define                                         hrBADBC 0x08U // Byte count is bad
#define                                        hrPIDERR 0x09U // Received Packet ID is corrupted
#define                                        hrPKTERR 0x0AU // All other packet errors
#define                                        hrCRCERR 0x0BU // USB CRC was incorrect
#define                                          hrKERR 0x0CU // K-state instead of response, Usually indicates wrong speed
#define                                          hrJERR 0x0DU // J-state instead of response, Usually indicates wrong speed
#define                                       hrTIMEOUT 0x0EU // Device timed out, and did not respond in time
#define                                        hrBABBLE 0x0FU // Line noise/unexpected data
#define                                  hrDISCONNECTED 0xEEU // Device was disconnected.
// SEI interaction defaults
#define                        UHS_HOST_TRANSFER_MAX_MS 10000   // (originally 5000) USB transfer timeout in milliseconds, per section 9.2.6.1 of USB 2.0 spec
#define                 UHS_HOST_TRANSFER_RETRY_MAXIMUM 3       // 3 retry limit for a transfer
#define                      UHS_HOST_DEBOUNCE_DELAY_MS 500     // settle delay in milliseconds
#define                          UHS_HUB_RESET_DELAY_MS 20      // hub port reset delay, 10 ms recomended, but can be up to 20 ms

//
// We only provide the minimum needed information for enumeration.
// Drivers should be able to set up what is needed with nothing more.
// A driver needs to know the following information:
// 1: address on the USB network, parent and port (aka UsbDeviceAddress)
// 2: endpoints
// 3: vid:pid, class, subclass, protocol
//

struct ENDPOINT_INFO {
        uint8_t bEndpointAddress;       // Endpoint address. Bit 7 indicates direction (0=OUT, 1=IN).
        uint8_t bmAttributes;           // Endpoint transfer type.
        uint16_t wMaxPacketSize;        // Maximum packet size.
        uint8_t bInterval;              // Polling interval in frames.
} __attribute__((packed));

struct INTERFACE_INFO {
        uint8_t bInterfaceNumber;
        uint8_t bAlternateSetting;
        uint8_t numep;
        uint8_t klass;
        uint8_t subklass;
        uint8_t protocol;
        ENDPOINT_INFO epInfo[16];
} __attribute__((packed));

struct ENUMERATION_INFO {
        uint16_t vid;
        uint16_t pid;
        uint16_t bcdDevice;
        uint8_t klass;
        uint8_t subklass;
        uint8_t protocol;
        uint8_t bMaxPacketSize0;
        uint8_t currentconfig;
        uint8_t parent;
        uint8_t port;
        uint8_t address;
        INTERFACE_INFO interface;
} __attribute__((packed));

/* USB Setup Packet Structure   */
typedef struct {
        // offset   description
        //   0      Bit-map of request type
         union {
                uint8_t bmRequestType;

                struct {
                        uint8_t recipient : 5;  // Recipient of the request
                        uint8_t type : 2;       // Type of request
                        uint8_t direction : 1;  // Direction of data transfer
                } __attribute__((packed));
        } ReqType_u;

        //   1      Request
        uint8_t bRequest;

        //   2      Depends on bRequest
        union {
                uint16_t wValue;

                struct {
                        uint8_t wValueLo;
                        uint8_t wValueHi;
                } __attribute__((packed));
        } wVal_u;
        //   4      Depends on bRequest
        uint16_t wIndex;
        //   6      Depends on bRequest
        uint16_t wLength;
        // 8 bytes total
} __attribute__((packed)) SETUP_PKT, *PSETUP_PKT;


// little endian :-)                                                                             8                                8                          8                         8                          16                      16
#define mkSETUP_PKT8(bmReqType, bRequest, wValLo, wValHi, wInd, total) ((uint64_t)(((uint64_t)(bmReqType)))|(((uint64_t)(bRequest))<<8)|(((uint64_t)(wValLo))<<16)|(((uint64_t)(wValHi))<<24)|(((uint64_t)(wInd))<<32)|(((uint64_t)(total)<<48)))
#define mkSETUP_PKT16(bmReqType, bRequest, wVal, wInd, total)          ((uint64_t)(((uint64_t)(bmReqType)))|(((uint64_t)(bRequest))<<8)|(((uint64_t)(wVal  ))<<16)                           |(((uint64_t)(wInd))<<32)|(((uint64_t)(total)<<48)))

// Big endian -- but we aren't able to use this :-/
//#define mkSETUP_PKT8(bmReqType, bRequest, wValLo, wValHi, wInd, total) ((uint64_t)(((uint64_t)(bmReqType))<<56)|(((uint64_t)(bRequest))<<48)|(((uint64_t)(wValLo))<<40)|(((uint64_t)(wValHi))<<32)|(((uint64_t)(wInd))<<16)|((uint64_t)(total)))
//#define mkSETUP_PKT16(bmReqType, bRequest, wVal, wInd, total)          ((uint64_t)(((uint64_t)(bmReqType))<<56)|(((uint64_t)(bRequest))<<48)                           |(((uint64_t)(wVal))<<32)  |(((uint64_t)(wInd))<<16)|((uint64_t)(total)))

#endif /* USBCORE_H */
