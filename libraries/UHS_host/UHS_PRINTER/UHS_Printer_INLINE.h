#if defined(LOAD_UHS_Printer) && defined(__UHS_Printer_H__) && !defined(UHS_Printer_LOADED)
#define UHS_Printer_LOADED

/**
 * Resets interface driver to unused state
 */
void UHS_NI UHS_Printer::DriverDefaults(void) {
        ready = false;
        if(pktbuf) {
                delete pktbuf;
                pktbuf = NULL;
        }
        pUsb->DeviceDefaults(UHS_Printer_MAX_ENDPOINTS, this);
}

UHS_NI UHS_Printer::UHS_Printer(UHS_USB_HOST_BASE *p) {
        pUsb = p; //pointer to USB class instance - mandatory
        ready = false;
        Printerbuf.init(128);
        pktbuf = NULL;
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
bool UHS_NI UHS_Printer::OKtoEnumerate(ENUMERATION_INFO *ei) {
        UHS_Printer_HOST_DEBUG("Printer: checking vid %04x, pid %04x, iface class %i, subclass %i\r\n",
                ei->vid, ei->pid, ei->interface.klass, ei->interface.subklass);
        return ((ei->interface.klass == UHS_USB_CLASS_PRINTER && ei->interface.subklass == UHS_USB_SUBCLASS_PRINTER));
}

/**
 *
 * @param ei Enumeration information
 * @return 0 always
 */
uint8_t UHS_NI UHS_Printer::SetInterface(ENUMERATION_INFO *ei) {
        UHS_Printer_HOST_DEBUG("Printer: SetInterface\r\n");
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
        UHS_Printer_HOST_DEBUG("Printer: found %i endpoints\r\n", bNumEP);
        bAddress = ei->address;
        epInfo[0].epAddr = 0;
        epInfo[0].maxPktSize = ei->bMaxPacketSize0;
        epInfo[0].bmNakPower = UHS_USB_NAK_MAX_POWER;
        bIface = ei->interface.bInterfaceNumber;
        bConfNum = ei->currentconfig;
        vid = ei->vid;
        pid = ei->pid;
        UHS_Printer_HOST_DEBUG("Printer: address %i, config %i, iface %i with %i endpoints\r\n", bAddress, bConfNum, bIface, bNumEP);
        // FIX-ME: qPollRate should be set from one of the interface epInfo, BUT WHICH ONE?!
        // If you know, please fix and submit a pull request.
        // For now default to the minimum allowed by M$, even if Linux allows lower.
        // This seems to work just fine on AKAI and Yamaha keyboards.
        if(qPollRate == 0) qPollRate = 8;
        return 0;
}

/**
 *
 * @return 0 for success
 */
uint8_t UHS_NI UHS_Printer::Start(void) {
        uint8_t rcode;
        UHS_Printer_HOST_DEBUG("Printer: Start\r\n");
        Printerbuf.clear();
        UHS_Printer_HOST_DEBUG("Printer: device detected\r\n");
        // Assign epInfo to epinfo pointer - this time all 3 endpoints
        rcode = pUsb->setEpInfoEntry(bAddress, bIface, bNumEP, epInfo);
        if(!rcode) {
                UHS_Printer_HOST_DEBUG("Printer: EpInfoEntry OK\r\n");
                pktbuf = new uint8_t(epInfo[epDataInIndex].maxPktSize);
                if(pktbuf) {
                        rcode = OnStart();
                        if(!rcode) {
                                UHS_Printer_HOST_DEBUG("Printer: OnStart OK\r\n");
                                qNextPollTime = millis() + qPollRate;
                                bPollEnable = true;
                                ready = true;
                        } else {
                                UHS_Printer_HOST_DEBUG("Printer: OnStart FAIL\r\n");
                        }
                } else {
                        rcode = UHS_HOST_ERROR_NOMEM;
                        UHS_Printer_HOST_DEBUG("Printer: -ENOMEM FAIL\r\n");
                }
        } else {
                UHS_Printer_HOST_DEBUG("Printer: EpInfoEntry FAIL\r\n");
        }
        if(rcode) Release();
        return rcode;
}




/**
 * Send raw data to Printer device
 *
 * @param bytes_send
 * @param dataptr
 * @return 0 for success
 */
uint8_t UHS_NI UHS_Printer::SendRawData(uint16_t bytes_send, uint8_t *dataptr) {
        if(!bAddress) return UHS_HOST_ERROR_UNPLUGGED;
        pUsb->DisablePoll();
        uint8_t rv = pUsb->outTransfer(bAddress, epInfo[epDataOutIndex].epAddr, bytes_send, dataptr);
        if(rv && rv != UHS_HOST_ERROR_NAK) Release();
        pUsb->EnablePoll();
        return rv;
}



#else
#error "Never include UHS_Printer_INLINE.h, include UHS_host.h instead"
#endif
