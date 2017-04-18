//////////////////////////////////////
// libraries that we will be using
//////////////////////////////////////
// Patch printf so we can use it.
#define LOAD_UHS_PRINTF_HELPER
// Load the USB Host System core
#define LOAD_USB_HOST_SYSTEM
// Load the Kinetis core
#define LOAD_UHS_KINETIS_FS_HOST
// Bulk Storage
#define LOAD_UHS_BULK_STORAGE
// USB hub
#define LOAD_UHS_HUB

// Uncomment to debug
//#define ENABLE_UHS_DEBUGGING 1
// Uncomment to make debugging very noisy
//#define DEBUG_PRINTF_EXTRA_HUGE 1

//////////////////////////////////////
// OPTIONS
//////////////////////////////////////

// Where to redirect debugging, also used for the program output
#define USB_HOST_SERIAL Serial1

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

#include <UHS_host.h>

UHS_KINETIS_FS_HOST KINETIS_Usb;
UHS_USBHub hub_KINETIS(&KINETIS_Usb);
UHS_Bulk_Storage Storage_KINETIS(&KINETIS_Usb);
uint8_t usbstate;
uint8_t laststate;
boolean tested;
boolean notified;
boolean lastEnable = false;

uint8_t buf[512]; // WARNING! Assumes a sector is 512bytes!

void test_bulk(uint8_t lun) {
        uint8_t rcode = 0;
        uint16_t loops = 0;
        tested = true;
        printf("\r\nTesting LUN %i", lun);
        uint32_t xnow = millis();
        while(!rcode && loops < 2048) {
                loops++;
                rcode = Storage_KINETIS.Read(lun, loops, 512, 1, buf);
        }
        uint32_t finish = millis();
        if(!rcode) {
                printf("\r\nRead passed, Read 2048 sectors (1024K) in %lums", (finish - xnow));
        } else {
                printf("\r\nERROR: Read Failed");
        }
        fflush(stdout);
}

void setup() {
        // USB data switcher, PC -> device.
        pinMode(5,OUTPUT),
        digitalWriteFast(5, HIGH);
        USB_HOST_SERIAL.begin(115200);
        while(KINETIS_Usb.Init(1000) !=0);
        printf("\r\n\r\ngo!\r\n");
        laststate = 0xff;
        tested = false;
        notified = false;
        printf("\r\nPlug in a storage device now...");
        fflush(stdout);
}

void loop() {
        usbstate = KINETIS_Usb.getUsbTaskState();
        if(usbstate != laststate) {
                printf("\r\nFSM state: 0x%2.2x", usbstate);
                laststate = usbstate;
                switch(usbstate) {
                        case UHS_USB_HOST_STATE_IDLE:
                                break;
                        case UHS_USB_HOST_STATE_ERROR:
                                printf("\r\nUSB state machine reached error state 0x%2.2x", KINETIS_Usb.usb_error);
                                break;
                        case UHS_USB_HOST_STATE_RUNNING:
                                if(hub_KINETIS.bPollEnable) {
                                        printf("\r\nHub Connected...\r\nPlug in a storage device now...");
                                }
                                break;
                        default:
                                break;

                }
                fflush(stdout);
        }
        boolean pbe = Storage_KINETIS.bPollEnable;
        boolean en = (pbe == lastEnable);
        if(!en) {
                lastEnable = pbe;
                if(pbe) {
                        printf("\r\nStorage is polling...");
                } else {
                        printf("\r\nStorage is not polling...");
                }
        }
        if((usbstate == UHS_USB_HOST_STATE_RUNNING) && Storage_KINETIS.bPollEnable && !tested) {
                if(!notified) {
                        printf("\r\nWaiting for media to become ready...");
                        notified = true;
                }
                for(uint8_t i = 0; i < MASS_MAX_SUPPORTED_LUN; i++) {
                        if(Storage_KINETIS.LUNIsGood(i)) {
                                test_bulk(i);
                        }
                }
                if(tested) printf("\r\nDone, you may now unplug storage device...");
        }

        if(!Storage_KINETIS.bPollEnable && tested) {
                           tested = false;
                           notified = false;
                           printf("\r\nPlug in a storage device now...");
        }
}

