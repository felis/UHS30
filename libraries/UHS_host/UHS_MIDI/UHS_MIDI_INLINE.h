/* Copyright (c) 2017 Yuuichi Akagawa

This software may be distributed and modified under the terms of the GNU
General Public License version 2 (GPL2) as published by the Free Software
Foundation and appearing in the file GPL2.TXT included in the packaging of
this file. Please note that GPL2 Section 2[b] requires that all works based
on this software must also be made publicly available under the terms of
the GPL2 ("Copyleft").

 */

/* USB MIDI interface */
/*
 * Note: This driver is support for MIDI Streaming class only.
 *       If your MIDI Controller is not work, probably you needs its vendor specific driver.
 */
#if defined(LOAD_UHS_MIDI) && defined(__UHS_MIDI_H__) && !defined(UHS_MIDI_LOADED)
#define UHS_MIDI_LOADED

UHS_NI UHS_MIDI::UHS_MIDI(UHS_USB_HOST_BASE *p) {
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
bool UHS_NI UHS_MIDI::OKtoEnumerate(ENUMERATION_INFO *ei) {
        MIDI_HOST_DEBUG("MIDI: checking vid %04x, pid %04x, iface class %i, subclass %i\r\n",
                ei->vid, ei->pid, ei->interface.klass, ei->interface.subklass);
        return ((ei->interface.klass == UHS_USB_CLASS_AUDIO && ei->interface.subklass == UHS_USB_SUBCLASS_MIDISTREAMING));
}

/**
 * Resets interface driver to unused state
 */
void UHS_NI UHS_MIDI::DriverDefaults(void) {
        pUsb->DeviceDefaults(MIDI_MAX_ENDPOINTS, this);
        ready = false;

        // initialize endpoint data structures
        for(uint8_t i = 0; i < MIDI_MAX_ENDPOINTS; i++) {
                epInfo[i].epAddr = (i == epDataInIndex) ? 0x81 : 0x01;
                epInfo[i].maxPktSize = 0;
                epInfo[i].bmSndToggle = 0;
                epInfo[i].bmRcvToggle = 0;
                epInfo[i].bmNakPower = (i) ? UHS_USB_NAK_NOWAIT : UHS_USB_NAK_MAX_POWER;
        }
}

uint8_t UHS_NI UHS_MIDI::SetInterface(ENUMERATION_INFO *ei) {
        MIDI_HOST_DEBUG("MIDI: SetInterface\r\n");
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
        MIDI_HOST_DEBUG("MIDI: found %i endpoints\r\n", bNumEP);
        bAddress = ei->address;
        epInfo[0].epAddr = 0;
        epInfo[0].maxPktSize = ei->bMaxPacketSize0;
        epInfo[0].bmNakPower = UHS_USB_NAK_MAX_POWER;
        bIface = ei->interface.bInterfaceNumber;
        bConfNum = ei->currentconfig;
        vid = ei->vid;
        pid = ei->pid;
        MIDI_HOST_DEBUG("MIDI: address %i, config %i, iface %i with %i endpoints\r\n", bAddress, bConfNum, bIface, bNumEP);
        return 0;
}

/**
 *
 * @return 0 for success
 */
uint8_t UHS_NI UHS_MIDI::Start(void) {
        uint8_t rcode;
        MIDI_HOST_DEBUG("MIDI: Start\r\n");

        MIDI_HOST_DEBUG("MIDI: device detected\r\n");
        // Assign epInfo to epinfo pointer - this time all 3 endpoints
        rcode = pUsb->setEpInfoEntry(bAddress, bNumEP, epInfo);
        if(!rcode) {
                MIDI_HOST_DEBUG("MIDI: EpInfoEntry OK\r\n");
                // Set Configuration Value
                rcode = pUsb->setConf(bAddress, bConfNum);
                if(!rcode) {
                        MIDI_HOST_DEBUG("MIDI: Configuration OK\r\n");
                        rcode = OnStart();
                        if(!rcode) {
                                MIDI_HOST_DEBUG("MIDI: OnStart OK\r\n");
                                qNextPollTime = millis() + qPollRate;
                                bPollEnable = true;
                                ready = true;
                        } else {
                                MIDI_HOST_DEBUG("MIDI: OnStart FAIL\r\n");
                        }
                } else {
                        MIDI_HOST_DEBUG("MIDI: Configuration FAIL\r\n");
                }
        } else {
                MIDI_HOST_DEBUG("MIDI: EpInfoEntry FAIL\r\n");
        }
        if(rcode) Release();
        readPtr = 0;
        return rcode;
}

/**
 * user hook
 */
void UHS_NI UHS_MIDI::Poll(void) {
        if((long)(millis() - qNextPollTime) >= 0L) {
                OnPoll(); // Do user stuff...
                qNextPollTime = millis() + qPollRate;
        }
}

/* Receive data from MIDI device */
uint8_t UHS_NI UHS_MIDI::RecvData(uint16_t *bytes_rcvd, uint8_t *dataptr) {
        *bytes_rcvd = (uint16_t)epInfo[epDataInIndex].maxPktSize;
        if(!bAddress) return UHS_HOST_ERROR_UNPLUGGED;
        pUsb->DisablePoll();
        uint8_t rv = pUsb->inTransfer(bAddress, epInfo[epDataInIndex].epAddr, bytes_rcvd, dataptr);
        if(rv && rv != UHS_HOST_ERROR_NAK) Release();
        pUsb->EnablePoll();
        if(*bytes_rcvd < (MIDI_EVENT_PACKET_SIZE - 4)) {
                dataptr[*bytes_rcvd] = '\0';
                dataptr[(*bytes_rcvd) + 1] = '\0';
        }
        return rv;
}

/* Receive data from MIDI device */
uint8_t UHS_NI UHS_MIDI::RecvData(uint8_t *outBuf, bool isRaw) {
        uint8_t rcode = 0; //return code
        uint16_t rcvd;

        if(bPollEnable == false) return 0;

        //Checking unprocessed message in buffer.
        if(readPtr != 0 && readPtr < MIDI_EVENT_PACKET_SIZE) {
                if(recvBuf[readPtr] == 0 && recvBuf[readPtr + 1] == 0) {
                        //no unprocessed message left in the buffer.
                } else {
                        goto RecvData_return_from_buffer;
                }
        }

        readPtr = 0;
        rcode = RecvData(&rcvd, recvBuf);
        if(rcode != 0) {
                return 0;
        }

        //if all data is zero, no valid data received.
        if(recvBuf[0] == 0 && recvBuf[1] == 0 && recvBuf[2] == 0 && recvBuf[3] == 0) {
                return 0;
        }

RecvData_return_from_buffer:
        uint8_t m;
        uint8_t cin = recvBuf[readPtr];
        if(isRaw == true) {
                *(outBuf++) = cin;
        }
        readPtr++;
        *(outBuf++) = m = recvBuf[readPtr++];
        *(outBuf++) = recvBuf[readPtr++];
        *(outBuf++) = recvBuf[readPtr++];
        return lookupMsgSize(m, cin);
}

/* Receive raw data from MIDI device */
uint8_t UHS_NI UHS_MIDI::RecvRawData(uint8_t *outBuf) {
        return RecvData(outBuf, true);
}

/* Send data to MIDI device */
uint8_t UHS_NI UHS_MIDI::SendData(uint8_t *dataptr, uint8_t nCable) {
        uint8_t buf[4];
        uint8_t msg;

        msg = dataptr[0];
        // SysEx long message ?
        if(msg == 0xf0) {
                return SendSysEx(dataptr, countSysExDataSize(dataptr), nCable);
        }

        buf[0] = (nCable << 4) | (msg >> 4);
        if(msg < 0xf0) msg = msg & 0xf0;


        //Building USB-MIDI Event Packets
        buf[1] = dataptr[0];
        buf[2] = dataptr[1];
        buf[3] = dataptr[2];

        switch(lookupMsgSize(msg)) {
                        //3 bytes message
                case 3:
                        if(msg == 0xf2) {//system common message(SPP)
                                buf[0] = (nCable << 4) | 3;
                        }
                        break;

                        //2 bytes message
                case 2:
                        if(msg == 0xf1 || msg == 0xf3) {//system common message(MTC/SongSelect)
                                buf[0] = (nCable << 4) | 2;
                        }
                        buf[3] = 0;
                        break;

                        //1 byte message
                case 1:
                default:
                        buf[2] = 0;
                        buf[3] = 0;
                        break;
        }
        if(!bAddress) return UHS_HOST_ERROR_UNPLUGGED;
        pUsb->DisablePoll();
        uint8_t rv = pUsb->outTransfer(bAddress, epInfo[epDataOutIndex].epAddr, 4, buf);
        if(rv && rv != UHS_HOST_ERROR_NAK) Release();
        pUsb->EnablePoll();
        return rv;
}

/* SysEx data size counter */
uint16_t UHS_NI UHS_MIDI::countSysExDataSize(uint8_t *dataptr) {
        unsigned int c = 1;

        if(*dataptr != 0xf0) { //not SysEx
                return 0;
        }

        //Search terminator(0xf7)
        while(*dataptr != 0xf7) {
                dataptr++;
                c++;

                //Limiter (default: 256 bytes)
                if(c > MIDI_MAX_SYSEX_SIZE) {
                        c = 0;
                        break;
                }
        }
        return c;
}

/* Send SysEx message to MIDI device */
uint8_t UHS_NI UHS_MIDI::SendSysEx(uint8_t *dataptr, uint16_t datasize, uint8_t nCable) {
        uint8_t buf[MIDI_EVENT_PACKET_SIZE];
        uint8_t rc = 0;
        uint16_t n = datasize;
        uint16_t pktSize = (n * 10 / 3 + 7) / 10 * 4; //Calculate total USB MIDI packet size
        uint8_t wptr = 0;
        uint8_t maxpkt = epInfo[epDataInIndex].maxPktSize;

        if(maxpkt > MIDI_EVENT_PACKET_SIZE) maxpkt = MIDI_EVENT_PACKET_SIZE;

        MIDI_HOST_DEBUG("SendSysEx:\r\t");
        MIDI_HOST_DEBUG(" Length:\t", datasize);
        MIDI_HOST_DEBUG(" Total pktSize:\t", pktSize);

        while(n > 0) {
                //Byte 0
                buf[wptr] = (nCable << 4) | 0x4; //x4 SysEx starts or continues

                switch(n) {
                        case 1:
                                buf[wptr++] = (nCable << 4) | 0x5; //x5 SysEx ends with following single byte.
                                buf[wptr++] = *(dataptr++);
                                buf[wptr++] = 0x00;
                                buf[wptr++] = 0x00;
                                n = n - 1;
                                break;
                        case 2:
                                buf[wptr++] = (nCable << 4) | 0x6; //x6 SysEx ends with following two bytes.
                                buf[wptr++] = *(dataptr++);
                                buf[wptr++] = *(dataptr++);
                                buf[wptr++] = 0x00;
                                n = n - 2;
                                break;
                        case 3:
                                buf[wptr] = (nCable << 4) | 0x7; //x7 SysEx ends with following three bytes.
                        default:
                                wptr++;
                                buf[wptr++] = *(dataptr++);
                                buf[wptr++] = *(dataptr++);
                                buf[wptr++] = *(dataptr++);
                                n = n - 3;
                                break;
                }

                if(wptr >= maxpkt || n == 0) { //Reach a maxPktSize or data end.
                        //                        USBTRACE2(" wptr:\t", wptr);
                        if(!bAddress) return UHS_HOST_ERROR_UNPLUGGED;
                        pUsb->DisablePoll();
                        rc = pUsb->outTransfer(bAddress, epInfo[epDataOutIndex].epAddr, wptr, buf);
                        pUsb->EnablePoll(); //ToDo
                        if(rc && rc != UHS_HOST_ERROR_NAK) {
                                Release();
                                break;
                        }
                        wptr = 0; //rewind data pointer
                }
        }
        return (rc);
}

/* Send raw data to MIDI device */
uint8_t UHS_NI UHS_MIDI::SendRawData(uint16_t bytes_send, uint8_t *dataptr) {
        if(!bAddress) return UHS_HOST_ERROR_UNPLUGGED;
        pUsb->DisablePoll();
        uint8_t rv = pUsb->outTransfer(bAddress, epInfo[epDataOutIndex].epAddr, bytes_send, dataptr);
        if(rv && rv != UHS_HOST_ERROR_NAK) Release();
        pUsb->EnablePoll();
        return rv;
}

uint8_t UHS_NI UHS_MIDI::extractSysExData(uint8_t *p, uint8_t *buf) {
        uint8_t rc = 0;
        uint8_t cin = *(p) & 0x0f;

        //SysEx message?
        if((cin & 0xc) != 4) return rc;

        switch(cin) {
                case 4:
                case 7:
                        *buf++ = *(p + 1);
                        *buf++ = *(p + 2);
                        *buf++ = *(p + 3);
                        rc = 3;
                        break;
                case 6:
                        *buf++ = *(p + 1);
                        *buf++ = *(p + 2);
                        rc = 2;
                        break;
                case 5:
                        *buf++ = *(p + 1);
                        rc = 1;
                        break;
                default:
                        break;
        }
        return (rc);
}
/* look up a MIDI message size from spec */
/*Return                                 */
/*  0 : undefined message                */

/*  0<: Vaild message size(1-3)          */
uint8_t UHS_NI UHS_MIDI::lookupMsgSize(uint8_t midiMsg, uint8_t cin) {
        uint8_t msgSize = 0;

        //SysEx message?
        cin = cin & 0x0f;
        if((cin & 0xc) == 4) {
                if(cin == 4 || cin == 7) return 3;
                if(cin == 6) return 2;
                if(cin == 5) return 1;
        }

        if(midiMsg < 0xf0) midiMsg &= 0xf0;
        switch(midiMsg) {
                        //3 bytes messages
                case 0xf2: //system common message(SPP)
                case 0x80: //Note off
                case 0x90: //Note on
                case 0xa0: //Poly KeyPress
                case 0xb0: //Control Change
                case 0xe0: //PitchBend Change
                        msgSize = 3;
                        break;

                        //2 bytes messages
                case 0xf1: //system common message(MTC)
                case 0xf3: //system common message(SongSelect)
                case 0xc0: //Program Change
                case 0xd0: //Channel Pressure
                        msgSize = 2;
                        break;

                        //1 byte messages
                case 0xf8: //system real-time message
                case 0xf9: //system real-time message
                case 0xfa: //system real-time message
                case 0xfb: //system real-time message
                case 0xfc: //system real-time message
                case 0xfe: //system real-time message
                case 0xff: //system real-time message
                        msgSize = 1;
                        break;

                        //undefine messages
                default:
                        break;
        }
        return msgSize;
}

#else
#error "Never include UHS_MIDI_INLINE.h, include UHS_host.h instead"
#endif
