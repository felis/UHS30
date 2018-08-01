// define the label of your filesystem. VOL_PATH must end in '/'
// Example:
// #define VOL_LABEL "/foo"
// #define VOL_PATH "/foo/"

#define VOL_LABEL "/"
#define VOL_PATH "/"

// inline library loading
// Patch printf so we can use it.
#define LOAD_UHS_PRINTF_HELPER
// Load the USB Host System core
#define LOAD_USB_HOST_SYSTEM
// Load USB Host Shield
#define LOAD_USB_HOST_SHIELD
// Bulk Storage
#define LOAD_UHS_BULK_STORAGE
// RTC/clock
#define LOAD_RTCLIB
// Filesystem
#define LOAD_GENERIC_STORAGE

#include <Arduino.h>
#ifdef true
#undef true
#endif
#ifdef false
#undef false
#endif

// This figures out how much of the demo we can use
#ifdef __AVR__
#include <avr/io.h>
#endif

#ifndef RAMSIZE
#if defined(RAMEND)
#if defined(RAMSTART)
#define RAMSIZE (RAMEND - RAMSTART)
#else
#define RAMSIZE RAMEND
#endif
#endif
#endif


#ifndef FLASHSIZE
#if defined(FLASHEND)
#if defined(FLASHSTART)
#define RAMSIZE (FLASHEND - FLASHSTART)
#else
#define FLASHSIZE FLASHEND
#endif
#endif
#endif
#ifdef RAMSIZE
#if (RAMSIZE < 4094)
#define RAM_TOO_SMALL 1
#endif
#endif
#ifdef FLASHSIZE
#if (FLASHSIZE < 65000)
#define FLASH_TOO_SMALL 1
#endif
#endif

#ifndef RAM_TOO_SMALL
#define RAM_TOO_SMALL 0
#endif
#ifndef FLASH_TOO_SMALL
#define FLASH_TOO_SMALL 0
#endif

#if RAM_TOO_SMALL || FLASH_TOO_SMALL
#define MAKE_BIG_DEMO 0
#else
#define MAKE_BIG_DEMO 1
// Use USB hub
#define LOAD_UHS_HUB
#endif


#if RAM_TOO_SMALL
#define TESTdsize 128
#else
#define TESTdsize 512
#endif

#define TESTcycles (1048576/TESTdsize)


#define _USE_FASTSEEK 0

/* The _CODE_PAGE specifies the OEM code page to be used on the target system.
 * Incorrect setting of the code page can cause a file open failure.
 *
 *  1    - ASCII only (ONLY VALID for non LFN cfg.)
 *  437  - U.S. (OEM)
 *  720  - Arabic (OEM)
 *  737  - Greek (OEM)
 *  775  - Baltic (OEM)
 *  850  - Multilingual Latin 1 (OEM)
 *  858  - Multilingual Latin 1 + Euro (OEM)
 *  852  - Latin 2 (OEM)
 *  855  - Cyrillic (OEM)
 *  866  - Russian (OEM)
 *  857  - Turkish (OEM)
 *  862  - Hebrew (OEM)
 *  874  - Thai (OEM, Windows)
 *  932  - Japanese Shift-JIS (DBCS, OEM, Windows)
 *  936  - Simplified Chinese GBK (DBCS, OEM, Windows)
 *  949  - Korean (DBCS, OEM, Windows)
 *  950  - Traditional Chinese Big5 (DBCS, OEM, Windows)
 *  1250 - Central Europe (Windows)
 *  1251 - Cyrillic (Windows)
 *  1252 - Latin 1 (Windows)
 *  1253 - Greek (Windows)
 *  1254 - Turkish (Windows)
 *  1255 - Hebrew (Windows)
 *  1256 - Arabic (Windows)
 *  1257 - Baltic (Windows)
 *  1258 - Vietnam (OEM, Windows)
 */
#define _USE_LFN 3
#define _CODE_PAGE 437

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

#define	_FS_LOCK 1


#include <RTClib.h>
#include <UHS_host.h>


MAX3421E_HOST UHS_Usb;
#if MAKE_BIG_DEMO
UHS_USBHub hub_1(&UHS_Usb);
PFAT_DIRINFO *de;
uint8_t *data;
#endif

uint8_t mounted = PFAT_VOLUMES;
uint8_t wasmounted = 0;


#if MAKE_BIG_DEMO

void show_dir(PFAT_DIRINFO *de) {
        int res;
        uint64_t fre;
        uint32_t numlo;
        uint32_t numhi;
        int fd = fs_opendir(VOL_PATH);
        if(fd > 0) {
                printf_P(PSTR("Directory of '" VOL_PATH "'\r\n"));
                do {
                        res = fs_readdir(fd, de);
                        if(!res) {
                                DateTime tstamp(de->fdate, de->ftime);
                                if(!(de->fattrib & AM_VOL)) {
                                        if(de->fattrib & AM_DIR) {
                                                printf_P(PSTR("d"));
                                        } else printf_P(PSTR("-"));

                                        if(de->fattrib & AM_RDO) {
                                                printf_P(PSTR("r-"));
                                        } else printf_P(PSTR("rw"));

                                        if(de->fattrib & AM_HID) {
                                                printf_P(PSTR("h"));
                                        } else printf_P(PSTR("-"));

                                        if(de->fattrib & AM_SYS) {
                                                printf_P(PSTR("s"));
                                        } else printf_P(PSTR("-"));


                                        if(de->fattrib & AM_ARC) {
                                                printf_P(PSTR("a"));
                                        } else printf_P(PSTR("-"));



                                        numlo = de->fsize % 100000000llu;
                                        numhi = de->fsize / 100000000llu;
                                        if(numhi) {
                                                printf(" %lu%08lu", numhi, numlo);
                                        } else {
                                                printf(" %12lu", numlo);
                                        }
                                        printf_P(PSTR(" %.4u-%.2u-%.2u"), tstamp.year(), tstamp.month(), tstamp.day());
                                        printf_P(PSTR(" %.2u:%.2u:%.2u"), tstamp.hour(), tstamp.minute(), tstamp.second());
                                        printf_P(PSTR(" %s"), de->fname);
                                        if(de->lfname[0] != 0) {
                                                printf_P(PSTR(" (%s)"), de->lfname);
                                        }
                                        printf_P(PSTR("\r\n"));
                                }
                        }

                } while(!res);
                //printf("CLOSEDIR\r\n");
                //fflush(stdout);
                //delay(1000);

                fs_closedir(fd);

                fre = fs_getfree(VOL_PATH);
                numlo = fre % 100000000llu;
                numhi = fre / 100000000llu;

                if(numhi) {
                        printf("%lu%08lu", numhi, numlo);
                } else {
                        printf("%lu", numlo);
                }
                printf_P(PSTR(" bytes available on disk.\r\n"));
        }

}
#endif

void setup() {
#if MAKE_BIG_DEMO
        de = (PFAT_DIRINFO *)malloc(sizeof (PFAT_DIRINFO));
        data = (uint8_t *)malloc(TESTdsize);
#endif
        while(!USB_HOST_SERIAL);
        USB_HOST_SERIAL.begin(115200);
        delay(10000);
        USB_HOST_SERIAL.println("Start.");
        // Initialize generic storage. This must be done before USB starts.
        Init_Generic_Storage(&UHS_Usb);
        while(UHS_Usb.Init(1000) != 0);
#if defined(SWI_IRQ_NUM)
        printf("\r\n\r\nSWI_IRQ_NUM %i\r\n", SWI_IRQ_NUM);
#endif
        printf("\r\n\r\nUSB HOST READY.\r\n");
}

uint8_t current_state = 128;
uint8_t last_state = 255;

void loop() {
        // The following does not work yet!
#if !USB_HOST_SHIELD_USE_ISR
        UHS_Usb.Task();
#endif

#if 1
        current_state = UHS_Usb.getUsbTaskState();
        if(current_state != last_state) {
                last_state = current_state;
                printf("USB HOST state %2.2x\r\n", current_state);
        }
#endif
        mounted = fs_ready(VOL_LABEL);
        if(mounted != wasmounted) {
                wasmounted = mounted;
                if(mounted != PFAT_VOLUMES) {
                        uint64_t fre;
#if MAKE_BIG_DEMO
                        uint32_t start;
                        uint32_t end;
                        uint32_t wt;
                        uint32_t rt;
#endif
                        int res;
                        int fd;
                        printf_P(PSTR(VOL_LABEL " mounted.\r\n"));
#if MAKE_BIG_DEMO
                        fre = fs_getfree(VOL_PATH);
                        if(fre > 2097152) {
                                printf_P(PSTR("Removing '" VOL_PATH "HeLlO.tXt' file... "));
                                fflush(stdout);
                                res = fs_unlink( VOL_PATH "hello.txt");
                                printf_P(PSTR("completed with %i\r\n"), res);
                                printf_P(PSTR("\r\nStarting Write test...\r\n"));
                                fd = fs_open( VOL_PATH "HeLlO.tXt", O_WRONLY | O_CREAT);
                                if(fd > 0) {
                                        printf_P(PSTR("File opened OK, fd = %i\r\n"), fd);
                                        char hi[] = "]-[ello \\/\\/orld!\r\n";
                                        res = fs_write(fd, hi, strlen(hi));
                                        printf_P(PSTR("Wrote %i bytes, "), res);
                                        fflush(stdout);
                                        res = fs_close(fd);
                                        printf_P(PSTR("File closed result = %i.\r\n"), res);
                                } else {
                                        printf_P(PSTR("Error %d (%u)\r\n"), fd, fs_err);
                                }
                                printf_P(PSTR("\r\nStarting Read test...\r\n"));
                                fd = fs_open( VOL_PATH "hElLo.TxT", O_RDONLY);
                                if(fd > 0) {
                                        res = 1;
                                        printf_P(PSTR("File opened OK, fd = %i, displaying contents...\r\n"), fd);

                                        while(res > 0) {
                                                res = fs_read(fd, data, TESTdsize);
                                                for(int i = 0; i < res; i++) {
                                                        if(data[i] == '\n') fputc('\r', stdout);
                                                        if(data[i] != '\r') fputc(data[i], stdout);
                                                }
                                        }
                                        printf_P(PSTR("\r\nRead completed, last read result = %i (%i), "), res, fs_err
                                                );
                                        fflush(stdout);
                                        res = fs_close(fd);
                                        printf_P(PSTR("file close result = %i.\r\n"), res);
                                        printf_P(PSTR("Testing rename\r\n"));
                                        fs_unlink( VOL_PATH "newtest.txt");
                                        res = fs_rename( VOL_PATH "HeLlO.tXt",  VOL_PATH "newtest.txt");
                                        printf_P(PSTR("file rename result = %i.\r\n"), res);
                                } else {
                                        printf_P(PSTR("File not found.\r\n"));
                                }
                                printf_P(PSTR("\r\nRemoving '" VOL_PATH "1MB.bin' file... "));
                                fflush(stdout);
                                res = fs_unlink( VOL_PATH "1MB.bin");
                                printf_P(PSTR("completed with %i\r\n"), res);
                                //show_dir(de);
                                printf_P(PSTR("1MB write timing test "));
                                fflush(stdout);

                                //for (int i = 0; i < 128; i++) data[i] = i & 0xff;
                                fd = fs_open( VOL_PATH "1MB.bin", O_WRONLY | O_CREAT);
                                if(fd > 0) {
                                        int i = 0;
                                        delay(500);
                                        start = millis();
                                        for(; i < TESTcycles; i++) {
                                                res = fs_write(fd, data, TESTdsize);
                                                if(fs_err
                                                        ) break;
                                        }
                                        printf_P(PSTR(" %i writes, (%i), "), i, fs_err
                                                );
                                        fflush(stdout);
                                        res = fs_close(fd);
                                        end = millis();
                                        wt = end - start;
                                        printf_P(PSTR("(%i), "), fs_err
                                                );
                                        printf_P(PSTR(" %lu ms (%lu sec)\r\n"), wt, (500 + wt) / 1000UL);
                                }
                                printf_P(PSTR("completed with %i\r\n"), fs_err);

                                //show_dir(de);
                                printf_P(PSTR("1MB read timing test "));
                                fflush(stdout);

                                fd = fs_open( VOL_PATH "1MB.bin", O_RDONLY);
                                if(fd > 0) {
                                        delay(500);
                                        start = millis();
                                        res = 1;
                                        int i = 0;
                                        while(res > 0) {
                                                res = fs_read(fd, data, TESTdsize);
                                                i++;
                                        }
                                        end = millis();
                                        i--;
                                        printf_P(PSTR("%i reads, (%i), "), i, fs_err);
                                        fflush(stdout);
                                        res = fs_close(fd);
                                        rt = end - start;
                                        printf_P(PSTR(" %lu ms (%lu sec)\r\n"), rt, (500 + rt) / 1000UL);
                                }
                                printf_P(PSTR("completed with %i\r\n"), fs_err);
                                show_dir(de);
                                printf_P(PSTR("\r\nFlushing caches..."));
                                fflush(stdout);
                                fs_sync(); // IMPORTANT! Sync all caches to all medias!
                                printf_P(PSTR("\r\nRemove and insert media...\r\n"));
                                fflush(stdout);

                        } else {
                                printf("Not enough space to run tests.\r\n");
                        }
#endif

                } else {
                        printf("No media. Waiting to mount "  VOL_LABEL "\r\n");
                }
        }
}
