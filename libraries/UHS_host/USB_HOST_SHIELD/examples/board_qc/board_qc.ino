/* USB Host Shield 3.0 library. Board quality control application, IRQ mode */
/* To see the output set your terminal speed to 115200 */
/* for GPIO test to pass you need to connect GPIN0 to GPOUT7, GPIN1 to GPOUT6, etc. */
/* otherwise press any key after getting GPIO error to complete the test */


// inline library loading
// Patch printf so we can use it.
#define LOAD_UHS_PRINTF_HELPER
// Load the USB Host System core
#define LOAD_USB_HOST_SYSTEM
// Load USB Host Shield
#define LOAD_USB_HOST_SHIELD


#include <Arduino.h>
#ifdef true
#undef true
#endif
#ifdef false
#undef false
#endif


#include <UHS_host.h>

uint8_t rcode;
uint8_t usbstate;
uint8_t laststate;
USB_DEVICE_DESCRIPTOR buf;

MAX3421E_HOST UHS_Usb;

uint8_t retries;
UHS_Device *p;
uint8_t dtp;

/* constantly transmits 0x55 via SPI to aid probing */
void halt55() {
        printf_P(PSTR("\r\nUnrecoverable error - test halted!!"));
        printf_P(PSTR("\r\n0x55 pattern is transmitted via SPI"));
        printf_P(PSTR("\r\nPress RESET to restart test"));
        fflush(stdout);

        while( 1 ) {
                UHS_Usb.regWr( 0x55, 0x55 );
        }
}

/* prints "Press any key" and returns when key is pressed */
void press_any_key() {
        printf_P(PSTR("\r\nPress any key to continue..."));
        fflush(stdout);
        while(USB_HOST_SERIAL.available()) USB_HOST_SERIAL.read(); //empty input buffer
        while( !USB_HOST_SERIAL.available() ); //wait for input
        while(USB_HOST_SERIAL.available()) USB_HOST_SERIAL.read(); //empty input buffer
        return;
}

void setup() {
        laststate = 0;
        USB_HOST_SERIAL.begin( 115200 );

#ifdef BOARD_MEGA_ADK
        // For Mega ADK, which has a Max3421e on-board, set MAX_RESET to output mode, and then set it to HIGH
        DDRJ |= 0x04; // output
        PORTJ |= 0x04;
#endif
        // Manually initialize ahead of time
        Init_dyn_SWI();
        UHS_printf_HELPER_init();
        SPI.begin();

        UHS_Usb.ss_pin = UHS_MAX3421E_SS;
        UHS_Usb.irq_pin = UHS_MAX3421E_INT;
        UHS_Usb.MAX3421E_SPI_Settings = SPISettings(UHS_MAX3421E_SPD, MSBFIRST, SPI_MODE0);

#ifdef BOARD_MEGA_ADK
        DDRE &= ~0x40; // input
        PORTE |= 0x40; // pullup
#else
        pinMode(UHS_Usb.irq_pin, INPUT);
        UHS_PIN_WRITE(UHS_Usb.irq_pin, HIGH);
#endif
        pinMode(UHS_Usb.ss_pin, OUTPUT);
        UHS_PIN_WRITE(UHS_Usb.ss_pin, HIGH);

        printf_P(PSTR("\r\nCircuits At Home 2011"));
        printf_P(PSTR("\r\nUSB Host Shield Quality Control Routine"));
        // Do a reset ahead of time, since sometimes the chip is in an unknown state.
        UHS_Usb.regWr(rPINCTL, (bmFDUPSPI | bmINTLEVEL | GPX_VBDET));
        if(UHS_Usb.reset() == 0) {
                printf_P(PSTR("Initial reset failed."));
                halt55();
        }

        /* SPI quick test - check revision register */
        printf_P(PSTR("\r\nReading REVISION register... Die revision "));
        fflush(stdout);
        {
                uint8_t tmpbyte = UHS_Usb.regRd( rREVISION );
                switch( tmpbyte ) {
                        case( 0x01):  //rev.01
                        case( 0x12):  //rev.02
                        case( 0x13):  //rev.03
                                printf_P(PSTR("%2.2x"), tmpbyte & 3);
                                break;
                        default:
                                printf_P(PSTR("invalid. Value returned: %2.2x"), tmpbyte);
                                halt55();
                                break;
                }//switch( tmpbyte...
        }//check revision register

        /* SPI long test */
        {
                printf_P(PSTR("\r\nSPI long test. Transfers 1MB of data. Each dot is 64K \r\n"));
                fflush(stdout);
                uint8_t sample_wr = 0;
                uint8_t sample_rd = 0;
                uint8_t gpinpol_copy = UHS_Usb.regRd( rGPINPOL );
                for( uint8_t i = 0; i < 16; i++ ) {
                        for( uint16_t j = 0; j < 65535; j++ ) {
                                UHS_Usb.regWr( rGPINPOL, sample_wr );
                                sample_rd = UHS_Usb.regRd( rGPINPOL );
                                if( sample_rd != sample_wr ) {
                                        printf_P(PSTR("\r\nTest failed. Wrote 0x%2.2x, read 0x%2.2x\r\n"), sample_wr, sample_rd);
                                        halt55();
                                }//if( sample_rd != sample_wr..
                                sample_wr++;
                        }//for( uint16_t j...
                        printf_P(PSTR("."));
                        fflush(stdout);
                }//for( uint8_t i...
                UHS_Usb.regWr( rGPINPOL, gpinpol_copy );
                printf_P(PSTR("\r\nSPI long test passed"));
        }//SPI long test

#ifndef BOARD_MEGA_ADK
        /* GPIO test , not possible on ADK */
        /* in order to simplify board layout, GPIN pins on text fixture are connected to GPOUT */
        /* in reverse order, i.e, GPIN0 is connected to GPOUT7, GPIN1 to GPOUT6, etc. */
        {
                uint8_t tmpbyte;
                printf_P(PSTR("\r\nGPIO test. Connect GPIN0 to GPOUT7, GPIN1 to GPOUT6, and so on"));
                fflush(stdout);
                for( uint8_t sample_gpio = 0; sample_gpio < 255; sample_gpio++ ) {
                        UHS_Usb.gpioWr( sample_gpio );
                        tmpbyte = UHS_Usb.gpioRd();
                        /* bit reversing code copied vetbatim from http://graphics.stanford.edu/~seander/bithacks.html#BitReverseObvious */
                        tmpbyte  = ((tmpbyte * 0x0802LU & 0x22110LU) | (tmpbyte * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16;
                                if( sample_gpio != tmpbyte ) {
                                printf_P(PSTR("\r\nTest failed. Wrote 0x%2.2x, read 0x%2.2x\r\n"), sample_gpio, tmpbyte);
                                press_any_key();
                                break;
                        }//if( sample_gpio != tmpbyte...
                }//for( uint8_t sample_gpio...
                        printf_P(PSTR("\r\nGPIO test passed."));
        }//GPIO test
#endif
        /* PLL test. Stops/starts MAX3421E oscillator several times */
        {
                printf_P(PSTR("\r\nPLL test. 100 chip resets will be performed"));
                fflush(stdout);
                /* check current state of the oscillator */
                if(!( UHS_Usb.regRd( rUSBIRQ ) & bmOSCOKIRQ )) {  //wrong state - should be on
                        printf_P(PSTR("\r\nCurrent oscillator state unexpected."));
                        press_any_key();
                }
                /* Restart oscillator */
                printf_P(PSTR("\r\nReset oscillator test."));
                fflush(stdout);
                for( uint16_t i = 0; i < 100; i++ ) {
                        printf_P(PSTR("\r\nReset number %i\t"), i+1);
                        fflush(stdout);
                        UHS_Usb.regWr( rUSBCTL, bmCHIPRES );  //reset
                        if( UHS_Usb.regRd( rUSBIRQ ) & bmOSCOKIRQ ) { //wrong state - should be off
                                printf_P(PSTR("\r\nCurrent oscillator state unexpected."));
                                halt55();
                        }
                        UHS_Usb.regWr( rUSBCTL, 0x00 ); //release from reset
                        uint16_t j = UHS_Usb.reset();
                        if( j == 0 ) {
                                printf_P(PSTR("PLL failed to stabilize"));
                                press_any_key();
                        } else {
                                printf_P(PSTR("Time to stabilize, about %u microseconds"), j);
                                fflush(stdout);
                        }
                }
        }//PLL test
        /* initializing USB stack */
        // SPI.end();
        if (UHS_Usb.Init(1000) == -1) {
                printf_P(PSTR("\r\nOSCOKIRQ failed to assert"));
                halt55();
        }
        printf_P(PSTR("\r\nChecking USB device communication.\r\n"));
}


void loop() {
        //delay( 1 );
        //UHS_Usb.Task();
        usbstate = UHS_Usb.getUsbTaskState();
        if(usbstate != laststate) {
                p = NULL;
                laststate=usbstate;
                /**/
                switch( usbstate ) {
                        case( UHS_USB_HOST_STATE_IDLE ):
                                printf_P(PSTR("\r\nWaiting for device..."));
                                break;
                        case( UHS_USB_HOST_STATE_RESET_DEVICE ):
                                printf_P(PSTR("\r\nDevice connected. Resetting..."));
                                break;
                        case( UHS_USB_HOST_STATE_WAIT_SOF ):
                                printf_P(PSTR("\r\nReset complete. Waiting for the first SOF..."));
                                break;
                        case( UHS_USB_HOST_STATE_ERROR ):
                                printf_P(PSTR("\r\nUSB Device detected.\r\nPerforming a bus reset... "));
                                UHS_Usb.doSoftReset(0, 0, 0);
                                p = UHS_Usb.addrPool.GetUsbDevicePtr(0);
                                if(p) {
                                        printf_P(PSTR("\r\nGot device pointer at 0"));
                                        dtp = 0;
                                }
                        // fall thru
                        //case( UHS_USB_HOST_STATE_RUNNING ):
                                if(!p) {
                                        UHS_Usb.doSoftReset(0, 0, 1);
                                        p = UHS_Usb.addrPool.GetUsbDevicePtr(1);
                                        if(!p) {
                                                printf_P(PSTR("\r\nError Getting device pointer"));
                                        } else {
                                                printf_P(PSTR("\r\nGot device pointer at 1"));
                                                dtp = 1;
                                        }
                                }

                                retries = 0;
                                again:
                                printf_P(PSTR("\r\nGetting device descriptor"));
                                rcode = UHS_Usb.getDevDescr( dtp, sizeof(USB_DEVICE_DESCRIPTOR), (uint8_t*)&buf );

                                if( rcode ) {
                                        printf_P(PSTR("\r\nError reading device descriptor. Error code 0x%2.2x\r\n"),rcode);
                                        if(rcode == UHS_HOST_ERROR_JERR && retries < 3) {
                                                delay(100);
                                                retries++;
                                                goto again;
                                        }
                                } else {
                                        /**/
                                        printf_P(PSTR("\r\n\r\nDescriptor Length:\t0x%2.2x"),buf.bLength);
                                        printf_P(PSTR("\r\nDescriptor type:\t0x%2.2x"),buf.bDescriptorType);
                                        printf_P(PSTR("\r\nUSB version:\t0x%4.4x"),buf.bcdUSB);
                                        printf_P(PSTR("\r\nDevice class:\t0x%2.2x"),buf.bDeviceClass);
                                        printf_P(PSTR("\r\nDevice Subclass:\t0x%2.2x"),buf.bDeviceSubClass);
                                        printf_P(PSTR("\r\nDevice Protocol:\t0x%2.2x"),buf.bDeviceProtocol);
                                        printf_P(PSTR("\r\nMax.packet size:\t0x%2.2x"),buf.bMaxPacketSize0);
                                        printf_P(PSTR("\r\nVendor  ID:\t0x%4.4x"),buf.idVendor);
                                        printf_P(PSTR("\r\nProduct ID:\t0x%4.4x"),buf.idProduct);
                                        printf_P(PSTR("\r\nRevision ID:\t0x%4.4x"),buf.bcdDevice);
                                        printf_P(PSTR("\r\nMfg.string index:\t0x%2.2x"),buf.iManufacturer);
                                        printf_P(PSTR("\r\nProd.string index:\t0x%2.2x"),buf.iProduct);
                                        printf_P(PSTR("\r\nSerial number index:\t0x%2.2x"),buf.iSerialNumber);
                                        printf_P(PSTR("\r\nNumber of conf.:\t0x%2.2x\r\n\r\n"),buf.bNumConfigurations);
                                        /**/
                                        printf_P(PSTR("\r\n\nAll tests passed. Press RESET to restart test"));
                                }
                                break;

                        default:
                                break;
                }//switch( usbstate...
                fflush(stdout);
        }
}//loop()...

