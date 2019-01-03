// inline library loading
// Load the USB Host System core
#define LOAD_USB_HOST_SYSTEM
// Load USB Host Shield
#define LOAD_USB_HOST_SHIELD
// Bulk Storage
#define LOAD_UHS_BULK_STORAGE
// USB hub
#define LOAD_UHS_HUB

#include <Arduino.h>
#ifdef true
#undef true
#endif
#ifdef false
#undef false
#endif

#include <UHS_host.h>

MAX3421E_HOST UHS_Usb;
UHS_USBHub hub_1(&UHS_Usb);
UHS_Bulk_Storage Storage_1(&UHS_Usb);
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
        E_Notify(PSTR("\r\nTesting LUN "), 0);
        USB_HOST_SERIAL.print(lun);
        uint32_t xnow = millis();
        while(!rcode && loops < 2048) {
                loops++;
                rcode = Storage_1.Read(lun, loops, 512, 1, buf);
        }
        uint32_t finish = millis();
        if(!rcode) {
                E_Notify(PSTR("\r\nRead passed, Read 2048 sectors (1024K) in "), 0);
                USB_HOST_SERIAL.print(finish - xnow);
                E_Notify(PSTR("ms "), 0);
        } else {
                E_Notify(PSTR("\r\nERROR: Read Failed"), 0);

        }
        //USB_HOST_SERIAL.flush();

}

void setup() {
        while(!USB_HOST_SERIAL);
        USB_HOST_SERIAL.begin(115200);

        while(UHS_Usb.Init(1000) !=0);
        E_Notify(PSTR("\r\n\r\ngo!\r\n"), 0);
        laststate = 0xff;
        tested = false;
        notified = false;
        E_Notify(PSTR("\r\nPlug in a storage device now..."), 0);
}

void loop() {
#if !USB_HOST_SHIELD_USE_ISR
        // This is broken
        UHS_Usb.Task();
#endif
        usbstate = UHS_Usb.getUsbTaskState();
        if(usbstate != laststate) {
                USB_HOST_SERIAL.print("\r\nFSM state: 0x");
                if(usbstate < 0x10) USB_HOST_SERIAL.print("0");
                USB_HOST_SERIAL.print(usbstate,HEX);
                laststate = usbstate;
                switch(usbstate) {
                        case UHS_USB_HOST_STATE_IDLE:
                                // E_Notify(PSTR("\r\nPlug in a storage device now..."), 0);
                                break;
                        case UHS_USB_HOST_STATE_ERROR:
                                E_Notify(PSTR("\r\nUSB state machine reached error state 0x"),0);
                                USB_HOST_SERIAL.print(UHS_Usb.usb_error, HEX);
                        break;
                        case UHS_USB_HOST_STATE_RUNNING:
                                if(hub_1.bPollEnable) {
                                        E_Notify(PSTR("\r\nHub Connected..."),0);
                                        E_Notify(PSTR("\r\nPlug in a storage device now..."), 0);
                                }
                                break;
                        default:
                                break;

                }
        }
        boolean pbe = Storage_1.bPollEnable;
        boolean en = (pbe == lastEnable);
        if(!en) {
                lastEnable = pbe;
                if(pbe) {
                        E_Notify(PSTR("\r\nStorage is polling..."),0);
                } else {
                        E_Notify(PSTR("\r\nStorage is not polling..."),0);
                }
        }
        if((usbstate == UHS_USB_HOST_STATE_RUNNING) && Storage_1.bPollEnable && !tested) {
                if(!notified) {
                        E_Notify(PSTR("\r\nWaiting for device to become ready..."),0);
                        notified = true;
                }
                for(uint8_t i = 0; i < MASS_MAX_SUPPORTED_LUN; i++) {
                        if(Storage_1.LUNIsGood(i)) {
                                test_bulk(i);
                        }
                }
        }

        if(!Storage_1.bPollEnable && tested) {
                           tested = false;
                           notified = false;
                           E_Notify(PSTR("\r\nPlug in a storage device now..."), 0);
        }
}

