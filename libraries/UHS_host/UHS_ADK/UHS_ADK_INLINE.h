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

#define UHS_PID_GOOGLE_ADK   0x2D00
#define UHS_PID_GOOGLE_ADB   0x2D01

/* requests */
#define UHS_ADK_GETPROTO      51  // check USB accessory protocol version
#define UHS_ADK_SENDSTR       52  // send identifying string
#define UHS_ADK_ACCSTART      53  // start device in accessory mode

#define UHS_ADK_bmREQ_GET     USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_VENDOR|USB_SETUP_RECIPIENT_DEVICE
#define UHS_ADK_bmREQ_SEND    USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_VENDOR|USB_SETUP_RECIPIENT_DEVICE

#define UHS_ADK_ID_MANUFACTURER   0
#define UHS_ADK_ID_MODEL          1
#define UHS_ADK_ID_DESCRIPTION    2
#define UHS_ADK_ID_VERSION        3
#define UHS_ADK_ID_URI            4
#define UHS_ADK_ID_SERIAL         5


UHS_NI UHS_ADK::UHS_ADK(UHS_USB_HOST_BASE *p) : ready(false) {
        pUsb = p; //pointer to USB class instance - mandatory
        // register in USB subsystem
        if(pUsb) {
                DriverDefaults();
                pUsb->RegisterDeviceClass(this);
        }
}

/**
 * Check if device is supported by this interface driver
 * @param ei Enumeration information
 * @return true if this interface driver can handle this interface description
 */
bool UHS_NI UHS_ADK::OKtoEnumerate(ENUMERATION_INFO *ei) {
        ADK_HOST_DEBUG("ADK: checking vid %4.4x, pid %4.4x\r\n", ei->vid, ei->pid);
        return (ei->vid == UHS_VID_GOOGLE);
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
        if(ei->pid == UHS_PID_GOOGLE_ADK || ei->pid == UHS_PID_GOOGLE_ADB) {
                ADK_HOST_DEBUG("ADK: device in accessory mode\r\n");
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
                ADK_HOST_DEBUG("ADK: found %i bulk endpoints\r\n", bNumEP);
        }
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

        // check if ADK device is already in accessory mode; if yes, configure and exit
        if(bNumEP == ADK_MAX_ENDPOINTS) {
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
        }

        ADK_HOST_DEBUG("ADK: probe device - get accessory protocol revision\r\n");
        adkproto = -1;
        rcode = getProto((uint8_t*) & adkproto);
        if(rcode) goto FailGetProto; // init fails
        ADK_HOST_DEBUG("ADK: protocol rev. %i\r\n", adkproto);

        delay(100);

        ADK_HOST_DEBUG("ADK: send strings\r\n");
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
        ADK_HOST_DEBUG("ADK: switch to accessory mode\r\n");
        rcode = switchAcc();
        if(rcode) goto FailSwAcc;

        rcode = UHS_USB_HOST_STATE_RESET_NOT_COMPLETE;
        delay(100); // Give Android a chance to do its reset. This is a guess, and possibly could be lower.
        goto SwAttempt; // switch to accessory mode attempted

        /* diagnostic messages */
FailSetDevTblEntry:
        NotifyFailSetDevTblEntry(rcode);
        goto Fail;

FailSetConfDescr:
        NotifyFailSetConfDescr(rcode);
        goto Fail;

FailGetProto:
        ADK_HOST_DEBUG("ADK: getProto: ");
        goto Fail;

FailSwAcc:
        ADK_HOST_DEBUG("ADK: swAcc: ");
        goto Fail;

Fail:
        if(rcode) NotifyFail(rcode);

SwAttempt:
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

/**
 * Get ADK protocol version
 * @return 16bit little endian ADK protocol version
 */
uint8_t UHS_NI UHS_ADK::getProto(uint8_t *adkproto) {
        return pUsb->ctrlReq(bAddress, mkSETUP_PKT16(UHS_ADK_bmREQ_GET, UHS_ADK_GETPROTO, 0, 0, 2), 2, adkproto);
}

/**
 * Send ADK string
 */
uint8_t UHS_NI UHS_ADK::sendStr(uint8_t index, const char *str) {
        return pUsb->ctrlReq(bAddress, mkSETUP_PKT16(UHS_ADK_bmREQ_SEND, UHS_ADK_SENDSTR, 0, index, strlen(str) + 1), strlen(str) + 1, (uint8_t*)str);
}

/**
 * Switch to accessory mode
 */
uint8_t UHS_NI UHS_ADK::switchAcc(void) {
        return pUsb->ctrlReq(bAddress, mkSETUP_PKT16(UHS_ADK_bmREQ_SEND, UHS_ADK_ACCSTART, 0, 0, 0), 0, NULL);
}

#else
#error "Never include UHS_ADK_INLINE.h, include UHS_host.h instead"
#endif
