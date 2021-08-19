/*
 * File:   UHS_KINETIS_EHCI.h
 * Author: root
 *
 * Created on July 31, 2016, 1:00 AM
 */


/*
 * Notes.
 *
 *
 * Supported: Asynchronous for bulk and control
 * Asynchronous List Queue Head Pointer
 *
 *
 * Stuff I won't support at first, these do not fit the model, and no interface drivers uses them.
 * isochronous interrupt
 * Periodic Frame List schedule is for all periodic isochronous and interrupt transfers.
 * Thus, we do not use PERIODICLISTBASE or FRINDEX.
 * This also means we don't have to directly deal with any transaction translators,
 * built-in or otherwise.
 *
 */



// TO-DO: TX/RX packets.

#ifndef UHS_KINETIS_EHCI_H
#define UHS_KINETIS_EHCI_H

#ifdef LOAD_UHS_KINETIS_EHCI
#if !defined(SWI_IRQ_NUM)
#error include dyn_swi.h first
#endif

// 1 = LED debug
#ifndef LED_STATUS
#define LED_STATUS 0
#endif
#if defined(__IMXRT1052__) || defined(__IMXRT1062__)
#define              IRQ_USBHS IRQ_USB2
#define            USBPHY_CTRL USBPHY2_CTRL
#define        USBPHY_CTRL_CLR USBPHY2_CTRL_CLR
#define        USBPHY_CTRL_SET USBPHY2_CTRL_SET
#define           USBHS_USBCMD USB2_USBCMD
#define           USBHS_USBSTS USB2_USBSTS
#define          USBHS_USBINTR USB2_USBINTR
#define          USBHS_FRINDEX USB2_FRINDEX
#define USBHS_PERIODICLISTBASE USB2_PERIODICLISTBASE
#define    USBHS_ASYNCLISTADDR USB2_ASYNCLISTADDR
#define          USBHS_PORTSC1 USB2_PORTSC1
#define          USBHS_USBMODE USB2_USBMODE
#define      USBHS_GPTIMER0CTL USB2_GPTIMER0CTRL
#define       USBHS_GPTIMER0LD USB2_GPTIMER0LD
#define      USBHS_GPTIMER1CTL USB2_GPTIMER1CTRL
#define       USBHS_GPTIMER1LD USB2_GPTIMER1LD
#define       USBHS_USBCMD_ASE USB_USBCMD_ASE
#define       USBHS_USBCMD_IAA USB_USBCMD_IAA
#define       USBHS_USBCMD_RST USB_USBCMD_RST
#define    USBHS_USBCMD_ITC(n) USB_USBCMD_ITC(n)
#define        USBHS_USBCMD_RS USB_USBCMD_RS
#define    USBHS_USBCMD_ASP(n) USB_USBCMD_ASP(n)
#define      USBHS_USBCMD_ASPE USB_USBCMD_ASPE
#define       USBHS_USBCMD_PSE USB_USBCMD_PSE
#define       USBHS_USBCMD_FS2 USB_USBCMD_FS_2
#define     USBHS_USBCMD_FS(n) USB_USBCMD_FS_1(n)
#define       USBHS_USBSTS_AAI USB_USBSTS_AAI
#define        USBHS_USBSTS_AS USB_USBSTS_AS
#define       USBHS_USBSTS_UAI ((uint32_t)(1<<18))
#define       USBHS_USBSTS_UPI ((uint32_t)(1<<19))
#define       USBHS_USBSTS_UEI USB_USBSTS_UEI
#define       USBHS_USBSTS_PCI USB_USBSTS_PCI
#define       USBHS_USBSTS_TI0 USB_USBSTS_TI0
#define       USBHS_USBSTS_TI1 USB_USBSTS_TI1
#define       USBHS_USBSTS_SEI USB_USBSTS_SEI
#define       USBHS_USBSTS_URI USB_USBSTS_URI
#define       USBHS_USBSTS_SLI USB_USBSTS_SLI
#define       USBHS_USBSTS_HCH USB_USBSTS_HCH
#define      USBHS_USBSTS_NAKI USB_USBSTS_NAKI
#define      USBHS_USBINTR_PCE USB_USBINTR_PCE
#define     USBHS_USBINTR_TIE0 USB_USBINTR_TIE0
#define     USBHS_USBINTR_TIE1 USB_USBINTR_TIE1
#define      USBHS_USBINTR_UEE USB_USBINTR_UEE
#define      USBHS_USBINTR_SEE USB_USBINTR_SEE
#define     USBHS_USBINTR_UPIE USB_USBINTR_UPIE
#define     USBHS_USBINTR_UAIE USB_USBINTR_UAIE
#define      USBHS_PORTSC_PFSC USB_PORTSC1_PFSC
#define        USBHS_PORTSC_PP USB_PORTSC1_PP
#define       USBHS_PORTSC_OCC USB_PORTSC1_OCC
#define       USBHS_PORTSC_PEC USB_PORTSC1_PEC
#define       USBHS_PORTSC_CSC USB_PORTSC1_CSC
#define       USBHS_PORTSC_CCS USB_PORTSC1_CCS
#define        USBHS_PORTSC_PE USB_PORTSC1_PE
#define       USBHS_PORTSC_HSP USB_PORTSC1_HSP
#define       USBHS_PORTSC_FPR USB_PORTSC1_FPR
#define        USBHS_PORTSC_PR USB_PORTSC1_PR
#define   USBHS_GPTIMERCTL_RST USB_GPTIMERCTRL_GPTRST
#define   USBHS_GPTIMERCTL_RUN USB_GPTIMERCTRL_GPTRUN
#define    USBHS_USBMODE_CM(n) USB_USBMODE_CM(n)
#define      USBHS_USB_SBUSCFG USB2_SBUSCFG
#ifdef ARDUINO_TEENSY41
#define     USBHS_USB_VBUS_SET GPIO8_DR_SET
#define     USBHS_USB_VBUS_CLR GPIO8_DR_CLR
#define     USBHS_USB_VBUS_BIT (1<<26)
#endif
#else
#define     USBHS_USB_VBUS_SET GPIOE_PSOR
#define     USBHS_USB_VBUS_CLR GPIOE_PCOR
#define     USBHS_USB_VBUS_BIT (1<<6)
#endif


#include <wiring.h>

#define UHS_KINETIS_EHCI_TOKEN_SETUP         (0x02)
#define UHS_KINETIS_EHCI_TOKEN_DATA_IN       (0x01)
#define UHS_KINETIS_EHCI_TOKEN_DATA_OUT      (0x00)

#define UHS_KINETIS_EHCI_MAXIMUM_PACKET      (16384)

typedef struct _uhs_kehci_qh {
        volatile uint32_t horizontalLinkPointer; /* queue head horizontal link pointer */
        volatile uint32_t staticEndpointStates[2]; /* static endpoint state and configuration information */
        volatile uint32_t currentQtdPointer; /* current qTD pointer */
        volatile uint32_t nextQtdPointer; /* next qTD pointer */
        volatile uint32_t alternateNextQtdPointer; /* alternate next qTD pointer */
        volatile uint32_t transferOverlayResults[6]; /* transfer overlay configuration and transfer results */
        uint32_t rspace[4]; // reserved space
} __attribute__ ((packed)) uhs_kehci_qh_t;

typedef struct _UHS_EHCI_TOKEN {

        union {
                uint32_t token;

                struct {
                        uint8_t status : 8; // 8bits status
                        uint8_t PID : 2; // 2 bits pid
                        uint8_t CERR : 2; // 2 bits error counter
                        uint8_t C_page : 3; // 3 bits current page
                        uint8_t ioc : 1; // 1 bit interrupt on complete -- we don't use this yet :-)
                        uint16_t length : 15; // 15 bits packet length
                        uint8_t toggle : 1; // 1 bit data 0/1 toggle
                } __attribute__ ((packed));

        };
} UHS_EHCI_TOKEN;

typedef struct _uhs_kehci_qtd {
        volatile uint32_t nextQtdPointer; /* QTD specification filed, the next QTD pointer */
        volatile uint32_t alternateNextQtdPointer; /* QTD specification filed, alternate next QTD pointer */
        volatile UHS_EHCI_TOKEN transferResults; /* QTD specification filed, transfer results fields */
        volatile uint32_t bufferPointers[5]; /* QTD specification filed, transfer buffer fields */
} __attribute__ ((packed)) uhs_kehci_qtd_t;


class UHS_KINETIS_EHCI : public UHS_USB_HOST_BASE, public dyn_SWI {
        uhs_kehci_qh_t QH __attribute__ ((aligned(64)));
        uhs_kehci_qtd_t qTD __attribute__ ((aligned(32)));
        uhs_kehci_qtd_t qHalt __attribute__ ((aligned(32)));
        uint8_t data_buf[UHS_KINETIS_EHCI_MAXIMUM_PACKET] __attribute__ ((aligned(4))); // I/O buffer

        volatile uint8_t hub_present;
        volatile uint8_t vbusState;
        volatile uint16_t sof_countdown;
        volatile uint16_t timer_countdown;
        volatile uint16_t nak_countdown;

        // TO-DO: pack into a struct/union and use one byte
        volatile bool insidetask;
        volatile bool busevent;
        volatile bool sofevent;
        volatile bool counted;
        volatile bool condet;
        volatile bool doingreset;
        volatile bool newError;
        volatile uint8_t isrError;
        volatile bool isrHappened;

        volatile uint32_t sof_mark; // Next time in MICROSECONDS that an SOF will be seen
        volatile uint32_t last_mark; // LAST time in MICROSECONDS that a packet was completely sent
        volatile uint8_t frame_counter;

#if LED_STATUS
        volatile bool CL1;
        volatile bool CL2;
        volatile bool CL3;
#endif


public:

        UHS_NI UHS_KINETIS_EHCI(void) {
                nak_countdown = 0;
                sof_countdown = 0;
                timer_countdown = 0;
                insidetask = false;
                busevent = false;
                sofevent = false;
                doingreset = false;
                hub_present = 0;
                last_mark = 0;
                frame_counter = 0;
                isrError = 0;
                newError = false;
                isrHappened = false;
#if LED_STATUS
                CL1 = false;
                CL2 = true;
                CL3 = false;
#endif
        };
        void ISRTask(void);
        void ISRbottom(void);
        void init_qTD(uint32_t len, uint32_t data01);
        void busprobe(void);

        virtual void VBUS_changed(void);

        // Note, this is not counting SOFs :-)

        virtual bool UHS_NI sof_delay(uint16_t x) {
                timer_countdown = x;
                while((timer_countdown != 0) && !condet) {
                }
                return (!condet);

        };

        virtual void UHS_NI vbusPower(VBUS_t state) {
#if defined(USBHS_USB_VBUS_BIT)
                if(state) {
                        USBHS_USB_VBUS_CLR = USBHS_USB_VBUS_BIT;
                } else {
                        USBHS_USB_VBUS_SET = USBHS_USB_VBUS_BIT;
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

        void UHS_NI IsHub(NOTUSED(bool p)) {
        };

        void UHS_NI ReleaseChildren(void) {
                hub_present = 0;
                for(uint8_t i = 0; i < UHS_HOST_MAX_INTERFACE_DRIVERS; i++)
                        if(devConfig[i])
                                devConfig[i]->Release();
        };

        virtual void UHS_NI doHostReset(void) {

                USBTRACE("\r\nBUS RESET.\r\n");

                // Issue a bus reset
                noInterrupts();
                doingreset = true;
                busevent = true;
                USBHS_PORTSC1 |= USBHS_PORTSC_PR;
                interrupts();
                while(busevent) {
                        DDSB();
                }
                noInterrupts();
                doingreset = false;
                DDSB();
                interrupts();
                //sofevent = true;
                // start SOF generation
                // YOUR CODE HERE to start SOF generation (IF REQUIRED!)

                // Wait for SOF
                // while(sofevent) {
                // }
                sof_delay(200);
                //sofevent = false;
        };

        int16_t UHS_NI Init(int16_t mseconds);

        int16_t UHS_NI Init(void) {
                return Init(INT16_MIN);
        };

        void dyn_SWISR(void) {
                ISRbottom();
        };

        virtual void UHS_NI suspend_host(void) {
        }; // NOP, AVR only

        virtual void UHS_NI resume_host(void) {
        }; // NOP, AVR only

};


#include "UHS_KINETIS_EHCI_INLINE.h"
#endif
#endif /* UHS_KINETIS_EHCI_H */
