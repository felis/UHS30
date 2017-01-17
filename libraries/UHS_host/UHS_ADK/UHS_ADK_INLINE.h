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

/* Google ADK interface */

#if defined(LOAD_UHS_ADK) && defined(__UHS_ADK_H__) && !defined(UHS_ADK_LOADED)
#define UHS_CDC_ACM_LOADED

/**
 * Check if the device might support accessory mode
 */
bool UHS_NI UHS_ADK_Enabler::OKtoEnumerate(ENUMERATION_INFO *ei) {
        ADK_HOST_DEBUG("ADK Enabler: checking vid %4.4x, pid %4.4x, iface class %i, subclass %i\r\n",
                ei->vid, ei->pid, ei->interface.klass, ei->interface.subklass);
        return ((   ei->vid == UHS_VID_GOOGLE
                 || ei->vid == UHS_VID_HTC_HIGH_TECH_COMPUTER
                 || ei->vid == UHS_VID_MOTOROLA_PCS
                 || ei->vid == UHS_VID_SAMSUNG_ELECTRONICS
                 || ei->vid == UHS_VID_SONY_ERICSSON_MOBILE_COMMUNICATIONS
                 || ei->vid == UHS_VID_QUALCOMM
                 || ei->vid == UHS_VID_COMNEON
                 || ei->vid == UHS_VID_ACTION_STAR_ENTERPRISE)
                && ei->pid != UHS_PID_GOOGLE_ADK && ei->pid != UHS_PID_GOOGLE_ADB);
}

uint8_t UHS_NI UHS_ADK_Enabler::SetInterface(ENUMERATION_INFO *ei) {
        ADK_HOST_DEBUG("ADK Enabler: SetInterface\r\n");
        bAddress = ei->address;
        epInfo[0].epAddr = 0;
        epInfo[0].maxPktSize = ei->bMaxPacketSize0;
        epInfo[0].bmNakPower = UHS_USB_NAK_MAX_POWER;
        bIface = ei->interface.bInterfaceNumber;
        bConfNum = ei->currentconfig;
        ADK_HOST_DEBUG("ADK Enabler: address %i, config %i, iface %i with %i endpoints\r\n", bAddress, bConfNum, bIface, bNumEP);
        return 0;
}

/**
 * Try to switch the device into accessory mode
 */
uint8_t UHS_NI UHS_ADK_Enabler::OnStart(void) {
        ADK_HOST_DEBUG("ADK Enabler: Start\r\n");
        uint8_t rcode;

        ADK_HOST_DEBUG("ADK Enabler: probe device - get accessory protocol revision\r\n");
        uint16_t adkproto = -1;
        rcode = getProto((uint8_t*) & adkproto);
        if(rcode) {
                ADK_HOST_DEBUG("ADK: getProto: ");
                goto Fail;
        }
        ADK_HOST_DEBUG("ADK Enabler: protocol rev. %i\r\n", adkproto);

        delay(100);

        ADK_HOST_DEBUG("ADK Enabler: send strings\r\n");
        sendStr(UHS_ADK_ID_MANUFACTURER, UHS_ADK_MANUFACTURER);
        delay(10);
        sendStr(UHS_ADK_ID_MODEL, UHS_ADK_MODEL);
        delay(10);
        sendStr(UHS_ADK_ID_DESCRIPTION, UHS_ADK_DESCRIPTION);
        delay(10);
        sendStr(UHS_ADK_ID_VERSION, UHS_ADK_VERSION);
        delay(10);
        sendStr(UHS_ADK_ID_URI, UHS_ADK_URI);
        delay(10);
        sendStr(UHS_ADK_ID_SERIAL, UHS_ADK_SERIAL);

        delay(100);

        // switch to accessory mode
        // the Android phone will reset
        ADK_HOST_DEBUG("ADK Enabler: switch to accessory mode\r\n");
        rcode = switchAcc();
        if(rcode) {
                ADK_HOST_DEBUG("ADK Enabler: swAcc: ");
                goto Fail;
        }

        rcode = UHS_USB_HOST_STATE_RESET_NOT_COMPLETE;

Fail:
        pUsb->ReleaseDevice(bAddress);
        return rcode;
}

/**
 * Get ADK protocol version
 * @return 16bit little endian ADK protocol version
 */
uint8_t UHS_NI UHS_ADK_Enabler::getProto(uint8_t *adkproto) {
        return pUsb->ctrlReq(bAddress, mkSETUP_PKT16(UHS_ADK_bmREQ_GET, UHS_ADK_GETPROTO, 0, 0, 2), 2, adkproto);
}

/**
 * Send ADK string
 */
uint8_t UHS_NI UHS_ADK_Enabler::sendStr(uint8_t index, const char *str) {
        return pUsb->ctrlReq(bAddress, mkSETUP_PKT16(UHS_ADK_bmREQ_SEND, UHS_ADK_SENDSTR, 0, index, strlen(str) + 1), strlen(str) + 1, (uint8_t*)str);
}

/**
 * Switch to accessory mode
 */
uint8_t UHS_NI UHS_ADK_Enabler::switchAcc(void) {
        return pUsb->ctrlReq(bAddress, mkSETUP_PKT16(UHS_ADK_bmREQ_SEND, UHS_ADK_ACCSTART, 0, 0, 0), 0, NULL);
}


UHS_NI UHS_ADK::UHS_ADK(UHS_USB_HOST_BASE *p) : ready(false) {
        pUsb = p; //pointer to USB class instance - mandatory
        enabler.pUsb = p;
        // register in USB subsystem
        if(pUsb) {
                DriverDefaults();
                pUsb->RegisterDeviceClass(this);
                pUsb->RegisterDeviceClass(&enabler);
        }
}

/**
 * Check if device is supported by this interface driver
 * @param ei Enumeration information
 * @return true if this interface driver can handle this interface description
 */
bool UHS_NI UHS_ADK::OKtoEnumerate(ENUMERATION_INFO *ei) {
        ADK_HOST_DEBUG("ADK: checking vid %4.4x, pid %4.4x, iface class %i, subclass %i\r\n",
                ei->vid, ei->pid, ei->interface.klass, ei->interface.subklass);
        return (ei->vid == UHS_VID_GOOGLE && (ei->pid == UHS_PID_GOOGLE_ADK || ei->pid == UHS_PID_GOOGLE_ADB));
}

/**
 * Resets interface driver to unused state
 */
void UHS_NI UHS_ADK::DriverDefaults(void) {
        pUsb->DeviceDefaults(ADK_MAX_ENDPOINTS, this);
        ready = false;

        // initialize endpoint data structures
        for(uint8_t i = 0; i < ADK_MAX_ENDPOINTS; i++) {
                epInfo[i].bmSndToggle = 0;
                epInfo[i].bmRcvToggle = 0;
                epInfo[i].bmNakPower = (i) ? UHS_USB_NAK_NOWAIT : UHS_USB_NAK_MAX_POWER;
        }
}

uint8_t UHS_NI UHS_ADK::SetInterface(ENUMERATION_INFO *ei) {
        ADK_HOST_DEBUG("ADK: SetInterface\r\n");
        for(uint8_t ep = 0; ep < ei->interface.numep; ep++) {
                if(ei->interface.epInfo[ep].bmAttributes == USB_TRANSFER_TYPE_BULK) {
                        uint8_t index = ((ei->interface.epInfo[ep].bEndpointAddress & USB_TRANSFER_DIRECTION_IN) == USB_TRANSFER_DIRECTION_IN) ? epDataInIndex : epDataOutIndex;
                        epInfo[index].epAddr = (ei->interface.epInfo[ep].bEndpointAddress & 0x0F);
                        epInfo[index].maxPktSize = (uint8_t)(ei->interface.epInfo[ep].wMaxPacketSize);
                        epInfo[index].epAttribs = 0;
                        epInfo[index].bmNakPower = (index == epDataInIndex) ? UHS_USB_NAK_NOWAIT : UHS_USB_NAK_MAX_POWER;
                        epInfo[index].bmSndToggle = 0;
                        epInfo[index].bmRcvToggle = 0;
                        bNumEP++;
                }
        }
        ADK_HOST_DEBUG("ADK: found %i endpoints\r\n", bNumEP);
        bAddress = ei->address;
        epInfo[0].epAddr = 0;
        epInfo[0].maxPktSize = ei->bMaxPacketSize0;
        epInfo[0].bmNakPower = UHS_USB_NAK_MAX_POWER;
        bIface = ei->interface.bInterfaceNumber;
        bConfNum = ei->currentconfig;
        ADK_HOST_DEBUG("ADK: address %i, config %i, iface %i with %i endpoints\r\n", bAddress, bConfNum, bIface, bNumEP);
        return 0;
}


/**
 * Connection initialization of an Android phone
 */
uint8_t UHS_NI UHS_ADK::OnStart(void) {
        ADK_HOST_DEBUG("ADK: Start\r\n");
        uint8_t rcode;

        ADK_HOST_DEBUG("ADK: Acc.mode device detected\r\n");
        // Assign epInfo to epinfo pointer - this time all 3 endpoins
        rcode = pUsb->setEpInfoEntry(bAddress, bNumEP, epInfo);
        if(rcode) goto FailSetDevTblEntry;

        // Set Configuration Value
        rcode = pUsb->setConf(bAddress, bConfNum);
        if(rcode) goto FailSetConfDescr;

        ADK_HOST_DEBUG("ADK: Configuration successful\r\n");
        ready = true;
        return 0; // successful configuration

        /* diagnostic messages */
FailSetDevTblEntry:
        NotifyFailSetDevTblEntry(rcode);
        goto Fail;

FailSetConfDescr:
        NotifyFailSetConfDescr(rcode);
        goto Fail;

Fail:
        if(rcode) NotifyFail(rcode);
        pUsb->ReleaseDevice(bAddress);
        return rcode;
}

/**
 * Read from the accessory
 */
uint8_t UHS_NI UHS_ADK::Read(uint16_t *bytes_rcvd, uint8_t *dataptr) {
        if(!bAddress) return UHS_HOST_ERROR_UNPLUGGED;
        pUsb->DisablePoll();
        uint8_t rv = pUsb->inTransfer(bAddress, epInfo[epDataInIndex].epAddr, bytes_rcvd, dataptr);
        if(rv && rv != UHS_HOST_ERROR_NAK) Release();
        pUsb->EnablePoll();
        return rv;
}

/**
 * Write to the accessory
 */
uint8_t UHS_NI UHS_ADK::Write(uint16_t nbytes, uint8_t *dataptr) {
        if(!bAddress) return UHS_HOST_ERROR_UNPLUGGED;
        pUsb->DisablePoll();
        uint8_t rv = pUsb->outTransfer(bAddress, epInfo[epDataOutIndex].epAddr, nbytes, dataptr);
        if(rv && rv != UHS_HOST_ERROR_NAK) Release();
        pUsb->EnablePoll();
        return rv;
}

#else
#error "Never include UHS_ADK_INLINE.h, include UHS_host.h instead"
#endif
