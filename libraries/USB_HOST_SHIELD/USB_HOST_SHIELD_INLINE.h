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

#if defined(USB_HOST_SHIELD_H) && !defined(USB_HOST_SHIELD_LOADED)
#define USB_HOST_SHIELD_LOADED
#include <Arduino.h>

#if !defined(digitalPinToInterrupt)
#error digitalPinToInterrupt not defined, complain to your board maintainer.
#endif


// uncomment to get 'printf' console debugging. NOT FOR UNO!
//#define DEBUG_PRINTF_EXTRA_HUGE_USB_HOST_SHIELD

#if DEBUG_PRINTF_EXTRA_HUGE
#ifdef DEBUG_PRINTF_EXTRA_HUGE_USB_HOST_SHIELD
#define MAX_HOST_DEBUG(...) printf(__VA_ARGS__)
#else
#define MAX_HOST_DEBUG(...) VOID0
#endif
#else
#define MAX_HOST_DEBUG(...) VOID0
#endif

#if USB_HOST_SHIELD_USE_ISR

// allow two slots. this makes the maximum allowed shield count TWO
// for AVRs this is limited to pins 2 and 3 ONLY
// for all other boards, one odd and one even pin number is allowed.
static MAX3421E_HOST *ISReven;
static MAX3421E_HOST *ISRodd;

static void UHS_NI call_ISReven(void) {
        ISReven->ISRTask();
}

static void UHS_NI call_ISRodd(void) {
        ISRodd->ISRTask();

}
#endif

/* write single byte into MAX3421e register */
void UHS_NI MAX3421E_HOST::regWr(uint8_t reg, uint8_t data) {
        SPI.beginTransaction(MAX3421E_SPI_Settings);
        UHS_PIN_WRITE(ss_pin, LOW);
#if USING_SPI4TEENSY3
        uint8_t c[2];
        c[0] = reg | 0x02;
        c[1] = data;
        spi4teensy3::send(c, 2);
#else
        SPI.transfer(reg | 0x02);
        SPI.transfer(data);
#endif
        UHS_PIN_WRITE(ss_pin, HIGH);
        SPI.endTransaction();
}


/* multiple-byte write                            */

/* returns a pointer to memory position after last written */
uint8_t* UHS_NI MAX3421E_HOST::bytesWr(uint8_t reg, uint8_t nbytes, uint8_t* data_p) {
        SPI.beginTransaction(MAX3421E_SPI_Settings);
        UHS_PIN_WRITE(ss_pin, LOW);
#if USING_SPI4TEENSY3
        spi4teensy3::send(reg | 0x02);
        spi4teensy3::send(data_p, nbytes);
        data_p += nbytes;
#else
        SPI.transfer(reg | 0x02);
        while(nbytes) {
                SPI.transfer(*data_p);
                nbytes--;
                data_p++; // advance data pointer
        }
#endif
        UHS_PIN_WRITE(ss_pin, HIGH);
        SPI.endTransaction();
        return (data_p);
}
/* GPIO write                                           */
/*GPIO byte is split between 2 registers, so two writes are needed to write one byte */

/* GPOUT bits are in the low nibble. 0-3 in IOPINS1, 4-7 in IOPINS2 */
void UHS_NI MAX3421E_HOST::gpioWr(uint8_t data) {
        regWr(rIOPINS1, data);
        data >>= 4;
        regWr(rIOPINS2, data);
        return;
}

/* single host register read    */
uint8_t UHS_NI MAX3421E_HOST::regRd(uint8_t reg) {
        SPI.beginTransaction(MAX3421E_SPI_Settings);
        UHS_PIN_WRITE(ss_pin, LOW);
#if USING_SPI4TEENSY3
        spi4teensy3::send(reg);
        uint8_t rv = spi4teensy3::receive();
#else
        SPI.transfer(reg);
        uint8_t rv = SPI.transfer(0);
#endif
        UHS_PIN_WRITE(ss_pin, HIGH);
        SPI.endTransaction();
        return (rv);
}
/* multiple-byte register read  */

/* returns a pointer to a memory position after last read   */
uint8_t* UHS_NI MAX3421E_HOST::bytesRd(uint8_t reg, uint8_t nbytes, uint8_t* data_p) {
        SPI.beginTransaction(MAX3421E_SPI_Settings);
        UHS_PIN_WRITE(ss_pin, LOW);
#if USING_SPI4TEENSY3
        spi4teensy3::send(reg);
        spi4teensy3::receive(data_p, nbytes);
        data_p += nbytes;
#else
        SPI.transfer(reg);
        while(nbytes) {
                *data_p++ = SPI.transfer(0);
                nbytes--;
        }
#endif
        UHS_PIN_WRITE(ss_pin, HIGH);
        SPI.endTransaction();
        return ( data_p);
}

/* GPIO read. See gpioWr for explanation */

/* GPIN pins are in high nibbles of IOPINS1, IOPINS2    */
uint8_t UHS_NI MAX3421E_HOST::gpioRd(void) {
        uint8_t gpin = 0;
        gpin = regRd(rIOPINS2); //pins 4-7
        gpin &= 0xf0; //clean lower nibble
        gpin |= (regRd(rIOPINS1) >> 4); //shift low bits and OR with upper from previous operation.
        return ( gpin);
}

/* reset MAX3421E. Returns number of microseconds it took for PLL to stabilize after reset
  or zero if PLL haven't stabilized in 65535 cycles */
uint16_t UHS_NI MAX3421E_HOST::reset(void) {
        uint16_t i = 0;
        regWr(rUSBCTL, bmCHIPRES);
        regWr(rUSBCTL, 0x00);
        int32_t now;
        uint32_t expires = micros() + 65535;
        while((int32_t)(micros() - expires) < 0L) {
                if((regRd(rUSBIRQ) & bmOSCOKIRQ)) {
                        break;
                }
        }
        now = (int32_t)(micros() - expires);
        if(now < 0L) {
                i = 65535 + now; // Note this subtracts, as now is negative
        }
        return (i);
}

uint8_t UHS_NI MAX3421E_HOST::VBUS_changed(void) {
        /* modify USB task state because Vbus changed or unknown */
        uint8_t speed = 1;
        // printf("\r\n\r\n\r\n\r\nSTATE %2.2x -> ", usb_task_state);
        switch(vbusState) {
                case LSHOST: // Low speed

                        speed = 0;
                        // Intentional fall-through
                case FSHOST: // Full speed
                        // Start device initialization if we are not initializing
                        // Resets to the device cause an IRQ
                        if((usb_task_state & UHS_USB_HOST_STATE_MASK) != UHS_USB_HOST_STATE_DETACHED) {
                                ReleaseChildren();
                                usb_task_state = UHS_USB_HOST_STATE_DEBOUNCE;
                                sof_countdown = 0;
                        }
                        break;
                case SE1: //illegal state
                        sof_countdown = 0;
                        ReleaseChildren();
                        usb_task_state = UHS_USB_HOST_STATE_ILLEGAL;
                        break;
                case SE0: //disconnected
                default:
                        sof_countdown = 0;
                        ReleaseChildren();
                        usb_task_state = UHS_USB_HOST_STATE_IDLE;
                        break;
        }

        // printf("0x%2.2x\r\n\r\n\r\n\r\n", usb_task_state);
        return speed;
};

/**
 *  Probe bus to determine device presence and speed,
 *  then switch host to detected speed.
 */
void UHS_NI MAX3421E_HOST::busprobe(void) {
        uint8_t bus_sample;

        bus_sample = regRd(rHRSL); //Get J,K status
        bus_sample &= (bmJSTATUS | bmKSTATUS); //zero the rest of the byte
        switch(bus_sample) { //start full-speed or low-speed host
                case(bmJSTATUS):
                        // Serial.println("J");
                        if((regRd(rMODE) & bmLOWSPEED) == 0) {
                                regWr(rMODE, MODE_FS_HOST); //start full-speed host
                                vbusState = FSHOST;
                        } else {
                                regWr(rMODE, MODE_LS_HOST); //start low-speed host
                                vbusState = LSHOST;
                        }
                        break;
                case(bmKSTATUS):
                        // Serial.println("K");
                        if((regRd(rMODE) & bmLOWSPEED) == 0) {
                                regWr(rMODE, MODE_LS_HOST); //start low-speed host
                                vbusState = LSHOST;
                        } else {
                                regWr(rMODE, MODE_FS_HOST); //start full-speed host
                                vbusState = FSHOST;
                        }
                        break;
                case(bmSE1): //illegal state
                        // Serial.println("I");
                        vbusState = SE1;
                        break;
                case(bmSE0): //disconnected state
                        // Serial.println("D");
                        regWr(rMODE, bmDPPULLDN | bmDMPULLDN | bmHOST);
                        vbusState = SE0;
                        break;
        }//end switch( bus_sample )
}

/**
 * Initialize USB hardware, turn on VBUS
 *
 * @param mseconds Delay energizing VBUS after mseconds, A value of INT16_MIN means no delay.
 * @return 0 on success, -1 on error
 */
int16_t UHS_NI MAX3421E_HOST::Init(int16_t mseconds) {
        usb_task_state = UHS_USB_HOST_STATE_INITIALIZE; //set up state machine
        //        Serial.print("MAX3421E 'this' USB Host @ 0x");
        //        Serial.println((uint32_t)this, HEX);
        //        Serial.print("MAX3421E 'this' USB Host Address Pool @ 0x");
        //        Serial.println((uint32_t)GetAddressPool(), HEX);
        Init_dyn_SWI();
        UHS_printf_HELPER_init();
        noInterrupts();
#ifdef BOARD_MEGA_ADK
        // For Mega ADK, which has a Max3421e on-board, set MAX_RESET to output mode, and then set it to HIGH
        pinMode(55, OUTPUT);
        UHS_PIN_WRITE(55, HIGH);
#endif
#if USING_SPI4TEENSY3
        // spi4teensy3 inits everything for us, except /SS
        // CLK, MOSI and MISO are hard coded for now.
        // spi4teensy3::init(0,0,0); // full speed, cpol 0, cpha 0
        spi4teensy3::init(); // full speed, cpol 0, cpha 0
#else
        SPI.begin();
#endif
        pinMode(irq_pin, INPUT_PULLUP);
        //UHS_PIN_WRITE(irq_pin, HIGH);
        pinMode(ss_pin, OUTPUT);
        UHS_PIN_WRITE(ss_pin, HIGH);

#ifdef USB_HOST_SHIELD_TIMING_PIN
        pinMode(USB_HOST_SHIELD_TIMING_PIN, OUTPUT);
        // My counter/timer can't work on an inverted gate signal
        // so we gate using a high pulse -- AJK
        UHS_PIN_WRITE(USB_HOST_SHIELD_TIMING_PIN, LOW);
#endif
        interrupts();

#if USB_HOST_SHIELD_USE_ISR
        int intr = digitalPinToInterrupt(irq_pin);
        if(intr == NOT_AN_INTERRUPT) return (-2);
        SPI.usingInterrupt(intr);
#else
        SPI.usingInterrupt(255);
#endif
        /* MAX3421E - full-duplex SPI, interrupt kind, vbus off */
        regWr(rPINCTL, (bmFDUPSPI | bmIRQ_SENSE | GPX_VBDET));
        if(reset() == 0) { //OSCOKIRQ hasn't asserted in time
                return ( -1);
        }

        // Delay a minimum of 1 second to ensure any capacitors are drained.
        // 1 second is required to make sure we do not smoke a Microdrive!
        if(mseconds != INT16_MIN) {
                if(mseconds < 1000) mseconds = 1000;
                delay(mseconds); // We can't depend on SOF timer here.
        }

        regWr(rMODE, bmDPPULLDN | bmDMPULLDN | bmHOST); // set pull-downs, Host

        // Enable interrupts on the MAX3421e
        regWr(rHIEN, IRQ_CHECK_MASK);
        // Enable interrupt pin on the MAX3421e, set pulse width for edge
        regWr(rCPUCTL, (bmIE | bmPULSEWIDTH));

        /* check if device is connected */
        regWr(rHCTL, bmSAMPLEBUS); // sample USB bus
        while(!(regRd(rHCTL) & bmSAMPLEBUS)); //wait for sample operation to finish

        busprobe(); //check if anything is connected
        islowspeed = (VBUS_changed() == 0);

        // GPX pin on. This is done here so that a change is detected if we have a switch connected.
        /* MAX3421E - full-duplex SPI, interrupt kind, vbus on */
        regWr(rPINCTL, (bmFDUPSPI | bmIRQ_SENSE));
        regWr(rHIRQ, bmBUSEVENTIRQ); // see data sheet.
        regWr(rHCTL, bmBUSRST); // issue bus reset to force generate yet another possible IRQ


#if USB_HOST_SHIELD_USE_ISR
        // Attach ISR to service IRQ from MAX3421e
        noInterrupts();
        if(irq_pin & 1) {
                ISRodd = this;
                attachInterrupt(UHS_GET_DPI(irq_pin), call_ISRodd, IRQ_SENSE);
        } else {
                ISReven = this;
                attachInterrupt(UHS_GET_DPI(irq_pin), call_ISReven, IRQ_SENSE);
        }
        interrupts();
#endif
        //printf("\r\nrPINCTL 0x%2.2X\r\n", rPINCTL);
        //printf("rCPUCTL 0x%2.2X\r\n", rCPUCTL);
        //printf("rHIEN 0x%2.2X\r\n", rHIEN);
        //printf("irq_pin %i\r\n", irq_pin);
        return 0;
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
uint8_t UHS_NI MAX3421E_HOST::SetAddress(uint8_t addr, uint8_t ep, UHS_EpInfo **ppep, uint16_t &nak_limit) {
        UHS_Device *p = addrPool.GetUsbDevicePtr(addr);

        if(!p)
                return UHS_HOST_ERROR_NO_ADDRESS_IN_POOL;

        if(!p->epinfo)
                return UHS_HOST_ERROR_NULL_EPINFO;

        *ppep = getEpInfoEntry(addr, ep);

        if(!*ppep)
                return UHS_HOST_ERROR_NO_ENDPOINT_IN_TABLE;

        nak_limit = (0x0001UL << (((*ppep)->bmNakPower > USB_NAK_MAX_POWER) ? USB_NAK_MAX_POWER : (*ppep)->bmNakPower));
        nak_limit--;
        /*
          USBTRACE2("\r\nAddress: ", addr);
          USBTRACE2(" EP: ", ep);
          USBTRACE2(" NAK Power: ",(*ppep)->bmNakPower);
          USBTRACE2(" NAK Limit: ", nak_limit);
          USBTRACE("\r\n");
         */
        regWr(rPERADDR, addr); //set peripheral address

        uint8_t mode = regRd(rMODE);

        //Serial.print("\r\nMode: ");
        //Serial.println( mode, HEX);
        //Serial.print("\r\nLS: ");
        //Serial.println(p->lowspeed, HEX);

        // Set bmLOWSPEED and bmHUBPRE in case of low-speed device, reset them otherwise
        regWr(rMODE, (p->lowspeed) ? mode | bmLOWSPEED | hub_present : mode & ~(bmHUBPRE | bmLOWSPEED));

        return 0;
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
uint8_t UHS_NI MAX3421E_HOST::InTransfer(UHS_EpInfo *pep, uint16_t nak_limit, uint16_t *nbytesptr, uint8_t* data) {
        uint8_t rcode = 0;
        uint8_t pktsize;

        uint16_t nbytes = *nbytesptr;
        MAX_HOST_DEBUG("Requesting %i bytes ", nbytes);
        uint8_t maxpktsize = pep->maxPktSize;

        *nbytesptr = 0;
        regWr(rHCTL, (pep->bmRcvToggle) ? bmRCVTOG1 : bmRCVTOG0); //set toggle value

        // use a 'break' to exit this loop
        while(1) {
                rcode = dispatchPkt(MAX3421E_tokIN, pep->epAddr, nak_limit); //IN packet to EP-'endpoint'. Function takes care of NAKS.
#if 0
                // This issue should be resolved now.
                if(rcode == hrTOGERR) {
                        //MAX_HOST_DEBUG("toggle wrong\r\n");
                        // yes, we flip it wrong here so that next time it is actually correct!
                        pep->bmRcvToggle = (regRd(rHRSL) & bmSNDTOGRD) ? 0 : 1;
                        regWr(rHCTL, (pep->bmRcvToggle) ? bmRCVTOG1 : bmRCVTOG0); //set toggle value
                        continue;
                }
#endif
                if(rcode) {
                        //MAX_HOST_DEBUG(">>>>>>>> Problem! dispatchPkt %2.2x\r\n", rcode);
                        break; //should be 0, indicating ACK. Else return error code.
                }
                /* check for RCVDAVIRQ and generate error if not present */
                /* the only case when absence of RCVDAVIRQ makes sense is when toggle error occurred. Need to add handling for that */
                if((regRd(rHIRQ) & bmRCVDAVIRQ) == 0) {
                        //MAX_HOST_DEBUG(">>>>>>>> Problem! NO RCVDAVIRQ!\r\n");
                        rcode = 0xf0; //receive error
                        break;
                }
                pktsize = regRd(rRCVBC); //number of received bytes
                MAX_HOST_DEBUG("Got %i bytes \r\n", pktsize);

                if(pktsize > nbytes) {  //certain devices send more than asked
                        //MAX_HOST_DEBUG(">>>>>>>> Warning: wanted %i bytes but got %i.\r\n", nbytes, pktsize);
                        pktsize = nbytes;
                }

                int16_t mem_left = (int16_t)nbytes - *((int16_t*)nbytesptr);

                if(mem_left < 0)
                        mem_left = 0;

                data = bytesRd(rRCVFIFO, ((pktsize > mem_left) ? mem_left : pktsize), data);

                regWr(rHIRQ, bmRCVDAVIRQ); // Clear the IRQ & free the buffer
                *nbytesptr += pktsize; // add this packet's byte count to total transfer length

                /* The transfer is complete under two conditions:           */
                /* 1. The device sent a short packet (L.T. maxPacketSize)   */
                /* 2. 'nbytes' have been transferred.                       */
                if((pktsize < maxpktsize) || (*nbytesptr >= nbytes)) // have we transferred 'nbytes' bytes?
                {
                        // Save toggle value
                        pep->bmRcvToggle = ((regRd(rHRSL) & bmRCVTOGRD)) ? 1 : 0;
                        //MAX_HOST_DEBUG("\r\n");
                        rcode = 0;
                        break;
                } // if
        } //while( 1 )
        return ( rcode);
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
uint8_t UHS_NI MAX3421E_HOST::OutTransfer(UHS_EpInfo *pep, uint16_t nak_limit, uint16_t nbytes, uint8_t *data) {
        uint8_t rcode = hrSUCCESS;
        uint8_t retry_count;
        uint8_t *data_p = data; //local copy of the data pointer
        uint16_t bytes_tosend;
        uint16_t nak_count;
        uint16_t bytes_left = nbytes;

        uint8_t maxpktsize = pep->maxPktSize;

        if(maxpktsize < 1 || maxpktsize > 64)
                return UHS_HOST_ERROR_BAD_MAX_PACKET_SIZE;

        unsigned long timeout = millis() + UHS_HOST_TRANSFER_MAX_MS;

        regWr(rHCTL, (pep->bmSndToggle) ? bmSNDTOG1 : bmSNDTOG0); //set toggle value

        while(bytes_left) {
                retry_count = 0;
                nak_count = 0;
                bytes_tosend = (bytes_left >= maxpktsize) ? maxpktsize : bytes_left;
                bytesWr(rSNDFIFO, bytes_tosend, data_p); //filling output FIFO
                regWr(rSNDBC, bytes_tosend); //set number of bytes
                regWr(rHXFR, (MAX3421E_tokOUT | pep->epAddr)); //dispatch packet
                while(!(regRd(rHIRQ) & bmHXFRDNIRQ)); //wait for the completion IRQ
                regWr(rHIRQ, bmHXFRDNIRQ); //clear IRQ
                rcode = (regRd(rHRSL) & 0x0f);

                while(rcode && ((long)(millis() - timeout) < 0L)) {
                        switch(rcode) {
                                case hrNAK:
                                        nak_count++;
                                        if(nak_limit && (nak_count == nak_limit))
                                                goto breakout;
                                        break;
                                case hrTIMEOUT:
                                        retry_count++;
                                        if(retry_count == UHS_HOST_TRANSFER_RETRY_MAXIMUM)
                                                goto breakout;
                                        break;
                                case hrTOGERR:
                                        // yes, we flip it wrong here so that next time it is actually correct!
                                        pep->bmSndToggle = (regRd(rHRSL) & bmSNDTOGRD) ? 0 : 1;
                                        regWr(rHCTL, (pep->bmSndToggle) ? bmSNDTOG1 : bmSNDTOG0); //set toggle value
                                        break;
                                default:
                                        goto breakout;
                        }//switch( rcode

                        /* process NAK according to Host out NAK bug */
                        regWr(rSNDBC, 0);
                        regWr(rSNDFIFO, *data_p);
                        regWr(rSNDBC, bytes_tosend);
                        regWr(rHXFR, (MAX3421E_tokOUT | pep->epAddr)); //dispatch packet
                        while(!(regRd(rHIRQ) & bmHXFRDNIRQ)); //wait for the completion IRQ
                        regWr(rHIRQ, bmHXFRDNIRQ); //clear IRQ
                        rcode = (regRd(rHRSL) & 0x0f);
                }//while( rcode && ....
                bytes_left -= bytes_tosend;
                data_p += bytes_tosend;
        }//while( bytes_left...
breakout:

        pep->bmSndToggle = (regRd(rHRSL) & bmSNDTOGRD) ? 1 : 0; //bmSNDTOG1 : bmSNDTOG0;  //update toggle
        return ( rcode); //should be 0 in all cases
}

/**
 * Send the actual packet.
 *
 * @param token
 * @param ep Endpoint
 * @param nak_limit how many NAKs before aborting, 0 == exit after timeout
 * @return 0 on success, 0xFF indicates NAK timeout. @see
 */
/* Assumes peripheral address is set and relevant buffer is loaded/empty       */
/* If NAK, tries to re-send up to nak_limit times                                                   */
/* If nak_limit == 0, do not count NAKs, exit after timeout                                         */
/* If bus timeout, re-sends up to USB_RETRY_LIMIT times                                             */

/* return codes 0x00-0x0f are HRSLT( 0x00 being success ), 0xff means timeout                       */
uint8_t UHS_NI MAX3421E_HOST::dispatchPkt(uint8_t token, uint8_t ep, uint16_t nak_limit) {
        unsigned long timeout = millis() + UHS_HOST_TRANSFER_MAX_MS;
        uint8_t tmpdata;
        uint8_t rcode = hrSUCCESS;
        uint8_t retry_count = 0;
        uint16_t nak_count = 0;

        for(;;) {
                regWr(rHXFR, (token | ep)); //launch the transfer
                while((long)(millis() - timeout) < 0L) //wait for transfer completion
                {
                        tmpdata = regRd(rHIRQ);

                        if(tmpdata & bmHXFRDNIRQ) {
                                regWr(rHIRQ, bmHXFRDNIRQ); //clear the interrupt
                                //rcode = 0x00;
                                break;
                        }//if( tmpdata & bmHXFRDNIRQ

                }//while ( millis() < timeout

                rcode = (regRd(rHRSL) & 0x0f); //analyze transfer result

                switch(rcode) {
                        case hrNAK:
                                nak_count++;
                                if(nak_limit && (nak_count == nak_limit))
                                        return (rcode);
                                delayMicroseconds(200);
                                break;
                        case hrTIMEOUT:
                                retry_count++;
                                if(retry_count == UHS_HOST_TRANSFER_RETRY_MAXIMUM)
                                        return (rcode);
                                break;
                        default:
                                return (rcode);
                }//switch( rcode
        }
}

//
// NULL is error, we don't need to know the reason.
//

UHS_EpInfo * UHS_NI MAX3421E_HOST::ctrlReqOpen(uint8_t addr, uint8_t bmReqType, uint8_t bRequest,
        uint8_t wValLo, uint8_t wValHi, uint16_t wInd, uint16_t total, uint8_t *dataptr) {
        uint8_t rcode;
        SETUP_PKT setup_pkt;

        UHS_EpInfo *pep = NULL;
        uint16_t nak_limit = 0;
        MAX_HOST_DEBUG("ctrlReqOpen: addr: 0x%2.2x bmReqType: 0x%2.2x bRequest: 0x%2.2x\r\nctrlReqOpen: wValLo: 0x%2.2x  wValHi: 0x%2.2x wInd: 0x%4.4x total: 0x%4.4x dataptr: 0x%4.4p\r\n", addr, bmReqType, bRequest, wValLo, wValHi, wInd, total, dataptr);
        rcode = SetAddress(addr, 0, &pep, nak_limit);

        if(!rcode) {
                /* fill in setup packet */
                setup_pkt.ReqType_u.bmRequestType = bmReqType;
                setup_pkt.bRequest = bRequest;
                setup_pkt.wVal_u.wValueLo = wValLo;
                setup_pkt.wVal_u.wValueHi = wValHi;
                setup_pkt.wIndex = wInd;
                setup_pkt.wLength = total;

                bytesWr(rSUDFIFO, 8, (uint8_t*)(&setup_pkt)); //transfer to setup packet FIFO

                rcode = dispatchPkt(MAX3421E_tokSETUP, 0, nak_limit); //dispatch packet
                if(!rcode) {
                        if(dataptr != NULL) {
                                if((bmReqType & 0x80) == 0x80) {
                                        pep->bmRcvToggle = 1; //bmRCVTOG1;
                                } else {
                                        pep->bmSndToggle = 1; //bmSNDTOG1;
                                }
                        }
                } else {
                        //                        Serial.println(">>>>>>>>>>>> dispatchPkt Failed <<<<<<<<<<<<<<");
                        //                        Serial.println(rcode, HEX);
                        //                        Serial.println(bmReqType, HEX);
                        //                        Serial.println(bRequest, HEX);
                        //                        Serial.println(">>>>>>>>>>>> dispatchPkt Failed <<<<<<<<<<<<<<");
                        pep = NULL;
                }
        }
        return pep;
}

uint8_t UHS_NI MAX3421E_HOST::ctrlReqRead(UHS_EpInfo *pep, uint16_t *left, uint16_t *read, uint16_t nbytes, uint8_t *dataptr) {
        *read = 0;
        uint16_t nak_limit = 0;
        MAX_HOST_DEBUG("ctrlReqRead left: %i\r\n", *left);
        if(*left) {
again:
                *read = nbytes;
                uint8_t rcode = InTransfer(pep, nak_limit, read, dataptr);
                if(rcode == hrTOGERR) {
                        // yes, we flip it wrong here so that next time it is actually correct!
                        pep->bmRcvToggle = (regRd(rHRSL) & bmSNDTOGRD) ? 0 : 1;
                        goto again;
                }

                if(rcode) {
                        MAX_HOST_DEBUG("ctrlReqRead ERROR: %2.2x, left: %i, read %i\r\n", rcode, *left, *read);
                        return rcode;
                }
                *left -= *read;
                MAX_HOST_DEBUG("ctrlReqRead left: %i, read %i\r\n", *left, *read);
        }
        return 0;
}

uint8_t UHS_NI MAX3421E_HOST::ctrlReqClose(UHS_EpInfo *pep, uint8_t bmReqType, uint16_t left, uint16_t nbytes, uint8_t *dataptr) {
        uint8_t rcode = 0;

        //Serial.println("Closing");
        //Serial.flush();
        if(((bmReqType & 0x80) == 0x80) && pep && left && dataptr) {
                //Serial.println("Drain");
                MAX_HOST_DEBUG("ctrlReqRead Sinking %i\r\n", left);
                // If reading, sink the rest of the data.
                while(left) {
                        uint16_t read = nbytes;
                        rcode = InTransfer(pep, 0, &read, dataptr);
                        if(rcode == hrTOGERR) {
                                // yes, we flip it wrong here so that next time it is actually correct!
                                pep->bmRcvToggle = (regRd(rHRSL) & bmSNDTOGRD) ? 0 : 1;
                                continue;
                        }
                        if(rcode) break;
                        left -= read;
                        if(read < nbytes) break;
                }
                //        } else {
                //                Serial.println("Nothing to drain");
        }
        if(!rcode) {
                //               Serial.println("Dispatching");
                rcode = dispatchPkt(((bmReqType & 0x80) == 0x80) ? MAX3421E_tokOUTHS : MAX3421E_tokINHS, 0, 0); //GET if direction
                //        } else {
                //                Serial.println("Bypassed Dispatch");
        }
        return rcode;
}

/**
 * Bottom half of the ISR task
 */
void UHS_NI MAX3421E_HOST::ISRbottom(void) {
        uint8_t x;
        //        Serial.print("Enter ");
        //        Serial.print((uint32_t)this,HEX);
        //        Serial.print(" ");
        //        Serial.println(usb_task_state, HEX);

        if(condet) {
                islowspeed = (VBUS_changed() == 0);
#if USB_HOST_SHIELD_USE_ISR
                noInterrupts();
#endif
                condet = false;
#if USB_HOST_SHIELD_USE_ISR
                interrupts();
#endif
        }
        switch(usb_task_state) {
                case UHS_USB_HOST_STATE_INITIALIZE:
                        // should never happen...
                        busprobe();
                        islowspeed = (VBUS_changed() == 0);
                        break;
                case UHS_USB_HOST_STATE_DEBOUNCE:
#if 1
                        // This seems to not be needed. The host controller has debounce built in.
                        sof_countdown = UHS_HOST_DEBOUNCE_DELAY_MS;
                        usb_task_state = UHS_USB_HOST_STATE_DEBOUNCE_NOT_COMPLETE;
                        break;
                case UHS_USB_HOST_STATE_DEBOUNCE_NOT_COMPLETE:
                        usb_task_state = UHS_USB_HOST_STATE_RESET_DEVICE;
                        break;
#endif
                case UHS_USB_HOST_STATE_RESET_DEVICE:
                        busevent = true;
                        usb_task_state = UHS_USB_HOST_STATE_RESET_NOT_COMPLETE;
                        regWr(rHIRQ, bmBUSEVENTIRQ); // see data sheet.
                        regWr(rHCTL, bmBUSRST); // issue bus reset
                        break;
                case UHS_USB_HOST_STATE_WAIT_BUS_READY:
                        usb_task_state = UHS_USB_HOST_STATE_CONFIGURING;
                        break; // don't fall through

                case UHS_USB_HOST_STATE_CONFIGURING:
                        usb_task_state = UHS_USB_HOST_STATE_CHECK;
                        x = Configuring(0, 0, islowspeed);
                        usb_error = x;
                        if(usb_task_state == UHS_USB_HOST_STATE_CHECK) {
                                if(x) {
                                        //                               Serial.print("Error 0x");
                                        //                               Serial.println(x, HEX);
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

                case UHS_USB_HOST_STATE_CHECK:
                        // Serial.println((uint32_t)__builtin_return_address(0), HEX);
                        break;
                case UHS_USB_HOST_STATE_CONFIGURING_DONE:
                        usb_task_state = UHS_USB_HOST_STATE_RUNNING;
                        break;
                case UHS_USB_HOST_STATE_RUNNING:
                        Poll_Others();
                        for(x = 0; (usb_task_state == UHS_USB_HOST_STATE_RUNNING) && (x < UHS_HOST_MAX_INTERFACE_DRIVERS); x++) {
                                if(devConfig[x]) {
                                        if(devConfig[x]->bPollEnable) devConfig[x]->Poll();
                                }
                        }
                        // fall thru
                default:
                        // Do nothing
                        break;
        } // switch( usb_task_state )
#if USB_HOST_SHIELD_USE_ISR
        if(condet) {
                islowspeed = (VBUS_changed() == 0);
                noInterrupts();
                condet = false;
                interrupts();
        }
#endif
#ifdef USB_HOST_SHIELD_TIMING_PIN
        // My counter/timer can't work on an inverted gate signal
        // so we gate using a high pulse -- AJK
        UHS_PIN_WRITE(USB_HOST_SHIELD_TIMING_PIN, LOW);
#endif
        usb_task_polling_disabled--;
}


/* USB main task. Services the MAX3421e */
#if !USB_HOST_SHIELD_USE_ISR

void UHS_NI MAX3421E_HOST::ISRTask(void) {
}
void UHS_NI MAX3421E_HOST::Task(void)
#else

void UHS_NI MAX3421E_HOST::Task(void) {
}

void UHS_NI MAX3421E_HOST::ISRTask(void)
#endif
{
        uint8_t tmpdata;

        counted = false;
        if(!UHS_PIN_READ(irq_pin)) {
                uint8_t HIRQALL = regRd(rHIRQ); //determine interrupt source
                uint8_t HIRQ = HIRQALL & IRQ_CHECK_MASK;
                uint8_t HIRQ_sendback = 0x00;
                if(HIRQ & bmFRAMEIRQ) {
                        HIRQ_sendback |= bmFRAMEIRQ;
                        if(sof_countdown) {
                                sof_countdown--;
                                counted = true;
                        }
                        if(sofevent && usb_task_state == UHS_USB_HOST_STATE_WAIT_SOF) {
                                sof_countdown = 20;
                                usb_task_state = UHS_USB_HOST_STATE_WAIT_BUS_READY;
                        }
                        sofevent = false;
                }
                if(HIRQ & bmCONDETIRQ) {
                        //                        Serial.print("CONDET ");
                        //                        Serial.println(usb_task_state, HEX);
                        HIRQ_sendback |= bmCONDETIRQ;
                        if(!busevent) {
                                condet = true;
                        }
                        busprobe();
                }
                if(HIRQ & bmBUSEVENTIRQ) {
                        //                        Serial.print("BUSEVT ");
                        //                        Serial.println(usb_task_state, HEX);
                        HIRQ_sendback |= bmBUSEVENTIRQ;
                        if(busevent && usb_task_state == UHS_USB_HOST_STATE_RESET_NOT_COMPLETE) {
                                usb_task_state = UHS_USB_HOST_STATE_WAIT_SOF;
                                sofevent = true;
                                tmpdata = regRd(rMODE) | bmSOFKAENAB; //start SOF generation
                                regWr(rMODE, tmpdata);
                        }
                        busevent = false;
                }
                regWr(rHIRQ, HIRQ_sendback);

                if(!sof_countdown && !counted && !usb_task_polling_disabled) {
                        usb_task_polling_disabled++;
#ifdef USB_HOST_SHIELD_TIMING_PIN
        // My counter/timer can't work on an inverted gate signal
        // so we gate using a high pulse -- AJK
        UHS_PIN_WRITE(USB_HOST_SHIELD_TIMING_PIN, HIGH);
#endif

#if defined(SWI_IRQ_NUM)
                        //                                Serial.println("--------------- Doing SWI ----------------");
                        //                                Serial.flush();
                        exec_SWI(this);
#else
#if USB_HOST_SHIELD_USE_ISR
                        // Enable interrupts
                        interrupts();
#endif /* USB_HOST_SHIELD_USE_ISR */
                        ISRbottom();
#endif /* SWI_IRQ_NUM */
                }
        }
}


#else
#error "Never include USB_HOST_SHIELD_INLINE.h, include UHS_host.h instead"
#endif
