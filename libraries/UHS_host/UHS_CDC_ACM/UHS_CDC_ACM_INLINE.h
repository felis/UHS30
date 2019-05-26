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

#if defined(LOAD_UHS_CDC_ACM) && defined(__UHS_CDC_ACM_H__) && !defined(UHS_CDC_ACM_LOADED)
#define UHS_CDC_ACM_LOADED

UHS_NI UHS_CDC_ACM::UHS_CDC_ACM(UHS_USB_HOST_BASE *p) {
        pUsb = p;
        if(pUsb) {
                DriverDefaults();
                pUsb->RegisterDeviceClass(this);
        }
}

/**
 *
 * @param ei Enumeration information
 * @return true if this interface driver can handle this interface description
 */

bool UHS_NI UHS_CDC_ACM::OKtoEnumerate(ENUMERATION_INFO *ei) {
        ACM_HOST_DEBUG("ACM: checking numep %i, klass %2.2x, subklass %2.2x\r\n", ei->interface.numep, ei->klass, ei->subklass);
        ACM_HOST_DEBUG("ACM: checking protocol %2.2x, interface.klass %2.2x, interface.subklass %2.2x\r\n", ei->protocol, ei->interface.klass, ei->interface.subklass);
        ACM_HOST_DEBUG("ACM: checking interface.protocol %2.2x\r\n", ei->interface.protocol);

        return (TEST_XR21B1411()||TEST_ACM_FTDI()||TEST_ACM_PROLIFIC()||TEST_ACM_PLAIN());
}

/**
 * Resets interface driver to unused state
 */
void UHS_NI UHS_CDC_ACM::DriverDefaults(void) {
        pUsb->DeviceDefaults(ACM_MAX_ENDPOINTS, this);
        ready = false;
        SbAddress = 0;
        MbAddress = 0;
        qPollRate = 0;
        adaptor = UHS_USB_ACM_PLAIN;
        _enhanced_status.enhanced = false;
        _enhanced_status.autoflow_RTS = false;
        _enhanced_status.autoflow_DSR = false;
        _enhanced_status.autoflow_XON = false;
        _enhanced_status.half_duplex = false;
        _enhanced_status.wide = false;

        for(uint8_t i = 0; i < ACM_MAX_ENDPOINTS; i++) {
                epInfo[i].epAddr = 0;
                epInfo[i].maxPktSize = (i) ? 0 : 8;
                epInfo[i].epAttribs = 0;
                epInfo[i].bmNakPower = (i == epDataInIndex) ? UHS_USB_NAK_NOWAIT : UHS_USB_NAK_MAX_POWER;
        }
}

/**
 * WARNING:
 *          Assumes UHS_USB_CLASS_COM_AND_CDC_CTRL and
 *          Assumes UHS_USB_CLASS_CDC_DATA are IN THE SAME ORDER
 *          CDC descriptors are NOT YET PARSED.
 *          For MOST cases it will work.
 *          If you know of a device that DOES NOT have them in the same order,
 *          PLEASE post a report and donate this device to us.
 *
 * @param ei Enumeration information
 * @return 0 always
 */
uint8_t UHS_NI UHS_CDC_ACM::SetInterface(ENUMERATION_INFO *ei) {
        uint8_t index;
        if(ei->interface.klass == UHS_USB_CLASS_CDC_DATA) {
                // data slave
                SbAddress = ei->address;
                // Will do this later if stuff breaks.
                // This would mean a separate Interface Driver, of course...
                //bDataIface = ei->interface.bInterfaceNumber;
        } else {
                // master
                MbAddress = ei->address;
                bControlIface = ei->interface.bInterfaceNumber;
        }
        ACM_HOST_DEBUG("ACM: SLAVE %i\r\n", SbAddress);
        ACM_HOST_DEBUG("ACM: MASTER %i\r\n", MbAddress);

        // Fill in the endpoint info structure
        for(uint8_t ep = 0; ep < ei->interface.numep; ep++) {
                if((ei->interface.epInfo[ep].bmAttributes == USB_TRANSFER_TYPE_BULK) && (ei->interface.klass == UHS_USB_CLASS_CDC_DATA)) {
                        index = ((ei->interface.epInfo[ep].bEndpointAddress & USB_TRANSFER_DIRECTION_IN) == USB_TRANSFER_DIRECTION_IN) ? epDataInIndex : epDataOutIndex;
                        epInfo[index].epAddr = (ei->interface.epInfo[ep].bEndpointAddress & 0x0F);
                        epInfo[index].maxPktSize = ei->interface.epInfo[ep].wMaxPacketSize;
                        epInfo[index].epAttribs = 0;
                        epInfo[index].bmNakPower = (index == epDataInIndex) ? UHS_USB_NAK_NOWAIT : UHS_USB_NAK_MAX_POWER;
                        epInfo[index].bmSndToggle = 0;
                        epInfo[index].bmRcvToggle = 0;
                        epInfo[index].bIface=ei->interface.bInterfaceNumber;
                        bNumEP++;
                } else if((ei->interface.epInfo[ep].bmAttributes == USB_TRANSFER_TYPE_INTERRUPT) && ((ei->interface.epInfo[ep].bEndpointAddress & USB_TRANSFER_DIRECTION_IN) == USB_TRANSFER_DIRECTION_IN)) {
                        index = epInterruptInIndex;
                        epInfo[index].epAddr = (ei->interface.epInfo[ep].bEndpointAddress & 0x0F);
                        epInfo[index].maxPktSize = ei->interface.epInfo[ep].wMaxPacketSize;
                        epInfo[index].epAttribs = 0;
                        epInfo[index].bmNakPower = UHS_USB_NAK_MAX_POWER;
                        epInfo[index].bmSndToggle = 0;
                        epInfo[index].bmRcvToggle = 0;
                        epInfo[index].bIface=ei->interface.bInterfaceNumber;
                        bNumEP++;
                        if(ei->interface.epInfo[ep].bInterval > qPollRate) qPollRate = ei->interface.epInfo[ep].bInterval;
                }
#if defined(LOAD_UHS_CDC_ACM_FTDI)
                else if((ei->interface.epInfo[ep].bmAttributes == USB_TRANSFER_TYPE_BULK) && TEST_ACM_FTDI()) {
                        index = ((ei->interface.epInfo[ep].bEndpointAddress & USB_TRANSFER_DIRECTION_IN) == USB_TRANSFER_DIRECTION_IN) ? epDataInIndex : epDataOutIndex;
                        epInfo[index].epAddr = (ei->interface.epInfo[ep].bEndpointAddress & 0x0F);
                        epInfo[index].maxPktSize = ei->interface.epInfo[ep].wMaxPacketSize;
                        epInfo[index].epAttribs = 0;
                        epInfo[index].bmNakPower = (index == epDataInIndex) ? UHS_USB_NAK_NOWAIT : UHS_USB_NAK_MAX_POWER;
                        epInfo[index].bmSndToggle = 0;
                        epInfo[index].bmRcvToggle = 0;
                        epInfo[index].bIface=ei->interface.bInterfaceNumber;
                        bNumEP++;
                }
#endif
        }
#if defined(LOAD_UHS_CDC_ACM_FTDI)
        if(TEST_ACM_FTDI() && !SbAddress) {
                ACM_HOST_DEBUG("ACM: FTDI chipset, emulating CDC-ACM.\r\n");
                SbAddress = MbAddress;
                st_modem = 0;
                st_line = 0;
        }
#endif

        if(SbAddress && (SbAddress == MbAddress)) {
                ACM_HOST_DEBUG("ACM: SLAVE and MASTER match!\r\n");
                ChipType = ei->bcdDevice;
                if(TEST_XR21B1411()) {
                        adaptor = UHS_USB_ACM_XR21B1411;
                } else if(TEST_ACM_PROLIFIC()) {
                        adaptor = UHS_USB_ACM_PROLIFIC;
#if defined(LOAD_UHS_CDC_ACM_FTDI)
                } else if(TEST_ACM_FTDI()) {
                        adaptor = UHS_USB_ACM_FTDI;
                        if(ChipType == FT232AM) {
                                // Read quirk. Are there others?
                                epInfo[epDataInIndex].bmRcvToggle = 1;
                        }
#endif
                } else {
                        adaptor = UHS_USB_ACM_PLAIN;
                }
                bAddress = SbAddress;
                epInfo[0].epAddr = 0;
                epInfo[0].maxPktSize = ei->bMaxPacketSize0;
                epInfo[0].bmNakPower = UHS_USB_NAK_MAX_POWER;
                bIface = ei->interface.bInterfaceNumber;
                // Both interfaces have finally been set and match
        } else {
                ACM_HOST_DEBUG("ACM: No match on SLAVE and MASTER\r\n");
        }
        return 0;
};

/**
 *
 * @return 0 for success
 */
uint8_t UHS_NI UHS_CDC_ACM::Start(void) {
        uint8_t rcode;
        ACM_HOST_DEBUG("ACM: Start\r\n");

        rcode = pUsb->setEpInfoEntry(bAddress, bIface, bNumEP, epInfo);
        if(!rcode) {
                ACM_HOST_DEBUG("ACM: EpInfoEntry OK\r\n");
                if(!rcode) {
                        half_duplex(false);
                        autoflowRTS(false);
                        autoflowDSR(false);
                        autoflowXON(false);
                        wide(false);
                        rcode = OnStart();
                        if(!rcode) {
                                ACM_HOST_DEBUG("ACM: OnStart OK\r\n");
                                qNextPollTime = millis() + qPollRate;
                                bPollEnable = true;
                                ready = true;
                        } else {
                                ACM_HOST_DEBUG("ACM: OnStart FAIL\r\n");
                        }
                } else {
                        ACM_HOST_DEBUG("ACM: EpInfoEntry FAIL\r\n");
                }
        }
        if(rcode) Release();
        return rcode;
}

void UHS_NI UHS_CDC_ACM::Poll(void) {
        if((long)(millis() - qNextPollTime) >= 0L) {
                OnPoll(); // Do user stuff...
                qNextPollTime = millis() + qPollRate;
        }
}

/**
 *
 * @param ep_ptr
 */
void UHS_NI UHS_CDC_ACM::PrintEndpointDescriptor(const USB_ENDPOINT_DESCRIPTOR * ep_ptr) {
        Notify(PSTR("Endpoint descriptor:"), 0x80);
        Notify(PSTR("\r\nLength:\t\t"), 0x80);
        D_PrintHex<uint8_t > (ep_ptr->bLength, 0x80);
        Notify(PSTR("\r\nType:\t\t"), 0x80);
        D_PrintHex<uint8_t > (ep_ptr->bDescriptorType, 0x80);
        Notify(PSTR("\r\nAddress:\t"), 0x80);
        D_PrintHex<uint8_t > (ep_ptr->bEndpointAddress, 0x80);
        Notify(PSTR("\r\nAttributes:\t"), 0x80);
        D_PrintHex<uint8_t > (ep_ptr->bmAttributes, 0x80);
        Notify(PSTR("\r\nMaxPktSize:\t"), 0x80);
        D_PrintHex<uint16_t > (ep_ptr->wMaxPacketSize, 0x80);
        Notify(PSTR("\r\nPoll Intrv:\t"), 0x80);
        D_PrintHex<uint8_t > (ep_ptr->bInterval, 0x80);
        Notify(PSTR("\r\n"), 0x80);
}

uint8_t UHS_NI UHS_CDC_ACM::Read(uint16_t *bytes_rcvd, uint8_t * dataptr) {
        if(!bAddress) return UHS_HOST_ERROR_UNPLUGGED;
        pUsb->DisablePoll();
        uint8_t rv = pUsb->inTransfer(bAddress, epInfo[epDataInIndex].epAddr, bytes_rcvd, dataptr);
#if defined(LOAD_UHS_CDC_ACM_FTDI)
        if(adaptor == UHS_USB_ACM_FTDI) {
                // FTDI stuffs status in the first 2 bytes.
                if(!rv) {
                        if(*bytes_rcvd > 1) {
                                // save modem and line status
                                st_modem = dataptr[0];
                                st_line = dataptr[1];
                                *bytes_rcvd -= 2;
                                if(*bytes_rcvd) {
                                        ACM_HOST_DEBUG("UHS_CDC_ACM::Read FTDI moving %x bytes\r\n", *bytes_rcvd);
                                        memmove(dataptr, (dataptr + 2), *bytes_rcvd);
                                }
                        } else {
                                // bug??
                                ACM_HOST_DEBUG("UHS_CDC_ACM::Read FTDI < 2 bytes, bug?: %x\r\n", *bytes_rcvd);
                                *bytes_rcvd = 0;
                        }
                }
        }
#endif
        if(rv && rv != UHS_HOST_ERROR_NAK) Release();
        pUsb->EnablePoll();
        return rv;
}

uint8_t UHS_NI UHS_CDC_ACM::Write(uint16_t nbytes, uint8_t * dataptr) {
        if(!bAddress) return UHS_HOST_ERROR_UNPLUGGED;
        pUsb->DisablePoll();
        uint8_t rv = pUsb->outTransfer(bAddress, epInfo[epDataOutIndex].epAddr, nbytes, dataptr);
        if(rv && rv != UHS_HOST_ERROR_NAK) Release();
        pUsb->EnablePoll();
        return rv;
}

uint8_t UHS_NI UHS_CDC_ACM::SetCommFeature(uint16_t fid, uint8_t nbytes, uint8_t * dataptr) {
        uint8_t rv = 0;
        if(!bAddress) return UHS_HOST_ERROR_UNPLUGGED;
        pUsb->DisablePoll();
#if defined(LOAD_UHS_CDC_ACM_FTDI)
        if(adaptor == UHS_USB_ACM_FTDI) {
                // TO-DO: Not sure what to do here yet...
        } else

#endif
                rv = pUsb->ctrlReq(bAddress, mkSETUP_PKT16(bmREQ_CDCOUT, UHS_CDC_SET_COMM_FEATURE, fid, bControlIface, nbytes), nbytes, dataptr);
        if(rv && rv != UHS_HOST_ERROR_NAK) Release();
        pUsb->EnablePoll();
        return rv;
}

uint8_t UHS_NI UHS_CDC_ACM::GetCommFeature(uint16_t fid, uint8_t nbytes, uint8_t * dataptr) {
        uint8_t rv = 0;
        if(!bAddress) return UHS_HOST_ERROR_UNPLUGGED;
        pUsb->DisablePoll();
#if defined(LOAD_UHS_CDC_ACM_FTDI)
        if(adaptor == UHS_USB_ACM_FTDI) {
                // TO-DO: Not sure what to do here yet...
        } else

#endif
                rv = pUsb->ctrlReq(bAddress, mkSETUP_PKT16(bmREQ_CDCIN, UHS_CDC_GET_COMM_FEATURE, fid, bControlIface, nbytes), nbytes, dataptr);
        if(rv && rv != UHS_HOST_ERROR_NAK) Release();
        pUsb->EnablePoll();
        return rv;
}

uint8_t UHS_NI UHS_CDC_ACM::ClearCommFeature(uint16_t fid) {
        uint8_t rv = 0;
        if(!bAddress) return UHS_HOST_ERROR_UNPLUGGED;
        pUsb->DisablePoll();
#if defined(LOAD_UHS_CDC_ACM_FTDI)
        if(adaptor == UHS_USB_ACM_FTDI) {
        } else
#endif
                rv = pUsb->ctrlReq(bAddress, mkSETUP_PKT16(bmREQ_CDCOUT, UHS_CDC_CLEAR_COMM_FEATURE, fid, bControlIface, 0), 0, NULL);
        if(rv && rv != UHS_HOST_ERROR_NAK) Release();
        pUsb->EnablePoll();
        return rv;
}

uint8_t UHS_NI UHS_CDC_ACM::SetLineCoding(const UHS_CDC_LINE_CODING * dataptr) {
        uint8_t rv;
        if(!bAddress) return UHS_HOST_ERROR_UNPLUGGED;
        pUsb->DisablePoll();
#if defined(LOAD_UHS_CDC_ACM_FTDI)
        if(adaptor == UHS_USB_ACM_FTDI) {
                rv = UHS_FTDI_SetLineCoding(dataptr);
        } else
#endif
                rv = pUsb->ctrlReq(bAddress, mkSETUP_PKT16(bmREQ_CDCOUT, UHS_CDC_SET_LINE_CODING, 0x0000U, bControlIface, sizeof (UHS_CDC_LINE_CODING)), sizeof (UHS_CDC_LINE_CODING), (uint8_t*)dataptr);
        if(rv && rv != UHS_HOST_ERROR_NAK) Release();
        pUsb->EnablePoll();
        //printf("\r\n%u\r\n", rv);
        return rv;
}

uint8_t UHS_NI UHS_CDC_ACM::GetLineCoding(UHS_CDC_LINE_CODING * dataptr) {
        uint8_t rv = 0;
        if(!bAddress) return UHS_HOST_ERROR_UNPLUGGED;
        pUsb->DisablePoll();
#if defined(LOAD_UHS_CDC_ACM_FTDI)
        if(adaptor == UHS_USB_ACM_FTDI) {
                // not implemented on chip?
        } else
#endif
                rv = pUsb->ctrlReq(bAddress, mkSETUP_PKT16(bmREQ_CDCIN, UHS_CDC_GET_LINE_CODING, 0x0000U, bControlIface, sizeof (UHS_CDC_LINE_CODING)), sizeof (UHS_CDC_LINE_CODING), (uint8_t*)dataptr);
        if(rv && rv != UHS_HOST_ERROR_NAK) Release();
        pUsb->EnablePoll();
        return rv;
}

uint8_t UHS_NI UHS_CDC_ACM::SetControlLineState(uint8_t state) {
        uint8_t rv;
        if(!bAddress) return UHS_HOST_ERROR_UNPLUGGED;
        pUsb->DisablePoll();
#if defined(LOAD_UHS_CDC_ACM_FTDI)
        if(adaptor == UHS_USB_ACM_FTDI) {
                rv = UHS_FTDI_SetControlLineState(state);
        } else
#endif
                rv = pUsb->ctrlReq(bAddress, mkSETUP_PKT8(bmREQ_CDCOUT, UHS_CDC_SET_CONTROL_LINE_STATE, state, 0, bControlIface, 0), 0, NULL);
        if(rv && rv != UHS_HOST_ERROR_NAK) Release();
        pUsb->EnablePoll();
        return rv;
}

uint8_t UHS_NI UHS_CDC_ACM::SendBreak(uint16_t duration) {
        uint8_t rv = 0;
        if(!bAddress) return UHS_HOST_ERROR_UNPLUGGED;
        pUsb->DisablePoll();
#if defined(LOAD_UHS_CDC_ACM_FTDI)
        if(adaptor == UHS_USB_ACM_FTDI) {
                // TO-DO
        } else
#endif
                rv = pUsb->ctrlReq(bAddress, mkSETUP_PKT16(bmREQ_CDCOUT, UHS_CDC_SEND_BREAK, duration, bControlIface, 0), 0, NULL);
        if(rv && rv != UHS_HOST_ERROR_NAK) Release();
        pUsb->EnablePoll();
        return rv;
}
#else
#error "Never include UHS_CDC_ACM_INLINE.h, include UHS_host.h instead"
#endif
