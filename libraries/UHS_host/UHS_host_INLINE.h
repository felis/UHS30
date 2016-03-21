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

#if defined(LOAD_USB_HOST_SYSTEM) && !defined(USB_HOST_SYSTEM_LOADED)
#define USB_HOST_SYSTEM_LOADED

#ifndef DEBUG_PRINTF_EXTRA_HUGE_UHS_HOST
#define DEBUG_PRINTF_EXTRA_HUGE_UHS_HOST 0
#endif

#if DEBUG_PRINTF_EXTRA_HUGE
#if DEBUG_PRINTF_EXTRA_HUGE_UHS_HOST
#define HOST_DUBUG(...) printf(__VA_ARGS__)
#else
#define HOST_DUBUG(...) VOID0
#endif
#else
#define HOST_DUBUG(...) VOID0
#endif

UHS_EpInfo* UHS_USB_HOST_BASE::getEpInfoEntry(uint8_t addr, uint8_t ep) {
        UHS_Device *p = addrPool.GetUsbDevicePtr(addr);

        if(!p || !p->epinfo)
                return NULL;

        UHS_EpInfo *pep = (UHS_EpInfo *)(p->epinfo);

        for(uint8_t i = 0; i < p->epcount; i++) {
                if((pep)->epAddr == ep)
                        return pep;

                pep++;
        }
        return NULL;
}

/**
 * Sets a device table entry for a device.
 * Each device is different and has different number of endpoints.
 * This function plugs endpoint record structure, defined in application, to devtable
 *
 * @param addr device address
 * @param epcount how many endpoints
 * @param eprecord pointer to the endpoint structure
 * @return
 */
uint8_t UHS_USB_HOST_BASE::setEpInfoEntry(uint8_t addr, uint8_t epcount, volatile UHS_EpInfo* eprecord) {
        if(!eprecord)
                return UHS_HOST_ERROR_BAD_ARGUMENT;

        UHS_Device *p = addrPool.GetUsbDevicePtr(addr);

        if(!p)
                return UHS_HOST_ERROR_NO_ADDRESS_IN_POOL;

        p->address.devAddress = addr;
        p->epinfo = eprecord;
        p->epcount = epcount;
        return 0;
}

/**
 * sets all enpoint addresses to zero.
 * Sets all max packet sizes to defaults
 * Clears all endpoint attributes
 * Sets bmNakPower to USB_NAK_DEFAULT
 * Sets binterface to zero.
 * Sets bNumEP to zero.
 * Sets bAddress to zero.
 * Clears qNextPollTime and sets bPollEnable to false.
 *
 * @param maxep How many endpoints to initialize
 * @param device pointer to the device driver instance (this)
 *
 */

void UHS_USB_HOST_BASE::DeviceDefaults(uint8_t maxep, UHS_USBInterface *interface) {

        for(uint8_t i = 0; i < maxep; i++) {
                interface->epInfo[i].epAddr = 0;
                interface->epInfo[i].maxPktSize = (i) ? 0 : 8;
                interface->epInfo[i].epAttribs = 0;
                interface->epInfo[i].bmNakPower = USB_NAK_DEFAULT;
        }
        interface->pUsb->GetAddressPool()->FreeAddress(interface->bAddress);
        interface->bIface = 0;
        interface->bNumEP = 1;
        interface->bAddress = 0;
        interface->qNextPollTime = 0;
        interface->bPollEnable = false;
}

/**
 * Perform a bus reset to the port of the connected device
 *
 * @param parent index to Parent
 * @param port what port on the parent
 * @param address address of the device
 * @return zero on success
 */

uint8_t UHS_USB_HOST_BASE::doSoftReset(uint8_t parent, uint8_t port, uint8_t address) {
        uint8_t rcode;

        if(parent == 0) {
                // Send a bus reset on the root interface.
                doHostReset();
        } else {
                // reset parent port
                devConfig[parent]->ResetHubPort(port);
        }

        //
        // Many devices require a delay before setting the address here...
        // We loop upon fails for up to 2 seconds instead.
        // Most devices will be happy without a retry.
        //
        uint8_t retries = 0;
        do {
                rcode = setAddr(0, address);
                if(!rcode) break;
                retries++;
                sof_delay(10);
        } while(retries < 200);
        HOST_DUBUG("%i retries.\r\n", retries);
        return rcode;
}

/*
 * Pseudo code so you may understand the code flow.
 *
 *      reset; (happens at the lower level)
 *      GetDevDescr();
 *      reset;
 *      If there are no configuration descriptors {
 *              //
 *              // Note: I know of no device that does this.
 *              // I suppose there could be one though.
 *              //
 *              try to enumerate.
 *      } else {
 *              last success count = 0
 *              best config = 0
 *              for each configuration descriptor {
 *                      for each interface descriptor {
 *                              get the endpoint descriptors for this interface.
 *                              Check to see if a driver can handle this interface.
 *                              If it can, add 1 to the success count.
 *                      }
 *                      if success count > last success count {
 *                              best config = current config
 *                              last success count = success count
 *                      }
 *              }
 *              set the device config to the best config
 *              for each best config interface descriptor {
 *                      initialize driver that can handle this interface config
 *              }
 *      }
 *
 *       NOTES:
 *              1: We do not need to save toggle states anymore and have not
 *                      needed to for some time, because the lower level driver
 *                      actually corrects wrong toggles on-the-fly for us.
 *
 *              2: We always do a second reset, since this stupid bug is
 *                      actually part of the specification documents that I
 *                      have found all over the net. Even Linux does it, and
 *                      many devices actually EXPECT this behavior. Some devices
 *                      will not enumerate without it. For devices that do not
 *                      need it, the additional reset is harmless. Here is an
 *                      example of one of these documents, see page Five:
 *                      http://www.ftdichip.com/Support/Documents/TechnicalNotes/TN_113_Simplified%20Description%20of%20USB%20Device%20Enumeration.pdf
 *
 *
 */

/**
 *
 * @param parent index to Parent
 * @param port what port on the parent
 * @param lowspeed the speed of the device is low speed
 * @return
 */
uint8_t UHS_USB_HOST_BASE::Configuring(uint8_t parent, uint8_t port, bool lowspeed) {
        //uint8_t bAddress = 0;
        HOST_DUBUG("\r\n\r\n\r\nConfiguring: parent = %i, port = %i, lowspeed = %i\r\n", parent, port, lowspeed ? 1 : 0);
        uint8_t rcode = 0;
        uint8_t retries = 0;
        uint8_t numinf = 0;
        // Since any descriptor we are interested in should not be > 18 bytes, there really is no need for a parser.
        // I can do everything in one reusable buffer. :-)
        //const uint8_t biggest = max(max(max(sizeof (USB_DEVICE_DESCRIPTOR), sizeof (USB_CONFIGURATION_DESCRIPTOR)), sizeof (USB_INTERFACE_DESCRIPTOR)), sizeof (USB_ENDPOINT_DESCRIPTOR));
        const uint8_t biggest = 18;
        uint8_t buf[biggest];

        USB_DEVICE_DESCRIPTOR *udd = reinterpret_cast<USB_DEVICE_DESCRIPTOR *>(buf);
        USB_CONFIGURATION_DESCRIPTOR *ucd = reinterpret_cast<USB_CONFIGURATION_DESCRIPTOR *>(buf);

        //USB *pUsb = this;
        UHS_Device *p = NULL;
        //EpInfo epInfo; // cap at 16, this should be fairly reasonable.
        ENUMERATION_INFO ei;
        uint8_t bestconf = 0;
        uint8_t bestsuccess = 0;

        uint8_t devConfigIndex;
        //for(devConfigIndex = 0; devConfigIndex < UHS_HOST_MAX_INTERFACE_DRIVERS; devConfigIndex++) {
        //        if((devConfig[devConfigIndex]->bAddress) && (!devConfig[devConfigIndex]->bPollEnable)) {
        //                devConfig[devConfigIndex]->bAddress = 0;
        //        }
        //}
        //        Serial.print("HOST USB Host @ 0x");
        //        Serial.println((uint32_t)this, HEX);
        //        Serial.print("HOST USB Host Address Pool @ 0x");
        //        Serial.println((uint32_t)GetAddressPool(), HEX);

        sof_delay(200);
        p = addrPool.GetUsbDevicePtr(0);
        if(!p) {
                HOST_DUBUG("Configuring error: USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL\r\n");
                return UHS_HOST_ERROR_NO_ADDRESS_IN_POOL;
        }

        p->lowspeed = lowspeed;
        p->epinfo[0].maxPktSize = 8; // lowspeed ? 8 : 16; if we get more than 8, this is fine.
        HOST_DUBUG("\r\n\r\nConfiguring...\r\n");
again:
        rcode = getDevDescr(0, biggest, (uint8_t*)buf);
        if(rcode) {
                if(rcode == UHS_HOST_ERROR_JERR && retries < 4) {
                        //
                        // Some devices return JERR when plugged in.
                        // Attempts to reinitialize the device usually works.
                        //
                        // I have a hub that will refuse to work and acts like
                        // this unless external power is supplied.
                        // So this may not always work, and you may be fooled.
                        //
                        sof_delay(100);
                        retries++;
                        goto again;
                } else if(rcode == hrDMA && retries < 4) {
                        if(p->epinfo[0].maxPktSize < 32) p->epinfo[0].maxPktSize = p->epinfo[0].maxPktSize << 1;

                        HOST_DUBUG("Configuring error: hrDMA. Retry with maxPktSize: %i\r\n", p->epinfo[0].maxPktSize);
                        doSoftReset(parent, port, 0);
                        retries++;
                        sof_delay(200);
                        goto again;
                }
                HOST_DUBUG("Configuring error: %2.2x Can't get USB_DEVICE_DESCRIPTOR\r\n", rcode);
                return rcode;
        }

        HOST_DUBUG("retries: %i\r\n", retries);
        ei.vid = udd->idVendor;
        ei.pid = udd->idProduct;
        ei.klass = udd->bDeviceClass;
        ei.subklass = udd->bDeviceSubClass;
        ei.protocol = udd->bDeviceProtocol;
        ei.bMaxPacketSize0 = udd->bMaxPacketSize0;
        ei.currentconfig = 0;
        ei.parent = parent;
        uint8_t configs = udd->bNumConfigurations;

        ei.address = addrPool.AllocAddress(parent, IsHub(ei.klass), port);

        if(!ei.address) {
                return UHS_HOST_ERROR_ADDRESS_POOL_FULL;
        }

        p = addrPool.GetUsbDevicePtr(ei.address);
        if(!p) {
                return UHS_HOST_ERROR_NO_ADDRESS_IN_POOL;
        }

        p->lowspeed = lowspeed;

        rcode = doSoftReset(parent, port, ei.address);

        if(rcode) {
                addrPool.FreeAddress(ei.address);
                HOST_DUBUG("Configuring error: %2.2x Can't set USB INTERFACE ADDRESS\r\n", rcode);
                return rcode;
        }

        if(configs < 1) {
                HOST_DUBUG("No interfaces?!\r\n");
                addrPool.FreeAddress(ei.address);
                // rcode = TestInterface(&ei);
                // Not implemented (yet)
                rcode = UHS_HOST_ERROR_DEVICE_NOT_SUPPORTED;
        } else {
                HOST_DUBUG("configs: %i\r\n", configs);
                for(uint8_t conf = 0; (!rcode) && (conf < configs); conf++) {
                        // read the config descriptor into a buffer.
                        rcode = getConfDescr(ei.address, sizeof (USB_CONFIGURATION_DESCRIPTOR), conf, buf);
                        if(rcode) {
                                HOST_DUBUG("Configuring error: %2.2x Can't get USB_INTERFACE_DESCRIPTOR\r\n", rcode);
                                rcode = UHS_HOST_ERROR_FailGetConfDescr;
                                continue;
                        }
                        ei.currentconfig = conf;
                        numinf = ucd->bNumInterfaces; // Does _not_ include alternates!
                        HOST_DUBUG("CONFIGURATION: %i, bNumInterfaces %i, wTotalLength %i\r\n", conf, numinf, ucd->wTotalLength);
                        uint8_t success = 0;
                        uint16_t inf = 0;
                        uint8_t data[ei.bMaxPacketSize0];
                        UHS_EpInfo *pep;
                        pep = ctrlReqOpen(ei.address, UHS_bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, ei.currentconfig, USB_DESCRIPTOR_CONFIGURATION, 0x0000, ucd->wTotalLength, data);
                        if(!pep) {
                                rcode = UHS_HOST_ERROR_NULL_EPINFO;
                                continue;
                        }
                        uint16_t left;
                        uint16_t read;
                        uint8_t offset;
                        rcode = initDescrStream(&ei, ucd, pep, data, &left, &read, &offset);
                        if(rcode) {
                                HOST_DUBUG("Configuring error: %2.2x Can't get USB_INTERFACE_DESCRIPTOR stream.\r\n", rcode);
                                break;
                        }
                        for(; (numinf) && (!rcode); inf++) {
                                // iterate for each interface on this config
                                rcode = getNextInterface(&ei, pep, data, &left, &read, &offset);
                                if(rcode == UHS_HOST_ERROR_END_OF_STREAM) {
                                        HOST_DUBUG("USB_INTERFACE END OF STREAM\r\n");
                                        ctrlReqClose(pep, UHS_bmREQ_GET_DESCR, left, ei.bMaxPacketSize0, data);
                                        rcode = 0;
                                        break;
                                }
                                if(rcode) {
                                        HOST_DUBUG("Configuring error: %2.2x Can't close USB_INTERFACE_DESCRIPTOR stream.\r\n", rcode);
                                        continue;
                                }
                                rcode = TestInterface(&ei);
                                if(!rcode) success++;
                                rcode = 0;
                        }
                        if(!inf) {
                                rcode = TestInterface(&ei);
                                if(!rcode) success++;
                                rcode = 0;
                        }
                        if(success > bestsuccess) {
                                bestconf = conf;
                                bestsuccess = success;
                        }
                }
                if(!bestsuccess) rcode = UHS_HOST_ERROR_DEVICE_NOT_SUPPORTED;
        }
        if(!rcode) {
                rcode = getConfDescr(ei.address, sizeof (USB_CONFIGURATION_DESCRIPTOR), bestconf, buf);
                if(rcode) {
                        HOST_DUBUG("Configuring error: %2.2x Can't get USB_INTERFACE_DESCRIPTOR\r\n", rcode);
                        rcode = UHS_HOST_ERROR_FailGetConfDescr;
                }
        }
        if(!rcode) {
                bestconf++;
                ei.currentconfig = bestconf;
                numinf = ucd->bNumInterfaces; // Does _not_ include alternates!
                HOST_DUBUG("CONFIGURATION: %i, bNumInterfaces %i, wTotalLength %i\r\n", bestconf, numinf, ucd->wTotalLength);
                if(!rcode) {
                        HOST_DUBUG("Best configuration is %i, enumerating interfaces.\r\n", bestconf);
                        uint16_t inf = 0;
                        uint8_t data[ei.bMaxPacketSize0];
                        UHS_EpInfo *pep;
                        pep = ctrlReqOpen(ei.address, UHS_bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, ei.currentconfig - 1, USB_DESCRIPTOR_CONFIGURATION, 0x0000, ucd->wTotalLength, data);
                        if(!pep) {
                                rcode = UHS_HOST_ERROR_NULL_EPINFO;

                        } else {
                                uint16_t left;
                                uint16_t read;
                                uint8_t offset;
                                rcode = initDescrStream(&ei, ucd, pep, data, &left, &read, &offset);
                                if(rcode) {
                                        HOST_DUBUG("Configuring error: %2.2x Can't get USB_INTERFACE_DESCRIPTOR stream.\r\n", rcode);
                                } else {
                                        for(; (numinf) && (!rcode); inf++) {
                                                // iterate for each interface on this config
                                                rcode = getNextInterface(&ei, pep, data, &left, &read, &offset);
                                                if(rcode == UHS_HOST_ERROR_END_OF_STREAM) {
                                                        ctrlReqClose(pep, UHS_bmREQ_GET_DESCR, left, ei.bMaxPacketSize0, data);
                                                        rcode = 0;
                                                        break;
                                                }
                                                if(rcode) {
                                                        HOST_DUBUG("Configuring error: %2.2x Can't close USB_INTERFACE_DESCRIPTOR stream.\r\n", rcode);
                                                        continue;
                                                }

                                                if(enumerateInterface(&ei) == UHS_HOST_MAX_INTERFACE_DRIVERS) {
                                                        HOST_DUBUG("No interface driver for this interface.");
                                                } else {
                                                        HOST_DUBUG("Interface Configured\r\n");
                                                }
                                        }
                                }
                        }
                } else {
                        HOST_DUBUG("Configuring error: %2.2x Can't set USB_INTERFACE_CONFIG stream.\r\n", rcode);
                }
        }

        if(!rcode) {
                rcode = setConf(ei.address, bestconf);
                if(rcode) {
                        HOST_DUBUG("Configuring error: %2.2x Can't set Configuration.\r\n", rcode);
                        addrPool.FreeAddress(ei.address);
                } else {
                        for(devConfigIndex = 0; devConfigIndex < UHS_HOST_MAX_INTERFACE_DRIVERS; devConfigIndex++) {
                                HOST_DUBUG("Driver %i ", devConfigIndex);
                                if(!devConfig[devConfigIndex]) {
                                        HOST_DUBUG("no driver at this index.\r\n");
                                        continue; // no driver
                                }
                                HOST_DUBUG("@ %2.2x ", devConfig[devConfigIndex]->bAddress);
                                if(devConfig[devConfigIndex]->bAddress) {
                                        if(!devConfig[devConfigIndex]->bPollEnable) {
                                                HOST_DUBUG("Initialize\r\n");
                                                rcode = devConfig[devConfigIndex]->Finalize();
                                                rcode = devConfig[devConfigIndex]->Start();
                                                if(!rcode) {
                                                        HOST_DUBUG("Total endpoints = (%i)%i\r\n", p->epcount,devConfig[devConfigIndex]->bNumEP);
                                                } else {
                                                        break;
                                                }
                                        } else {
                                                HOST_DUBUG("Already initialized.\r\n");
                                                continue; // consumed
                                        }
                                } else {
                                        HOST_DUBUG("Skipped\r\n");
                                }
                        }
#if defined(UHS_HID_LOADED)
                        // Now do HID
#endif
                        }
        } else {
                addrPool.FreeAddress(ei.address);
        }
        return rcode;
}

/**
 * Removes a device from the tables
 *
 * @param addr address of the device
 * @return nothing
 */
void UHS_USB_HOST_BASE::ReleaseDevice(uint8_t addr) {
        if(addr) {
#if defined(UHS_HID_LOADED)
                // Release any HID children
                UHS_HID_Release(this, addr);
#endif
                for(uint8_t i = 0; i < UHS_HOST_MAX_INTERFACE_DRIVERS; i++) {
                        if(!devConfig[i]) continue;
                        if(devConfig[i]->bAddress == addr) {
                                devConfig[i]->Release();
                                break;
                        }
                }
        }
}

/**
 * Gets the device descriptor, or part of it from endpoint Zero.
 *
 * @param addr Address of the device
 * @param nbytes how many bytes to return
 * @param dataptr pointer to the data to return
 * @return status of the request, zero is success.
 */
uint8_t UHS_USB_HOST_BASE::getDevDescr(uint8_t addr, uint16_t nbytes, uint8_t* dataptr) {
        return ( ctrlReq(addr, UHS_bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, 0x00, USB_DESCRIPTOR_DEVICE, 0x0000, nbytes, nbytes, dataptr));
}

uint8_t UHS_USB_HOST_BASE::getConfDescr(uint8_t addr, uint16_t nbytes, uint8_t conf, uint8_t* dataptr) {
        return ( ctrlReq(addr, UHS_bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, conf, USB_DESCRIPTOR_CONFIGURATION, 0x0000, nbytes, nbytes, dataptr));
}

/**
 * Get the string descriptor from a device
 *
 * @param addr Address of the device
 * @param ns
 * @param index
 * @param langid language ID
 * @param dataptr pointer to the data to return
 * @return status of the request, zero is success.
 */
uint8_t UHS_USB_HOST_BASE::getStrDescr(uint8_t addr, uint16_t ns, uint8_t index, uint16_t langid, uint8_t* dataptr) {
        return ( ctrlReq(addr, UHS_bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, index, USB_DESCRIPTOR_STRING, langid, ns, ns, dataptr));
}

//
//set address
//

/**
 * Set the address of a device to a new address via endpoint Zero.
 *
 * @param oldaddr current address
 * @param newaddr new address
 * @return status of the request, zero is success.
 */
uint8_t UHS_USB_HOST_BASE::setAddr(uint8_t oldaddr, uint8_t newaddr) {
        uint8_t rcode = ctrlReq(oldaddr, UHS_bmREQ_SET, USB_REQUEST_SET_ADDRESS, newaddr, 0x00, 0x0000, 0x0000, 0x0000, NULL);
        sof_delay(300); // Older spec says you should wait at least 200ms
        return rcode;
}

//
//set configuration
//

/**
 * Set the configuration for the device to use via endpoint Zero.
 *
 * @param addr Address of the device
 * @param conf_value configuration index value
 * @return status of the request, zero is success.
 */
uint8_t UHS_USB_HOST_BASE::setConf(uint8_t addr, uint8_t conf_value) {
        return ( ctrlReq(addr, UHS_bmREQ_SET, USB_REQUEST_SET_CONFIGURATION, conf_value, 0x00, 0x0000, 0x0000, 0x0000, NULL));
}

/* rcode 0 if no errors. rcode 01-0f is relayed from HRSL                       */
uint8_t UHS_USB_HOST_BASE::outTransfer(uint8_t addr, uint8_t ep, uint16_t nbytes, uint8_t* data) {
        UHS_EpInfo *pep = NULL;
        uint16_t nak_limit = 0;
        HOST_DUBUG("outTransfer: addr: 0x%2.2x ep: 0x%2.2x nbytes: 0x%4.4x data: 0x%p\r\n", addr, ep, nbytes, data);

        uint8_t rcode = SetAddress(addr, ep, &pep, nak_limit);
        HOST_DUBUG("outTransfer: SetAddress 0x%2.2x\r\n", rcode);
        if(!rcode)
                rcode = OutTransfer(pep, nak_limit, nbytes, data);
        return rcode;
};

uint8_t UHS_USB_HOST_BASE::inTransfer(uint8_t addr, uint8_t ep, uint16_t *nbytesptr, uint8_t* data) {
        UHS_EpInfo *pep = NULL;
        uint16_t nak_limit = 0;

        uint8_t rcode = SetAddress(addr, ep, &pep, nak_limit);

        //        if(rcode) {
        //                USBTRACE3("(USB::InTransfer) SetAddress Failed ", rcode, 0x81);
        //                USBTRACE3("(USB::InTransfer) addr requested ", addr, 0x81);
        //                USBTRACE3("(USB::InTransfer) ep requested ", ep, 0x81);
        //                return rcode;
        //        }
        if(!rcode)
                rcode = InTransfer(pep, nak_limit, nbytesptr, data);
        return rcode;

}

uint8_t UHS_USB_HOST_BASE::initDescrStream(ENUMERATION_INFO *ei, USB_CONFIGURATION_DESCRIPTOR *ucd, UHS_EpInfo *pep, uint8_t *data, uint16_t *left, uint16_t *read, uint8_t *offset) {
        if(!ei || !ucd) return UHS_HOST_ERROR_BAD_ARGUMENT;
        if(!pep) return UHS_HOST_ERROR_NULL_EPINFO;
        *left = ucd->wTotalLength;
        *read = 0;
        *offset = 1;
        uint8_t rcode;
        pep->maxPktSize = ei->bMaxPacketSize0;
        rcode = getone(pep, left, read, data, offset);
        return rcode;
}

uint8_t UHS_USB_HOST_BASE::getNextInterface(ENUMERATION_INFO *ei, UHS_EpInfo *pep, uint8_t data[], uint16_t *left, uint16_t *read, uint8_t *offset) {
        uint16_t remain;
        uint8_t ty;
        uint8_t rcode = UHS_HOST_ERROR_END_OF_STREAM;
        uint8_t *ptr;
        uint8_t epc = 0;
        ei->interface.numep = 0;
        ei->interface.klass = 0;
        ei->interface.subklass = 0;
        ei->interface.protocol = 0;
        while(*left + *read) {
                remain = data[*offset]; // bLength
                rcode = getone(pep, left, read, data, offset);
                if(rcode)
                        return rcode;
                ty = data[*offset]; // bDescriptorType
                HOST_DUBUG("bLength: %i ", remain);
                HOST_DUBUG("bDescriptorType: %2.2x\r\n", ty);
                remain--;
                if(ty == USB_DESCRIPTOR_INTERFACE) {
                        HOST_DUBUG("INTERFACE DESCRIPTOR FOUND\r\n");
                        ptr = (uint8_t *)(&(ei->interface.bInterfaceNumber));
                        for(int i = 0; i < 6; i++) {
                                rcode = getone(pep, left, read, data, offset);
                                if(rcode)
                                        return rcode;
                                *ptr = data[*offset];
                                ptr++;
                        }
                        rcode = getone(pep, left, read, data, offset);
                        if(rcode)
                                return rcode;
                        // Now at iInterface
                        // Get endpoints.
                        HOST_DUBUG("Getting %i endpoints\r\n", ei->interface.numep);
                        if(!ei->interface.numep) {
                                rcode = getone(pep, left, read, data, offset);
                                if(rcode)
                                        return rcode;
                        }
                        while(epc < ei->interface.numep) {
                                rcode = getone(pep, left, read, data, offset);
                                if(rcode) {
                                        HOST_DUBUG("ENDPOINT DESCRIPTOR DIED WAY EARLY\r\n");
                                        return rcode;
                                }
                                remain = data[*offset]; // bLength
                                rcode = getone(pep, left, read, data, offset);
                                if(rcode) {
                                        HOST_DUBUG("ENDPOINT DESCRIPTOR DIED EARLY\r\n");
                                        return rcode;
                                }
                                ty = data[*offset]; // bDescriptorType
                                HOST_DUBUG("bLength: %i ", remain);
                                HOST_DUBUG("bDescriptorType: %2.2x\r\n", ty);
                                remain -= 2;
                                if(ty == USB_DESCRIPTOR_ENDPOINT) {
                                        HOST_DUBUG("ENDPOINT DESCRIPTOR: %i\r\n", epc);
                                        ptr = (uint8_t *)(&(ei->interface.epInfo[epc].bEndpointAddress));
                                        for(unsigned int i = 0; i< sizeof (ENDPOINT_INFO); i++) {
                                                rcode = getone(pep, left, read, data, offset);
                                                if(rcode) {
                                                        HOST_DUBUG("ENDPOINT DESCRIPTOR DIED LATE\r\n");
                                                        return rcode;
                                                }
                                                *ptr = data[*offset];
                                                ptr++;
                                                remain--;
                                        }
                                        epc++;
                                        HOST_DUBUG("ENDPOINT DESCRIPTOR OK\r\n");
                                }
                                rcode = eat(pep, left, read, data, offset, &remain);
                                if(rcode) {
                                        HOST_DUBUG("ENDPOINT DESCRIPTOR DIED EATING\r\n");
                                        return rcode;
                                }
                                remain = 0;
                        }
                        remain = 1;
                        // queue ahead, but do not report if error.
                        eat(pep, left, read, data, offset, &remain);
                        HOST_DUBUG("ENDPOINT DESCRIPTORS FILLED\r\n");
                        return 0;
                } else {
                        rcode = eat(pep, left, read, data, offset, &remain);
                        if(rcode)
                                return rcode;
                }
                rcode = UHS_HOST_ERROR_END_OF_STREAM;
        }
        return rcode;
}

uint8_t UHS_USB_HOST_BASE::seekInterface(ENUMERATION_INFO *ei, uint16_t inf, USB_CONFIGURATION_DESCRIPTOR *ucd) {
        if(!ei || !ucd) return UHS_HOST_ERROR_BAD_ARGUMENT;
        uint8_t data[ei->bMaxPacketSize0];
        UHS_EpInfo *pep;
        pep = ctrlReqOpen(ei->address, UHS_bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, ei->currentconfig,
                USB_DESCRIPTOR_CONFIGURATION, 0x0000, ucd->wTotalLength, data);
        if(!pep) return UHS_HOST_ERROR_NULL_EPINFO;
        uint16_t left = ucd->wTotalLength;
        uint8_t cinf = 0;
        uint8_t ty;
        uint8_t epc = 0;
        uint16_t remain = ucd->bLength;
        uint16_t read = 0;
        uint8_t offset = remain;
        uint8_t *ptr;
        uint8_t rcode;
        ei->interface.numep = 0;
        ei->interface.klass = 0;
        ei->interface.subklass = 0;
        ei->interface.protocol = 0;
        pep->maxPktSize = ei->bMaxPacketSize0;

        rcode = getone(pep, &left, &read, data, &offset);
        if(rcode)
                return rcode;
        HOST_DUBUG("\r\nGetting interface: %i\r\n", inf);
        inf++;
        while(cinf != inf && (left + read)) {
                //HOST_DUBUG("getInterface: cinf: %i inf: %i left: %i read: %i offset: %i remain %i\r\n", cinf, inf, left, read, offset, remain);
                // Go past current descriptor
                HOST_DUBUG("Skip: %i\r\n", remain);
                rcode = eat(pep, &left, &read, data, &offset, &remain);
                if(rcode)
                        return rcode;
                remain = data[offset] - 1; // bLength
                rcode = getone(pep, &left, &read, data, &offset);
                if(rcode)
                        return rcode;
                ty = data[offset]; // bDescriptorType
                HOST_DUBUG("bLength: %i ", remain + 1);
                HOST_DUBUG("bDescriptorType: %2.2x\r\n", ty);
                if(ty == USB_DESCRIPTOR_INTERFACE) {
                        HOST_DUBUG("INTERFACE DESCRIPTOR: %i\r\n", cinf);
                        cinf++;
                        if(cinf == inf) {
                                // Get the interface descriptor information.
                                ptr = (uint8_t *)(&(ei->interface.bInterfaceNumber));
                                for(int i = 0; i < 6; i++) {
                                        rcode = getone(pep, &left, &read, data, &offset);
                                        if(rcode)
                                                return rcode;
                                        *ptr = data[offset];
                                        ptr++;
                                }
                                rcode = getone(pep, &left, &read, data, &offset);
                                if(rcode)
                                        return rcode;
                                // Now at iInterface
                                remain = 0;
                                // Get endpoints.
                                HOST_DUBUG("Getting %i endpoints\r\n", ei->interface.numep);
                                while(epc < ei->interface.numep) {
                                        rcode = getone(pep, &left, &read, data, &offset);
                                        if(rcode)
                                                return rcode;
                                        remain = data[offset] - 1; // bLength
                                        rcode = getone(pep, &left, &read, data, &offset);
                                        if(rcode)
                                                return rcode;
                                        ty = data[offset]; // bDescriptorType
                                        HOST_DUBUG("bLength: %i ", remain + 1);
                                        HOST_DUBUG("bDescriptorType: %2.2x\r\n", ty);
                                        if(ty == USB_DESCRIPTOR_ENDPOINT) {
                                                HOST_DUBUG("ENDPOINT DESCRIPTOR: %i\r\n", epc);
                                                ptr = (uint8_t *)(&(ei->interface.epInfo[epc].bEndpointAddress));
                                                for(unsigned int i = 0; i< sizeof (ENDPOINT_INFO); i++) {
                                                        rcode = getone(pep, &left, &read, data, &offset);
                                                        if(rcode)
                                                                return rcode;
                                                        *ptr = data[offset];
                                                        ptr++;
                                                }
                                                epc++;
                                                remain = 0;
                                        } else {
                                                rcode = eat(pep, &left, &read, data, &offset, &remain);
                                                if(rcode)
                                                        return rcode;
                                                remain = 0;
                                        }
                                }
                        }
                }
        }

        return ctrlReqClose(pep, UHS_bmREQ_GET_DESCR, left, ei->bMaxPacketSize0, data);
}

uint8_t UHS_USB_HOST_BASE::getone(UHS_EpInfo *pep, uint16_t *left, uint16_t *read, uint8_t *dataptr, uint8_t *offset) {
        uint8_t rcode = 0;
        *offset += 1;
        if(*offset < *read) {
                return 0;
        } else if(*left > 0) {
                // uint16_t num = *left;
                uint16_t num = pep->maxPktSize;
                if(num > *left) num = *left;
                *offset = 0;
                rcode = ctrlReqRead(pep, left, read, num, dataptr);
                if(rcode == 0) {
                        if(*read == 0) {
                                rcode = UHS_HOST_ERROR_END_OF_STREAM;
                        } else if(*read < num) *left = 0;
                }
        } else {
                rcode = UHS_HOST_ERROR_END_OF_STREAM;
        }
        return rcode;
}

uint8_t UHS_USB_HOST_BASE::eat(UHS_EpInfo *pep, uint16_t *left, uint16_t *read, uint8_t *dataptr, uint8_t *offset, uint16_t *yum) {
        uint8_t rcode;
        HOST_DUBUG("eating %i\r\n", *yum);
        while(*yum) {
                *yum -= 1;
                rcode = getone(pep, left, read, dataptr, offset);
                if(rcode) break;
        }
        return rcode;
}

uint8_t UHS_USB_HOST_BASE::ctrlReq(uint8_t addr, uint8_t bmReqType, uint8_t bRequest, uint8_t wValLo, uint8_t wValHi,
        uint16_t wInd, uint16_t total, uint16_t nbytes, uint8_t* dataptr) {
        //bool direction = bmReqType & 0x80; //request direction, IN or OUT
        uint8_t rcode = 0;

        //        Serial.println("");
        UHS_EpInfo *pep = ctrlReqOpen(addr, bmReqType, bRequest, wValLo, wValHi, wInd, total, dataptr);
        if(!pep) {
                //                Serial.println("No pep");
                return UHS_HOST_ERROR_NULL_EPINFO;
        }
        //        Serial.println("Opened");
        uint16_t left = total;
        if(dataptr != NULL) //data stage, if present
        {
                if((bmReqType & 0x80) == 0x80) //IN transfer
                {

                        while(left) {
                                // Bytes read into buffer
                                uint16_t read = nbytes;
                                rcode = ctrlReqRead(pep, &left, &read, nbytes, dataptr);

                                if(rcode) {
                                        return rcode;
                                }

                                if(read < nbytes)
                                        break;
                        }
                } else //OUT transfer
                {
                        rcode = OutTransfer(pep, 0, nbytes, dataptr);
                }
                if(rcode) {
                        //return error
                        return ( rcode);
                }
        }

        //        Serial.println("Close Phase");
        //        Serial.flush();
        // Status stage
        rcode = ctrlReqClose(pep, bmReqType, left, nbytes, dataptr);
        //        Serial.println("Closed");
        return rcode;
}

uint8_t UHS_USB_HOST_BASE::EPClearHalt(uint8_t addr, uint8_t ep) {
        return ctrlReq(addr, USB_SETUP_HOST_TO_DEVICE | USB_SETUP_TYPE_STANDARD | USB_SETUP_RECIPIENT_ENDPOINT, USB_REQUEST_CLEAR_FEATURE, USB_FEATURE_ENDPOINT_HALT, 0, ep, 0, 0, NULL);
}

uint8_t UHS_USB_HOST_BASE::TestInterface(ENUMERATION_INFO *ei) {

        uint8_t devConfigIndex;
        uint8_t rcode = 0;
        HOST_DUBUG("TestInterface VID:%4.4x PID:%4.4x Class:%2.2x Subclass:%2.2x Protocol %2.2x\r\n", ei->vid, ei->pid, ei->klass, ei->subklass, ei->protocol);
        HOST_DUBUG("Interface data: Class:%2.2x Subclass:%2.2x Protocol %2.2x, number of endpoints %i\r\n", ei->interface.klass, ei->interface.subklass, ei->interface.subklass, ei->interface.numep);
        HOST_DUBUG("Parent: %2.2x, bAddress: %2.2x\r\n", ei->parent, ei->address);
        for(devConfigIndex = 0; devConfigIndex < UHS_HOST_MAX_INTERFACE_DRIVERS; devConfigIndex++) {
                if(!devConfig[devConfigIndex]) {
                        HOST_DUBUG("No driver at index %i\r\n", devConfigIndex);
                        continue; // no driver
                }
                if(devConfig[devConfigIndex]->bAddress) {
                        HOST_DUBUG("Driver %i is already consumed @ %2.2x\r\n", devConfigIndex, devConfig[devConfigIndex]->bAddress);
                        continue; // consumed
                }

                if(devConfig[devConfigIndex]->OKtoEnumerate(ei)) {
                        HOST_DUBUG("Driver %i supports this interface\r\n", devConfigIndex);
                        break;
                }
        }
        if(devConfigIndex == UHS_HOST_MAX_INTERFACE_DRIVERS) {
                rcode = UHS_HOST_ERROR_DEVICE_NOT_SUPPORTED;
#if defined(UHS_HID_LOADED)
                // Check HID here, if it is, then lie
                if(ei->klass == UHS_USB_CLASS_HID) {
                        devConfigIndex = UHS_HID_INDEX; // for debugging, otherwise this has no use.
                        rcode = 0;
                }
#endif
        }
        if(!rcode) HOST_DUBUG("Driver %i can be used for this interface\r\n", devConfigIndex);
        else HOST_DUBUG("No driver for this interface.\r\n");
        return rcode;
};

uint8_t UHS_USB_HOST_BASE::enumerateInterface(ENUMERATION_INFO *ei) {
        uint8_t devConfigIndex;

        HOST_DUBUG("AttemptConfig: parent = %i, port = %i\r\n", ei->parent, ei->port);

#if defined(UHS_HID_LOADED)
        // Check HID here, if it is, then lie
        if(ei->klass == UHS_USB_CLASS_HID || ei->interface.klass == UHS_USB_CLASS_HID) {
                UHS_HID_SetUSBInterface(this, ENUMERATION_INFO * ei);
                devConfigIndex = UHS_HID_INDEX;
        } else
#endif
                for(devConfigIndex = 0; devConfigIndex < UHS_HOST_MAX_INTERFACE_DRIVERS; devConfigIndex++) {
                        if(!devConfig[devConfigIndex]) {
                                HOST_DUBUG("No driver at index %i\r\n", devConfigIndex);
                                continue; // no driver
                        }
                        if(devConfig[devConfigIndex]->bAddress) {
                                HOST_DUBUG("Driver %i is already consumed @ %2.2x\r\n", devConfigIndex, devConfig[devConfigIndex]->bAddress);
                                continue; // consumed
                        }

                        if(devConfig[devConfigIndex]->OKtoEnumerate(ei)) {
                                HOST_DUBUG("Driver %i supports this interface\r\n", devConfigIndex);
                                if(!devConfig[devConfigIndex]->SetInterface(ei)) break;
                                else devConfigIndex = UHS_HOST_MAX_INTERFACE_DRIVERS;
                        }
                }
        return devConfigIndex;
};


////////////////////////////////////////////////////////////////////////////////
// Vendor Specific Interface Class
////////////////////////////////////////////////////////////////////////////////

/**
 *
 * @param ei Enumeration information
 * @return true if this interface driver can handle this interface description
 */
bool UHS_NI UHS_VSI::OKtoEnumerate(ENUMERATION_INFO *ei) {
        return (
                (ei->subklass == UHS_USB_CLASS_VENDOR_SPECIFIC) ||
                (ei->interface.subklass == UHS_USB_CLASS_VENDOR_SPECIFIC)
                );
}

/**
 * Copy the entire ENUMERATION_INFO structure
 * @param ei Enumeration information
 * @return 0
 */
uint8_t UHS_NI UHS_VSI::SetInterface(ENUMERATION_INFO *ei) {
        bNumEP = 1;
        bAddress = ei->address;

        eInfo.address = ei->address;
        eInfo.bMaxPacketSize0 = ei->bMaxPacketSize0;
        eInfo.currentconfig = ei->currentconfig;
        eInfo.interface.bAlternateSetting = ei->interface.bAlternateSetting;
        eInfo.interface.bInterfaceNumber = ei->interface.bInterfaceNumber;
        eInfo.interface.numep = ei->interface.numep;
        eInfo.interface.protocol = ei->interface.protocol;
        eInfo.interface.subklass = ei->interface.subklass;
        eInfo.klass = ei->klass;
        eInfo.parent = ei->parent;
        eInfo.pid = ei->pid;
        eInfo.port = ei->port;
        eInfo.protocol = ei->protocol;
        eInfo.subklass = ei->subklass;
        eInfo.vid = ei->vid;
        for(uint8_t i = 0; i < eInfo.interface.numep; i++) {
                eInfo.interface.epInfo[i].bEndpointAddress = ei->interface.epInfo[i].bEndpointAddress;
                eInfo.interface.epInfo[i].bInterval = ei->interface.epInfo[i].bInterval;
                eInfo.interface.epInfo[i].bmAttributes = ei->interface.epInfo[i].bmAttributes;
                eInfo.interface.epInfo[i].wMaxPacketSize = ei->interface.epInfo[i].wMaxPacketSize;
        }
        return 0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


#if 0

/* TO-DO: Move this silliness to a NONE driver.
 * When we have a generic NONE driver we can:
 *  o Extract ALL device information to help users with a new device.
 *  o Use an unknown device from a sketch, kind of like usblib does.
 *    This will aid in making more drivers in a faster way.
 */
uint8_t UHS_USB_HOST_BASE::DefaultAddressing(uint8_t parent, uint8_t port, bool lowspeed) {
        uint8_t rcode;
        UHS_Device *p0 = NULL, *p = NULL;

        // Get pointer to pseudo device with address 0 assigned
        p0 = addrPool.GetUsbDevicePtr(0);

        if(!p0)
                return UHS_HOST_ERROR_NO_ADDRESS_IN_POOL;

        if(!p0->epinfo)
                return UHS_HOST_ERROR_NULL_EPINFO;

        p0->lowspeed = (lowspeed) ? true : false;

        // Allocate new address according to device class
        uint8_t bAddress = addrPool.AllocAddress(parent, false, port);

        if(!bAddress)
                return UHS_HOST_ERROR_ADDRESS_POOL_FULL;

        p = addrPool.GetUsbDevicePtr(bAddress);

        if(!p)
                return UHS_HOST_ERROR_NO_ADDRESS_IN_POOL;

        p->lowspeed = lowspeed;

        // Assign new address to the device
        rcode = setAddr(0, bAddress);

        if(rcode) {
                addrPool.FreeAddress(bAddress);
                bAddress = 0;
                return rcode;
        }
        return 0;
}
#endif









// TO-DO: Endpoint copy and bNumEP set method to reduce driver code size even more!

#else
#error "Never include UHS_host_INLINE.h, include UHS_host.h instead"
#endif
