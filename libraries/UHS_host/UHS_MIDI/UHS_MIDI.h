/* Copyright (c) 2017 Yuuichi Akagawa

This software may be distributed and modified under the terms of the GNU
General Public License version 2 (GPL2) as published by the Free Software
Foundation and appearing in the file GPL2.TXT included in the packaging of
this file. Please note that GPL2 Section 2[b] requires that all works based
on this software must also be made publicly available under the terms of
the GPL2 ("Copyleft").

 */

/* USB MIDI interface support header */
/*
 * Note: This driver is support for MIDI Streaming class only.
 *       If your MIDI Controller is not work, probably you needs its vendor specific driver.
 */
#if !defined(__UHS_MIDI_H__)
#define __UHS_MIDI_H__

#include <UHS_host.h>

#define MIDI_MAX_ENDPOINTS 3 //endpoint 0, bulk_IN(MIDI), bulk_OUT(MIDI)
#define MIDI_EVENT_PACKET_SIZE 64
#define MIDI_MAX_SYSEX_SIZE   256

#if DEBUG_PRINTF_EXTRA_HUGE
#ifdef DEBUG_PRINTF_EXTRA_HUGE_MIDI_HOST
#define MIDI_HOST_DEBUG(...) printf(__VA_ARGS__)
#else
#define MIDI_HOST_DEBUG(...) VOID0
#endif
#else
#define MIDI_HOST_DEBUG(...) VOID0
#endif

class UHS_MIDI : public UHS_USBInterface {
protected:
        uint16_t countSysExDataSize(uint8_t *dataptr);

public:
        static const uint8_t epDataInIndex = 1; // DataIn endpoint index(MIDI)
        static const uint8_t epDataOutIndex = 2; // DataOUT endpoint index(MIDI)
        volatile UHS_EpInfo epInfo[MIDI_MAX_ENDPOINTS];
        volatile bool ready; // device ready indicator
        uint8_t qPollRate; // How fast to poll maximum
        uint16_t pid, vid;

        /* MIDI Event packet buffer */
        uint8_t recvBuf[MIDI_EVENT_PACKET_SIZE];
        uint8_t readPtr;

        UHS_MIDI(UHS_USB_HOST_BASE *p);

        // Methods for receiving and sending data
        uint8_t RecvData(uint16_t *bytes_rcvd, uint8_t *dataptr);
        uint8_t RecvData(uint8_t *outBuf, bool isRaw = false);
        uint8_t RecvRawData(uint8_t *outBuf);
        uint8_t SendData(uint8_t *dataptr, uint8_t nCable = 0);
        uint8_t lookupMsgSize(uint8_t midiMsg, uint8_t cin = 0);
        uint8_t SendSysEx(uint8_t *dataptr, uint16_t datasize, uint8_t nCable = 0);
        uint8_t extractSysExData(uint8_t *p, uint8_t *buf);
        uint8_t SendRawData(uint16_t bytes_send, uint8_t *dataptr);

        uint8_t Start(void);
        bool OKtoEnumerate(ENUMERATION_INFO *ei);
        uint8_t SetInterface(ENUMERATION_INFO *ei);
        void DriverDefaults(void);

        bool Polling(void) {
                return bPollEnable;
        }

        void Poll(void);

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

#if defined(LOAD_UHS_MIDI) && !defined(UHS_MIDI_LOADED)
#include "UHS_MIDI_INLINE.h"
#endif
#endif // __UHS_MIDI_H__
