// Load the USB Host System core
#define LOAD_USB_HOST_SYSTEM
// Load USB Host Shield
#define LOAD_USB_HOST_SHIELD
// Use USB hub, you might want this for multiple devices.
#define LOAD_UHS_HUB

// Patch printf so we can use it.
#define LOAD_UHS_PRINTF_HELPER
#define DEBUG_PRINTF_EXTRA_HUGE 0
#define DEBUG_PRINTF_EXTRA_HUGE_USB_HID 1

#define LOAD_UHS_HID

#include <Arduino.h>
#ifdef true
#undef true
#endif
#ifdef false
#undef false
#endif

#include <UHS_host.h>

class myHID_processor : public UHS_HID_PROCESSOR {
public:
        myHID_processor(void) {}

        void onRelease(UHS_HID_base *d) {
                printf_P(PSTR("HID driver type %d no longer available.\r\n"), d->driver);
        }
        void onStart(UHS_HID_base *d) {
                printf_P(PSTR("HID driver type %d started, Subclass %02x, Protocol %02x\r\n"), d->driver, d->parent->bSubClass, d->parent->bProtocol);
        }
        void onPoll(UHS_HID_base *d, uint8_t *data, uint16_t length) {
                switch(d->driver) {
                        case UHS_HID_raw:
                                printf_P(PSTR("RAW input %d bytes interface %d, Subclass %02x, Protocol %02x Data:"), length, d->parent->bIface, d->parent->bSubClass, d->parent->bProtocol);
                                for(int i=0; i < length; i++) {
                                        printf_P(PSTR(" %02x"), data[i]);
                                }
                                printf_P(PSTR("\r\n"));
                                break;
                        default:
                                break;
                }
        }
};

myHID_processor HID_processor1;
myHID_processor HID_processor2;
MAX3421E_HOST UHS_Usb;
UHS_USBHub hub_1(&UHS_Usb);
UHS_HID hid1(&UHS_Usb, &HID_processor1);
UHS_HID hid2(&UHS_Usb, &HID_processor2);

void setup() {
        USB_HOST_SERIAL.begin(115200);
        while(UHS_Usb.Init(1000) != 0);
        printf_P(PSTR("\r\nHID RAW demo Begin.\r\n"));
}

void loop() {
        delay(1);
}
