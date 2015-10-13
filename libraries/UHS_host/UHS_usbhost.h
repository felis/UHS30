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

#if !defined(_UHS_host_h_)
#error "Never include UHS_usbhost.h directly; include UHS_host.h instead"
#else
#if !defined(_USBHOST_H_)
#define _USBHOST_H_

// Host SIE result codes.
// The SIE result is stored in the low nybble.
// Return these result codes from your host controller driver to match the error condition
// ALL Non-zero values are errors that are not handled in the base class.
// Values > 0x0F are driver or other internal error conditions.
#define UHS_HOST_ERROR_NONE      0x00 // No error
#define UHS_HOST_ERROR_BUSY      0x01 // transfer pending
#define UHS_HOST_ERROR_NAK       0x04 // Peripheral returned NAK
#define UHS_HOST_ERROR_STALL     0x05 // Peripheral returned STALL
#define UHS_HOST_ERROR_TOGERR    0x06 // Toggle error/ISO over-underrun
#define UHS_HOST_ERROR_JERR      0x0D // J-state instead of response
#define UHS_HOST_ERROR_BADRQ     0x0A // Packet error. Increase max packet.
#define UHS_HOST_ERROR_TIMEOUT   0x0E // Device did not respond in time

// Addressing error codes
#define ADDR_ERROR_INVALID_INDEX                        0xA0
#define ADDR_ERROR_INVALID_ADDRESS                      0xA1

// Driver error codes
#define UHS_HOST_ERROR_DEVICE_NOT_SUPPORTED             0xD1
#define UHS_HOST_ERROR_DEVICE_INIT_INCOMPLETE           0xD2
#define UHS_HOST_ERROR_CANT_REGISTER_DEVICE_CLASS       0xD3
#define UHS_HOST_ERROR_ADDRESS_POOL_FULL                0xD4
#define UHS_HOST_ERROR_HUB_ADDRESS_OVERFLOW             0xD5
#define UHS_HOST_ERROR_NO_ADDRESS_IN_POOL               0xD6
#define UHS_HOST_ERROR_NULL_EPINFO                      0xD7
#define UHS_HOST_ERROR_BAD_ARGUMENT                     0xD8
#define UHS_HOST_ERROR_DEVICE_DRIVER_BUSY               0xD9
#define UHS_HOST_ERROR_BAD_MAX_PACKET_SIZE              0xDA
#define UHS_HOST_ERROR_NO_ENDPOINT_IN_TABLE             0xDB
#define UHS_HOST_ERROR_UNPLUGGED                        0xDE
#define UHS_HOST_ERROR_FailGetDevDescr                  0xE1
#define UHS_HOST_ERROR_FailSetDevTblEntry               0xE2
#define UHS_HOST_ERROR_FailGetConfDescr                 0xE3
#define UHS_HOST_ERROR_END_OF_STREAM                    0xEF

// Host base class specific Error codes
#define UHS_HOST_ERROR_NOT_IMPLEMENTED                  0xFE
#define UHS_HOST_ERROR_TRANSFER_TIMEOUT                 0xFF

class UHS_USBInterface; // forward class declaration

// enumerator to turn the VBUS on/off

typedef enum {
        vbus_on = 0,
        vbus_off = 1
} VBUS_t;

// All host SEI use this base class

class UHS_USB_HOST_BASE {
public:
        AddressPool addrPool;
        UHS_USBInterface* devConfig[UHS_HOST_MAX_INTERFACE_DRIVERS];
        volatile uint8_t usb_error;
        volatile uint8_t usb_task_state;
        volatile uint8_t usb_task_polling_disabled;
        volatile uint8_t usb_host_speed;
        volatile uint8_t hub_present;

        UHS_USB_HOST_BASE(void) {
                for(int i = 0; i < UHS_HOST_MAX_INTERFACE_DRIVERS; i++) {
                        devConfig[i] = NULL;
                }
                usb_task_polling_disabled = 0;
                usb_task_state = UHS_USB_HOST_STATE_INITIALIZE; //set up state machine
                usb_host_speed = 0;
                usb_error = 0;
        };

        /////////////////////////////////////////////
        //
        // Virtual methods that interface to the SIE
        // Overriding each is mandatory.
        //
        /////////////////////////////////////////////

        /**
         * Delay for x milliseconds
         * Override if your controller provides an SOF IRQ, which may involve
         * some sort of reentrant ISR or workaround with interrupts enabled.
         *
         * @param x how many milliseconds to delay
         * @return true if delay completed without a state change, false if delay aborted
         */
        virtual bool UHS_NI sof_delay(uint16_t x) {
                if(!(usb_task_state & UHS_USB_HOST_STATE_MASK)) return false;
                uint8_t current_state = usb_task_state;
                while(current_state == usb_task_state && x--) {
                        delay(1);
                }
                return (current_state == usb_task_state);
        };

        virtual UHS_EpInfo * UHS_NI ctrlReqOpen(uint8_t addr, uint8_t bmReqType, uint8_t bRequest, uint8_t wValLo, uint8_t wValHi, uint16_t wInd, uint16_t total, uint8_t* dataptr) {
                return NULL;
        };

        virtual void UHS_NI vbusPower(VBUS_t state) {
        };

        virtual void UHS_NI Task(void) {
        };

        virtual uint8_t UHS_NI SetAddress(uint8_t addr, uint8_t ep, UHS_EpInfo **ppep, uint16_t &nak_limit) {
                return UHS_HOST_ERROR_NOT_IMPLEMENTED;
        };

        virtual uint8_t UHS_NI OutTransfer(UHS_EpInfo *pep, uint16_t nak_limit, uint16_t nbytes, uint8_t *data) {
                return UHS_HOST_ERROR_NOT_IMPLEMENTED;
        };

        virtual uint8_t UHS_NI InTransfer(UHS_EpInfo *pep, uint16_t nak_limit, uint16_t *nbytesptr, uint8_t *data) {
                return UHS_HOST_ERROR_NOT_IMPLEMENTED;
        };

        virtual uint8_t UHS_NI ctrlReqClose(UHS_EpInfo *pep, uint8_t bmReqType, uint16_t left, uint16_t nbytes, uint8_t *dataptr) {
                return UHS_HOST_ERROR_NOT_IMPLEMENTED;
        };

        virtual uint8_t UHS_NI ctrlReqRead(UHS_EpInfo *pep, uint16_t *left, uint16_t *read, uint16_t nbytes, uint8_t *dataptr) {
                return UHS_HOST_ERROR_NOT_IMPLEMENTED;
        };

        virtual uint8_t UHS_NI dispatchPkt(uint8_t token, uint8_t ep, uint16_t nak_limit) {
                return UHS_HOST_ERROR_NOT_IMPLEMENTED;
        };

        virtual uint8_t UHS_NI init(void) {
                return 0;
        };

        virtual void UHS_NI doHostReset(void) {
        };

        virtual int16_t UHS_NI Init(int16_t mseconds) {
                return -1;
        };

        virtual int16_t UHS_NI Init(void) {
                return Init(INT16_MIN);
        };

        virtual bool IsHub(uint8_t klass) {
                return (klass == UHS_USB_CLASS_HUB);
        };

        /////////////////////////////////////////////
        //
        // Built-ins, No need to override
        //
        /////////////////////////////////////////////

        inline void DisablePoll(void) {
                noInterrupts();
                usb_task_polling_disabled++;
#ifdef SWI_IRQ_NUM
                __DSB();
#endif
                interrupts();
        }

        inline void EnablePoll(void) {
                noInterrupts();
                usb_task_polling_disabled--;
#ifdef SWI_IRQ_NUM
                __DSB();
#endif
                interrupts();
        }

        uint8_t UHS_NI seekInterface(ENUMERATION_INFO *ei, uint16_t inf, USB_CONFIGURATION_DESCRIPTOR *ucd);

        uint8_t UHS_NI setEpInfoEntry(uint8_t addr, uint8_t epcount, volatile UHS_EpInfo* eprecord_ptr);

        uint8_t UHS_NI EPClearHalt(uint8_t addr, uint8_t ep);

        uint8_t UHS_NI ctrlReq(uint8_t addr, uint8_t bmReqType, uint8_t bRequest, uint8_t wValLo, uint8_t wValHi, uint16_t wInd, uint16_t total, uint16_t nbytes, uint8_t* dataptr);

        uint8_t UHS_NI getDevDescr(uint8_t addr, uint16_t nbytes, uint8_t* dataptr);

        uint8_t UHS_NI getConfDescr(uint8_t addr, uint16_t nbytes, uint8_t conf, uint8_t* dataptr);

        uint8_t UHS_NI setAddr(uint8_t oldaddr, uint8_t newaddr);

        uint8_t UHS_NI setConf(uint8_t addr, uint8_t conf_value);

        uint8_t UHS_NI getStrDescr(uint8_t addr, uint16_t nbytes, uint8_t index, uint16_t langid, uint8_t* dataptr);

        // uint8_t UHS_NI DefaultAddressing(uint8_t parent, uint8_t port, bool lowspeed);

        void UHS_NI ReleaseDevice(uint8_t addr);

        uint8_t UHS_NI Configuring(uint8_t parent, uint8_t port, bool lowspeed);

        void UHS_NI DeviceDefaults(uint8_t maxep, UHS_USBInterface *device);

        UHS_EpInfo* UHS_NI getEpInfoEntry(uint8_t addr, uint8_t ep);

        inline uint8_t getUsbTaskState(void) {
                return ( usb_task_state);
        };

        inline AddressPool* GetAddressPool(void) {
                return &addrPool;
        };

        int UHS_NI RegisterDeviceClass(UHS_USBInterface *pdev) {
                for(uint8_t i = 0; i < UHS_HOST_MAX_INTERFACE_DRIVERS; i++) {
                        if(!devConfig[i]) {
                                devConfig[i] = pdev;
                                return i;
                        }
                }
                //return UHS_HOST_ERROR_CANT_REGISTER_DEVICE_CLASS;
                return -1;
        };
#if 0

        inline void ForEachUsbDevice(UsbDeviceHandleFunc pfunc) {
                addrPool.ForEachUsbDevice(pfunc);
        };
#endif

        uint8_t TestInterface(ENUMERATION_INFO *ei);
        uint8_t enumerateInterface(ENUMERATION_INFO *ei);
        uint8_t getNextInterface(ENUMERATION_INFO *ei, UHS_EpInfo *pep, uint8_t data[], uint16_t *left, uint16_t *read, uint8_t *offset);
        uint8_t initDescrStream(ENUMERATION_INFO *ei, USB_CONFIGURATION_DESCRIPTOR *ucd, UHS_EpInfo *pep, uint8_t *data, uint16_t *left, uint16_t *read, uint8_t *offset);
        uint8_t outTransfer(uint8_t addr, uint8_t ep, uint16_t nbytes, uint8_t* data);
        uint8_t inTransfer(uint8_t addr, uint8_t ep, uint16_t *nbytesptr, uint8_t* data);
        uint8_t doSoftReset(uint8_t parent, uint8_t port, uint8_t address);
        uint8_t getone(UHS_EpInfo *pep, uint16_t *left, uint16_t *read, uint8_t *dataptr, uint8_t *offset);
        uint8_t eat(UHS_EpInfo *pep, uint16_t *left, uint16_t *read, uint8_t *dataptr, uint8_t *offset, uint16_t *eat);

};

// All device interface drivers use this subclass
class UHS_USBInterface {
public:

        UHS_USB_HOST_BASE *pUsb; // Parent USB host
        volatile uint8_t bNumEP; // total number of EP in this interface
        volatile UHS_EpInfo epInfo[16]; // This is a stub, override in the driver header.

        volatile uint8_t bAddress; // address of the device
        volatile uint8_t bConfNum; // configuration number
        volatile uint8_t bIface; // interface value
        volatile bool bPollEnable; // poll enable flag, operating status
        volatile uint32_t qNextPollTime; // next poll time

        /**
         * Resets interface driver to unused state
         */
        virtual void DriverDefaults(void) {
                pUsb->DeviceDefaults(bNumEP, this);
        };

        /**
         * Checks if this interface is supported.
         *
         * @param ei
         * @return true if the interface is supported
         */
        virtual bool OKtoEnumerate(ENUMERATION_INFO *ei) {
                return false;
        };

        /**
         * Configures any needed endpoint information for an interface.
         *
         * @param ei
         * @return zero on success
         */
        virtual uint8_t SetInterface(ENUMERATION_INFO *ei) {
                return UHS_HOST_ERROR_NOT_IMPLEMENTED;
        };

        /**
         * Interface specific additional setup and enumeration that
         * can't occur when the descriptor stream is open.
         * Also used for collection of unclaimed interfaces, to link to the master.
         *
         * @return zero on success
         */
        virtual uint8_t Finalize(void) {
                return 0;
        };

        /**
         *  Executed after interface is finalized but, before polling has started.
         *
         * @return 0 on success
         */
        virtual uint8_t OnStart(void) {
                return 0;
        };

        /**
         * Start interface polling
         * @return
         */
        virtual uint8_t Start(void) {
                uint8_t rcode = OnStart();
                if(!rcode) bPollEnable = true;
                return rcode;
        };

        /**
         * Executed before anything else in Release().
         *
         */
        virtual void OnRelease(void) {
                return;
        };

        /**
         * Release resources when device is disconnected
         */
        virtual void Release(void) {
                OnRelease();
                DriverDefaults();
                return;
        };

        /**
         * Executed when there is an important change detected during polling.
         * Examples:
         * Media status change for bulk, e.g. ready, not-ready, media changed, door opened.
         * Button state/joystick position/etc changes on a HID device.
         * Flow control status change on a communication device, e.g. CTS on serial
         */
        virtual void OnPoll(void) {
                return;
        };

        /**
         * Poll interface driver
         */
        virtual void Poll() {
                OnPoll();
        };

        /**
         * This is only for a hub.
         * @param port
         */
        virtual void ResetHubPort(uint8_t port) {
                return;
        };

        /**
         *
         * @return true if this interface is Vendor Specific.
         */
        virtual bool IsVSI() {
                return false;
        }
};

/**
 *
 * Vendor Specific interface class.
 * This is used by a partner interface.
 * It can also be used to force-enumerate an interface that
 * can use this interface directly.
 * You can also add an instance of this class within the interface constructor
 * if you expect the interface.
 *
 */

class UHS_VSI: public UHS_USBInterface {
public:
        volatile UHS_EpInfo epInfo[1];
        volatile ENUMERATION_INFO eInfo;
        UHS_VSI(UHS_USB_HOST_BASE *p);
        bool OKtoEnumerate(ENUMERATION_INFO *ei);
        uint8_t SetInterface(ENUMERATION_INFO *ei);
        virtual void DriverDefaults(void);
        virtual void Release(void);

        uint8_t GetAddress(void) {
                return bAddress;
        };

        virtual bool IsVSI() {
                return true;
        }

};

#endif //_USBHOST_H_
#endif
