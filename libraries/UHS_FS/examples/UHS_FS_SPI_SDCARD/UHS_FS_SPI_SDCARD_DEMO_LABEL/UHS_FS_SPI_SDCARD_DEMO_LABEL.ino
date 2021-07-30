#if defined(__MK66FX1M0__)
// Teensy 3.6 :-)
#define SDCARD_CS_PIN 62
#define SDCARD_DETECT_PIN 37

#else
#define SDCARD_CS_PIN 5
// this pin needs to be on a real IRQ.
// All boards should be able to use pin2.
#define SDCARD_DETECT_PIN 2
#endif

// inline library loading
// Patch printf so we can use it.
#define LOAD_UHS_PRINTF_HELPER
// RTC/clock
#define LOAD_RTCLIB
// Filesystem
#define LOAD_GENERIC_STORAGE

#define UHS_USE_SDCARD
#define UHS_MAX_SD_CARDS 1

/* The _CODE_PAGE specifies the OEM code page to be used on the target system.
   Incorrect setting of the code page can cause a file open failure.

    1    - ASCII only (ONLY VALID for non LFN cfg.)
    437  - U.S. (OEM)
    720  - Arabic (OEM)
    737  - Greek (OEM)
    775  - Baltic (OEM)
    850  - Multilingual Latin 1 (OEM)
    858  - Multilingual Latin 1 + Euro (OEM)
    852  - Latin 2 (OEM)
    855  - Cyrillic (OEM)
    866  - Russian (OEM)
    857  - Turkish (OEM)
    862  - Hebrew (OEM)
    874  - Thai (OEM, Windows)
    932  - Japanese Shift-JIS (DBCS, OEM, Windows)
    936  - Simplified Chinese GBK (DBCS, OEM, Windows)
    949  - Korean (DBCS, OEM, Windows)
    950  - Traditional Chinese Big5 (DBCS, OEM, Windows)
    1250 - Central Europe (Windows)
    1251 - Cyrillic (Windows)
    1252 - Latin 1 (Windows)
    1253 - Greek (Windows)
    1254 - Turkish (Windows)
    1255 - Hebrew (Windows)
    1256 - Arabic (Windows)
    1257 - Baltic (Windows)
    1258 - Vietnam (OEM, Windows)
 */
// default 3, 437
//#define _USE_LFN 3
//#define _CODE_PAGE 437

// Set how many file and directory objects allowed opened at once.
// Values from >= 1 are acceptable.
//
// Caution! These use some RAM even if you do not use them all.
// Use a setting that is practical!
//
// A setting of |  max files opened     | max directories open
// -------------+-----------------------+---------------------
// 1            |       1               | 1
// 2            |       2               | 2
// ...          |       ...             | ...
// 254          |       254             | 254
// 255          |       255             | 255
// ...          |       ...             | ...
// 1000         |       1000            | 1000
// ...          |       ...             | ...

// default 1
//#define	_FS_LOCK 1


#include <Arduino.h>
#ifdef true
#undef true
#endif
#ifdef false
#undef false
#endif
#ifdef __AVR__
#include <avr/io.h>
#endif





















//#include <stdio.h>
#include <UHS_FS.h>

uint8_t mounted = 0;
uint8_t wasmounted = 0; // Initial state that's expected is none.

void setup() {
#if !defined(STDIO_IS_OK_TO_USE_AS_IS)
        while(!USB_HOST_SERIAL);
        USB_HOST_SERIAL.begin(115200);
        delay(1000);
        UHS_printf_HELPER_init();
        USB_HOST_SERIAL.println("Start.");
#else
        printf_P(PSTR("Start."));
#endif
        // Initialize generic storage.
        int detpins[UHS_MAX_SD_CARDS] = {SDCARD_DETECT_PIN}; // list of sdcard detection pins
        int cspins[UHS_MAX_SD_CARDS] = {SDCARD_CS_PIN}; // list of CS pins
        Init_Generic_Storage(detpins, cspins);
}

uint8_t current_state = 128;
uint8_t last_state = 255;
char *VOL_LABEL;

void loop() {
        mounted = fs_mountcount();
        if(mounted != wasmounted) {
                wasmounted = mounted;
                if(mounted == 1) {
                        VOL_LABEL = fs_mount_lbl(0);
                        if(VOL_LABEL != NULL) {
                                printf_P(PSTR("Volume %s is mounted.\r\n"), VOL_LABEL);
                                bool good = false;
                                uint8_t p;
                                uint8_t c;
                                while(!good) {
                                        yield();
                                        p = 0;
                                        c = 0;
                                        printf_P(PSTR("Enter a valid new label, 11 characters maximum. Control-C to abort.\r\n"), VOL_LABEL);
                                        fflush(stdout);
                                        char label[12];
                                        for(int i = 0; i < 12; i++) {
                                                label[i] = 0;
                                        }
                                        while(c != 0x0d && c != 0x0a && c != 0x03 && p < 12) {
                                                yield();
                                                if(USB_HOST_SERIAL.available()) {
                                                        c = toupper(USB_HOST_SERIAL.read());
                                                        printf("%c", c);
                                                        fflush(stdout);
                                                        if(c != 0x0a && c != 0x0d) {
                                                                label[p++] = c;
                                                        }
                                                }
                                        }
                                        printf("\r\n");
                                        fflush(stdout);
                                        if(c == 0x03) {
                                                good = true;
                                        } else {
                                                c = fs_setlabel(VOL_LABEL, (const char *)&label);
                                                if(c == FR_OK) {
                                                        good = true;
                                                } else {
                                                        printf_P(PSTR("Error %2.2x.\r\n"), c);
                                                        if(c == FR_INVALID_NAME) {
                                                                printf_P(PSTR("Error, name invalid. Try again.\r\n"));
                                                        } else {
                                                                good = true;
                                                                c = 0x03;
                                                        }
                                                }
                                        }
                                }
                                // we're done with the volume label, free the memory.
                                free(VOL_LABEL);
                                VOL_LABEL = fs_mount_lbl(0);
                                if(c != 0x03) {
                                        if(VOL_LABEL != NULL) {
                                                printf_P(PSTR("Volume %s is mounted.\r\n"), VOL_LABEL);
                                                // you can do other stuff here...
                                        }
                                }
                        }
                } else {
                        printf_P(PSTR("No media.\r\n"));
                }

        }
}
