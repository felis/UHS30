#include <Arduino.h>
#ifdef true
#undef true
#endif
#ifdef false
#undef false
#endif

#define LOAD_USB_HOST_SYSTEM
#define LOAD_UHS_KINETIS_FS_HOST
#define ENABLE_UHS_DEBUGGING 1
#define DEBUG_PRINTF_EXTRA_HUGE 1

#define USB_HOST_SERIAL SERIAL_PORT_HARDWARE
// SERIAL_PORT_MONITOR

#include <stdio.h>

#ifndef __AVR__
#ifndef printf_P
#define printf_P(...) printf(__VA_ARGS__)
#endif
#endif

#if defined(__arm__)
#include <dyn_SWI.h>
#endif

#include <Wire.h>
#include <SPI.h>
#include <UHS_host.h>
#include <UHS_KINETIS_FS_HOST.h>

uint8_t rcode;
uint8_t usbstate;
uint8_t laststate;
USB_DEVICE_DESCRIPTOR buf;

UHS_KINETIS_FS_HOST UHS_Usb;

#if defined(__AVR__)
extern "C" {

        static FILE tty_stdio;
        static FILE tty_stderr;

        static int tty_stderr_putc(char c, FILE *t) {
                USB_HOST_SERIAL.write(c);
                return 0;
        }

        static int tty_stderr_flush(FILE *t) {
                USB_HOST_SERIAL.flush();
                return 0;
        }

        static int tty_std_putc(char c, FILE *t) {
                USB_HOST_SERIAL.write(c);
                return 0;
        }

        static int tty_std_getc(FILE *t) {
                while(!USB_HOST_SERIAL.available());
                return USB_HOST_SERIAL.read();
        }

        static int tty_std_flush(FILE *t) {
                USB_HOST_SERIAL.flush();
                return 0;
        }
}
#else
#if defined(CORE_TEENSY)
extern "C" {

        int _write(int fd, const char *ptr, int len) {
                int j;
                for(j = 0; j < len; j++) {
                        if(fd == 1)
                                USB_HOST_SERIAL.write(*ptr++);
                        else if(fd == 2)
                                USB_HOST_SERIAL.write(*ptr++);
                }
                return len;
        }

        int _read(int fd, char *ptr, int len) {
                if(len > 0 && fd == 0) {
                        while(!USB_HOST_SERIAL.available());
                        *ptr = USB_HOST_SERIAL.read();
                        return 1;
                }
                return 0;
        }

#include <sys/stat.h>

        int _fstat(int fd, struct stat *st) {
                memset(st, 0, sizeof (*st));
                st->st_mode = S_IFCHR;
                st->st_blksize = 1024;
                return 0;
        }

        int _isatty(int fd) {
                return (fd < 3) ? 1 : 0;
        }
}
#endif // TEENSY_CORE
#endif // AVR

uint8_t retries;
UHS_Device *p;
uint8_t dtp;

void setup() {
        laststate = 0;
        USB_HOST_SERIAL.begin(115200);
#if defined(__AVR__)
        // Set up stdio/stderr
        tty_stdio.put = tty_std_putc;
        tty_stdio.get = tty_std_getc;
        tty_stdio.flags = _FDEV_SETUP_RW;
        tty_stdio.udata = 0;

        tty_stderr.put = tty_stderr_putc;
        tty_stderr.get = NULL;
        tty_stderr.flags = _FDEV_SETUP_WRITE;
        tty_stderr.udata = 0;

        stdout = &tty_stdio;
        stdin = &tty_stdio;
        stderr = &tty_stderr;
#endif

#ifdef BOARD_MEGA_ADK
        // For Mega ADK, which has a Max3421e on-board, set MAX_RESET to output mode, and then set it to HIGH
        pinMode(55, OUTPUT);
        UHS_PIN_WRITE(55, HIGH);
#endif
#if defined(SWI_IRQ_NUM)
        Init_dyn_SWI();
#endif
        while(UHS_Usb.Init(1000) == -1);
        printf_P(PSTR("\r\n\r\n\r\n\r\nUSB HOST READY.\r\n"));

}

void loop() {
        usbstate = UHS_Usb.getUsbTaskState();
        if(usbstate != laststate) {
                p = NULL;
                laststate=usbstate;
                switch( usbstate ) {
                        case( UHS_USB_HOST_STATE_ERROR ):
                                UHS_Usb.doSoftReset(0, 0, 0);
                                p = UHS_Usb.addrPool.GetUsbDevicePtr(0);
                                if(p) {
                                        dtp = 0;
                                }
                        // fall thru
                        case( UHS_USB_HOST_STATE_RUNNING ):
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
                                        printf_P(PSTR("\r\nDescriptor Length:\t0x%2.2x"),buf.bLength);
                                        printf_P(PSTR("\r\nDescriptor type:\t0x%2.2x"),buf.bDescriptorType);
                                        printf_P(PSTR("\r\nUSB version:\t\t0x%4.4x"),buf.bcdUSB);
                                        printf_P(PSTR("\r\nDevice class:\t\t0x%2.2x"),buf.bDeviceClass);
                                        printf_P(PSTR("\r\nDevice Subclass:\t0x%2.2x"),buf.bDeviceSubClass);
                                        printf_P(PSTR("\r\nDevice Protocol:\t0x%2.2x"),buf.bDeviceProtocol);
                                        printf_P(PSTR("\r\nMax.packet size:\t0x%2.2x"),buf.bMaxPacketSize0);
                                        printf_P(PSTR("\r\nVendor  ID:\t\t0x%4.4x"),buf.idVendor);
                                        printf_P(PSTR("\r\nProduct ID:\t\t0x%4.4x"),buf.idProduct);
                                        printf_P(PSTR("\r\nRevision ID:\t\t0x%4.4x"),buf.bcdDevice);
                                        printf_P(PSTR("\r\nMfg.string index:\t0x%2.2x"),buf.iManufacturer);
                                        printf_P(PSTR("\r\nProd.string index:\t0x%2.2x"),buf.iProduct);
                                        printf_P(PSTR("\r\nSerial number index:\t0x%2.2x"),buf.iSerialNumber);
                                        printf_P(PSTR("\r\nNumber of conf.:\t0x%2.2x"),buf.bNumConfigurations);
                                        /**/
                                        printf_P(PSTR("\r\n\nInsert Another device."));
                                }
                                break;

                        default:
                                break;
                }
                fflush(stdout);
        }
}
