//////////////////////////////////////
// libraries that we will be using
//////////////////////////////////////
#define LOAD_USB_HOST_SYSTEM
#define LOAD_UHS_KINETIS_FS_HOST
#define LOAD_UHS_BULK_STORAGE
#define LOAD_RTCLIB
#define LOAD_GENERIC_STORAGE

// Uncomment to debug
//#define ENABLE_UHS_DEBUGGING 1
// Uncomment to make debugging very noisy
//#define DEBUG_PRINTF_EXTRA_HUGE 1

//////////////////////////////////////
// OPTIONS
//////////////////////////////////////

// Where to redirect debugging, also used for the program output
#define USB_HOST_SERIAL Serial1

#define TESTdsize 512
#define TESTcycles (1048576/TESTdsize)

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

// Use long file names
#define _USE_LFN 3


//////////////////////////////////////
// Includes
//////////////////////////////////////

// Arduino.h, if not already included
#include <Arduino.h>

// RTClib needs this
#include <Wire.h>

// Load the wanted libraries here
#include <dyn_SWI.h> // provides a software interrupt fo use to run code as a light thread
#include <RTClib.h> // Clock functions
#include <UHS_host.h> // UHS USB HOST base classes
#include <UHS_KINETIS_FS_HOST.h> // Kinetis hardware driver
#include <UHS_HUB.h> // HUB interface driver
#include <UHS_BULK_STORAGE.h> // Bulk storage interface driver
#include <UHS_FS.h> // File system driver

UHS_KINETIS_FS_HOST KINETIS_Usb;
UHS_USBHub hub_KINETIS(&KINETIS_Usb);

PFAT_DIRINFO *de;
uint8_t *data;
uint8_t mounted = PFAT_VOLUMES;
uint8_t wasmounted = 0;

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

// Else we are using CMSIS DAP
#endif // TEENSY_CORE

void show_dir(PFAT_DIRINFO *de) {
        int res;
        uint64_t fre;
        uint32_t numlo;
        uint32_t numhi;
        int fd = fs_opendir("/");
        if(fd > 0) {
                printf("Directory of '/'\r\n");
                do {
                        res = fs_readdir(fd, de);
                        if(!res) {
                                DateTime tstamp(de->fdate, de->ftime);
                                if(!(de->fattrib & AM_VOL)) {
                                        if(de->fattrib & AM_DIR) {
                                                printf("d");
                                        } else printf("-");

                                        if(de->fattrib & AM_RDO) {
                                                printf("r-");
                                        } else printf("rw");

                                        if(de->fattrib & AM_HID) {
                                                printf("h");
                                        } else printf("-");

                                        if(de->fattrib & AM_SYS) {
                                                printf("s");
                                        } else printf("-");

                                        if(de->fattrib & AM_ARC) {
                                                printf("a");
                                        } else printf("-");

                                        numlo = de->fsize % 100000000llu;
                                        numhi = de->fsize / 100000000llu;
                                        if(numhi) {
                                                printf(" %lu%08lu", numhi, numlo);
                                        } else {
                                                printf(" %12lu", numlo);
                                        }
                                        printf(" %.4u-%.2u-%.2u", tstamp.year(), tstamp.month(), tstamp.day());
                                        printf(" %.2u:%.2u:%.2u", tstamp.hour(), tstamp.minute(), tstamp.second());
                                        printf(" %s", de->fname);
                                        if(de->lfname[0] != 0) {
                                                printf(" (%s)", de->lfname);
                                        }
                                        printf("\r\n");
                                }
                        }

                } while(!res);

                fs_closedir(fd);

                fre = fs_getfree("/");
                numlo = fre % 100000000llu;
                numhi = fre / 100000000llu;

                if(numhi) {
                        printf("%lu%08lu", numhi, numlo);
                } else {
                        printf("%lu", numlo);
                }
                printf(" bytes available on disk.\r\n");
        }
}

void setup() {
        de = (PFAT_DIRINFO *)malloc(sizeof (PFAT_DIRINFO));
        data = (uint8_t *)malloc(TESTdsize);
        Serial1.begin(115200);
        printf("\r\n\r\nStart.");
        Init_dyn_SWI();
        // Initialize generic storage. This must be done before USB starts.
        Init_Generic_Storage(&KINETIS_Usb);
        printf("\r\n\r\nSWI_IRQ_NUM %i\r\n", SWI_IRQ_NUM);
        while(KINETIS_Usb.Init(1000) != 0);
        printf("\r\n\r\nUSB HOST READY.\r\n");
}

uint8_t current_state = 128;
uint8_t last_state = 255;

void loop() {
#if !USB_HOST_SHIELD_USE_ISR
        KINETIS_Usb.Task();
#endif

#if 0
        current_state = KINETIS_Usb.getUsbTaskState();
        if(current_state != last_state) {
                last_state = current_state;
                printf("USB HOST state %2.2x\r\n", current_state);
        }
#endif
        mounted = fs_ready("/");
        if(mounted != wasmounted) {
                wasmounted = mounted;
                if(mounted != PFAT_VOLUMES) {
                        uint64_t fre;
                        uint32_t start;
                        uint32_t end;
                        uint32_t wt;
                        uint32_t rt;
                        int res;
                        int fd;
                        printf("/ mounted.\r\n");
                        fre = fs_getfree("/");
                        if(fre > 2097152) {
                                printf("Removing '/HeLlO.tXt' file... ");
                                fflush(stdout);
                                res = fs_unlink("/hello.txt");
                                printf("completed with %i\r\n", res);
                                printf("\r\nStarting Write test...\r\n");
                                fd = fs_open("/HeLlO.tXt", O_WRONLY | O_CREAT);
                                if(fd > 0) {
                                        printf("File opened OK, fd = %i\r\n", fd);
                                        char hi[] = "]-[ello \\/\\/orld!\r\n";
                                        res = fs_write(fd, hi, strlen(hi));
                                        printf("Wrote %i bytes, ", res);
                                        fflush(stdout);
                                        res = fs_close(fd);
                                        printf("File closed result = %i.\r\n", res);
                                } else {
                                        printf("Error %d (%u)\r\n", fd, fs_err);
                                }
                                printf("\r\nStarting Read test...\r\n");
                                fd = fs_open("/hElLo.TxT", O_RDONLY);
                                if(fd > 0) {
                                        res = 1;
                                        printf("File opened OK, fd = %i, displaying contents...\r\n", fd);

                                        while(res > 0) {
                                                res = fs_read(fd, data, TESTdsize);
                                                for(int i = 0; i < res; i++) {
                                                        if(data[i] == '\n') fputc('\r', stdout);
                                                        if(data[i] != '\r') fputc(data[i], stdout);
                                                }
                                        }
                                        printf("\r\nRead completed, last read result = %i (%i), ", res, fs_err
                                                );
                                        fflush(stdout);
                                        res = fs_close(fd);
                                        printf("file close result = %i.\r\n", res);
                                        printf("Testing rename\r\n");
                                        fs_unlink("/newtest.txt");
                                        res = fs_rename("/HeLlO.tXt", "/newtest.txt");
                                        printf("file rename result = %i.\r\n", res);
                                } else {
                                        printf("File not found.\r\n");
                                }
                                printf("\r\nRemoving '/1MB.bin' file... ");
                                fflush(stdout);
                                res = fs_unlink("/1MB.bin");
                                printf("completed with %i\r\n", res);
                                printf("1MB write timing test ");
                                fflush(stdout);

                                //for (int i = 0; i < 128; i++) data[i] = i & 0xff;
                                fd = fs_open("/1MB.bin", O_WRONLY | O_CREAT);
                                if(fd > 0) {
                                        int i = 0;
                                        delay(500);
                                        start = millis();
                                        for(; i < TESTcycles; i++) {
                                                res = fs_write(fd, data, TESTdsize);
                                                if(fs_err
                                                        ) break;
                                        }
                                        printf(" %i writes, (%i), ", i, fs_err);
                                        fflush(stdout);
                                        res = fs_close(fd);
                                        end = millis();
                                        wt = end - start;
                                        printf("(%i), ", fs_err);
                                        printf(" %lu ms (%lu sec)\r\n", wt, (500 + wt) / 1000UL);
                                }
                                printf("completed with %i\r\n", fs_err);

                                //show_dir(de);
                                printf("1MB read timing test ");
                                fflush(stdout);

                                fd = fs_open("/1MB.bin", O_RDONLY);
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
                                        printf("%i reads, (%i), ", i, fs_err);
                                        fflush(stdout);
                                        res = fs_close(fd);
                                        rt = end - start;
                                        printf(" %lu ms (%lu sec)\r\n", rt, (500 + rt) / 1000UL);
                                }
                                printf("completed with %i\r\n", fs_err);
                                show_dir(de);
                                printf("\r\nFlushing caches...");
                                fflush(stdout);
                                fs_sync(); // IMPORTANT! Sync all caches to all medias!
                                printf("\r\nRemove and insert media...\r\n");
                                fflush(stdout);

                        } else {
                                printf("Not enough space to run tests.\r\n");
                        }

                } else {
                        printf("No media. Waiting to mount /\r\n");
                }
        }
}

