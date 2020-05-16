#if defined(LOAD_UHS_PRINTER) && defined(__UHS_PRINTER_H__) && !defined(UHS_PRINTER_LOADED)
#define UHS_PRINTER_LOADED

uint8_t UHS_NI UHS_PRINTER::quirk_check(void) {
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
        uint8_t q = quirk_check();
        if(q == UHS_PRINTER_QUIRK_BAD_CLASS) return true;
        return ((ei->interface.klass == UHS_USB_CLASS_PRINTER && ei->interface.subklass == UHS_PRINTER_USB_SUBCLASS) && ((ei->interface.protocol > 0) &&(ei->interface.protocol < 4)));
        // This is not optimal, needs to pick highest rank with best features,
        // so perhaps we need another test with a numeric ranking to promote the best one.
        // This needs to happen in core. I'll implement it later if this becomes an issue.
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

                        if(qPollRate == 0 && ei->interface.epInfo[ep].bmAttributes == USB_TRANSFER_TYPE_INTERRUPT) {
                                qPollRate = ei->interface.epInfo[ep].bInterval;
                        }
                }
        }
        UHS_PRINTER_HOST_DEBUG("PRINTER: found %i endpoints\r\n", bNumEP);
        bAddress = ei->address;
        epInfo[0].epAddr = 0;
        epInfo[0].maxPktSize = ei->bMaxPacketSize0;
        epInfo[0].bmNakPower = UHS_USB_NAK_MAX_POWER;
        bIface = ei->interface.bInterfaceNumber;
        bConfNum = ei->currentconfig;
        vid = ei->vid;
        pid = ei->pid;

        quirks = quirk_check(); // note any strange crap.

        UHS_PRINTER_HOST_DEBUG("PRINTER: address %i, config %i, iface %i with %i endpoints\r\n", bAddress, bConfNum, bIface, bNumEP);

        if(qPollRate == 0) qPollRate = 8; // Sane default for bad polling rate.
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
        // Assign epInfo to epinfo pointer - this time all 3 endpoints
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
 * rx data from bi-dir or 1284
 *
 * @param len maximum length of the data buffer, returns with amount read.
 * @param data the data buffer.
 * @return  0 for success, otherwise return error code.
 */
uint8_t UHS_NI UHS_PRINTER::read(uint16_t *len, uint8_t *data) {
        // TO-DO: If not bi-dir or 1284, return an unimplemented error
        if(!bAddress) return UHS_HOST_ERROR_UNPLUGGED;
        if((quirks & UHS_PRINTER_QUIRK_BIDIR) == UHS_PRINTER_QUIRK_BIDIR) return UHS_HOST_ERROR_UNSUPPORTED_REQUEST;

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
        uint8_t rv = 0;
        uint16_t t;
        while(len) {
                if(!bAddress) return UHS_HOST_ERROR_UNPLUGGED;
                t = len;
                if(t > epInfo[epDataOutIndex].maxPktSize) t = epInfo[epDataOutIndex].maxPktSize;
                pUsb->DisablePoll();
                rv = check_status();
                if(rv && rv != UHS_HOST_ERROR_NAK) {
                        pUsb->DisablePoll();
                        Release();
                        pUsb->EnablePoll();
                        break;
                }
                OnPoll();
                rv = pUsb->outTransfer(bAddress, epInfo[epDataOutIndex].epAddr, t, data);
                pUsb->EnablePoll();
                if(rv && rv != UHS_HOST_ERROR_NAK) {
                        pUsb->DisablePoll();
                        Release();
                        pUsb->EnablePoll();
                        break;
                }
                len -= t;
                data += t;
        }
        return rv;
}



#else
#error "Never include UHS_PRINTER_INLINE.h, include UHS_host.h instead"
#endif
