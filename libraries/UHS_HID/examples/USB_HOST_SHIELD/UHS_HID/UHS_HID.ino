#define LOAD_USB_HOST_SYSTEM
#define LOAD_USB_HOST_SHIELD
#define LOAD_UHS_HID
#define _USE_MAX3421E_HOST 1
#define USB_HOST_SERIAL SERIAL_PORT_MONITOR
#define USB_HOST_SHIELD_USE_ISR 1

#include <Arduino.h>
#ifdef true
#undef true
#endif
#ifdef false
#undef false
#endif


#if defined(__arm__)
#include <dyn_SWI.h>
#endif

#include <stdio.h>
#include <Wire.h>
#include <SPI.h>
#include <UHS_host.h>
#include <USB_HOST_SHIELD.h>
#include <UHS_HUB.h>


MAX3421E_HOST MAX3421E_Usb;
UHS_USBHub hub_MAX3421E(&MAX3421E_Usb);




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



void setup() {
        Serial.begin( 115200 );
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
#if defined(SWI_IRQ_NUM)
        Init_dyn_SWI();
#endif
        while(MAX3421E_Usb.Init(1000) != 0);


}

void loop() {
        delay(1);
}
