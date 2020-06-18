#if defined(LOAD_UHS_PRINTER) && defined(__UHS_PRINTER_H__) && !defined(UHS_PRINTER_LOADED)
#define UHS_PRINTER_LOADED

uint8_t UHS_NI UHS_PRINTER::quirk_check(ENUMERATION_INFO *ei) {
        uint8_t q = 0x00U;
        size_t s = 0;
        quirk_printer_struct r;
        // Anything awful?
        do {
                memcpy_P(&r, &quirk_printers[s], sizeof (quirk_printer_struct));
                if(r.productId == 0x0000U && r.vendorId == 0x0000U) break;
                if(r.productId == ei->pid && r.vendorId == ei->vid) q = r.quirks;
                s++;
        } while(q == 0);

        return q;
}

/**
 * Resets interface driver to unused state
 */
void UHS_NI UHS_PRINTER::DriverDefaults(void) {
        ready = false;
        bAlternateSetting = 255;
        Interface = 255;
        pUsb->DeviceDefaults(UHS_PRINTER_MAX_ENDPOINTS, this);
}

UHS_NI UHS_PRINTER::UHS_PRINTER(UHS_USB_HOST_BASE *p) {
        pUsb = p; //pointer to USB class instance - mandatory
        ready = false;
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
bool UHS_NI UHS_PRINTER::OKtoEnumerate(ENUMERATION_INFO *ei) {
        UHS_PRINTER_HOST_DEBUG("PRINTER: checking vid %04x, pid %04x, iface class %i, subclass %i\r\n",
                ei->vid, ei->pid, ei->interface.klass, ei->interface.subklass);

        // Check if vid:pid has UHS_PRINTER_QUIRK_BAD_CLASS, and skip the subclass test.
        uint8_t q = quirk_check(ei);
        if(q == UHS_PRINTER_QUIRK_BAD_CLASS) return true;
        return ((ei->interface.klass == UHS_USB_CLASS_PRINTER && ei->interface.subklass == UHS_PRINTER_USB_SUBCLASS) && ((ei->interface.protocol > 0) &&(ei->interface.protocol < 4)));
}

/**
 *
 * @param ei Enumeration information
 * @return 0 always
 */
uint8_t UHS_NI UHS_PRINTER::SetInterface(ENUMERATION_INFO *ei) {
        UHS_PRINTER_HOST_DEBUG("PRINTER: SetInterface\r\n");
        qPollRate = 0;
        for(uint8_t ep = 0; ep < ei->interface.numep; ep++) {
                if(ei->interface.epInfo[ep].bmAttributes == USB_TRANSFER_TYPE_BULK || ei->interface.epInfo[ep].bmAttributes == USB_TRANSFER_TYPE_INTERRUPT) {
                        uint8_t index = ((ei->interface.epInfo[ep].bEndpointAddress & USB_TRANSFER_DIRECTION_IN) == USB_TRANSFER_DIRECTION_IN) ? epDataInIndex : epDataOutIndex;
                        epInfo[index].epAddr = (ei->interface.epInfo[ep].bEndpointAddress & 0x0F);
                        epInfo[index].maxPktSize = ei->interface.epInfo[ep].wMaxPacketSize;
                        epInfo[index].epAttribs = 0;
                        epInfo[index].bmNakPower = (index == epDataInIndex) ? UHS_USB_NAK_NOWAIT : UHS_USB_NAK_MAX_POWER;
                        epInfo[index].bmSndToggle = 0;
                        epInfo[index].bmRcvToggle = 0;
                        bNumEP++;
                        UHS_PRINTER_HOST_DEBUG("PRINTER: index 0x%2.2x epAddr 0x%2.2x maxPktSize 0x%2.2x\r\n", index, epInfo[index].epAddr, epInfo[index].maxPktSize);
                        if(qPollRate == 0 && ei->interface.epInfo[ep].bmAttributes == USB_TRANSFER_TYPE_INTERRUPT) {
                                qPollRate = ei->interface.epInfo[ep].bInterval;
                        }
                }
        }
        UHS_PRINTER_HOST_DEBUG("PRINTER: bAlternateSetting %d bInterfaceNumber %d\r\n", ei->interface.bAlternateSetting, ei->interface.bInterfaceNumber);
        UHS_PRINTER_HOST_DEBUG("PRINTER: found %i endpoints\r\n", bNumEP);

        Interface = ei->interface.bInterfaceNumber;
        bAlternateSetting = ei->interface.bAlternateSetting;
        bAddress = ei->address;
        epInfo[0].epAddr = 0;
        epInfo[0].maxPktSize = ei->bMaxPacketSize0;
        epInfo[0].bmNakPower = UHS_USB_NAK_MAX_POWER;
        bIface = ei->interface.bInterfaceNumber;
        bConfNum = ei->currentconfig;
        vid = ei->vid;
        pid = ei->pid;

        quirks = quirk_check(ei); // note any strange crap.

        UHS_PRINTER_HOST_DEBUG("PRINTER: address %i, config %i, iface %i with %i endpoints\r\n", bAddress, bConfNum, bIface, bNumEP);

        if(qPollRate <= 99) qPollRate = 100; // Sane default for bad polling rate.
        return 0;
}

/**
 *
 * @return 0 for success
 */
uint8_t UHS_NI UHS_PRINTER::Start(void) {
        uint8_t rcode;
        UHS_PRINTER_HOST_DEBUG("PRINTER: Start\r\n");
        UHS_PRINTER_HOST_DEBUG("PRINTER: device detected\r\n");
        // Assign epInfo to epinfo pointer - this time all endpoints
        rcode = pUsb->setEpInfoEntry(bAddress, bIface, bNumEP, epInfo);
        if(!rcode) {
                UHS_PRINTER_HOST_DEBUG("PRINTER: EpInfoEntry OK\r\n");
                rcode = OnStart();
                if(!rcode) {
                        UHS_PRINTER_HOST_DEBUG("PRINTER: OnStart OK\r\n");
                        qNextPollTime = millis() + qPollRate;
                        bPollEnable = true;
                        ready = true;
                } else {
                        UHS_PRINTER_HOST_DEBUG("PRINTER: OnStart FAIL\r\n");
                }
        } else {
                UHS_PRINTER_HOST_DEBUG("PRINTER: EpInfoEntry FAIL\r\n");
        }
        if(rcode) Release();
        return rcode;
}

/**
 * Get and store status byte from interface into status
 *
 * @return  0 for success, otherwise return error code.
 */
uint8_t UHS_NI UHS_PRINTER::check_status(void) {
        if(!bAddress) return UHS_HOST_ERROR_UNPLUGGED;
        uint8_t rv = 0;

        // TO-DO: Get printer status and store it for use in OnPoll() or writing

        return rv;
}

void UHS_NI UHS_PRINTER::Poll(void) {
        if((long)(millis() - qNextPollTime) >= 0L) {
                int rv;
                rv = check_status();
                if(rv && rv != UHS_HOST_ERROR_NAK) {
                        Release();
                        return;
                }
                OnPoll(); // Do user stuff...
                qNextPollTime = millis() + qPollRate;
        }
}

/**
 *
 * @return 1 == uni-directional, 2 == bidirectional, ZERO is unknown
 */
uint8_t UHS_NI UHS_PRINTER::printer_type(void) {
        uint8_t rv = 0;
        if(bNumEP == 2) {
                rv = 1;
        } else if(bNumEP == 3) {
                rv = 2;
        }
        return rv;
}

/**
 *
 * @return 0 for OK
 */
uint8_t UHS_NI UHS_PRINTER::select_printer(void) {
        uint8_t rv = 0;
        pUsb->DisablePoll();                       //bmReqType,     bRequest,              wVal, wInd,           total
        rv = pUsb->ctrlReq(bAddress, mkSETUP_PKT16(USB_SETUP_RECIPIENT_INTERFACE, USB_REQUEST_SET_INTERFACE, bAlternateSetting, Interface, 0), 0, NULL);
        pUsb->EnablePoll();
        return rv;
}

/**
 *
 * @return -1 for not selected, 0 for selected, or error
 */
int16_t UHS_NI UHS_PRINTER::printer_selected(void) {
        uint16_t rv = 0;
        uint8_t check = 0xffu;
        pUsb->DisablePoll();
        rv = pUsb->ctrlReq(bAddress, mkSETUP_PKT16(USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE, USB_REQUEST_GET_INTERFACE, 0, Interface, 1), 1, &check);
        pUsb->EnablePoll();
        if(!rv) {
                if(check != bAlternateSetting) rv = -1;
        }
        return rv;

}

/**
 * rx data from bi-dir or, possibly 1284?
 *
 * @param len maximum length of the data buffer, returns with amount read.
 * @param data the data buffer.
 * @return  0 for success, otherwise return error code.
 */
uint8_t UHS_NI UHS_PRINTER::read(uint16_t *len, NOTUSED(uint8_t *data)) { // suppress warning for now
        if(!bAddress) return UHS_HOST_ERROR_UNPLUGGED;
        if((quirks & UHS_PRINTER_QUIRK_BIDIR) == UHS_PRINTER_QUIRK_BIDIR) return UHS_HOST_ERROR_UNSUPPORTED_REQUEST;
        if(printer_type() != 2) return UHS_HOST_ERROR_UNSUPPORTED_REQUEST;
        uint8_t rv = 0;

        // TO-DO: implement. For now, we return no data.
        *len = 0U;

        return rv;
}

/**
 * Send data to PRINTER device
 *
 * @param len length of the data to be sent.
 * @param data data to be sent.
 * @return 0 for success, otherwise return error code.
 */
uint8_t UHS_NI UHS_PRINTER::write(uint16_t len, uint8_t *data) {
        if(!bAddress) return UHS_HOST_ERROR_UNPLUGGED;
        int16_t ck = printer_selected();
        if(ck) {
                if(ck < 0) return UHS_HOST_ERROR_UNSUPPORTED_REQUEST;
                return (ck&0xffu);
        }
        uint8_t rv = 0;
        uint16_t t;
        while(len) {
                if(!bAddress) return UHS_HOST_ERROR_UNPLUGGED;
                t = len;
                if(t > epInfo[epDataOutIndex].maxPktSize) t = epInfo[epDataOutIndex].maxPktSize;
                pUsb->DisablePoll();
                rv = check_status();
                pUsb->EnablePoll();
                if(rv && rv != UHS_HOST_ERROR_NAK) {
                        Release();
                        break;
                }
                pUsb->DisablePoll();
                OnPoll();
                rv = pUsb->outTransfer(bAddress, epInfo[epDataOutIndex].epAddr, t, data);
                pUsb->EnablePoll();
                if(rv && rv != UHS_HOST_ERROR_NAK) {
                        printf("death 2: 0x%2.2x\r\n", rv);
                        Release();
                        break;
                }
                if(rv == UHS_HOST_ERROR_NAK) t = 0;
                len -= t;
                data += t;
        }
        return rv;
}



#else
#error "Never include UHS_PRINTER_INLINE.h, include UHS_host.h instead"
#endif
