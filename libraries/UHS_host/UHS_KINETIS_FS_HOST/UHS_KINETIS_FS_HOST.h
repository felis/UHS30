/* Copyright (C) 2015-2016 Andrew J. Kroll
   and
Copyright (C) 2011 Circuits At Home, LTD. All rights reserved.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

Contact information
-------------------

Circuits At Home, LTD
Web      :  http://www.circuitsathome.com
e-mail   :  support@circuitsathome.com
 */

#ifndef UHS_KINETIS_FS_HOST_H
#define	UHS_KINETIS_FS_HOST_H
#ifdef LOAD_UHS_KINETIS_FS_HOST
#if !defined(SWI_IRQ_NUM)
#error include dyn_swi.h first
#endif
#if DEBUG_PRINTF_EXTRA_HUGE
#ifdef DEBUG_PRINTF_EXTRA_HUGE_USB_HOST_KINETIS
#define KINETIS_HOST_DEBUG(...) printf(__VA_ARGS__)
#else
#define KINETIS_HOST_DEBUG(...) VOID0
#endif
#else
#define KINETIS_HOST_DEBUG(...) VOID0
#endif

// define some stuff
#define UHS_KINETIS_FS_bmKSTATUS     0x00  // full speed
#define UHS_KINETIS_FS_bmJSTATUS     0x80  // low speed
#define UHS_KINETIS_FS_bmSE0         0xC0  // SE0 - disconnect state
#define UHS_KINETIS_FS_bmSE1         0x40  // SE1 - illegal state

#define UHS_KINETIS_FS_SE0     0
#define UHS_KINETIS_FS_SE1     1
#define UHS_KINETIS_FS_FSHOST  2
#define UHS_KINETIS_FS_LSHOST  3

#define UHS_KINETIS_FS_bmHUBPRE    0x04

#define UHS_KINETIS_FS_TOKEN_SETUP         (0xD0)
#define UHS_KINETIS_FS_TOKEN_DATA_IN       (0x90)
#define UHS_KINETIS_FS_TOKEN_DATA_OUT      (0x10)
#define USB_ADDR_LSEN  (0x80) // low speed enable bit

// can be from 8 to 64
#define UHS_KINETIS_FS_EP0_SIZE    (64)


#define UHS_KINETIS_FS_BDT_OWN     0x80
#define UHS_KINETIS_FS_BDT_DATA1   0x40
#define UHS_KINETIS_FS_BDT_DATA0   0x00
#define UHS_KINETIS_FS_BDT_DTS     0x08
#define UHS_KINETIS_FS_BDT_STALL   0x04
#define UHS_KINETIS_FS_BDT_PID(n)  (((n) >> 2) & 15)

#define UHS_KINETIS_FS_BDT_DESC(count, data)       (UHS_KINETIS_FS_BDT_OWN | UHS_KINETIS_FS_BDT_DTS \
				        | ((data) ? UHS_KINETIS_FS_BDT_DATA1 : UHS_KINETIS_FS_BDT_DATA0) \
				        | ((count) << 16))

#define UHS_KINETIS_FS_TX   1
#define UHS_KINETIS_FS_RX   0
#define UHS_KINETIS_FS_ODD  1
#define UHS_KINETIS_FS_EVEN 0
#define UHS_KINETIS_FS_DATA0 0
#define UHS_KINETIS_FS_DATA1 1
#define UHS_KINETIS_FS_index(endpoint, tx, odd) (((endpoint) << 2) | ((tx) << 1) | (odd))
#define UHS_KINETIS_FS_stat2bufferdescriptor(stat) (table + ((stat) >> 2))

#define UHS_KINETIS_FS_PID_DATA0       0x3
#define UHS_KINETIS_FS_PID_DATA1       0xB
#define UHS_KINETIS_FS_PID_ACK         0x2
#define UHS_KINETIS_FS_PID_STALL       0xE
#define UHS_KINETIS_FS_PID_NAK         0xA
#define UHS_KINETIS_FS_PID_BUS_TIMEOUT 0x0
#define UHS_KINETIS_FS_PID_DATA_ERROR  0xF

// Buffer Descriptor Table
// desc contains information about the transfer
// addr points to the transmitted/received data buffer
typedef struct {
        uint32_t desc;
        void * addr;
} __attribute__((packed)) bdt_t;

class UHS_KINETIS_FS_HOST : public UHS_USB_HOST_BASE , public dyn_SWI {

        volatile uint8_t hub_present;
        volatile uint8_t vbusState;
        volatile uint16_t sof_countdown;
        volatile uint16_t timer_countdown;

        // TO-DO: pack into a struct/union and use one byte
        volatile bool insidetask;
        volatile bool busevent;
        volatile bool sofevent;
        volatile bool counted;
	volatile bool condet;

        volatile uint32_t sof_mark; // Next time in MICROSECONDS that an SOF will be seen
        volatile uint32_t last_mark; // LAST time in MICROSECONDS that a packet was completely sent
        volatile uint8_t frame_counter;

        //
        // The bdt table is the way we talk to the USB SIE
        //
        // Two entries per endpoint direction allows ping-pong buffering.
        // EP0 has two directions, so in total we need 4 entries.
        // NOTE: We do not actually do this. My code is so fast, we do not need them. 8-)
        // As a side-benefit, we save a little bit of room. \o/
        //
        // Unfortunately required to be in a special named section.
        // This effectively would disallow > 1 port, but there are no devices like this.
        // Basically I use it as a name-space qualifier by declaring it here.
        // That helps to avoid name collisions in other code as it is a member.
        //
        __attribute__ ((section(".usbdescriptortable"), used)) static bdt_t table[4];
        __attribute__ ((section(".usbdescriptortable"), used)) static uint8_t data_in_buf[UHS_KINETIS_FS_EP0_SIZE] __attribute__ ((aligned (4)));  // to receive data in as host
        __attribute__ ((section(".usbdescriptortable"), used)) static uint8_t data_out_buf[UHS_KINETIS_FS_EP0_SIZE] __attribute__ ((aligned (4)));  // to send data out as host
        // mark that a token was done and store its pid while inside the isr
        volatile bool newToken;
        volatile uint8_t isrPid;
        volatile bool newError;
        volatile uint8_t isrError;
        // bdt entry of token done
        volatile bdt_t b_newToken;

        // last transfer: byte count and address
        //uint32_t last_count;
        //void *last_address;
        //bool last_tx;

        uint8_t sof_threshold; // depending on the max packet size this number will be different, set at init.

        // implementation helper members and methods
        // sends and receive data
        void endpoint0_transmit(const void *data, uint32_t len);
        void endpoint0_receive(const void *data, uint32_t len);

        // Prints information about a token that just completed
        //static void debug_tokdne(uint8_t stat);

        const uint8_t *ep0_tx_ptr;
        uint16_t ep0_tx_len;
        uint8_t ep0_tx_bdt_bank;
        uint8_t ep0_tx_data_toggle;

        const uint8_t *ep0_rx_ptr;
        uint16_t ep0_rx_len;
        uint8_t ep0_rx_bdt_bank;
        uint8_t ep0_rx_data_toggle;


        uint8_t setup_command_buffer[8]; // for setup commands

public:
        UHS_NI UHS_KINETIS_FS_HOST (void) {
                sof_countdown = 0;
                timer_countdown = 0;
                insidetask = false;
                busevent = false;
                sofevent = false;
                hub_present = 0;
                last_mark = 0;
                frame_counter = 0;

                //b_newToken = 0;
                ep0_tx_ptr = nullptr;
                ep0_rx_ptr = nullptr;
                isrError = 0;
                isrPid = 0;
                newError = false;
                newToken = false;

                //last_count = 0;
                //last_address = nullptr;
                //last_tx = false;
        };

        void ISRTask(void);
        void ISRbottom(void);

        // helper functions too
        void busprobe(void);
        virtual void VBUS_changed(void);

        // Note, this is not counting SOFs :-)
        virtual bool UHS_NI sof_delay(uint16_t x) {
                timer_countdown = x;
                while((timer_countdown != 0) && !condet) {
                }
                return (!condet);

        };

        // VBUS is always powered for Teensy 3.x (other KINETIS products may not do that though...)
        virtual void UHS_NI vbusPower(VBUS_t state) {
#if defined(UHS_USB_VBUS)
        if(state == vbus_on) {
                digitalWriteFast(UHS_USB_VBUS, HIGH);
        } else {
                digitalWriteFast(UHS_USB_VBUS, LOW);
        }
#endif
        };

        virtual void Task(void); // {};

        virtual uint8_t SetAddress(uint8_t addr, uint8_t ep, UHS_EpInfo **ppep, uint16_t &nak_limit);
        virtual uint8_t OutTransfer(UHS_EpInfo *pep, uint16_t nak_limit, uint16_t nbytes, uint8_t *data);
        virtual uint8_t InTransfer(UHS_EpInfo *pep, uint16_t nak_limit, uint16_t *nbytesptr, uint8_t *data);
        virtual UHS_EpInfo *ctrlReqOpen(uint8_t addr, uint64_t Request, uint8_t *dataptr);
        virtual uint8_t ctrlReqClose(UHS_EpInfo *pep, uint8_t bmReqType, uint16_t left, uint16_t nbytes, uint8_t *dataptr);
        virtual uint8_t ctrlReqRead(UHS_EpInfo *pep, uint16_t *left, uint16_t *read, uint16_t nbytes, uint8_t *dataptr);
        virtual uint8_t dispatchPkt(uint8_t token, uint8_t ep, uint16_t nak_limit);

        void UHS_NI IsHub(bool p) {
                if(p) {
                        hub_present = UHS_KINETIS_FS_bmHUBPRE;
                } else {
                        hub_present = 0;
                }
        };

        void UHS_NI ReleaseChildren(void) {
                hub_present = 0;
                for(uint8_t i = 0; i < UHS_HOST_MAX_INTERFACE_DRIVERS; i++)
                        if(devConfig[i])
                                devConfig[i]->Release();
        };

        virtual void UHS_NI doHostReset(void) {

                KINETIS_HOST_DEBUG("\r\nBUS RESET.\r\n");
                // Issue a bus reset
                // YOUR CODE HERE to issue a BUS_RESET
                USB0_CTL |= USB_CTL_RESET;
                sof_delay(20); // delay at least 20 ms
                USB0_CTL &= ~USB_CTL_RESET;

                sofevent = true;
                // start SOF generation
                // YOUR CODE HERE to start SOF generation (IF REQUIRED!)
                USB0_CTL |= USB_CTL_USBENSOFEN; // start generating SOFs

                // Wait for SOF
                while(sofevent) {
                }
                sof_delay(200);
        };

        int16_t UHS_NI Init(int16_t mseconds);

        int16_t UHS_NI Init(void) {
                return Init(INT16_MIN);
        };

        void dyn_SWISR(void) {
                ISRbottom();
        };

        virtual void UHS_NI suspend_host(void) {}; // NOP, AVR only
        virtual void UHS_NI resume_host(void) {}; // NOP, AVR only

};

// This is required so that linkage works.
// For some odd reason GCC is not smart enough to pull this into existence.
// Perhaps this is a bug?
bdt_t UHS_KINETIS_FS_HOST::table[];
uint8_t UHS_KINETIS_FS_HOST::data_in_buf[];
uint8_t UHS_KINETIS_FS_HOST::data_out_buf[];


#include "UHS_KINETIS_FS_HOST_INLINE.h"

#else
#endif


#endif
