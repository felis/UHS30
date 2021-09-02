//////////////////////////////////////
// libraries that we will be using
//////////////////////////////////////

// inline library loading
// Patch printf so we can use it.
#define LOAD_UHS_PRINTF_HELPER
// Load the Kinetis EHCI core
#define LOAD_UHS_KINETIS_EHCI
// Load the USB Host System core
#define LOAD_USB_HOST_SYSTEM
// Bulk Storage
#define LOAD_UHS_BULK_STORAGE
// RTC/clock
#define LOAD_RTCLIB
// USB hub
#define LOAD_UHS_HUB
// Filesystem
#define LOAD_GENERIC_STORAGE

// Redirect debugging and printf
//#define USB_HOST_SERIAL Serial1

//#define DEBUG_PRINTF_EXTRA_HUGE 1
//#define DEBUG_PRINTF_EXTRA_HUGE_UHS_HOST 1
//#define DEBUG_PRINTF_EXTRA_HUGE_UHS_BULK_STORAGE 1

//////////////////////////////////////
// OPTIONS
//////////////////////////////////////

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


//////////////////////////////////////
// Includes
//////////////////////////////////////

// Arduino.h, if not already included
#include <Arduino.h>
#ifdef true
#undef true
#endif
#ifdef false
#undef false
#endif


// Load the wanted libraries here
#include <RTClib.h> // Clock functions
#include <UHS_host.h> // UHS USB HOST base classes

UHS_KINETIS_EHCI UHS_Usb;
UHS_USBHub hub_1(&UHS_Usb);

PFAT_DIRINFO *de;
uint8_t *data;
char *VOL_LABEL;
uint8_t mounted = PFAT_VOLUMES;
uint8_t wasmounted = 0;

void show_dir(PFAT_DIRINFO *de, char *vol) {
        int res;
        uint64_t fre;
        uint32_t numlo;
        uint32_t numhi;
        int fd = fs_opendir(vol);
        if(fd > 0) {
                printf_P(PSTR("Directory of '%s'\r\n"), vol);
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
                                                printf_P(PSTR(" %lu%08lu"), numhi, numlo);
                                        } else {
                                                printf_P(PSTR(" %12lu"), numlo);
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
                fs_closedir(fd);

                fre = fs_getfree(vol);
                numlo = fre % 100000000llu;
                numhi = fre / 100000000llu;

                if(numhi) {
                        printf_P(PSTR("%lu%08lu"), numhi, numlo);
                } else {
                        printf_P(PSTR("%lu"), numlo);
                }
                printf_P(PSTR(" bytes available on disk.\r\n"));
        }

}

void setup() {
        delay(2000);
        de = (PFAT_DIRINFO *)malloc(sizeof (PFAT_DIRINFO));
        data = (uint8_t *)malloc(1024);
        while(!USB_HOST_SERIAL) yield();
        USB_HOST_SERIAL.begin(115200);
        printf("\r\n\r\nStart.");
        // Initialize generic storage. This must be done before USB starts.
        Init_Generic_Storage(&UHS_Usb);
        printf("\r\n\r\nSWI_IRQ_NUM %i\r\n", SWI_IRQ_NUM);
        while(UHS_Usb.Init(1000) != 0);
        printf("\r\n\r\nUSB HOST READY.\r\n");
}

void loop() {
        mounted = fs_mountcount();
        if(mounted != wasmounted) {
                int actual = 0;
                for(int i = 0; i < PFAT_VOLUMES; i++) {
                        // get volume name.
                        VOL_LABEL = fs_mount_lbl(i);
                        if(VOL_LABEL == NULL) continue; // not allocated.
                        printf("volume %i mounted as %s, type %i\r\n", i, VOL_LABEL, Fats[i]->ffs->fs_type);
                        // actually a mounted volume?
                        if(Fats[i]->ffs->fs_type && Fats[i]->ffs->fs_type < 4) {
                                actual++;
                        }
                        free(VOL_LABEL); // IMPORTANT!! free() the heap!
                }
                printf("%i volumes on-line, %i mounted\r\n", mounted, actual);
                if(mounted > wasmounted) {
                        // something new mounted, iterate showing directories of mounted volumes.
                        for(int i = 0; i < PFAT_VOLUMES; i++) {
                                // get volume name.
                                VOL_LABEL = fs_mount_lbl(i);
                                if(VOL_LABEL == NULL) continue; // not allocated.
                                printf("volume %i mounted as %s, type %i\r\n", i, VOL_LABEL, Fats[i]->ffs->fs_type);
                                // actually a mounted volume?
                                if(Fats[i]->ffs->fs_type && Fats[i]->ffs->fs_type < 4) {
                                        show_dir(de, VOL_LABEL);
                                }
                                free(VOL_LABEL); // IMPORTANT!! free() the heap!
                                // unmount it via IOCTL
                                int eject = 0;
                                Fats[i]->disk_ioctl(CTRL_EJECT, &eject);
                                eject = 0;
                                Fats[i]->disk_ioctl(CTRL_POWER, &eject);
                        }
                        // } else if(mounted > 0) {
                        // } else {
                }
                actual = 0;
                for(int i = 0; i < PFAT_VOLUMES; i++) {
                        // get volume name.
                        VOL_LABEL = fs_mount_lbl(i);
                        if(VOL_LABEL == NULL) continue; // not allocated.
                        printf("volume %i mounted as %s, type %i\r\n", i, VOL_LABEL, Fats[i]->ffs->fs_type);
                        // actually a mounted volume?
                        if(Fats[i]->ffs->fs_type && Fats[i]->ffs->fs_type < 4) {
                                actual++;
                        }
                        free(VOL_LABEL); // IMPORTANT!! free() the heap!
                }
                wasmounted = mounted;
                printf("%i volumes on-line, %i mounted\r\n", mounted, actual);
        }
}
