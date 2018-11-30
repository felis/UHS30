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

#ifndef UHS_PRINTF_HELPER_H
#define	UHS_PRINTF_HELPER_H

#if defined(LOAD_UHS_PRINTF_HELPER)
#include <Arduino.h>
#ifdef true
#undef true
#endif
#ifdef false
#undef false
#endif

#ifndef SERIAL_PORT_MONITOR
#define SERIAL_PORT_MONITOR Serial
#endif

#ifndef USB_HOST_SERIAL
#if defined(SERIAL_PORT_USBVIRTUAL) && defined(LOAD_UHS_KINETIS_FS_HOST)
#define USB_HOST_SERIAL SERIAL_PORT_HARDWARE
#else
#define USB_HOST_SERIAL SERIAL_PORT_MONITOR
#endif
#endif



#if !defined(NOTUSED)
#define NOTUSED(...)  __VA_ARGS__ __attribute__((unused))
#endif


#ifndef __AVR__
#ifndef printf_P
#define printf_P(...) printf(__VA_ARGS__)
#endif
#endif

#if defined(ARDUINO_ARCH_PIC32)
/*
 * For printf() output with pic32 Arduino
 */
extern "C"
{
        void _mon_putc(char s) {
                USB_HOST_SERIAL.write(s);
        }
        int _mon_getc() {
                while(!USB_HOST_SERIAL.available());
                return USB_HOST_SERIAL.read();
        }
}

#elif defined(__AVR__)
extern "C" {

        static FILE tty_stdio;
        static FILE tty_stderr;

        static int NOTUSED(tty_stderr_putc(char c, NOTUSED(FILE *t)));
        static int NOTUSED(tty_stderr_flush (NOTUSED(FILE *t)));
        static int NOTUSED(tty_std_putc(char c, NOTUSED(FILE *t)));
        static int NOTUSED(tty_std_getc(NOTUSED(FILE *t)));
        static int NOTUSED(tty_std_flush(NOTUSED(FILE *t)));

        static int tty_stderr_putc(char c, NOTUSED(FILE *t)) {
                USB_HOST_SERIAL.write(c);
                return 0;
        }

        static int tty_stderr_flush (NOTUSED(FILE *t)) {
                USB_HOST_SERIAL.flush();
                return 0;
        }

        static int tty_std_putc(char c, NOTUSED(FILE *t)) {
                USB_HOST_SERIAL.write(c);
                return 0;
        }

        static int tty_std_getc(NOTUSED(FILE *t)) {
                while(!USB_HOST_SERIAL.available());
                return USB_HOST_SERIAL.read();
        }

        static int tty_std_flush(NOTUSED(FILE *t)) {
                USB_HOST_SERIAL.flush();
                return 0;
        }
}
#elif defined(CORE_TEENSY)
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
#elif defined(ARDUINO_SAMD_ZERO) || defined(ARDUINO_SAM_DUE) ||  defined(ARDUINO_spresense_ast)
// Nothing to do, stdout/stderr is on programming port
#else
#error no STDOUT
#endif // defined(ARDUINO_ARCH_PIC32)



#if defined(__AVR__)
// The only wierdo in the bunch...
void UHS_printf_HELPER_init(void) {
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

}
#else
// Only AVR (so far) needs to init.
#define UHS_printf_HELPER_init() (void(0))
#endif

#else
#define UHS_printf_HELPER_init() (void(0))
#endif  /* load.... */
#endif	/* UHS_PRINTF_HELPER_H */

