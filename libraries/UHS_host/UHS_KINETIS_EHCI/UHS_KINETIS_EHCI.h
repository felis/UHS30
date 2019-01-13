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
#define	UHS_KINETIS_EHCI_H

#ifdef LOAD_UHS_KINETIS_EHCI
#if !defined(SWI_IRQ_NUM)
#error include dyn_swi.h first
#endif

// 1 = LED debug
#ifndef LED_STATUS
#define LED_STATUS 1
#endif


#include <wiring.h>

#if defined(UHS_FUTURE)
#ifndef UHS_KEHCI_MAX_FRAMES
#define UHS_KEHCI_MAX_FRAMES (1024)
#endif
#ifndef UHS_KEHCI_MAX_QH
#define UHS_KEHCI_MAX_QH (8)
#endif
#ifndef UHS_KEHCI_MAX_QTD
#define UHS_KEHCI_MAX_QTD (8)
#endif
#ifndef UHS_KEHCI_MAX_ITD
#define UHS_KEHCI_MAX_ITD (8)
#endif
#ifndef UHS_KEHCI_MAX_SITD
#define UHS_KEHCI_MAX_SITD (8)
#endif
#ifndef UHS_KEHCI_MAX_ISO
#define UHS_KEHCI_MAX_ISO UHS_KEHCI_MAX_ITD
#endif
#endif // UHS_FUTURE

typedef struct _uhs_kehci_qh {
        uint32_t horizontalLinkPointer; /* queue head horizontal link pointer */
        uint32_t staticEndpointStates[2]; /* static endpoint state and configuration information */
        volatile uint32_t currentQtdPointer; /* current qTD pointer */
        volatile uint32_t nextQtdPointer; /* next qTD pointer */
        volatile uint32_t alternateNextQtdPointer; /* alternate next qTD pointer */
        volatile uint32_t transferOverlayResults[6]; /* transfer overlay configuration and transfer results */
        uint32_t rspace[4]; // reserved space
} uhs_kehci_qh_t;

typedef struct _uhs_kehci_qtd {
        volatile uint32_t nextQtdPointer; /* QTD specification filed, the next QTD pointer */
        volatile uint32_t alternateNextQtdPointer; /* QTD specification filed, alternate next QTD pointer */
        volatile uint32_t transferResults; /* QTD specification filed, transfer results fields */
        volatile uint32_t bufferPointers[5]; /* QTD specification filed, transfer buffer fields */
} uhs_kehci_qtd_t;

#if defined(UHS_FUTURE)

typedef struct _uhs_kehci_itd {
        uint32_t nextLinkPointer; /* ITD specification filed, the next linker pointer */
        uint32_t transactions[8]; /* ITD specification filed, transactions information */
        uint32_t bufferPointers[7]; /* ITD specification filed, transfer buffer fields */

        /* add space */
        struct _uhs_kehci_itd *nextItdPointer; /* Next ITD pointer */
        uint32_t frameEntryIndex; /* The ITD inserted frame value */
        uint32_t reserved[6]; /* Reserved fields for 32 bytes align */
} uhs_kehci_itd_t;

typedef struct _uhs_kehci_sitd {
        uint32_t nextLinkPointer; /* SITD specification filed, the next linker pointer */
        uint32_t endpointStates[2]; /* SITD specification filed, endpoint configuration information */
        uint32_t transferResults[3]; /* SITD specification filed, transfer result fields */
        uint32_t backPointer; /* SITD specification filed, back pointer */

        /* reserved space */
        uint16_t frameEntryIndex; /* The SITD inserted frame value */
        uint8_t nextSitdIndex; /* The next SITD index; Get the next SITD pointer through adding base address with the
                                 index. 0xFF means invalid. */
        uint8_t reserved; /* Reserved fields for 32 bytes align */
} uhs_kehci_sitd_t;

/*
 *
 * reference from fs stack structures, we'll be doing something smarter here...
 */
typedef struct _usb_host_ehci_iso {
        struct _usb_host_ehci_iso *next; /* Next instance pointer */
        usb_host_pipe_t *ehciPipePointer; /* This ISO's EHCI pipe pointer */
        usb_host_transfer_t *ehciTransferHead; /* Transfer list head on this ISO pipe */
        usb_host_transfer_t *ehciTransferTail; /* Transfer list head on this ISO pipe */

        uint16_t lastLinkFrame; /*!< It means that the inserted frame for ISO ITD/SITD. 0xFFFF is invalid. For ITD, it is a
                               micro-frame value. For SITD, it is a frame value */
} usb_host_ehci_iso_t;

#define UHS_KEHCI_ehciUnitBase_MAX \
((sizeof(uhs_kehci_qh_t) * UHS_KEHCI_MAX_QH) + \
        (sizeof (uhs_kehci_qtd_t) * UHS_KEHCI_MAX_QTD)  + \
        (sizeof (uhs_kehci_itd_t) * UHS_KEHCI_MAX_ITD)  + \
        (sizeof (uhs_kehci_sitd_t) * UHS_KEHCI_MAX_SITD) + \
        (sizeof (usb_host_ehci_iso_t) * UHS_KEHCI_MAX_ISO))

//(sizeof(usb_host_ehci_pipe_t) * USB_HOST_CONFIG_MAX_PIPES);

typedef struct _Qs {
        uhs_kehci_qh_t qh[UHS_KEHCI_MAX_QH] __attribute__((aligned(64)));
        uhs_kehci_qtd_t qtd[UHS_KEHCI_MAX_QTD] __attribute__((aligned(32)));
#if defined(UHS_FUTURE)
        uhs_kehci_itd_t itd[UHS_KEHCI_MAX_ITD] __attribute__((aligned(32)));
        uhs_kehci_sitd_t sitd[UHS_KEHCI_MAX_SITD] __attribute__((aligned(32)));
#endif
} Qs_t;

#endif // UHS_FUTURE

class UHS_KINETIS_EHCI : public UHS_USB_HOST_BASE, public dyn_SWI {
#if defined(UHS_FUTURE)
        // Still needed???
        volatile uint32_t frame[UHS_KEHCI_MAX_FRAMES] __attribute__((aligned(4096)));
#endif

        //volatile Qs_t Q;
        uhs_kehci_qh_t QH __attribute__((aligned(64)));
        uhs_kehci_qtd_t qTD __attribute__((aligned(32)));
        uhs_kehci_qtd_t qHalt __attribute__((aligned(32)));

#if defined(UHS_FUTURE)
        volatile uint32_t qh[12] __attribute__((aligned(64)));
        uint32_t qtd_dummy[8] __attribute__((aligned(32)));
        uint32_t qtd_setup[8] __attribute__((aligned(32)));
        uint32_t qtd_in[8] __attribute__((aligned(32)));
        uint32_t qtd_outack[8] __attribute__((aligned(32)));
        uint32_t setupbuf[2] __attribute__((aligned(8)));
        uint32_t inbuf[16] __attribute__((aligned(64)));
#endif

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
        void UHS_NI poopOutStatus();
        void ISRTask(void);
        void ISRbottom(void);
        void init_qTD(void *buf, uint32_t len, uint32_t pid, uint32_t data01, bool irq);

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
                if(state) {
                        GPIOE_PCOR = (1 << 6); // turn off USB host power
                } else {
                        GPIOE_PSOR = (1 << 6); // turn on USB host power
                }
        };

        virtual void Task(void); // {};

        virtual uint8_t SetAddress(uint8_t addr, uint8_t ep, UHS_EpInfo **ppep, uint16_t &nak_limit);
        virtual uint8_t OutTransfer(UHS_EpInfo *pep, uint16_t nak_limit, uint16_t nbytes, uint8_t *data);
        virtual uint8_t InTransfer(UHS_EpInfo *pep, uint16_t nak_limit, uint16_t *nbytesptr, uint8_t *data);
        virtual UHS_EpInfo *ctrlReqOpen(uint8_t addr, uint64_t Request, uint8_t *dataptr);
        virtual uint8_t ctrlReqClose(UHS_EpInfo *pep, uint8_t bmReqType, uint16_t left, uint16_t nbytes, uint8_t *dataptr);
        virtual uint8_t ctrlReqRead(UHS_EpInfo *pep, uint16_t *left, uint16_t *read, uint16_t nbytes, uint8_t *dataptr);
        virtual uint8_t dispatchPkt(uint8_t token, uint8_t ep, uint16_t nak_limit);

        bool UHS_NI IsHub(uint8_t klass) {
                if(klass == UHS_USB_CLASS_HUB) {
                        //                        hub_present = UHS_KINETIS_FS_bmHUBPRE;
                        return true;
                }
                return false;
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

};


#include "UHS_KINETIS_EHCI_INLINE.h"
#endif
#endif	/* UHS_KINETIS_EHCI_H */

