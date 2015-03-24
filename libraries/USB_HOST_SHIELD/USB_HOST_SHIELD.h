/* Copyright (C) 2014 Circuits At Home, LTD. All rights reserved.

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

#if 0

#if !defined(SPI_HAS_TRANSACTION)
#error "Your SPI library installation is too old."
#else
#if !defined(SPI_ATOMIC_VERSION)
#error "Your SPI library installation is too old."
#elif SPI_ATOMIC_VERSION < 1
#error "Your SPI library installation is too old."
#endif
#endif

#endif

// This most likely is broken.
#if USING_SPI4TEENSY3
#include <spi4teensy3.h>
#include <sys/types.h>
#endif

#if !defined(XMEM_SOFT_CLI)
#define XMEM_SOFT_CLI() VOID0
#define XMEM_SOFT_SEI() VOID0
#endif

#if !defined(UHS_USB_USES_ISRS)
#if defined(USE_MULTIPLE_APP_API)
#define UHS_USB_USES_ISRS 0
#else
#define UHS_USB_USES_ISRS 1
#endif
#else
#define UHS_USB_USES_ISRS 1
#endif


#if !USB_HOST_SHIELD_USE_ISR

//
// Polled defaults
//

#ifdef BOARD_BLACK_WIDDOW
#define UHS_MAX3421E_SS 6
#define UHS_MAX3421E_INT 3
#elif defined(CORE_TEENSY) && (defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB1286__))
#if EXT_RAM
// Teensy++ 2.0 with XMEM2
#define UHS_MAX3421E_SS 20
#define UHS_MAX3421E_INT 7
#else
#define UHS_MAX3421E_SS 9
#define UHS_MAX3421E_INT 8
#endif
#define UHS_MAX3421E_SPD
#elif defined(BOARD_MEGA_ADK)
#define UHS_MAX3421E_SS 53
#define UHS_MAX3421E_INT 54
#elif defined(ARDUINO_AVR_BALANDUINO)
#define UHS_MAX3421E_SS 20
#define UHS_MAX3421E_INT 19
#else
#define UHS_MAX3421E_SS 10
#define UHS_MAX3421E_INT 9
#endif

#else

//
// Interrupt defaults. Int0 or Int1
//
#ifdef BOARD_BLACK_WIDDOW
#error "HELP! Please send us an email, I don't know the values for Int0 and Int1 on the Black Widow board!"
#elif defined(CORE_TEENSY) && (defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB1286__))

// TO-DO!

#if EXT_RAM
// Teensy++ 2.0 with XMEM2
#define UHS_MAX3421E_SS 20
#define UHS_MAX3421E_INT 7
#else
#define UHS_MAX3421E_SS 9
#define UHS_MAX3421E_INT 8
#endif

#elif defined(BOARD_MEGA_ADK)
#error "HELP! Please send us an email, I don't know the values for Int0 and Int1 on the MEGA ADK board!"
#elif defined(ARDUINO_AVR_BALANDUINO)
#error "ISR mode is currently not supported on the Balanduino. Please set USB_HOST_SHIELD_USE_ISR to 0."
#else
#define UHS_MAX3421E_SS 10
#ifdef __AVR__
#if defined(__AVR_ATmega32U4__)
#define INT_FOR_PIN2 1
#define INT_FOR_PIN3 0
#else
// Everybody else???
#define INT_FOR_PIN2 0
#define INT_FOR_PIN3 1
#endif
#define UHS_MAX3421E_INT 3
#else
// Non-avr
#define UHS_MAX3421E_INT 9
#endif
#endif


#endif
#if !defined(UHS_MAX3421E_SPD)
#define UHS_MAX3421E_SPD 25000000
#endif

// NOTE: On the max3421e the irq enable and irq bits are in the same position.

// IRQs used if CPU polls
#define   ENIBITSPOLLED (bmCONDETIE | bmBUSEVENTIE  | bmFRAMEIE)
// IRQs used if CPU is interrupted
#define      ENIBITSISR (bmCONDETIE | bmBUSEVENTIE | bmFRAMEIE /* | bmRCVDAVIRQ | bmSNDBAVIRQ | bmHXFRDNIRQ */ )

#if !USB_HOST_SHIELD_USE_ISR
#define IRQ_CHECK_MASK (ENIBITSPOLLED & ICLRALLBITS)
#else
#define IRQ_CHECK_MASK (ENIBITSISR & ICLRALLBITS)
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
        uint8_t ss;
        uint8_t irq;
        // Will use the defaults UHS_MAX3421E_SS, UHS_MAX3421E_INT and speed

        UHS_NI MAX3421E_HOST(void) {
                sof_countdown = 0;
                islowspeed = false;
                busevent = false;
                sofevent = false;
                condet = false;
                ss = UHS_MAX3421E_SS;
                irq = UHS_MAX3421E_INT;
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
                ss = pss;
                irq = pirq;
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
                ss = pss;
                irq = pirq;
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
                regWr(rPINCTL, (bmFDUPSPI | bmINTLEVEL) | (uint8_t)(state));
        };

        virtual void Task(void);

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
#if defined(USE_MULTIPLE_APP_API)
                        xmem::Yield();
#endif
                        Task();
#endif
                }

                sofevent = true;
                uint8_t tmpdata = regRd(rMODE) | bmSOFKAENAB; //start SOF generation
                regWr(rHIRQ, bmFRAMEIRQ); // see data sheet.
                regWr(rMODE, tmpdata);
                while(sofevent) {

#if !USB_HOST_SHIELD_USE_ISR

#if defined(USE_MULTIPLE_APP_API)
                        xmem::Yield();
#endif
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
