/* Copyright (C) 2015 Circuits At Home, LTD. All rights reserved.

This software may be distributed and modified under the terms of the GNU
General Public License version 2 (GPL2) as published by the Free Software
Foundation and appearing in the file GPL2.TXT included in the packaging of
this file. Please note that GPL2 Section 2[b] requires that all works based
on this software must also be made publicly available under the terms of
the GPL2 ("Copyleft").

Contact information
-------------------

Circuits At Home, LTD
Web      :  www.circuitsathome.com
e-mail   :  support@circuitsathome.com
 */

//PIC32MX-specific

#if defined(UHS_PIC32_H) && !defined(UHS_PIC32_LOADED)
#define UHS_PIC32_LOADED

static UHS_PIC32 *ISR_pic32;

bdt_t UHS_PIC32::table[];

static void UHS_NI call_ISR_pic32(void) {
        ISR_pic32->ISRTask();
}

/**
 *  Probe bus to determine device presence and speed,
 *  then switch host to detected speed.
 * NOT USED
 */
void UHS_NI UHS_PIC32::busprobe(void) {
        uint8_t bus_sample;

        //Get J,K status
        // bus_sample = YOUR CODE HERE
        // PIC SIE only shows the J, K or SE0 states, as far as I know there's no way to get SE1
        // low speed is indicated by a J state after attach
        //continue
        //bus_sample = (USB0_CTL & 0xC0); // bit 7 has J state, bit 6 has SE0 state
        bus_sample = U1CON & 0xc0;
        
        //printf("\r\nU1CON: %x \r\n", U1CON);
        //printf("\r\nbus_sample: %x \r\n", bus_sample);

        HOST_DUBUG("bus_sample: %x", bus_sample);
        // 0xC0 disconnected
        // 0x80 low speed
        // 0x40 disconnected
        // 0x00 full speed
        switch(bus_sample) { //start full-speed or low-speed host
                case(UHS_PIC32_bmJSTATUS): // full speed
                        USBTRACE("full speed\r\n");
                        
                        U1IEbits.ATTACHIE = 1;  //attach interrupt enable
                        U1ADDR = 0;
                        U1EP0bits.LSPD = 0; //low speed direct connect enable. todo: see if this can be removed
                        U1SOF = 0x4a;
                        
                        //USB0_INTEN &= ~USB_INTEN_ATTACHEN;  //attach interrupt enable
                        //USB0_ADDR = 0;
                        //USB0_ENDPT0 &= ~USB_ENDPT_HOSTWOHUB; // no hub present, communicate directly with device
                        //USB0_SOFTHLD = 0x4A; // set to 0x4A for 64 byte transfers, 0x12 for 8-byte, 0x1A=16-bytes

                        islowspeed = false;
                        
                        U1CONbits.SOFEN = 1;    //start SOF generation
                        //USB0_CTL |= USB_CTL_USBENSOFEN; // start SOF generation
                        
                        vbusState = UHS_PIC32_FSHOST;
                        break;
                case(UHS_PIC32_bmKSTATUS): // low speed
                        USBTRACE("low speed\r\n");
                        U1IEbits.ATTACHIE = 1;
                        U1ADDR = 0;
                        U1EP0bits.LSPD = 1; //low speed direct connect enable. todo: see if this can be removed
                        U1SOF = 0x4a;

                        //USB0_ADDR = USB_ADDR_LSEN; // low speed enable, address 0
                        //USB0_ENDPT0 |= USB_ENDPT_HOSTWOHUB; // no hub present, communicate directly with device
                        //USB0_SOFTHLD = 0x4A; // set to 0x4A for 64 byte transfers, 0x12 for 8-byte, 0x1A=16-bytes

                        islowspeed = true;
                        
                        //USB0_CTL |= USB_CTL_USBENSOFEN; // start SOF generation
                        
                        U1CONbits.SOFEN = 1;    //start SOF generation
                        vbusState = UHS_PIC32_LSHOST;
                        break;
                case(UHS_PIC32_bmSE0): //disconnected state
                        USBTRACE("disconnected\r\n");
                        
                        
                        // Set D+ and D- low
                        //USB0_ADDR = 0;
                        //USB0_OTGCTL = USB_OTGCTL_DPLOW | USB_OTGCTL_DMLOW; // enable D+ and D- pulldowns
                        //USB0_CTL &= ~USB_CTL_USBENSOFEN;
                        //USB0_INTEN |= USB_INTEN_ATTACHEN;
                        
                        U1ADDR = 0;
                        U1OTGCONbits.DPPULDWN = 1;
                        U1OTGCONbits.DMPULDWN = 1;
                        U1CONbits.SOFEN = 0;    //stop SOF generation
                        U1IEbits.ATTACHIE = 1;  //attach interrupt enable
                        
                        vbusState = UHS_PIC32_SE0;
                        break;
                case(UHS_PIC32_bmSE1): // invalid state - shall never happen IRL
                        USBTRACE("disconnected2\r\n");
                        U1ADDR = 0;
                        U1OTGCONbits.DPPULDWN = 1;
                        U1OTGCONbits.DMPULDWN = 1;
                        U1CONbits.SOFEN = 0;    //stop SOF generation
                        U1IEbits.ATTACHIE = 1;  //attach interrupt enable

                        vbusState = UHS_PIC32_SE1;

                        break;
        }//end switch( bus_sample )
}

//  NOT USED
uint8_t UHS_NI UHS_PIC32::VBUS_changed(void) {
        /* modify USB task state because Vbus changed or unknown */
        uint8_t speed = 1;
        // printf("\r\n\r\n\r\n\r\nSTATE %2.2x -> ", usb_task_state);
        switch(vbusState) {
                case UHS_PIC32_LSHOST: // Low speed

                        speed = 0;
                        //intentional fallthrough
                case UHS_PIC32_FSHOST: // Full speed
                        // Start device initialization if we are not initializing
                        // Resets to the device cause an IRQ
                        // if((usb_task_state & UHS_USB_HOST_STATE_MASK) != UHS_USB_HOST_STATE_DETACHED) {
                        ReleaseChildren();
                        timer_countdown = 0;
                        sof_countdown = 0;
                        usb_task_state = UHS_USB_HOST_STATE_DEBOUNCE;
                        //}
                        break;
                case UHS_PIC32_SE1: //illegal state
                        ReleaseChildren();
                        timer_countdown = 0;
                        sof_countdown = 0;
                        usb_task_state = UHS_USB_HOST_STATE_ILLEGAL;
                        // USB0_INTEN |= USB_INTEN_ATTACHEN;
                        break;
                case UHS_PIC32_SE0: //disconnected
                default:
                        ReleaseChildren();
                        timer_countdown = 0;
                        sof_countdown = 0;
                        usb_task_state = UHS_USB_HOST_STATE_IDLE;
                        U1IEbits.ATTACHIE = 1;  //attach interrupt enable
                        break;
        }
        // printf("0x%2.2x\r\n\r\n\r\n\r\n", usb_task_state);
        return speed;
};

/**
 * Bottom half of the ISR task
 */
void UHS_NI UHS_PIC32::ISRbottom(void) {
        uint8_t x;
        if(condet) {
                islowspeed = (VBUS_changed() == 0);
                noInterrupts();
                condet = false;
                interrupts();
        }

        //printf("ISRbottom, usb_task_state: %x \r\n", (uint8_t)UHS_USB_HOST_STATE_INITIALIZE);

        switch(usb_task_state) {
                case UHS_USB_HOST_STATE_INITIALIZE: // initial state
                        //printf("ISRbottom, UHS_USB_HOST_STATE_INITIALIZE\r\n");

                        // if an attach happens we will detect it in the isr
                        // update usb_task_state and check speed (so we replace busprobe and VBUS_changed funcs)

                        //busprobe();
                        //islowspeed = (VBUS_changed() == 0);
                        break;
                case UHS_USB_HOST_STATE_DEBOUNCE:
                        //printf("ISRbottom, UHS_USB_HOST_STATE_DEBOUNCE\r\n");
                        sof_countdown = UHS_HOST_DEBOUNCE_DELAY_MS;
                        usb_task_state = UHS_USB_HOST_STATE_DEBOUNCE_NOT_COMPLETE;
                        break;
                case UHS_USB_HOST_STATE_DEBOUNCE_NOT_COMPLETE://settle time for just attached device
                        //printf("ISRbottom, UHS_USB_HOST_STATE_DEBOUNCE_NOT_COMPLETE\r\n");
                        usb_task_state = UHS_USB_HOST_STATE_RESET_DEVICE;
                        break;

                case UHS_USB_HOST_STATE_RESET_DEVICE:
                        //printf("ISRbottom, UHS_USB_HOST_STATE_RESET_DEVICE\r\n");
                        busevent = true;
                        usb_task_state = UHS_USB_HOST_STATE_RESET_NOT_COMPLETE;
                        //issue bus reset
                        //USB0_CTL |= USB_CTL_RESET;
                        U1CONbits.USBRST = 1;    
                        timer_countdown = 20;
                        // sof_countdown = 20; // use the timer
                        break;
                case UHS_USB_HOST_STATE_RESET_NOT_COMPLETE:
                        //printf("ISRbottom, UHS_USB_HOST_STATE_RESET_NOT_COMPLETE\r\n");
                        //USB0_CTL &= ~USB_CTL_RESET; // stop bus reset
                        //USB0_CTL |= USB_CTL_USBENSOFEN; // start SOF generation
                        U1CONbits.USBRST = 0;
                        U1CONbits.SOFEN = 1;
                        usb_task_state = UHS_USB_HOST_STATE_WAIT_BUS_READY;
                        // We delay two extra ms to ensure that at least one SOF has been sent.
                        // This trick is performed by just moving to the next state.
                        break;
                case UHS_USB_HOST_STATE_WAIT_BUS_READY:
                        //printf("ISRbottom, UHS_USB_HOST_STATE_WAIT_BUS_READY\r\n");
                        usb_task_state = UHS_USB_HOST_STATE_CONFIGURING;
                        break; // don't fall through

                case UHS_USB_HOST_STATE_CONFIGURING:
                        HOST_DUBUG("ISRbottom, UHS_USB_HOST_STATE_CONFIGURING\r\n");
                        usb_task_state = UHS_USB_HOST_STATE_CHECK;
                        x = Configuring(0, 0, islowspeed);
                        if(usb_task_state == UHS_USB_HOST_STATE_CHECK) {
                                if(x) {
                                        if(x == hrJERR) {
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
                        HOST_DUBUG("ISRbottom, UHS_USB_HOST_STATE_CONFIGURING_DONE\r\n");
                        usb_task_state = UHS_USB_HOST_STATE_RUNNING;
                        break;
                case UHS_USB_HOST_STATE_CHECK:
                        // Serial.println((uint32_t)__builtin_return_address(0),HEX);
                        break;
                case UHS_USB_HOST_STATE_RUNNING:
                        //printf("ISRbottom, UHS_USB_HOST_STATE_RUNNING\r\n");
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
        } // switch( usb_task_state )
        if(condet) {
                islowspeed = (VBUS_changed() == 0);
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
 *      2: If DMA bandwidth is not enough, hrNAK is returned.
 *              Drivers that have NAK processing know to retry.
 */

void UHS_NI UHS_PIC32::Task(void) {
}

void UHS_NI UHS_PIC32::ISRTask(void) {

        counted = false;
        uint8_t stat;

        //uint8_t status = USB0_ISTAT & USB0_INTEN; // mask off bits that did not cause the interrupt.
        uint8_t status = U1IR & U1IE;
        uint8_t HW_CLEAR = 0;
        //uint8_t otg_status = USB0_OTGISTAT; // otg interrupts
        uint8_t otg_status = U1OTGIR;
        if((status & _U1IR_STALLIF_MASK)) { // stall
                HOST_DUBUG("ISR: stall\r\n");
                uint8_t err = U1EIR;
                U1EIR = err; // clear errors that we don't care about
                newError = true;
                isrError = hrSTALL;
                HW_CLEAR |= _U1IR_STALLIF_MASK;
        } else {
                if((status & _U1IR_UERRIF_MASK)) { // error
                        uint8_t err = U1EIR; //error status register
                        U1EIR = err; // clear error
                        //USBTRACE2X("ISR: error: ", err); USBTRACE("\r\n");
                        //printf("\aISR: error: %2.2x \r\n", err);
                        isrError = 0;
                        if(err & _U1EIR_PIDEF_MASK) {
                                isrError = hrWRONGPID; // Received wrong Packet ID
                        } else if(err & _U1EIR_CRC16EF_MASK) {
                                isrError = hrCRCERR; // USB CRC was incorrect
                        } else if(err & (_U1EIR_DFN8EF_MASK | _U1EIR_BTSEF_MASK)) {
                                isrError = hrBABBLE; // Line noise/unexpected data
                        } else if(err & _U1EIR_DMAEF_MASK) {
                                isrError = hrDMA; // Data was truncated. Device sent too much.
                        } else if(err & _U1EIR_BTOEF_MASK) {
                                // Device didn't respond in time. It is most likely unplugged.
                                USBTRACE("\r\n\r\n*** ISR sees error 0x10, Device unplugged? ***\r\n\r\n");
                                isrError = UHS_HOST_ERROR_UNPLUGGED; //hrNAK;
#if 0
                                // Ignore these, for now.
                        } else if(err & _U1EIR_CRC5EF_MASK) {
                                isrError =
#endif
                        }

                        // update to signal non-isr code what happened
                        if(isrError) newError = true;
                        HW_CLEAR |= _U1IR_UERRIF_MASK;
                }
        }

        if((status & _U1IR_TRNIF_MASK)) { // Token done
                stat = USB0_STAT;
                bdt_t *p_newToken = UHS_PIC32_stat2bufferdescriptor(stat);
                b_newToken.desc = p_newToken->desc;
                b_newToken.addr = p_newToken->addr;
                uint32_t pid = UHS_PIC32_BDT_PID(b_newToken.desc);
                isrPid = pid;
                // update to signal non-isr code what happened
                switch(pid & 0x0f) {
                        case 0x00:
                                isrError = hrTIMEOUT;
                                break;
                        case 0x0a:
                                isrError = hrNAK;
                                break;
                        case 0x0e:
                                isrError = hrSTALL;
                                break;
                        case 0x0f:
                                if(isrError != hrDMA) {
                                        isrError = hrNAK; // Error was due to memory latency.
                                }
                                break;
                                // What do we do with these??
                        case 0x03:
                        case 0x0b:
                        case 0x02:
                                // isrError = hrSUCCESS;
                                break;
                }

                if(isrError) {
                        newError = true;
                } else {
                        newToken = true;
                }
#if DEBUG_PRINTF_EXTRA_HUGE
                uint8_t count = b_newToken.desc >> 16;
                uint8_t *buf = (uint8_t *)b_newToken.addr;
                HOST_DUBUG("ISR: Token Done. Pid: %lx", pid);
                HOST_DUBUG(", count: %x", count);
                if(count) {
                        HOST_DUBUG(". Data: %lx", *(uint32_t *)(buf));
                }
                HOST_DUBUG("\r\n");

#endif
                HW_CLEAR |= _U1IR_TRNIF_MASK;
                // return;
        }

        if(status & _U1IR_DETACHIF_MASK) { // device detach
                HOST_DUBUG("ISR: detach\r\n");
                condet = true; // ALWAYS. todo: verify, for detach it should rather be always false
                busprobe();
                HW_CLEAR |= _U1IR_DETACHIF_MASK;
        }

        if((status & _U1IR_ATTACHIF_MASK)) { // device attached to the bus
                HOST_DUBUG("ISR: Attach\r\n");
                condet = true;
                busprobe();
                HW_CLEAR |= _U1IR_ATTACHIF_MASK;
        }

        if((status & _U1IR_SOFIF_MASK)) {
                // mark the time in microseconds + 1 millisecond.
                // this is used to know how soon the _next_ SOF will be.
                sof_mark = (long)(micros() + 1000);
                if(sof_countdown) {
                        sof_countdown--;
                        counted = true;
                }
                sofevent = false;
                HW_CLEAR |= _U1IR_SOFIF_MASK;
        }

#if 0
        if((status & _U1IR_IDLEIF_MASK)) { // idle condition detected
                HOST_DUBUG("ISR: idle\r\n");
                // serial_print("sleep\n");
                HW_CLEAR |= _U1IR_IDLEIF_MASK;
        }

        if((status & _U1IR_RESUMEIF_MASK)) { // resume
                HOST_DUBUG("ISR: resume\r\n");
                // serial_print("resume\n");
                HW_CLEAR |= _U1IR_RESUMEIF_MASK;
        }
#endif
        if((otg_status & _U1OTGIR_T1MSECIF_MASK)) { // 1 ms timer
                if(timer_countdown) {
                        timer_countdown--;
                        counted = true;
                }
                U1OTGIR = _U1OTGIR_T1MSECIF_MASK; //todo: make bit clear consistent with the rest
        }

        U1EIR = HW_CLEAR; // ack all interrupt flags here at the same time.

        __DSB();
        if(!timer_countdown && !sof_countdown && !counted && !usb_task_polling_disabled) {
                usb_task_polling_disabled++;

                // ARM uses SWI for bottom half
                // NVIC disallows reentrant IRQ
                exec_SWI(this);
        }
}

/**
 * Setting device address-related data in the SIE
 *
 * @param addr USB device address
 * @param ep Endpoint
 * @param ppep pointer to the pointer to a valid UHS_EpInfo structure
 * @param nak_limit how many NAKs before aborting
 * @return 0 on success
 */
uint8_t UHS_NI UHS_PIC32::SetAddress(uint8_t addr, uint8_t ep, UHS_EpInfo **ppep, uint16_t & nak_limit) {
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

        nak_limit = (0x0001UL << (((*ppep)->bmNakPower > USB_NAK_MAX_POWER) ? USB_NAK_MAX_POWER : (*ppep)->bmNakPower));
        nak_limit--;

        USBTRACE2("\r\nAddress: ", addr);
        USBTRACE2(" EP: ", ep);
        USBTRACE2(" NAK Power: ", (*ppep)->bmNakPower);
        USBTRACE2(" NAK Limit: ", nak_limit);
        USBTRACE("\r\n");

        // address and low speed enable
        U1ADDR = addr | ((p->lowspeed) ? USB_ADDR_LSEN : 0);

        //Serial.print("\r\nMode: ");
        //Serial.println( mode, HEX);
        //Serial.print("\r\nLS: ");
        //Serial.println(p->lowspeed, HEX);

        // Disable automatic retries for 1 NAK, Set hub for low-speed device
        U1EP0 = _U1EP0_EPRXEN_MASK | _U1EP0_EPTXEN_MASK | _U1EP0_EPHSHK_MASK |
                ((nak_limit != 1U) ? 0 : _U1EP0_RETRYDIS_MASK) |
                ((p->lowspeed) ? _U1EP0_LSPD_MASK : 0);

        // set USB0_SOFTHLD depending on the maxPktSize
        // NOTE: This should actually be set per-packet, however this works.
        uint8_t mps = (*ppep)->maxPktSize;
        if(mps < 9) {
                U1SOF = (18 + 16) *2; // 18;
        } else if(mps < 17) {
                U1SOF = (26 + 16)*2; // 26;
        } else if(mps < 33) {
                U1SOF = (42 + 16)*2; // 42;
        } else {
                U1SOF = (74 + 16)*2; // 74;
        }

        return 0;
}

/**
 * Send the actual packet.
 *
 * @param token
 * @param ep Endpoint
 * @param nak_limit how many NAKs before aborting
 * @return 0 on success
 */
/* Assumes peripheral address is set and relevant buffer is loaded/empty       */
/* If NAK, tries to re-send up to nak_limit times                                                   */
/* If nak_limit == 0, do not count NAKs, exit after timeout                                         */
/* If bus timeout, re-sends up to USB_RETRY_LIMIT times                                             */

/* return codes 0x00-0x0f are HRSLT( 0x00 being success ), 0xff means timeout                       */

uint8_t UHS_NI UHS_PIC32::dispatchPkt(uint8_t token, uint8_t ep, uint16_t nak_limit) {
        // Short packets cause problems with bandwidth on a hub.
        // Limit the bandwidth to 2 125uS frames so we do not overload
        // the transaction translator on the hub, other devices could fail too.
        //unsigned long timeout = millis() + UHS_HOST_TRANSFER_MAX_MS;
        //uint8_t retry_count = 0;
        uint8_t rcode;
        //isrError = 0;
        //newError = false;
        //newToken = false;

        //HOST_DUBUG("dispatchPkt: token %x, ep: %i, nak_limit: %i \r\n", token, ep, nak_limit);
        //printf("dispatchPkt: token %x, ep: %i, nak_limit: %i \r\n", token, ep, nak_limit);

        rcode = hrTIMEOUT;
        newError = false;
        newToken = false;
        isrError = 0;

        // Only do this if a hub is connected.
        // The code below this will eat some of the time.
        if(hub_present) {
                while((long)(last_mark + 205) >= (long)(micros()));
        }
        // SPEC: 12000 bits per 1mS frame +/- 16 bits
        // that should make 12 bits per uS, but... we need to add how long until launch happens
        // We also are not checking if we are transmitting at low speed.
        // TO-DO: Use a different multiplier if low speed.
        uint32_t tft = ((U1SOF) / 12LU);
        if((long)sof_mark <= (long)(micros() + tft)) {
                // wait for SOF
                noInterrupts();
                sofevent = true;
                //__DSB();
                _sync();
                interrupts();
                while(sofevent && !condet);
        }

        if(!condet) {
                last_mark = micros(); // hopefully now in sync with the SOF
                U1TOK = token | ep; //  Dispatch to endpoint.
                //wait for transfer completion
                while(/*(long)(millis() - timeout) < 0L &&*/ !condet) {
                        if(newError || newToken) {
                                if(newError) {
                                        newError = false;
                                        rcode = isrError;
                                        isrError = 0;
                                        //printf("New rcode: 0x%2.2x\r\n", rcode);
                                } // error
                                if(newToken) { // token completed

                                        if(rcode == hrTIMEOUT) rcode = hrSUCCESS;
                                        newToken = false;
                                } // token completed
                                //USBTRACE2("isrPid: ", isrPid);
                                //USBTRACE2(", rcode: ", rcode);
                                //if(isrPid || rcode) {
                                //        printf("dispatchPkt: token %x, ep: %i, nak_limit: %i \r\n", token, ep, nak_limit);
                                //        printf("isrPid: 0x%8.8x, rcode: 0x%2.2x\r\n", isrPid, rcode);
                                //}
                                break;
                        }

                }
        }

        if(condet) {
                rcode = UHS_HOST_ERROR_UNPLUGGED;
        }
        //if(rcode) printf("Final rcode: 0x%2.2x, isrPid: 0x%8.8x\r\n", rcode, isrPid);
        //        if(rcode == hrTIMEOUT) {
        //                retry_count++;
        //                if(retry_count == UHS_HOST_TRANSFER_RETRY_MAXIMUM) break;
        //        } else break;
        //}// while(true)
        //uint32_t a = micros();
        //printf("<.>");
        //fflush(stdout);
        //uint32_t b = micros();
        //printf("\rDelay %d\r\n", (b-a));
        //fflush(stdout);
        //if(ep == 0 || nak_limit == 1)
        //delayMicroseconds(100);
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
uint8_t UHS_NI UHS_PIC32::OutTransfer(UHS_EpInfo *pep, uint16_t nak_limit, uint16_t nbytes, uint8_t * data) {

        uint8_t rcode = hrSUCCESS;
        uint16_t bytes_tosend;
        uint16_t bytes_left = nbytes;

        uint8_t maxpktsize = pep->maxPktSize;
        uint8_t ep = pep->epAddr;

        USBTRACE2("OutTransfer: Sending nbytes: ", nbytes);

        if(maxpktsize < 1 || maxpktsize > 64)
                rcode = UHS_HOST_ERROR_BAD_MAX_PACKET_SIZE;

        uint8_t* p_buffer = data; // local copy
        // regWr(rHCTL, (pep->bmSndToggle) ? bmSNDTOG1 : bmSNDTOG0); //set toggle value
        ep0_tx_data_toggle = pep->bmSndToggle;

        while(bytes_left && !rcode) {
                HOST_DUBUG(", maxpktsize: %i, bytes_left: %i", maxpktsize, bytes_left);

                bytes_tosend = (bytes_left >= maxpktsize) ? maxpktsize : bytes_left;
                endpoint0_transmit(p_buffer, bytes_tosend); // setup internal buffer
                rcode = dispatchPkt(UHS_PIC32_TOKEN_DATA_OUT, ep, nak_limit); //dispatch packet to ep
                bytes_left -= bytes_tosend;
                p_buffer += bytes_tosend;
        }//while( bytes_left...

        //pep->bmSndToggle = (regRd(rHRSL) & bmSNDTOGRD) ? 1 : 0; //bmSNDTOG1 : bmSNDTOG0;  //update toggle
        pep->bmSndToggle = ep0_tx_data_toggle;
        return ( rcode); //should be 0 in all cases
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
uint8_t UHS_NI UHS_PIC32::InTransfer(UHS_EpInfo *pep, uint16_t nak_limit, uint16_t *nbytesptr, uint8_t * data) {
        uint8_t rcode = 0;
        uint8_t pktsize;
        uint16_t nbytes = *nbytesptr;
        uint8_t maxpktsize = pep->maxPktSize;
        uint8_t ep = pep->epAddr;

        USBTRACE2("Requesting ", nbytes);
        USBTRACE2("maxpktsize: ", maxpktsize);

        *nbytesptr = 0;

        // data0/1 toggle automatic
        // regWr(rHCTL, (pep->bmRcvToggle) ? bmRCVTOG1 : bmRCVTOG0); //set toggle value
        ep0_rx_data_toggle = pep->bmRcvToggle;

        uint32_t datalen = 0;

        uint8_t* p_buffer = data;
        datalen = maxpktsize;

        while(1) {
                if(datalen + *nbytesptr > nbytes) { // get less than maxpktsize if we don't need a full maxpktsize
                        datalen = nbytes - *nbytesptr;
                }
                HOST_DUBUG("datalen: %lu \r\n", datalen);
                endpoint0_receive(data_in_buf, datalen); // setup internal buffer
                rcode = dispatchPkt(UHS_PIC32_TOKEN_DATA_IN, ep, nak_limit); //dispatch packet

                pktsize = b_newToken.desc >> 16; // how many bytes we actually got

                HOST_DUBUG("pktsize: %i \r\n", pktsize);
#if ENABLE_UHS_DEBUGGING
                uint8_t i = 0;
                HOST_DUBUG("-----Data packet: ");
                for(i = 0; i < pktsize; i++) {
                        HOST_DUBUG("%02x ", data_in_buf[i]);
                }
                HOST_DUBUG("\r\n");
#endif

                if(rcode) {
                        // DMA error: we got more data than expected.
                        // This means that the device's mackpaxketsize is actually larger than maxpktsize
                        // copy the packet that we received and return so that Configuring can deal with it.
                        //if(rcode == hrDMA) {
                        //        memcpy(p_buffer, data_in_buf, pktsize); // copy packet into buffer
                        //        *nbytesptr += pktsize; // add to the number of bytes read
                        //        break;
                        //}
                        //HOST_DUBUG(">>>>hrUNDEF>>>> InTransfer Problem! dispatchPkt ", rcode);
                        break; //should be 0, indicating ACK. Else return error code.
                }

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
        return ( rcode);
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
UHS_EpInfo * UHS_NI UHS_PIC32::ctrlReqOpen(uint8_t addr, uint8_t bmReqType, uint8_t bRequest, uint8_t wValLo, uint8_t wValHi, uint16_t wInd, uint16_t total, uint8_t * dataptr) {

        //serial_print("setup phase\n");

        uint8_t rcode;
        SETUP_PKT setup_pkt;

        UHS_EpInfo *pep = NULL;
        uint16_t nak_limit = 0;
        HOST_DUBUG("ctrlReqOpen: addr: 0x%2.2x bmReqType: 0x%2.2x bRequest: 0x%2.2x\r\nctrlReqOpen: wValLo: 0x%2.2x  wValHi: 0x%2.2x wInd: 0x%4.4x total: 0x%4.4x dataptr: %p\r\n", addr, bmReqType, bRequest, wValLo, wValHi, wInd, total, dataptr);
        rcode = SetAddress(addr, 0, &pep, nak_limit);

        if(!rcode) {

                // const uint8_t *data = NULL;
                uint32_t datalen = 0;

                setup_pkt.ReqType_u.bmRequestType = bmReqType;
                setup_pkt.bRequest = bRequest;
                setup_pkt.wVal_u.wValueLo = wValLo;
                setup_pkt.wVal_u.wValueHi = wValHi;
                setup_pkt.wIndex = wInd;
                setup_pkt.wLength = total;


                datalen = 8;
                //data = setup_command_buffer;
                ep0_tx_data_toggle = UHS_PIC32_DATA0; // setup always uses DATA0
                endpoint0_transmit(&setup_pkt, datalen); // setup internal buffer

                rcode = dispatchPkt(UHS_PIC32_TOKEN_SETUP, 0, nak_limit); //dispatch packet

                if(!rcode) {
                        if(dataptr != NULL) {
                                // data phase begins with DATA1 after setup
                                if((bmReqType & 0x80) == 0x80) {
                                        pep->bmRcvToggle = UHS_PIC32_DATA1; //bmRCVTOG1;
                                } else {
                                        pep->bmSndToggle = UHS_PIC32_DATA1; //bmSNDTOG1;
                                }
                        }
                } else {
                        USBTRACE(">>>>>>>>>>>> dispatchPkt Failed <<<<<<<<<<<<<< \r\n");
                        USBTRACE2("rcode: ", rcode);
                        USBTRACE2(", bmReqType: ", bmReqType);
                        USBTRACE2(", bRequest: ", bRequest);
                        USBTRACE(">>>>>>>>>>>> dispatchPkt Failed <<<<<<<<<<<<<< \r\n");
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
uint8_t UHS_NI UHS_PIC32::ctrlReqRead(UHS_EpInfo *pep, uint16_t *left, uint16_t *read, uint16_t nbytes, uint8_t * dataptr) {
        *read = 0;
        uint8_t rcode = 0;
        uint16_t nak_limit = 0;
        HOST_DUBUG("ctrlReqRead left: %i\r\n", *left);
        if(*left) {
                *read = nbytes;
                rcode = InTransfer(pep, nak_limit, read, dataptr);

                if(rcode) {
                        HOST_DUBUG("ctrlReqRead ERROR: %2.2x, left: %i, read %i\r\n", rcode, *left, *read);
                } else {
                        *left -= *read;
                        HOST_DUBUG("ctrlReqRead left: %i, read %i\r\n", *left, *read);
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
uint8_t UHS_NI UHS_PIC32::ctrlReqClose(UHS_EpInfo *pep, uint8_t bmReqType, uint16_t left, uint16_t nbytes, uint8_t * dataptr) {
        uint8_t rcode = 0;

        //HOST_DUBUG("Inside ctrlReqClose. bmReqType: %x, left: %x, nbytes: %x \r\n", bmReqType, left, nbytes);

        if(((bmReqType & 0x80) == 0x80) && pep && left && dataptr) {
                //Serial.println("Drain");
                HOST_DUBUG("ctrlReqRead Sinking %i\r\n", left);
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
                        ep0_tx_data_toggle = UHS_PIC32_DATA1; // make sure we use DATA1 for status phase
                        endpoint0_transmit(NULL, 0); // setup internal buffer, 0 bytes
                        rcode = dispatchPkt(UHS_PIC32_TOKEN_DATA_OUT, 0, 0);
                } else {
                        ep0_rx_data_toggle = UHS_PIC32_DATA1; // make sure we use DATA1 for status phase
                        endpoint0_receive(NULL, 0); // setup internal buffer, 0 bytes
                        rcode = dispatchPkt(UHS_PIC32_TOKEN_DATA_IN, 0, 0);
                }
                //        } else {
                //                Serial.println("Bypassed Dispatch");
        }
        return rcode;
}

/**
 * Initialize USB hardware, turn on VBUS
 *USB_ISTAT_SOFTOK
 * @param mseconds Delay energizing VBUS after mseconds, A value of INT16_MIN means no delay.
 * @return 0 on success, -1 on error
 */
int16_t UHS_NI UHS_PIC32::Init(int16_t mseconds) {

        ISR_pic32 = this;

        // assume 48 MHz clock already running
        // SIM - enable clock
//        SIM_SCGC4 |= SIM_SCGC4_USBOTG;

        // reset USB module
        //USB0_USBTRC0 = USB_USBTRC_USBRESET;
        //while((USB0_USBTRC0 & USB_USBTRC_USBRESET) != 0); // wait for reset to end
        
        U1IE = 0;       //disable all module interrupts
        U1IR = 0xff;    //clear all interrupt flags
        U1EIE = 0;
        U1EIR = 0xff;
        U1OTGIE = 0;
        U1OTGIR = 0xff;
        
        U1CONbits.HOSTEN = 1;   //USB module on
        U1CONbits.SOFEN = 0;    //SOF generation off
        U1CONbits.PPBRST = 1;   //ping-pong buffers reset
        U1CONbits.PPBRST = 0;
        
        
        // set buffer desc table base addr
        U1BDTP1 = ((uint32_t)table) >> 8;
        U1BDTP2 = ((uint32_t)table) >> 16;
        U1BDTP3 = ((uint32_t)table) >> 24;

        // initialize BDT toggle bits
        //USB0_CTL = USB_CTL_ODDRST;
        ep0_tx_bdt_bank = 0;
        ep0_rx_bdt_bank = 0;
        ep0_tx_data_toggle = UHS_PIC32_BDT_DATA0;
        ep0_rx_data_toggle = UHS_PIC32_BDT_DATA0;

        // setup buffers
        table[UHS_PIC32_index(0, UHS_PIC32_RX, UHS_PIC32_EVEN)].desc = UHS_PIC32_BDT_DESC(UHS_PIC32_EP0_SIZE, 0);
        table[UHS_PIC32_index(0, UHS_PIC32_RX, UHS_PIC32_EVEN)].addr = ep0_rx0_buf;
        table[UHS_PIC32_index(0, UHS_PIC32_RX, UHS_PIC32_ODD)].desc = UHS_PIC32_BDT_DESC(UHS_PIC32_EP0_SIZE, 0);
        table[UHS_PIC32_index(0, UHS_PIC32_RX, UHS_PIC32_ODD)].addr = ep0_rx1_buf;

        table[UHS_PIC32_index(0, UHS_PIC32_TX, UHS_PIC32_EVEN)].desc = UHS_PIC32_BDT_DESC(UHS_PIC32_EP0_SIZE, 0);
        table[UHS_PIC32_index(0, UHS_PIC32_TX, UHS_PIC32_EVEN)].addr = ep0_tx0_buf;
        table[UHS_PIC32_index(0, UHS_PIC32_TX, UHS_PIC32_ODD)].desc = UHS_PIC32_BDT_DESC(UHS_PIC32_EP0_SIZE, 0);
        table[UHS_PIC32_index(0, UHS_PIC32_TX, UHS_PIC32_ODD)].addr = ep0_tx1_buf;

        // clear interrupts
        //USB0_ERRSTAT = 0xFF;
        //USB0_ISTAT = 0xFF;
        //USB0_OTGISTAT = 0xFF;

        //USB0_USBFRMADJUST = 0x00; - absent in pic32
        // enable USB
        //USB0_USBTRC0 |= 0x40; // undocumented bit
        //USB0_USBCTRL = 0;

        //USB0_OTGCTL = USB_OTGCTL_DPLOW | USB_OTGCTL_DMLOW; // enable D+ and D- pulldowns, disable D+ pullup

        U1OTGCONbits.DPPULDWN = 1; 
        U1OTGCONbits.DMPULDWN = 1;
        
//        USB0_INTEN = USB_INTEN_ATTACHEN |
//                USB_INTEN_TOKDNEEN |
//                USB_INTEN_STALLEN |
//                USB_INTEN_ERROREN |
//                USB_INTEN_SOFTOKEN |
//                USB_INTEN_USBRSTEN |
//                USB_INTEN_SLEEPEN; // enable attach interrupt, token done, stall, error and sleep

        //enable interrupt sources
        U1IEbits.ATTACHIE = 1;
        U1IEbits.DETACHIE = 1;
        U1IEbits.TRNIE = 1;
        U1IEbits.STALLIE = 1;
        U1IEbits.IDLEIE = 1;
        U1IEbits.SOFIE = 1;
        U1IEbits.UERRIE = 1;
        
        U1OTGIEbits.T1MSECIE = 1;   //enable 1 ms timer intterupt
        U1EIE = 0xff;           //enable all USB error interrupts
        
        //USB0_OTGICR = USB_OTGICR_ONEMSECEN; // activate 1mU1IEbits.U1IEbits.s timer interrupt
        //USB0_ERREN = 0xFF; // enable all error interrupts

        // switch isr for USB
        IFS1CLR = _IFS1_USBIF_MASK;
        IPC7CLR = _IPC7_USBIP_MASK | _IPC7_USBIS_MASK;
        IPC7SET = _IPC7_USBIP_MASK & (0x00000004 << _IPC7_USBIP_POSITION);

        //NVIC_DISABLE_IRQ(IRQ_USBOTG);
        //NVIC_SET_PRIORITY(IRQ_USBOTG, 112);
        //pic32 calls interrupts differently _VectorsRam[IRQ_USBOTG + 16] = call_ISR_kinetis;
        __DSB();

        IEC1SET = _IEC1_USBIE_MASK;
        //NVIC_ENABLE_IRQ(IRQ_USBOTG);

        // set address to zero during enumeration
        //USB0_ADDR = 0;
        U1ADDR = 0;

        //USB0_CTL = USB_CTL_HOSTMODEEN; // host mode enable
        // USB0_CTL &= ~USB_CTL_USBENSOFEN; // disable SOF generation to avoid noise until we detect attach

        return 0;
}

void UHS_NI UHS_PIC32::endpoint0_transmit(const void *data, uint32_t len) {
#if ENABLE_UHS_DEBUGGING && DEBUG_PRINTF_EXTRA_HUGE_UHS_HOST
        uint32_t i = 0;
        uint8_t *real_data = (uint8_t *)data;
        HOST_DUBUG("tx0: ");
        // serial_print("tx0: ");
        for(i = 0; i < len; i++) {
                //D_PrintHex(*(real_data + i), 0x80);
                HOST_DUBUG("%x ", *(real_data + i));
        }
        HOST_DUBUG(", %lx", len);
#endif
        last_count = len;
        last_address = (void *)data;
        last_tx = true;

        table[UHS_PIC32_index(0, UHS_PIC32_TX, ep0_tx_bdt_bank)].addr = (void *)data;
        table[UHS_PIC32_index(0, UHS_PIC32_TX, ep0_tx_bdt_bank)].desc = UHS_PIC32_BDT_DESC(len, ep0_tx_data_toggle);
        ep0_tx_data_toggle ^= 1;
        ep0_tx_bdt_bank ^= 1;
}

void UHS_NI UHS_PIC32::endpoint0_receive(const void *data, uint32_t len) {
        //#if ENABLE_UHS_DEBUGGING
#if 1
        HOST_DUBUG("endpoint0_receive: %lx", len);
#endif

        last_count = len;
        last_address = (void *)data;
        last_tx = false;

        table[UHS_PIC32_index(0, UHS_PIC32_RX, ep0_rx_bdt_bank)].addr = (void *)data;
        table[UHS_PIC32_index(0, UHS_PIC32_RX, ep0_rx_bdt_bank)].desc = UHS_PIC32_BDT_DESC(len, ep0_rx_data_toggle);
        ep0_rx_data_toggle ^= 1;
        ep0_rx_bdt_bank ^= 1;
}

#else
#error "Never include UHS_PIC32_INLINE.h, include UHS_PIC32.h instead"
#endif