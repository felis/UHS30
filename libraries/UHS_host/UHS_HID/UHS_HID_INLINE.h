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

#if defined(LOAD_UHS_HID) && defined(__UHS_HID_H__) && !defined(UHS_HID_LOADED)
#define UHS_HID_LOADED

#if DEBUG_PRINTF_EXTRA_HUGE
#if DEBUG_PRINTF_EXTRA_HUGE_USB_HID
#define HID_DEBUG(...) printf(__VA_ARGS__)
#else
#define HID_DEBUG(...) VOID0
#endif
#else
#define HID_DEBUG(...) VOID0
#endif

#ifndef AJK_NI
#define AJK_NI __attribute__((noinline))
#endif

UHS_NI UHS_HID::UHS_HID(UHS_USB_HOST_BASE *p, UHS_HID_PROCESSOR *hp) {
        hidProcessor = hp;
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
bool UHS_NI UHS_HID::OKtoEnumerate(ENUMERATION_INFO *ei) {
        return ((ei->klass == UHS_USB_CLASS_HID) || (ei->interface.klass == UHS_USB_CLASS_HID));
}

uint8_t UHS_NI UHS_HID::SetInterface(ENUMERATION_INFO *ei) {
        int index;
        bAddress = ei->address;
        bIface = ei->interface.bInterfaceNumber;
        bSubClass = ei->interface.subklass;
        bProtocol = ei->interface.protocol;
        for(uint8_t ep = 0; ep < ei->interface.numep; ep++) {
                if(ei->interface.epInfo[ep].bmAttributes == USB_TRANSFER_TYPE_INTERRUPT) {
                        if((ei->interface.epInfo[ep].bEndpointAddress & USB_TRANSFER_DIRECTION_IN) == USB_TRANSFER_DIRECTION_IN) {
                                pollRate = ei->interface.epInfo[ep].bInterval;
                        }
                        index = ((ei->interface.epInfo[ep].bEndpointAddress & USB_TRANSFER_DIRECTION_IN) == USB_TRANSFER_DIRECTION_IN) ? epInterruptInIndex : epInterruptOutIndex;
                        epInfo[index].epAddr = (ei->interface.epInfo[ep].bEndpointAddress & 0x0F);
                        epInfo[index].maxPktSize = ei->interface.epInfo[ep].wMaxPacketSize;
                        epInfo[index].epAttribs = 0;
                        epInfo[index].bmNakPower = UHS_USB_NAK_NOWAIT;
                        epInfo[index].bmSndToggle = 0;
                        epInfo[index].bmRcvToggle = 0;
                        epInfo[index].bIface=ei->interface.bInterfaceNumber;
                }
        }
        epInfo[0].epAddr = 0;
        epInfo[0].maxPktSize = ei->bMaxPacketSize0;
        epInfo[0].bmNakPower = UHS_USB_NAK_MAX_POWER;

        if(pollRate == 0) {
                pollRate = 100;
        }
        return 0;
}

void AJK_NI UHS_HID::OnRelease(void) {
        if(hidProcessor != NULL && hiddriver != NULL) {
                hiddriver->driverRelease();
        }
        return;
};

uint8_t AJK_NI UHS_HID::ReportDescr(uint16_t wIndex, uint16_t nbytes, uint8_t *buffer) {
        uint8_t rv;
        pUsb->DisablePoll();
        rv = pUsb->ctrlReq(bAddress, mkSETUP_PKT8(0x81U, USB_REQUEST_GET_DESCRIPTOR, 0x00U, 0x22U, wIndex, nbytes), nbytes, buffer);
        pUsb->EnablePoll();
        return rv;
}

uint8_t UHS_NI UHS_HID::SetIdle(uint8_t iface, uint8_t reportID, uint8_t duration) {
        uint8_t rv;
        pUsb->DisablePoll();
        rv = pUsb->ctrlReq(bAddress, mkSETUP_PKT8(0x21U, 0x0AU, reportID, duration, iface, 0x0000U), 0, NULL);
        pUsb->EnablePoll();
        return rv;
}

uint8_t UHS_NI UHS_HID::SetProtocol(uint8_t iface, uint8_t protocol) {
        uint8_t rv;
        pUsb->DisablePoll();
        rv = pUsb->ctrlReq(bAddress, mkSETUP_PKT8(0x21, 0x0B, protocol, 0x00, iface, 0x0000U), 0, NULL);
        pUsb->EnablePoll();
        return rv;
}

uint8_t UHS_NI UHS_HID::SetReport(uint8_t iface, uint8_t report_type, uint8_t report_id, uint16_t nbytes, uint8_t* dataptr) {
        uint8_t rv;
        pUsb->DisablePoll();
        rv = pUsb->ctrlReq(bAddress, mkSETUP_PKT8(0x21U, 0x09U, report_id, report_type, iface, nbytes), nbytes, dataptr);
        pUsb->EnablePoll();
        return rv;
}

uint8_t UHS_NI UHS_HID::Start(void) {
        HID_DEBUG("HID START, A %02x I %02x O %02x\r\n", bAddress, epInfo[epInterruptInIndex].epAddr, epInfo[epInterruptOutIndex].epAddr);
        uint8_t rcode = pUsb->setEpInfoEntry(bAddress, bIface, 3, epInfo);
        if(rcode) {
                Release();
                return rcode;
        }
        // Compare bSubClass operate the correct driver class
        // Default class is HID RAW
        hiddriver = new UHS_HID_RAW(this);
        switch(bSubClass) {
                case UHS_HID_BOOT_SUBCLASS:
#if defined(LOAD_UHS_HIDRAWBOOT_KEYBOARD)
                        if(bProtocol == UHS_HID_PROTOCOL_HIDBOOT_KEYBOARD) {
                                delete hiddriver;
                                hiddriver = new UHS_HIDBOOT_keyboard(this);
                        }
#endif
#if defined(LOAD_UHS_HIDRAWBOOT_MOUSE)
                        if(bProtocol == UHS_HID_PROTOCOL_HIDBOOT_MOUSE) {
                                delete hiddriver;
                                hiddriver = new UHS_HIDBOOT_mouse(this);
                        }
#endif
                        break;
                default:
                        break;
        }
        if(hiddriver != NULL) {
                hiddriver->driverStart();
                qNextPollTime = millis() + pollRate;
                bPollEnable = true;
                return 0;
        }
        Release();
        return UHS_HOST_ERROR_NOMEM;
}

void UHS_NI UHS_HID::Poll(void) {
        if((long)(millis() - qNextPollTime) >= 0L) {
                if(hiddriver != NULL) {
                        hiddriver->driverPoll();
                }
                qNextPollTime = millis() + pollRate;
        }
}

void UHS_NI UHS_HID::DriverDefaults(void) {
        pUsb->DeviceDefaults(3, this);
        if(hiddriver != NULL) delete hiddriver;
        hiddriver = NULL;
        for(uint8_t i = 0; i < 3; i++) {
                epInfo[i].epAddr = 0;
                epInfo[i].maxPktSize = (i) ? 0 : 8;
                epInfo[i].epAttribs = 0;
                epInfo[i].bmNakPower = (i == epInterruptInIndex) ? UHS_USB_NAK_NOWAIT : UHS_USB_NAK_MAX_POWER;
        }

}

#else
#error "Never include USB_HID_INLINE.h, include USB_HID.h instead"
#endif
