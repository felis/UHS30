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

#if defined(UHS_KINETIS_FS_HOST_H) && !defined(UHS_KINETIS_FS_HOST_LOADED)
#define UHS_KINETIS_FS_HOST_LOADED

static UHS_KINETIS_FS_HOST *_UHS_KINETIS_THIS_;

static void UHS_NI call_ISR_kinetis(void) {
        _UHS_KINETIS_THIS_->ISRTask();
}

/**
 *  Probe bus to determine device presence and speed,
 *  then switch host to detected speed.
 * NOT USED
 */
void UHS_NI UHS_KINETIS_FS_HOST::busprobe(void) {
        uint8_t bus_sample;

        //Get J,K status
        // bus_sample = YOUR CODE HERE
        // kinetis only shows the J, K or SE0 states, as far as I know there's no way to get SE1
        // low speed is indicated by a J state after attach
        bus_sample = (USB0_CTL & 0xC0); // bit 7 has J state, bit 6 has SE0 state

        //printf("\r\nUSB0_CTL: %x \r\n", USB0_CTL);
        //printf("\r\nbus_sample: %x \r\n", bus_sample);

        HOST_DEBUG("bus_sample: %x ", bus_sample);
        // 0xC0 disconnected
        // 0x80 low speed
        // 0x40 disconnected
        // 0x00 full speed
        switch(bus_sample) { //start full-speed or low-speed host
                case(UHS_KINETIS_FS_bmJSTATUS): // full speed
                        HOST_DEBUG("full speed\r\n");
                        USB0_OTGCTL &= ~(USB_OTGCTL_DPLOW | USB_OTGCTL_DMLOW); // disable D+ and D- pulldowns
                        USB0_INTEN &= ~USB_INTEN_ATTACHEN;
                        USB0_ADDR = 0;
                        USB0_ENDPT0 &= ~USB_ENDPT_HOSTWOHUB; // no hub present, communicate directly with device
                        USB0_SOFTHLD = 0x4A; // set to 0x4A for 64 byte transfers, 0x12 for 8-byte, 0x1A=16-bytes

                        usb_host_speed = 1;
                        USB0_CTL |= USB_CTL_USBENSOFEN; // start SOF generation
                        vbusState = UHS_KINETIS_FS_FSHOST;
                        break;
                case(UHS_KINETIS_FS_bmKSTATUS): // low speed
                        HOST_DEBUG("low speed\r\n");
                        USB0_OTGCTL &= ~(USB_OTGCTL_DPLOW | USB_OTGCTL_DMLOW); // disable D+ and D- pulldowns
                        USB0_INTEN &= ~USB_INTEN_ATTACHEN;
                        USB0_ADDR = USB_ADDR_LSEN; // low speed enable, address 0
                        USB0_ENDPT0 |= USB_ENDPT_HOSTWOHUB; // no hub present, communicate directly with device
                        USB0_SOFTHLD = 0x4A; // set to 0x4A for 64 byte transfers, 0x12 for 8-byte, 0x1A=16-bytes
                        usb_host_speed = 0;
                        USB0_CTL |= USB_CTL_USBENSOFEN; // start SOF generation
                        vbusState = UHS_KINETIS_FS_LSHOST;
                        break;
                case(UHS_KINETIS_FS_bmSE0): //disconnected state
                        HOST_DEBUG("disconnected\r\n");
                        vbusState = UHS_KINETIS_FS_SE0;
                        USB0_OTGCTL = USB_OTGCTL_DPLOW | USB_OTGCTL_DMLOW; // enable D+ and D- pulldowns
                        USB0_ADDR = 0;
                        USB0_CTL &= ~USB_CTL_USBENSOFEN;
                        USB0_INTEN |= USB_INTEN_ATTACHEN;
                        break;
                case(UHS_KINETIS_FS_bmSE1): // second disconnected state
                        HOST_DEBUG("disconnected2\r\n");
                        vbusState = UHS_KINETIS_FS_SE1;
                        USB0_OTGCTL = USB_OTGCTL_DPLOW | USB_OTGCTL_DMLOW; // enable D+ and D- pulldowns
                        USB0_ADDR = 0;
                        USB0_CTL &= ~USB_CTL_USBENSOFEN;
                        USB0_INTEN |= USB_INTEN_ATTACHEN;
                        break;
        }
}

void UHS_NI UHS_KINETIS_FS_HOST::VBUS_changed(void) {
        /* modify USB task state because Vbus changed or unknown */
        uint8_t speed = 1;
        // printf("\r\n\r\n\r\n\r\nSTATE %2.2x -> ", usb_task_state);
        switch(vbusState) {
                case UHS_KINETIS_FS_LSHOST: // Low speed

                        speed = 0;
                        //intentional fallthrough
                case UHS_KINETIS_FS_FSHOST: // Full speed
                        // Start device initialization if we are not initializing
                        // Resets to the device cause an IRQ
                        ReleaseChildren();
                        timer_countdown = 0;
                        sof_countdown = 0;
                        usb_task_state = UHS_USB_HOST_STATE_DEBOUNCE;
                        break;
                case UHS_KINETIS_FS_SE1: //illegal state
                        ReleaseChildren();
                        timer_countdown = 0;
                        sof_countdown = 0;
                        usb_task_state = UHS_USB_HOST_STATE_ILLEGAL;
                        break;
                case UHS_KINETIS_FS_SE0: //disconnected
                default:
                        ReleaseChildren();
                        timer_countdown = 0;
                        sof_countdown = 0;
                        usb_task_state = UHS_USB_HOST_STATE_IDLE;
                        USB0_INTEN |= USB_INTEN_ATTACHEN;
                        break;
        }
        // printf("0x%2.2x\r\n\r\n\r\n\r\n", usb_task_state);
        usb_host_speed = speed;
        return;
};

/**
 * Bottom half of the ISR task
 */
void UHS_NI UHS_KINETIS_FS_HOST::ISRbottom(void) {
        uint8_t x;
        if(condet) {
                VBUS_changed();
                noInterrupts();
                condet = false;
                interrupts();
        }

        switch(usb_task_state) {
                case UHS_USB_HOST_STATE_INITIALIZE: // initial state
                        //printf("ISRbottom, UHS_USB_HOST_STATE_INITIALIZE\r\n");
                        // if an attach happens we will detect it in the isr
                        // update usb_task_state and check speed (so we replace busprobe and VBUS_changed funcs)
                        break;
                case UHS_USB_HOST_STATE_DEBOUNCE:
                        //settle time for just attached device
                        //printf("ISRbottom, UHS_USB_HOST_STATE_DEBOUNCE\r\n");
                        sof_countdown = UHS_HOST_DEBOUNCE_DELAY_MS;
                        usb_task_state = UHS_USB_HOST_STATE_DEBOUNCE_NOT_COMPLETE;
                        break;
                case UHS_USB_HOST_STATE_DEBOUNCE_NOT_COMPLETE:
                        //printf("ISRbottom, UHS_USB_HOST_STATE_DEBOUNCE_NOT_COMPLETE\r\n");
                        usb_task_state = UHS_USB_HOST_STATE_RESET_DEVICE;
                        break;

                case UHS_USB_HOST_STATE_RESET_DEVICE:
                        //printf("ISRbottom, UHS_USB_HOST_STATE_RESET_DEVICE\r\n");
                        busevent = true;
                        usb_task_state = UHS_USB_HOST_STATE_RESET_NOT_COMPLETE;
                        //issue bus reset
                        USB0_CTL |= USB_CTL_RESET;
                        timer_countdown = 20;
                        // sof_countdown = 20; // use the timer
                        break;
                case UHS_USB_HOST_STATE_RESET_NOT_COMPLETE:
                        //printf("ISRbottom, UHS_USB_HOST_STATE_RESET_NOT_COMPLETE\r\n");
                        USB0_CTL &= ~USB_CTL_RESET; // stop bus reset
                        USB0_CTL |= USB_CTL_USBENSOFEN; // start SOF generation
                        usb_task_state = UHS_USB_HOST_STATE_WAIT_BUS_READY;
                        sof_countdown = 1;
                        // We delay two extra ms to ensure that at least one SOF has been sent.
                        // This trick is performed by just moving to the next state.
                        break;
                case UHS_USB_HOST_STATE_WAIT_BUS_READY:
                        //printf("ISRbottom, UHS_USB_HOST_STATE_WAIT_BUS_READY\r\n");
                        usb_task_state = UHS_USB_HOST_STATE_CONFIGURING;
                        break; // don't fall through

                case UHS_USB_HOST_STATE_CONFIGURING:
                        HOST_DEBUG("ISRbottom, UHS_USB_HOST_STATE_CONFIGURING\r\n");
                        usb_task_state = UHS_USB_HOST_STATE_CHECK;
                        x = Configuring(0, 1, usb_host_speed);
                        if(usb_task_state == UHS_USB_HOST_STATE_CHECK) {
                                if(x) {
                                        if(x == UHS_HOST_ERROR_JERR) {
                                                usb_task_state = UHS_USB_HOST_STATE_IDLE;
                                        } else if(x != UHS_HOST_ERROR_DEVICE_INIT_INCOMPLETE) {
                                                usb_error = x;
                                                usb_task_state = UHS_USB_HOST_STATE_ERROR;
                                        }
                                } else
                                        usb_task_state = UHS_USB_HOST_STATE_CONFIGURING_DONE;
                        }
                        break;
                case UHS_USB_HOST_STATE_CONFIGURING_DONE:
                        HOST_DEBUG("ISRbottom, UHS_USB_HOST_STATE_CONFIGURING_DONE\r\n");
                        usb_task_state = UHS_USB_HOST_STATE_RUNNING;
                        break;
                case UHS_USB_HOST_STATE_CHECK:
                        break;
                case UHS_USB_HOST_STATE_RUNNING:
                        //printf("ISRbottom, UHS_USB_HOST_STATE_RUNNING\r\n");
                        Poll_Others();
                        for(x = 0; (usb_task_state == UHS_USB_HOST_STATE_RUNNING) && (x < UHS_HOST_MAX_INTERFACE_DRIVERS); x++) {
                                if(devConfig[x]) {
                                        if(devConfig[x]->bPollEnable) devConfig[x]->Poll();
                                }
                        }
                        // fall thru
                default:
                        //printf("ISRbottom, default\r\n");
                        // Do nothing
                        break;
        } // switch(usb_task_state)
        if(condet) {
                VBUS_changed();
                noInterrupts();
                condet = false;
                interrupts();
        }
        usb_task_polling_disabled--;
}

/**
 * Top half of the ISR. This is what services the hardware, and calls the bottom half as-needed.
 *
 * SPECIAL NOTES:
 *      1: After an error, set isrError to zero.
 *
 *      2: If DMA bandwidth is not enough, UHS_HOST_ERROR_NAK is returned.
 *              Drivers that have NAK processing know to retry.
 */

void UHS_NI UHS_KINETIS_FS_HOST::Task(void) {
}

void UHS_NI UHS_KINETIS_FS_HOST::ISRTask(void) {

        counted = false;
        uint8_t stat;

        uint8_t status = USB0_ISTAT & USB0_INTEN; // mask off bits that did not cause the interrupt.
        uint8_t HW_CLEAR = 0;
        uint8_t otg_status = USB0_OTGISTAT; // otg interrupts

        if((status & USB_ISTAT_STALL)) { // stall
                KINETIS_HOST_DEBUG("ISR: stall\r\n");
                uint8_t err = USB0_ERRSTAT;
                USB0_ERRSTAT = err; // clear errors that we don't care about
                newError = true;
                isrError = UHS_HOST_ERROR_STALL;
                HW_CLEAR |= USB_ISTAT_STALL;
        } else {
                if((status & USB_ISTAT_ERROR)) { // error
                        uint8_t err = USB0_ERRSTAT;
                        USB0_ERRSTAT = err; // clear error
                        //USBTRACE2X("ISR: error: ", err); USBTRACE("\r\n");
                        //printf("\aISR: error: %2.2x \r\n", err);
                        isrError = 0;
                        if(err & USB_ERRSTAT_PIDERR) {
                                KINETIS_HOST_DEBUG("PIDERR\r\n");
                                isrError = UHS_HOST_ERROR_WRONGPID; // Received wrong Packet ID
                        } else if(err & USB_ERRSTAT_CRC16) {
                                KINETIS_HOST_DEBUG("CRCERR\r\n");
                                isrError = UHS_HOST_ERROR_CRC; // USB CRC was incorrect
                        } else if(err & (USB_ERRSTAT_DFN8 | USB_ERRSTAT_BTSERR)) {
                                KINETIS_HOST_DEBUG("STUFFERR\r\n");
                              isrError = UHS_HOST_ERROR_BABBLE; // Line noise/unexpected data
                        } else if(err & USB_ERRSTAT_DMAERR) {
                                KINETIS_HOST_DEBUG("DMAERR\r\n");
                                isrError = UHS_HOST_ERROR_DMA; // Data was truncated. Device sent too much.
                        } else if(err & USB_ERRSTAT_BTOERR) {
                                KINETIS_HOST_DEBUG("BTOERR\r\n");
                                // Device didn't respond in time. It is most likely unplugged.
                                KINETIS_HOST_DEBUG("\r\n\r\n*** ISR sees error 0x10, Device unplugged? ***\r\n\r\n");
                                isrError = UHS_HOST_ERROR_UNPLUGGED; //UHS_HOST_ERROR_NAK;
                        } else if(err & USB_ERRSTAT_CRC5EOF) {
                                // Not enough time alloted.
                                KINETIS_HOST_DEBUG("CRC5EOF\r\n");
                                isrError = UHS_HOST_ERROR_BABBLE;
                        }

                        // update to signal non-isr code what happened
                        if(isrError) newError = true;
                        HW_CLEAR |= USB_ISTAT_ERROR;
                }
        }

        if((status & USB_ISTAT_TOKDNE)) { // Token done
                stat = USB0_STAT;
                bdt_t *p_newToken = UHS_KINETIS_FS_stat2bufferdescriptor(stat);
                b_newToken.desc = p_newToken->desc;
                b_newToken.addr = p_newToken->addr;
                uint32_t pid = UHS_KINETIS_FS_BDT_PID(b_newToken.desc);
                isrPid = pid;

                // update to signal non-isr code what happened
                switch(pid & 0x0f) {
                        case 0x00:
                                KINETIS_HOST_DEBUG("ERROR_TIMEOUT TX=%i\r\n", stat & USB_STAT_TX);
                                isrError = UHS_HOST_ERROR_TIMEOUT;
                                break;
                        case 0x0a:
                                isrError = UHS_HOST_ERROR_NAK;
                                break;
                        case 0x0e:
                                KINETIS_HOST_DEBUG("ERROR_STALL TX=%i\r\n", stat & USB_STAT_TX);
                                isrError = UHS_HOST_ERROR_STALL;
                                break;
                        case 0x0f:
                                // Data Error
                                KINETIS_HOST_DEBUG("Data Error TX=%i\r\n", stat & USB_STAT_TX);
                                if(!isrError || isrError != UHS_HOST_ERROR_DMA) {
                                        KINETIS_HOST_DEBUG("\r\nMEMORY LATENCY PROBLEM? %2.2x\r\n", isrError);
                                        //isrError = UHS_HOST_ERROR_MEM_LAT; // Error was due to memory latency.
                                        isrError = UHS_HOST_ERROR_NAK;
                                } else {
                                        KINETIS_HOST_DEBUG("Other Data Error %2.2x TX=%i\r\n", isrError, stat & USB_STAT_TX);
                                        isrError = UHS_HOST_ERROR_NAK;
                                }
                                break;
                        default:
                                //case 0x02:
                                //case 0x03:
                                //case 0x0b:
                                // Completed good packets
                                break;
                }

                if(isrError) {
                        newError = true;
                } else {
                        newToken = true;
                }
#if DEBUG_PRINTF_EXTRA_HUGE_UHS_HOST
                uint8_t count = b_newToken.desc >> 16;
                uint8_t *buf = (uint8_t *)b_newToken.addr;
                HOST_DEBUG("ISR: TOKDNE. Pid: %lx", pid);
                HOST_DEBUG(", count: %x", count);
                if(count) {
                        HOST_DEBUG(". Data: %lx", *(uint32_t *)(buf));
                }
                HOST_DEBUG("\r\n");

#endif
                HW_CLEAR |= USB_ISTAT_TOKDNE;
                // return;
        }

        if(status & USB_ISTAT_USBRST) { // bus reset
                HOST_DEBUG("ISR: reset\r\n");
                condet = true; // ALWAYS
                busprobe();
                HW_CLEAR |= USB_ISTAT_USBRST;
        }

        if((status & USB_ISTAT_ATTACH)) { // device attached to the usb
                HOST_DEBUG("ISR: Attach\r\n");
                condet = true;
                busprobe();
                HW_CLEAR |= USB_ISTAT_ATTACH;
        }

        if((status & USB_ISTAT_SOFTOK)) {
                // mark the time in microseconds + 1 millisecond.
                // this is used to know how soon the _next_ SOF will be.
                sof_mark = (long)(micros() + 1000);
                if(sof_countdown) {
                        sof_countdown--;
                        counted = true;
                }
                sofevent = false;
                HW_CLEAR |= USB_ISTAT_SOFTOK;
        }

#if 0
        if((status & USB_ISTAT_SLEEP)) { // sleep
                HOST_DEBUG("ISR: sleep\r\n");
                // serial_print("sleep\n");
                HW_CLEAR |= USB_ISTAT_SLEEP;
        }

        if((status & USB_ISTAT_RESUME)) { // resume
                HOST_DEBUG("ISR: resume\r\n");
                // serial_print("resume\n");
                HW_CLEAR |= USB_ISTAT_RESUME;
        }
#endif
        if((otg_status & USB_OTGISTAT_ONEMSEC)) { // 1 ms timer
                if(timer_countdown) {
                        timer_countdown--;
                        counted = true;
                }
                USB0_OTGISTAT = USB_OTGISTAT_ONEMSEC;
        }

        USB0_ISTAT = HW_CLEAR; // ack all USB0_ISTAT flags here at the same time.

        DDSB();
        if(!timer_countdown && !sof_countdown && !counted && !usb_task_polling_disabled) {
                usb_task_polling_disabled++;

                // ARM uses SWI for bottom half
                // NVIC disallows reentrant IRQ
                exec_SWI(this);
        }
}

/**
 * Setup UHS_EpInfo structure
 *
 * @param addr USB device address
 * @param ep Endpoint
 * @param ppep pointer to the pointer to a valid UHS_EpInfo structure
 * @param nak_limit how many NAKs before aborting
 * @return 0 on success
 */
uint8_t UHS_NI UHS_KINETIS_FS_HOST::SetAddress(uint8_t addr, uint8_t ep, UHS_EpInfo **ppep, uint16_t & nak_limit) {
        // it's almost a copy of the MAX code

        UHS_Device *p = addrPool.GetUsbDevicePtr(addr);

        if(!p) {
                return UHS_HOST_ERROR_NO_ADDRESS_IN_POOL;
        }


        if(!p->epinfo) {
                return UHS_HOST_ERROR_NULL_EPINFO;
        }

        *ppep = getEpInfoEntry(addr, ep);

        if(!*ppep) {
                return UHS_HOST_ERROR_NO_ENDPOINT_IN_TABLE;
        }

        nak_limit = (0x0001UL << (((*ppep)->bmNakPower > UHS_USB_NAK_MAX_POWER) ? UHS_USB_NAK_MAX_POWER : (*ppep)->bmNakPower));
        nak_limit--;

        //USBTRACE2("\r\nAddress: ", addr);
        //USBTRACE2(" EP: ", ep);
        //USBTRACE2(" NAK Power: ", (*ppep)->bmNakPower);
        //USBTRACE2(" NAK Limit: ", nak_limit);
        //USBTRACE2(" RETRY DISABLE: ", ((nak_limit > 2U) ? 0 : 1));
        //USBTRACE("\r\n");

        //HOST_DEBUG("\r\nAddress: %2.2x. EP: %2.2x, NAK Power: %2.2x, NAK Limit: %u\r\n", addr, ep, (*ppep)->bmNakPower, nak_limit);
        // address and low speed enable
        USB0_ADDR = addr | ((p->speed) ? 0 : USB_ADDR_LSEN);

        //Serial.print("\r\nMode: ");
        //Serial.println(mode, HEX);
        //Serial.print("\r\nLS: ");
        //Serial.println(p->speed, HEX);

        // Disable automatic retries for NAK < 2, Set hub for low-speed device
        USB0_ENDPT0 = USB_ENDPT_EPRXEN | USB_ENDPT_EPTXEN | USB_ENDPT_EPHSHK |
                //((nak_limit != 1U) ? 0 : USB_ENDPT_RETRYDIS) |
                ((nak_limit > 1U) ? 0 : USB_ENDPT_RETRYDIS) |
                ((p->speed) ? 0 : USB_ENDPT_HOSTWOHUB);

        // set USB0_SOFTHLD depending on the maxPktSize
        // NOTE: This should actually be set per-packet, however this works.
        uint8_t mps = (*ppep)->maxPktSize;
        if(mps < 9) {
                USB0_SOFTHLD = (18 + 16) *2; // 18;
        } else if(mps < 17) {
                USB0_SOFTHLD = (26 + 16)*2; // 26;
        } else if(mps < 33) {
                USB0_SOFTHLD = (42 + 16)*2; // 42;
        } else {
                USB0_SOFTHLD = (74 + 16)*2; // 74;
        }

        return UHS_HOST_ERROR_NONE;
}

/**
 * Send the actual packet.
 * Assumes peripheral address is set and relevant buffer is loaded/empty
 * If NAK, tries to re-send up to nak_limit times
 * If nak_limit == 0, do not count NAKs, exit after timeout
 * If bus timeout, re-sends up to USB_RETRY_LIMIT times
 *
 * @param token
 * @param ep Endpoint
 * @param nak_limit how many NAKs before aborting
 * @return codes 0x00-0x0f are HRSLT(0x00 being success), 0xff means timeout
 */
uint8_t UHS_NI UHS_KINETIS_FS_HOST::dispatchPkt(uint8_t token, uint8_t ep, uint16_t nak_limit) {
        // Short packets cause problems with bandwidth on a hub.
        // Limit the bandwidth to 2 125uS frames so we do not overload
        // the transaction translator on the hub, other devices could fail too.
        // Better would be to find how many TT's are on the hub, though...
        uint8_t rcode;

        //HOST_DEBUG("dispatchPkt: token %x, ep: %i, nak_limit: %i \r\n", token, ep, nak_limit);

        rcode = UHS_HOST_ERROR_TIMEOUT;
        newError = false;
        newToken = false;
        isrError = UHS_HOST_ERROR_NONE;

        // Only do this if a hub is connected.
        // The code below this will eat some of the time.
        if(hub_present) {
                while((long)(last_mark + 205) >= (long)(micros()));
        }
        // SPEC: 12000 bits per 1mS frame +/- 16 bits
        // that should make 12 bits per uS, but...
        // we need to add how long until launch happens, 2uS seems OK
        // We also are not checking if we are transmitting at low speed.
        // TO-DO: Use a different divisor if low speed.
        uint32_t tft = 2+((USB0_SOFTHLD) / 12LU);
        if((long)sof_mark <= (long)(micros() + tft)) {
                // wait for SOF
                noInterrupts();
                sofevent = true;
                DDSB();
                interrupts();
                while(sofevent && !condet);
        }

        if(!condet) {
                last_mark = micros(); // hopefully now in sync with the SOF
                // need to find out what is hogging RAM access
                USB0_TOKEN = token | ep; //  Dispatch to endpoint.
                // wait for transfer completion
                while(!condet) {
                        //__asm__ volatile ("wfi");
                        if(newError || newToken) {
                                if(newError) {
                                        newError = false;
                                        rcode = isrError;
                                        isrError = 0;
                                        //printf("New rcode: 0x%2.2x\r\n", rcode);
                                } // error
                                if(newToken) { // token completed
                                        if(rcode == UHS_HOST_ERROR_TIMEOUT) rcode = UHS_HOST_ERROR_NONE;
                                        newToken = false;
                                } // token completed
                                //USBTRACE2("isrPid: ", isrPid);
                                //USBTRACE2(", rcode: ", rcode);
                                //if(isrPid || rcode) {
                                //        printf("dispatchPkt: token %x, ep: %i, nak_limit: %i \r\n", token, ep, nak_limit);
                                //        printf("isrPid: 0x%8.8x, rcode: 0x%2.2x\r\n", isrPid, rcode);
                                //}
                                if(nak_limit && rcode == UHS_HOST_ERROR_NAK) {
                                        nak_limit--;
                                        if(nak_limit) continue; // retry
                                }
                                break;
                        }
                }
        }

        if(condet) {
                rcode = UHS_HOST_ERROR_UNPLUGGED;
        }
        return (rcode);
}

/**
 * Transmit a packet
 *
 * @param pep pointer to a valid UHS_EpInfo structure
 * @param nak_limit how many NAKs before aborting
 * @param nbytes number of bytes of data to send
 * @param data pointer to data buffer
 * @return 0 on success
 */
uint8_t UHS_NI UHS_KINETIS_FS_HOST::OutTransfer(UHS_EpInfo *pep, uint16_t nak_limit, uint16_t nbytes, uint8_t * data) {

        uint8_t rcode = UHS_HOST_ERROR_NONE;
        uint16_t bytes_tosend;
        uint16_t bytes_left = nbytes;

        uint8_t maxpktsize = pep->maxPktSize;
        uint8_t ep = pep->epAddr;

        USBTRACE2("OutTransfer: Sending nbytes: ", nbytes);

        if(maxpktsize < 1 || maxpktsize > 64)
                rcode = UHS_HOST_ERROR_BAD_MAX_PACKET_SIZE;

        uint8_t* p_buffer = data; // local copy
        ep0_tx_data_toggle = pep->bmSndToggle;

        while(bytes_left && !rcode) {
                HOST_DEBUG(", maxpktsize: %i, bytes_left: %i\r\n", maxpktsize, bytes_left);

                bytes_tosend = (bytes_left >= maxpktsize) ? maxpktsize : bytes_left;
                endpoint0_transmit(p_buffer, bytes_tosend); // setup internal buffer
                rcode = dispatchPkt(UHS_KINETIS_FS_TOKEN_DATA_OUT, ep, nak_limit); //dispatch packet to ep
                if(rcode == UHS_HOST_ERROR_MEM_LAT) {
                        // something we need to do here, but what?
                        // datasheet isn't clear, just says that these are transient
                        // Never see these either.
                        // printf("\r\nOutTransfer MEMLAT\r\n");
                        rcode = UHS_HOST_ERROR_NAK;
                        break;
                }
                bytes_left -= bytes_tosend;
                p_buffer += bytes_tosend;
        }//while( bytes_left...

        pep->bmSndToggle = ep0_tx_data_toggle;
        return (rcode); //should be 0 in all cases
}

/**
 * Receive a packet
 *
 * @param pep pointer to a valid UHS_EpInfo structure
 * @param nak_limit how many NAKs before aborting
 * @param nbytesptr pointer to maximum number of bytes of data to receive
 * @param data pointer to data buffer
 * @return 0 on success
 */
uint8_t UHS_NI UHS_KINETIS_FS_HOST::InTransfer(UHS_EpInfo *pep, uint16_t nak_limit, uint16_t *nbytesptr, uint8_t * data) {
        uint8_t rcode = 0;
        uint8_t pktsize;
        uint16_t nbytes = *nbytesptr;
        uint8_t maxpktsize = pep->maxPktSize;
        uint8_t ep = pep->epAddr;

        //USBTRACE2("Requesting ", nbytes);
        //USBTRACE2("maxpktsize: ", maxpktsize);

        *nbytesptr = 0;

        // data0/1 toggle is automatic
        ep0_rx_data_toggle = pep->bmRcvToggle;

        uint32_t datalen = 0;
        uint8_t* p_buffer = data;
        datalen = maxpktsize;

        while(1) {
                //digitalWriteFast(2, HIGH);
                if(datalen + *nbytesptr > nbytes) { // get less than maxpktsize if we don't need a full maxpktsize
                        datalen = nbytes - *nbytesptr;
                }
                HOST_DEBUG("datalen: %lu \r\n", datalen);
                endpoint0_receive(data_in_buf, datalen); // setup internal buffer
                rcode = dispatchPkt(UHS_KINETIS_FS_TOKEN_DATA_IN, ep, nak_limit); //dispatch packet
                //digitalWriteFast(2, LOW);
                pktsize = b_newToken.desc >> 16; // how many bytes we actually got
                HOST_DEBUG("pktsize: %i \r\n", pktsize);
                if(rcode == UHS_HOST_ERROR_MEM_LAT) {
                        // something we need to do here, but what?
                        // datasheet isn't clear, just says that these are transient
                        printf("\r\nInTransfer MEMLAT\r\n");
                        //rcode = UHS_HOST_ERROR_NAK;
                }
                if(rcode) {
                        // DMA error: we got more data than expected.
                        // This means that the device's mackpaxketsize is actually larger than maxpktsize
                        // copy the packet that we received and return so that Configuring can deal with it.
                        //if(rcode == UHS_HOST_ERROR_DMA) {
                        //        memcpy(p_buffer, data_in_buf, pktsize); // copy packet into buffer
                        //        *nbytesptr += pktsize; // add to the number of bytes read
                        //        break;
                        //}
                        //HOST_DEBUG(">>>>hrUNDEF>>>> InTransfer Problem! dispatchPkt ", rcode);
                        break; //should be 0, indicating ACK. Else return error code.
                }

#if ENABLE_UHS_DEBUGGING
                uint8_t i = 0;
                HOST_DEBUG("-----Data packet: ");
                for(i = 0; i < pktsize; i++) {
                        HOST_DEBUG("%02x ", data_in_buf[i]);
                }
                HOST_DEBUG("\r\n");
#endif
                if(pktsize + *nbytesptr > nbytes) { // adjust pktsize if we don't need all the bytes we just got
                        pktsize = nbytes - *nbytesptr;
                }

                memcpy(p_buffer, data_in_buf, pktsize); // copy packet into buffer
                p_buffer += pktsize; // advance pointer for next packet
                *nbytesptr += pktsize; // add to the number if bytes read

                /* The transfer is complete under two conditions:           */
                /* 1. The device sent a short packet (L.T. maxPacketSize)   */
                /* 2. 'nbytes' have been transferred.                       */
                if((pktsize < maxpktsize) || (*nbytesptr >= nbytes)) {
                        pep->bmRcvToggle = ep0_rx_data_toggle;
                        rcode = 0;
                        break;
                }
        }
        //digitalWriteFast(2, LOW);
        return (rcode);
}

/**
 * Open a control request
 *
 * @param addr USB device address
 * @param bmReqType request type bits
 * @param bRequest request bits
 * @param wValLo low byte
 * @param wValHi high byte
 * @param wInd index
 * @param total total or maximum number of bytes of data
 * @param dataptr pointer to data buffer
 * @return NULL pointer on fail, pointer to a valid UHS_EpInfo structure on success
 */
UHS_EpInfo * UHS_NI UHS_KINETIS_FS_HOST::ctrlReqOpen(uint8_t addr, uint64_t Request, uint8_t *dataptr) {
        //serial_print("setup phase\n");

        uint8_t rcode;

        UHS_EpInfo *pep = NULL;
        uint16_t nak_limit = 0;
        //HOST_DEBUG("ctrlReqOpen: addr: 0x%2.2x bmReqType: 0x%2.2x bRequest: 0x%2.2x\r\nctrlReqOpen: wValLo: 0x%2.2x  wValHi: 0x%2.2x wInd: 0x%4.4x total: 0x%4.4x dataptr: %p\r\n", addr, bmReqType, bRequest, wValLo, wValHi, wInd, total, dataptr);
        rcode = SetAddress(addr, 0, &pep, nak_limit);

        if(!rcode) {

                // const uint8_t *data = NULL;
                uint32_t datalen = 0;

                datalen = 8;
                //data = setup_command_buffer;
                ep0_tx_data_toggle = UHS_KINETIS_FS_DATA0; // setup always uses DATA0
                endpoint0_transmit(&Request, datalen); // setup internal buffer

                rcode = dispatchPkt(UHS_KINETIS_FS_TOKEN_SETUP, 0, nak_limit); //dispatch packet

                if(!rcode) {
                        if(dataptr != NULL) {
                                // data phase begins with DATA1 after setup
                                if(((Request) /* bmReqType */ & 0x80) == 0x80) {
                                        pep->bmRcvToggle = UHS_KINETIS_FS_DATA1; //bmRCVTOG1;
                                } else {
                                        pep->bmSndToggle = UHS_KINETIS_FS_DATA1; //bmSNDTOG1;
                                }
                        }
                } else {
                        // USBTRACE(">>>>>>>>>>>> ctrlReqOpen Failed <<<<<<<<<<<<<< \r\n");
                        // USBTRACE2("rcode: ", rcode);
                        // USBTRACE2(", bmReqType: ", bmReqType);
                        // USBTRACE2(", bRequest: ", bRequest);
                        // USBTRACE(">>>>>>>>>>>> ctrlReqOpen Failed <<<<<<<<<<<<<< \r\n");
                        pep = NULL;
                }
        }

        return pep;
}

/**
 * Read data from the control request
 *
 * @param pep pointer to a valid UHS_EpInfo structure
 * @param left pointer to how many bytes are left in the virtual stream
 * @param read pointer to how many bytes to read into the data buffer
 * @param nbytes maximum number of bytes of data
 * @param dataptr pointer to data buffer
 * @return 0 on success
 */
uint8_t UHS_NI UHS_KINETIS_FS_HOST::ctrlReqRead(UHS_EpInfo *pep, uint16_t *left, uint16_t *read, uint16_t nbytes, uint8_t * dataptr) {
        *read = 0;
        uint8_t rcode = 0;
        uint16_t nak_limit = 0;
        HOST_DEBUG("ctrlReqRead left: %i\r\n", *left);
        if(*left) {
                *read = nbytes;
                rcode = InTransfer(pep, nak_limit, read, dataptr);

                if(rcode) {
                        HOST_DEBUG("ctrlReqRead ERROR: %2.2x, left: %i, read %i\r\n", rcode, *left, *read);
                } else {
                        *left -= *read;
                        HOST_DEBUG("ctrlReqRead left: %i, read %i\r\n", *left, *read);
                }
        }
        return rcode;
}

/**
 * Close a control request, drains any remaining data if reading.
 *
 * @param pep pointer to a valid UHS_EpInfo structure
 * @param bmReqType request type bits
 * @param left pointer to how many bytes are left in the virtual stream
 * @param nbytes maximum number of bytes of data
 * @param dataptr pointer to data buffer
 * @return 0 on success
 */
uint8_t UHS_NI UHS_KINETIS_FS_HOST::ctrlReqClose(UHS_EpInfo *pep, uint8_t bmReqType, uint16_t left, uint16_t nbytes, uint8_t * dataptr) {
        uint8_t rcode = 0;

        //HOST_DEBUG("Inside ctrlReqClose. bmReqType: %x, left: %x, nbytes: %x \r\n", bmReqType, left, nbytes);

        if(((bmReqType & 0x80) == 0x80) && pep && left && dataptr) {
                //Serial.println("Drain");
                HOST_DEBUG("ctrlReqClose Sinking %i\r\n", left);
                // If reading, sink the rest of the data.
                while(left) {
                        uint16_t read = nbytes;
                        rcode = InTransfer(pep, 0, &read, dataptr);
                        if(rcode) break;
                        left -= read;
                        if(read < nbytes) break;
                }
                //        } else {
                //                Serial.println("Nothing to drain");
        }
        if(!rcode) {
                //               Serial.println("Dispatching");
                if(((bmReqType & 0x80) == 0x80)) {
                        ep0_tx_data_toggle = UHS_KINETIS_FS_DATA1; // make sure we use DATA1 for status phase
                        endpoint0_transmit(NULL, 0); // setup internal buffer, 0 bytes
                        rcode = dispatchPkt(UHS_KINETIS_FS_TOKEN_DATA_OUT, 0, 0);
                } else {
                        ep0_rx_data_toggle = UHS_KINETIS_FS_DATA1; // make sure we use DATA1 for status phase
                        endpoint0_receive(NULL, 0); // setup internal buffer, 0 bytes
                        rcode = dispatchPkt(UHS_KINETIS_FS_TOKEN_DATA_IN, 0, 0);
                }
                //        } else {
                //                Serial.println("Bypassed Dispatch");
        }
        return rcode;
}

/**
 * Initialize USB hardware, turn on VBUS
 *
 * @param mseconds Delay energizing VBUS after mseconds, A value of INT16_MIN means no delay.
 * @return 0 on success, -1 on error
 */
int16_t UHS_NI UHS_KINETIS_FS_HOST::Init(int16_t mseconds) {
#if defined(UHS_USB_VBUS)
        pinMode(UHS_USB_VBUS, OUTPUT);
        //digitalWriteFast(UHS_USB_VBUS, LOW);
#endif
        vbusPower(vbus_off);
        /*
         * Nybble       Master
         * 0            Core ICODE
         * 1            Core ISYS
         * 2            DMA
         * 3            USB OTG
         *
         * Register     Slave
         * 0            Flash
         * 1            SRAM
         * 2            Preph bridge 0
         * 3            Preph bridge 1
         * 4            ??????? Undocumented, but present.
         */


        // Optimize AXBS
        //AXBS_PRS0 = 0x00000321U; // Flash
        //AXBS_PRS1 = 0x00000321U; // SRAM
        //AXBS_PRS2 = 0x00000321U; // br0
        //AXBS_PRS3 = 0x00000321U; // br1
        //AXBS_PRS4 = 0x00000321U; // ???
        //AXBS_PRS5 = 0x76543210U;
        //AXBS_PRS6 = 0x76543210U;
        //AXBS_PRS7 = 0x76543210U;

        //AXBS_CRS0 = 0x00000003U; // Flash
        //AXBS_CRS1 = 0x00000003U; // SRAM
        //AXBS_CRS2 = 0x00000003U; // br0
        //AXBS_CRS3 = 0x00000003U; // br1
        //AXBS_CRS4 = 0x00000003U; // ???
        //AXBS_CRS5 = 0x00U;
        //AXBS_CRS6 = 0x00U;
        //AXBS_CRS7 = 0x00U;

        // Access to these cause fault.
        // Why? Not available?
        //AXBS_MGPCR0 = 0x00U;
        //AXBS_MGPCR1 = 0x00U;
        //AXBS_MGPCR2 = 0x00U;
        //AXBS_MGPCR3 = 0x00U;
        //AXBS_MGPCR4 = 0x00U;
        //AXBS_MGPCR5 = 0x00U;
        //AXBS_MGPCR6 = 0x00U;
        //AXBS_MGPCR7 = 0x00U;

        // Change SRAM[LU] priority to prefer backdoor (DMA/USB) over CPU.
        // 0=RR, 1=SRR, 2=CPU, 3=DMA
        //MCM_CR = MCM_CR_SRAMLAP(1) | MCM_CR_SRAMUAP(1);


        Init_dyn_SWI();
        //UHS_printf_HELPER_init();
        _UHS_KINETIS_THIS_ = this;

        // assume 48 MHz clock already running
        // SIM - enable clock
        SIM_SCGC4 |= SIM_SCGC4_USBOTG;
#ifdef HAS_KINETIS_MPU
        MPU_RGDAAC0 |= 0x03000000;
#endif
#if F_CPU == 180000000 || F_CPU == 216000000
        // if using IRC48M, turn on the USB clock recovery hardware
        USB0_CLK_RECOVER_IRC_EN = USB_CLK_RECOVER_IRC_EN_IRC_EN | USB_CLK_RECOVER_IRC_EN_REG_EN;
        USB0_CLK_RECOVER_CTRL = USB_CLK_RECOVER_CTRL_CLOCK_RECOVER_EN |
                USB_CLK_RECOVER_CTRL_RESTART_IFRTRIM_EN;
#endif

        // reset USB module
        USB0_USBTRC0 = USB_USBTRC_USBRESET;
        while((USB0_USBTRC0 & USB_USBTRC_USBRESET) != 0); // wait for reset to end

        // set desc table base addr
        USB0_BDTPAGE1 = ((uint32_t)table) >> 8;
        USB0_BDTPAGE2 = ((uint32_t)table) >> 16;
        USB0_BDTPAGE3 = ((uint32_t)table) >> 24;

        // initialize BDT toggle bits
        USB0_CTL = USB_CTL_ODDRST;
        ep0_tx_bdt_bank = 0;
        ep0_rx_bdt_bank = 0;
        ep0_tx_data_toggle = UHS_KINETIS_FS_BDT_DATA0;
        ep0_rx_data_toggle = UHS_KINETIS_FS_BDT_DATA0;

        // setup buffers
        // to-do: re-use (steal) Paul's allocated buffers, but ONLY if using his stuff.

        table[UHS_KINETIS_FS_index(0, UHS_KINETIS_FS_RX, UHS_KINETIS_FS_EVEN)].desc = UHS_KINETIS_FS_BDT_DESC(UHS_KINETIS_FS_EP0_SIZE, 0);
        table[UHS_KINETIS_FS_index(0, UHS_KINETIS_FS_RX, UHS_KINETIS_FS_EVEN)].addr = ep0_rx0_buf;

        table[UHS_KINETIS_FS_index(0, UHS_KINETIS_FS_TX, UHS_KINETIS_FS_EVEN)].desc = UHS_KINETIS_FS_BDT_DESC(UHS_KINETIS_FS_EP0_SIZE, 0);
        table[UHS_KINETIS_FS_index(0, UHS_KINETIS_FS_TX, UHS_KINETIS_FS_EVEN)].addr = ep0_tx0_buf;

        // This would allow ping-pong buffers, but my code is so fast, we do not need them. 8-)
        //table[UHS_KINETIS_FS_index(0, UHS_KINETIS_FS_RX, UHS_KINETIS_FS_ODD)].desc = UHS_KINETIS_FS_BDT_DESC(UHS_KINETIS_FS_EP0_SIZE, 0);
        //table[UHS_KINETIS_FS_index(0, UHS_KINETIS_FS_RX, UHS_KINETIS_FS_ODD)].addr = ep0_rx1_buf;
        //table[UHS_KINETIS_FS_index(0, UHS_KINETIS_FS_TX, UHS_KINETIS_FS_ODD)].desc = UHS_KINETIS_FS_BDT_DESC(UHS_KINETIS_FS_EP0_SIZE, 0);
        //table[UHS_KINETIS_FS_index(0, UHS_KINETIS_FS_TX, UHS_KINETIS_FS_ODD)].addr = ep0_tx1_buf;

        // clear interrupts
        USB0_ERRSTAT = 0xFF;
        USB0_ISTAT = 0xFF;
        USB0_OTGISTAT = 0xFF;

        USB0_USBFRMADJUST = 0x00;
        // enable USB
        //USB0_USBTRC0 |= 0x40; // undocumented bit
        USB0_USBCTRL = 0;

        USB0_OTGCTL = USB_OTGCTL_DPLOW | USB_OTGCTL_DMLOW; // enable D+ and D- pulldowns, disable D+ pullup

        USB0_INTEN = USB_INTEN_ATTACHEN |
                USB_INTEN_TOKDNEEN |
                USB_INTEN_STALLEN |
                USB_INTEN_ERROREN |
                USB_INTEN_SOFTOKEN |
                USB_INTEN_USBRSTEN |
                USB_INTEN_SLEEPEN; // enable attach interrupt, token done, stall, error and sleep
        USB0_OTGICR = USB_OTGICR_ONEMSECEN; // activate 1ms timer interrupt
        USB0_ERREN = 0xFF; // enable all error interrupts

        // switch isr for USB
        NVIC_DISABLE_IRQ(IRQ_USBOTG);
        NVIC_SET_PRIORITY(IRQ_USBOTG, 112);
        _VectorsRam[IRQ_USBOTG + 16] = call_ISR_kinetis;
        DDSB();
        NVIC_ENABLE_IRQ(IRQ_USBOTG);

        // set address to zero during enumeration
        USB0_ADDR = 0;

        USB0_CTL = USB_CTL_HOSTMODEEN; // host mode enable
        // USB0_CTL &= ~USB_CTL_USBENSOFEN; // disable SOF generation to avoid noise until we detect attach

        vbusPower(vbus_on);
//#if defined(UHS_USB_VBUS)
//        digitalWriteFast(UHS_USB_VBUS, HIGH);
//#endif
        return 0;
}

void UHS_NI UHS_KINETIS_FS_HOST::endpoint0_transmit(const void *data, uint32_t len) {
#if ENABLE_UHS_DEBUGGING && DEBUG_PRINTF_EXTRA_HUGE_UHS_HOST
        if(len > 0) {
                uint32_t i = 0;
                uint8_t *real_data = (uint8_t *)data;
                HOST_DEBUG("tx0: ");
                for(i = 0; i < len; i++) {
                        HOST_DEBUG("%x ", *(real_data + i));
                }
                HOST_DEBUG(", %lx\r\n", len);
        }
#endif
        last_count = len;
        last_address = (void *)data;
        last_tx = true;

        table[UHS_KINETIS_FS_index(0, UHS_KINETIS_FS_TX, ep0_tx_bdt_bank)].addr = (void *)data;
        table[UHS_KINETIS_FS_index(0, UHS_KINETIS_FS_TX, ep0_tx_bdt_bank)].desc = UHS_KINETIS_FS_BDT_DESC(len, ep0_tx_data_toggle);
        ep0_tx_data_toggle ^= 1;
        ep0_tx_bdt_bank ^= 1;
}

void UHS_NI UHS_KINETIS_FS_HOST::endpoint0_receive(const void *data, uint32_t len) {
        //#if ENABLE_UHS_DEBUGGING
#if 1
        HOST_DEBUG("endpoint0_receive: %lx", len);
#endif

        last_count = len;
        last_address = (void *)data;
        last_tx = false;

        table[UHS_KINETIS_FS_index(0, UHS_KINETIS_FS_RX, ep0_rx_bdt_bank)].addr = (void *)data;
        table[UHS_KINETIS_FS_index(0, UHS_KINETIS_FS_RX, ep0_rx_bdt_bank)].desc = UHS_KINETIS_FS_BDT_DESC(len, ep0_rx_data_toggle);
        ep0_rx_data_toggle ^= 1;
        ep0_rx_bdt_bank ^= 1;
}

#else
#error "Never include UHS_KINETIS_FS_HOST_INLINE.h, include UHS_host.h instead"
#endif
