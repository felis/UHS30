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
#if !defined(__UHS_CDC_ACM_H__)
#define __UHS_CDC_ACM_H__

#define ACM_MAX_ENDPOINTS 4

// Exception PIDS
#define UHS_CDC_PROLIFIC_PID_1 0x0609U
#define UHS_CDC_PROLIFIC_PID_2 0x2303U

// Types of adaptor
#define UHS_USB_ACM_PLAIN 0
#define TEST_ACM_PLAIN() (((ei->interface.klass == UHS_USB_CLASS_COM_AND_CDC_CTRL) && (ei->interface.subklass == UHS_CDC_SUBCLASS_ACM) && (ei->interface.protocol == UHS_CDC_PROTOCOL_ITU_T_V_250)) || (ei->interface.klass == UHS_USB_CLASS_CDC_DATA))
#define UHS_USB_ACM_XR21B1411 1
#define TEST_XR21B1411() (((ei->vid == 0x2890U) && (ei->pid == 0x0201U)) || ((ei->vid == UHS_VID_EXAR) && (ei->pid == 0x1411U)))

#define UHS_USB_ACM_FTDI 2
#define TEST_ACM_FTDI() (ei->vid == UHS_VID_FUTURE_TECHNOLOGY_DEVICES_INTERNATIONAL)

#define UHS_USB_ACM_PROLIFIC 3
#define TEST_ACM_PROLIFIC() (ei->vid == UHS_VID_PROLIFIC_TECHNOLOGY && (ei->pid == UHS_CDC_PROLIFIC_PID_1 || ei->pid == UHS_CDC_PROLIFIC_PID_2))

#if defined(LOAD_UHS_CDC_ACM_XR21B1411)
#endif
#if defined(LOAD_UHS_CDC_ACM_FTDI)
#endif
#if defined(LOAD_UHS_CDC_ACM_PROLIFIC)
#endif

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
                        bool half_duplex : 1; // Has half-duplex capability.
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
        uint8_t adaptor;

        uint16_t ChipType; // Type of chip

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
                pUsb->DisablePoll();
                bool rv = ready;
                pUsb->EnablePoll();
                return rv;
        };

        virtual tty_features enhanced_status(void) {
                return _enhanced_status;
        };

#if defined(LOAD_UHS_CDC_ACM_XR21B1411)

        uint8_t XR_read_register(uint16_t reg, uint16_t *val) {
                                pUsb->DisablePoll();
                uint8_t rv= (pUsb->ctrlReq(bAddress, mkSETUP_PKT16(XR_READ_REQUEST_TYPE, 1, 0x0000U, reg, 2), 2, (uint8_t *)val));
                pUsb->EnablePoll();
                return rv;
        }

        uint8_t XR_write_register(uint16_t reg, uint16_t val) {
                                pUsb->DisablePoll();
                uint8_t rv= (pUsb->ctrlReq(bAddress, mkSETUP_PKT16(XR_WRITE_REQUEST_TYPE, 0, val, reg, 0), 0, NULL));
                pUsb->EnablePoll();
                return rv;
        }
#endif

        virtual tty_features enhanced_features(void) {
                tty_features rv;
#if defined(LOAD_UHS_CDC_ACM_XR21B1411)
                if(adaptor == UHS_USB_ACM_XR21B1411) {
                        rv.enhanced = true;
                        rv.autoflow_RTS = true;
                        rv.autoflow_DSR = true;
                        rv.autoflow_XON = true;
                        rv.half_duplex = true;
                        rv.wide = true;
                } else
#endif
                {
                        rv.enhanced = false;
                        rv.autoflow_RTS = false;
                        rv.autoflow_DSR = false;
                        rv.autoflow_XON = false;
                        rv.half_duplex = false;
                        rv.wide = false;
                }
                return rv;
        };

        virtual void autoflowRTS(NOTUSED(bool s)) {
#if defined(LOAD_UHS_CDC_ACM_XR21B1411)
                if(adaptor == UHS_USB_ACM_XR21B1411) {
                        uint16_t val;
                        uint8_t rval;
                        rval = XR_read_register(XR_REG_ACM_FLOW_CTL, &val);
                        if(!rval) {
                                if(s) {
                                        val &= XR_REG_FLOW_CTL_HALF_DPLX;
                                        val |= XR_REG_FLOW_CTL_HW;
                                } else {
                                        val &= XR_REG_FLOW_CTL_HALF_DPLX;
                                }
                                rval = XR_write_register(XR_REG_ACM_FLOW_CTL, val);
                                if(!rval) {
                                        rval = XR_write_register(XR_REG_ACM_GPIO_MODE, XR_REG_GPIO_MODE_GPIO);
                                        if(!rval) {
                                                // ACM commands apply the new settings.
                                                UHS_CDC_LINE_CODING LCT;
                                                rval = GetLineCoding(&LCT);
                                                if(!rval) {
                                                        rval = SetLineCoding(&LCT);
                                                        if(!rval) {
                                                                _enhanced_status.autoflow_XON = false;
                                                                _enhanced_status.autoflow_DSR = false;
                                                                _enhanced_status.autoflow_RTS = s;
                                                        }
                                                }
                                        }
                                }
                        }
                }
#endif
        };

        virtual void autoflowDSR(NOTUSED(bool s)) {
#if defined(LOAD_UHS_CDC_ACM_XR21B1411)
                if(adaptor == UHS_USB_ACM_XR21B1411) {
                        uint16_t val;
                        uint8_t rval;
                        rval = XR_read_register(XR_REG_ACM_FLOW_CTL, &val);
                        if(!rval) {
                                if(s) {
                                        val &= XR_REG_FLOW_CTL_HALF_DPLX;
                                        val |= XR_REG_FLOW_CTL_HW;
                                } else {
                                        val &= XR_REG_FLOW_CTL_HALF_DPLX;
                                }
                                rval = XR_write_register(XR_REG_ACM_FLOW_CTL, val);
                                if(!rval) {
                                        if(s) {
                                                rval = XR_write_register(XR_REG_ACM_GPIO_MODE, XR_REG_GPIO_MODE_FC_DTRDSR);
                                        } else {
                                                rval = XR_write_register(XR_REG_ACM_GPIO_MODE, XR_REG_GPIO_MODE_GPIO);
                                        }
                                        if(!rval) {
                                                // ACM commands apply the new settings.
                                                UHS_CDC_LINE_CODING LCT;
                                                rval = GetLineCoding(&LCT);
                                                if(!rval) {
                                                        rval = SetLineCoding(&LCT);
                                                        if(!rval) {
                                                                _enhanced_status.autoflow_XON = false;
                                                                _enhanced_status.autoflow_RTS = false;
                                                                _enhanced_status.autoflow_DSR = s;
                                                        }
                                                }
                                        }
                                }
                        }
                }
#endif
        };

        virtual void autoflowXON(NOTUSED(bool s)) {
#if defined(LOAD_UHS_CDC_ACM_XR21B1411)
                if(adaptor == UHS_USB_ACM_XR21B1411) {
                        // NOTE: hardware defaults to the normal XON/XOFF
                        uint16_t val;
                        uint8_t rval;
                        rval = XR_read_register(XR_REG_ACM_FLOW_CTL, &val);
                        if(!rval) {
                                if(s) {
                                        val &= XR_REG_FLOW_CTL_HALF_DPLX;
                                        val |= XR_REG_FLOW_CTL_SW;
                                } else {
                                        val &= XR_REG_FLOW_CTL_HALF_DPLX;
                                }
                                rval = XR_write_register(XR_REG_ACM_FLOW_CTL, val);
                                if(!rval) {
                                        rval = XR_write_register(XR_REG_ACM_GPIO_MODE, XR_REG_GPIO_MODE_GPIO);
                                        if(!rval) {
                                                // ACM commands apply the new settings.
                                                UHS_CDC_LINE_CODING LCT;
                                                rval = GetLineCoding(&LCT);
                                                if(!rval) {
                                                        rval = SetLineCoding(&LCT);
                                                        if(!rval) {
                                                                _enhanced_status.autoflow_RTS = false;
                                                                _enhanced_status.autoflow_DSR = false;
                                                                _enhanced_status.autoflow_XON = s;
                                                        }
                                                }
                                        }
                                }
                        }
                }
#endif
        };

        virtual void half_duplex(NOTUSED(bool s)) {
#if defined(LOAD_UHS_CDC_ACM_XR21B1411)
                if(adaptor == UHS_USB_ACM_XR21B1411) {
                        uint16_t val;
                        uint8_t rval;
                        rval = XR_read_register(XR_REG_ACM_FLOW_CTL, &val);
                        if(!rval) {
                                if(s) {
                                        val |= XR_REG_FLOW_CTL_HALF_DPLX;
                                } else {
                                        val &= XR_REG_FLOW_CTL_MODE_MASK;
                                }
                                rval = XR_write_register(XR_REG_ACM_FLOW_CTL, val);
                                if(!rval) {
                                        // ACM commands apply the new settings.
                                        UHS_CDC_LINE_CODING LCT;
                                        rval = GetLineCoding(&LCT);
                                        if(!rval) {
                                                rval = SetLineCoding(&LCT);
                                                if(!rval) {
                                                        _enhanced_status.half_duplex = s;
                                                }
                                        }
                                }
                        }
                }
#endif
        };

        virtual void wide(NOTUSED(bool s)) {
#if defined(LOAD_UHS_CDC_ACM_XR21B1411)
                if(adaptor == UHS_USB_ACM_XR21B1411) {
                }
#endif
        };

        // UsbConfigXtracter implementation
        void EndpointXtract(uint8_t conf, uint8_t iface, uint8_t alt, uint8_t proto, const USB_ENDPOINT_DESCRIPTOR *ep);
};
#if defined(LOAD_UHS_CDC_ACM) && !defined(UHS_CDC_ACM_LOADED)
#include "UHS_CDC_ACM_INLINE.h"
#endif
#endif // __UHS_CDC_ACM_H__
