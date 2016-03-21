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

#ifndef USB_HOST_SHIELD_H
#define USB_HOST_SHIELD_H


#ifdef LOAD_USB_HOST_SHIELD
#include "UHS_max3421e.h"
#include <SPI.h>


#if !defined(SPI_HAS_TRANSACTION)
#error "Your SPI library installation is too old."
#else
#if !defined(SPI_ATOMIC_VERSION)
#warning "Your SPI library installation lacks 'SPI_ATOMIC_VERSION'. Please complaint to the maintainer."
#elif SPI_ATOMIC_VERSION < 1
#error "Your SPI library installation is too old."
#endif

#endif

#if !defined(USB_HOST_SHIELD_USE_ISR)
#if defined(USE_MULTIPLE_APP_API)
#define USB_HOST_SHIELD_USE_ISR 0
#else
#define USB_HOST_SHIELD_USE_ISR 1
#endif
#else
#define USB_HOST_SHIELD_USE_ISR 1
#endif



#if !USB_HOST_SHIELD_USE_ISR
#error NOISR Polled mode _NOT SUPPORTED YET_

//
// Polled defaults
//

#ifdef BOARD_BLACK_WIDDOW
#define UHS_MAX3421E_SS_ 6
#define UHS_MAX3421E_INT_ 3
#elif defined(CORE_TEENSY) && (defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB1286__))
#if EXT_RAM
// Teensy++ 2.0 with XMEM2
#define UHS_MAX3421E_SS_ 20
#define UHS_MAX3421E_INT_ 7
#else
#define UHS_MAX3421E_SS_ 9
#define UHS_MAX3421E_INT_ 8
#endif
#define UHS_MAX3421E_SPD
#elif defined(BOARD_MEGA_ADK)
#define UHS_MAX3421E_SS_ 53
#define UHS_MAX3421E_INT_ 54
#elif defined(ARDUINO_AVR_BALANDUINO)
#define UHS_MAX3421E_SS_ 20
#define UHS_MAX3421E_INT_ 19
#else
#define UHS_MAX3421E_SS_ 10
#define UHS_MAX3421E_INT_ 9
#endif

#else
#if defined(ARDUINO_ARCH_PIC32)
// PIC32 only allows edge interrupts, isn't that lovely? We'll emulate it...
#if CHANGE < 2
#error core too old.
#endif

#define IRQ_IS_EDGE 0
#ifndef digitalPinToInterrupt
// great, this isn't implemented.
#warning digitalPinToInterrupt is not defined, complain here https://github.com/chipKIT32/chipKIT-core/issues/114
#if defined(_BOARD_UNO_) || defined(_BOARD_UC32_)
#define digitalPinToInterrupt(p) ((p) == 2 ? 1 : ((p) == 7 ? 2 : ((p) == 8 ? 3 : ((p) == 35 ? 4 : ((p) == 38 ? 0 : NOT_AN_INTERRUPT)))))
#warning digitalPinToInterrupt is now defined until this is taken care of.
#else
#error digitalPinToInterrupt not defined for your board, complain here https://github.com/chipKIT32/chipKIT-core/issues/114
#endif
#endif
#else
#define IRQ_IS_EDGE 0
#endif

// SAMD uses an enum for this instead of a define. Isn't that just dandy?
#if !defined(NOT_AN_INTERRUPT) && !defined(ARDUINO_ARCH_SAMD)
#warning NOT_AN_INTERRUPT not defined, possible problems ahead.
#warning If NOT_AN_INTERRUPT is an enum or something else, complain to UHS30 developers on github.
#warning Otherwise complain to your board core developer/maintainer.
#define NOT_AN_INTERRUPT -1
#endif

//
// Interrupt defaults. Int0 or Int1
//
#ifdef BOARD_BLACK_WIDDOW
#error "HELP! Please send us an email, I don't know the values for Int0 and Int1 on the Black Widow board!"
#elif defined(CORE_TEENSY) && (defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB1286__))

// TO-DO!

#if EXT_RAM
// Teensy++ 2.0 with XMEM2
#define UHS_MAX3421E_SS_ 20
#define UHS_MAX3421E_INT_ 7
#else
#define UHS_MAX3421E_SS_ 9
#define UHS_MAX3421E_INT_ 8
#endif

#elif defined(ARDUINO_AVR_BALANDUINO)
#error "ISR mode is currently not supported on the Balanduino. Please set USB_HOST_SHIELD_USE_ISR to 0."
#else
#define UHS_MAX3421E_SS_ 10
#ifdef __AVR__
#if defined(__AVR_ATmega32U4__)
#define INT_FOR_PIN2 1
#define INT_FOR_PIN3 0
#else
// Everybody else???
#define INT_FOR_PIN2 0
#define INT_FOR_PIN3 1
#endif
#define UHS_MAX3421E_INT_ 3
#else
// Non-avr
#if defined(ARDUINO_ARCH_PIC32)
// UNO32 External Interrupts:
// Pin 38 (INT0), Pin 2 (INT1), Pin 7 (INT2), Pin 8 (INT3), Pin 35 (INT4)
#define UHS_MAX3421E_INT_ 7
#else
#define UHS_MAX3421E_INT_ 9
#endif
#endif
#endif
#endif

#if !defined(UHS_MAX3421E_SPD)
#if defined(ARDUINO_SAMD_ZERO)
#define UHS_MAX3421E_SPD 10000000
#elif defined(ARDUINO_ARCH_PIC32)
#define UHS_MAX3421E_SPD 18000000
#else
#define UHS_MAX3421E_SPD 25000000
#endif
#endif

#ifndef UHS_MAX3421E_INT
#define UHS_MAX3421E_INT UHS_MAX3421E_INT_
#endif

#ifndef UHS_MAX3421E_SS
#define UHS_MAX3421E_SS UHS_MAX3421E_SS_
#endif

// NOTE: On the max3421e the irq enable and irq bits are in the same position.

// IRQs used if CPU polls
#define   ENIBITSPOLLED (bmCONDETIE | bmBUSEVENTIE  | bmFRAMEIE)
// IRQs used if CPU is interrupted
#define      ENIBITSISR (bmCONDETIE | bmBUSEVENTIE | bmFRAMEIE /* | bmRCVDAVIRQ | bmSNDBAVIRQ | bmHXFRDNIRQ */ )

#if !USB_HOST_SHIELD_USE_ISR
#define IRQ_CHECK_MASK (ENIBITSPOLLED & ICLRALLBITS)
#define IRQ_IS_EDGE 0
#else
#define IRQ_CHECK_MASK (ENIBITSISR & ICLRALLBITS)
#endif

#if IRQ_IS_EDGE
// Note: UNO32 Interrupts can only be RISING, or FALLING.
// This poses an interesting problem, since we want to use a LOW level.
// The MAX3421E provides for pulse width control for an IRQ.
// We do need to watch the timing on this, as a second IRQ could cause
// a missed IRQ, since we read the level of the line to check if the IRQ
// is actually for this chip. The only other alternative is to add a capacitor
// and an NPN transistor, and use two lines. We can try this first, though.
// Worse case, we can ignore reading the pin for verification on UNO32.
// Too bad there is no minimum low width setting.
//
//   Single    Clear     First  Second   Clear first      Clear last
//   IRQ       Single    IRQ    IRQ      Second active    pending IRQ
//      |      |         |      |        |                |
//      V      V         V      V        V                V
// _____        _________        _        _                _______
//      |______|         |______| |______| |______________|
//
#define IRQ_SENSE FALLING
#if defined(ARDUINO_ARCH_PIC32)
#define bmPULSEWIDTH PUSLEWIDTH10_6
#define bmIRQ_SENSE 0
#else
#define bmPULSEWIDTH PUSLEWIDTH1_3
#define bmIRQ_SENSE 0
#endif
#else
#define IRQ_SENSE LOW
#define bmPULSEWIDTH 0
#define bmIRQ_SENSE bmINTLEVEL
#endif



class MAX3421E_HOST :
public UHS_USB_HOST_BASE
#if defined(SWI_IRQ_NUM)
, public dyn_SWI
#endif
{
        // TO-DO: move these into the parent class.
        volatile uint8_t vbusState;
        volatile uint16_t sof_countdown;
        volatile bool islowspeed; // This should be a number to indicate speed.

        // TO-DO: pack into a struct/union and use one byte
        volatile bool busevent;
        volatile bool sofevent;
        volatile bool counted;
        volatile bool condet;

public:
        SPISettings MAX3421E_SPI_Settings;
        uint8_t ss_pin;
        uint8_t irq_pin;
        // Will use the defaults UHS_MAX3421E_SS, UHS_MAX3421E_INT and speed

        UHS_NI MAX3421E_HOST(void) {
                sof_countdown = 0;
                islowspeed = false;
                busevent = false;
                sofevent = false;
                condet = false;
                ss_pin = UHS_MAX3421E_SS;
                irq_pin = UHS_MAX3421E_INT;
                MAX3421E_SPI_Settings = SPISettings(UHS_MAX3421E_SPD, MSBFIRST, SPI_MODE0);
                hub_present = 0;
        };

        // Will use user supplied pins, and UHS_MAX3421E_SPD

        UHS_NI MAX3421E_HOST(uint8_t pss, uint8_t pirq) {
                sof_countdown = 0;
                islowspeed = false;
                busevent = false;
                sofevent = false;
                condet = false;
                ss_pin = pss;
                irq_pin = pirq;
                MAX3421E_SPI_Settings = SPISettings(UHS_MAX3421E_SPD, MSBFIRST, SPI_MODE0);
                hub_present = 0;
        };

        // Will use user supplied pins, and speed

        UHS_NI MAX3421E_HOST(uint8_t pss, uint8_t pirq, uint32_t pspd) {
                sof_countdown = 0;
                islowspeed = false;
                busevent = false;
                sofevent = false;
                condet = false;
                ss_pin = pss;
                irq_pin = pirq;
                MAX3421E_SPI_Settings = SPISettings(pspd, MSBFIRST, SPI_MODE0);
                hub_present = 0;
        };

        virtual bool UHS_NI sof_delay(uint16_t x) {
                sof_countdown = x;
                while((sof_countdown != 0) && !condet) {

#if !USB_HOST_SHIELD_USE_ISR
                        Task();
#endif
                }
                //                Serial.println("...Wake");
                return (!condet);
        };

        virtual UHS_EpInfo *ctrlReqOpen(uint8_t addr, uint8_t bmReqType, uint8_t bRequest, uint8_t wValLo, uint8_t wValHi, uint16_t wInd, uint16_t total, uint8_t* dataptr);

        virtual void UHS_NI vbusPower(VBUS_t state) {
                regWr(rPINCTL, (bmFDUPSPI | bmIRQ_SENSE) | (uint8_t)(state));
        };

        void UHS_NI Task(void);

        virtual uint8_t SetAddress(uint8_t addr, uint8_t ep, UHS_EpInfo **ppep, uint16_t &nak_limit);
        virtual uint8_t OutTransfer(UHS_EpInfo *pep, uint16_t nak_limit, uint16_t nbytes, uint8_t *data);
        virtual uint8_t InTransfer(UHS_EpInfo *pep, uint16_t nak_limit, uint16_t *nbytesptr, uint8_t *data);
        virtual uint8_t ctrlReqClose(UHS_EpInfo *pep, uint8_t bmReqType, uint16_t left, uint16_t nbytes, uint8_t *dataptr);
        virtual uint8_t ctrlReqRead(UHS_EpInfo *pep, uint16_t *left, uint16_t *read, uint16_t nbytes, uint8_t *dataptr);
        virtual uint8_t dispatchPkt(uint8_t token, uint8_t ep, uint16_t nak_limit);

        void UHS_NI ReleaseChildren(void) {
                for(uint8_t i = 0; i < UHS_HOST_MAX_INTERFACE_DRIVERS; i++)
                        if(devConfig[i])
                                devConfig[i]->Release();
                hub_present = 0;
        };

        virtual bool IsHub(uint8_t klass) {
                if(klass == UHS_USB_CLASS_HUB) {
                        hub_present = bmHUBPRE;
                        return true;
                }
                return false;
        };

        virtual uint8_t VBUS_changed(void);

        virtual void UHS_NI doHostReset(void) {
                busevent = true;
                regWr(rHIRQ, bmBUSEVENTIRQ); // see data sheet.
                regWr(rHCTL, bmBUSRST); //issue bus reset
                while(busevent) {

#if !USB_HOST_SHIELD_USE_ISR
                        Task();
#endif
                }

                sofevent = true;
                uint8_t tmpdata = regRd(rMODE) | bmSOFKAENAB; //start SOF generation
                regWr(rHIRQ, bmFRAMEIRQ); // see data sheet.
                regWr(rMODE, tmpdata);
                while(sofevent) {

#if !USB_HOST_SHIELD_USE_ISR
                        Task();
#endif
                }
        };


        int16_t UHS_NI Init(int16_t mseconds);

        int16_t UHS_NI Init(void) {
                return Init(INT16_MIN);
        };

        void ISRTask(void);
        void ISRbottom(void);
        void busprobe(void);
        uint16_t reset(void);

        // MAX3421e specific
        void regWr(uint8_t reg, uint8_t data);
        void gpioWr(uint8_t data);
        uint8_t regRd(uint8_t reg);
        uint8_t gpioRd(void);
        uint8_t* bytesWr(uint8_t reg, uint8_t nbytes, uint8_t* data_p);
        uint8_t* bytesRd(uint8_t reg, uint8_t nbytes, uint8_t* data_p);

        // ARM/NVIC specific, used to emulate reentrant ISR.
#if defined(SWI_IRQ_NUM)
        void dyn_SWISR(void) {
                ISRbottom();
        };
#endif



};
#if !defined(USB_HOST_SHIELD_LOADED)
#include "USB_HOST_SHIELD_INLINE.h"
#endif
#else
#error "define LOAD_USB_HOST_SHIELD in your sketch, never include USB_HOST_SHIELD.h in a driver."
#endif
#endif /* USB_HOST_SHIELD_H */
