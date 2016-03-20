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
#if !defined(__UHS_CDC_ACM_H__)
#define __UHS_CDC_ACM_H__
#include <UHS_CDC.h>
#define ACM_MAX_ENDPOINTS 4

// Exception PIDS
#define UHS_CDC_PROLIFIC_PID_1 0x0609U
#define UHS_CDC_PROLIFIC_PID_2 0x2303U

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
} tty_features;

class UHS_CDC_ACM : public UHS_USBInterface {
protected:
        uint8_t MbAddress; // master
        uint8_t SbAddress; // slave
        //uint8_t bConfNum; // configuration number
        uint8_t bControlIface; // Control interface value
        //uint8_t bDataIface; // Data interface value
        uint8_t qPollRate; // How fast to poll maximum
        volatile bool ready; //device ready indicator
        tty_features _enhanced_status; // current status

        void PrintEndpointDescriptor(const USB_ENDPOINT_DESCRIPTOR* ep_ptr);

public:
        static const uint8_t epDataInIndex = 1; // DataIn endpoint index
        static const uint8_t epDataOutIndex = 2; // DataOUT endpoint index
        static const uint8_t epInterruptInIndex = 3; // InterruptIN  endpoint index
        volatile UHS_EpInfo epInfo[ACM_MAX_ENDPOINTS];

        UHS_CDC_ACM(UHS_USB_HOST_BASE *p);
        uint8_t SetCommFeature(uint16_t fid, uint8_t nbytes, uint8_t *dataptr);
        uint8_t GetCommFeature(uint16_t fid, uint8_t nbytes, uint8_t *dataptr);
        uint8_t ClearCommFeature(uint16_t fid);
        uint8_t SetLineCoding(const UHS_CDC_LINE_CODING *dataptr);
        uint8_t GetLineCoding(UHS_CDC_LINE_CODING *dataptr);
        uint8_t SetControlLineState(uint8_t state);
        uint8_t SendBreak(uint16_t duration);
        uint8_t GetNotif(uint16_t *bytes_rcvd, uint8_t *dataptr);

        // Methods for receiving and sending data
        uint8_t Read(uint16_t *nbytesptr, uint8_t *dataptr);
        uint8_t Write(uint16_t nbytes, uint8_t *dataptr);


        uint8_t Start(void);
        bool OKtoEnumerate(ENUMERATION_INFO *ei);
        uint8_t SetInterface(ENUMERATION_INFO *ei);

        void DriverDefaults(void);

        //bool available(void) {
        //
        //};

        uint8_t GetAddress(void) {
                return bAddress;
        };

        bool Polling(void) {
                return bPollEnable;
        }

        void Poll(void);

        virtual bool isReady() {
                return ready;
        };

        virtual tty_features enhanced_status(void) {
                return _enhanced_status;
        };

        virtual tty_features enhanced_features(void) {
                tty_features rv;
                rv.enhanced = false;
                rv.autoflow_RTS = false;
                rv.autoflow_DSR = false;
                rv.autoflow_XON = false;
                rv.half_duplex = false;
                rv.wide = false;
                return rv;
        };

        virtual void autoflowRTS(NOTUSED(bool s)) {
        };

        virtual void autoflowDSR(NOTUSED(bool s)) {
        };

        virtual void autoflowXON(NOTUSED(bool s)) {
        };

        virtual void half_duplex(NOTUSED(bool s)) {
        };

        virtual void wide(NOTUSED(bool s)) {
        };

        // UsbConfigXtracter implementation
        void EndpointXtract(uint8_t conf, uint8_t iface, uint8_t alt, uint8_t proto, const USB_ENDPOINT_DESCRIPTOR *ep);
};
#if defined(LOAD_UHS_CDC_ACM) && !defined(UHS_CDC_ACM_LOADED)
#include "UHS_CDC_ACM_INLINE.h"
#endif
#endif // __UHS_CDC_ACM_H__
