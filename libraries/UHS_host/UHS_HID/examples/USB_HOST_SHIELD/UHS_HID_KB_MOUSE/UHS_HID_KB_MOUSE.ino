// Load the USB Host System core
#define LOAD_USB_HOST_SYSTEM
// Load USB Host Shield
#define LOAD_USB_HOST_SHIELD
// Use USB hub, you might need this even for a combo dongle.
#define LOAD_UHS_HUB

// Patch printf so we can use it.
#define LOAD_UHS_PRINTF_HELPER
#define DEBUG_PRINTF_EXTRA_HUGE 1
#define DEBUG_PRINTF_EXTRA_HUGE_UHS_HOST 0
#define DEBUG_PRINTF_EXTRA_HUGE_USB_HID 0

#define LOAD_UHS_HID
#define LOAD_UHS_HIDRAWBOOT_KEYBOARD
#define LOAD_UHS_HIDRAWBOOT_MOUSE

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

        myHID_processor(void) {
        }

        void onRelease(UHS_HID_base *d) {
                printf_P("HID device unplugged driver type %d no longer available.\r\n", d->driver);
        }

        void onStart(UHS_HID_base *d) {
                printf_P("HID driver type %d started, Subclass %02x, Protocol %02x ", d->driver, d->parent->bSubClass, d->parent->bProtocol);
                switch(d->driver) {
                        case UHS_HID_raw:
                                printf_P(PSTR("HID-RAW"));
                                break;
                        case UHS_HID_mouse:
                                printf_P(PSTR("HIDBOOT-RAW-MOUSE"));
                                break;
                        case UHS_HID_keyboard:
                                printf_P(PSTR("HIDBOOT-RAW-KEYBOARD"));
                                // This twinkles the LEDs a few times as an example.
                                for(uint8_t i = 0, led = 0x40U, rv=0; i < 10; i++) {
                                        while(led) {
                                                led >>= 1;
                                                rv = ((UHS_HIDBOOT_keyboard *)d)->SetLEDs(led);
                                                if(rv != 0) return; // skip onStart if unplugged.
                                                if(!d->parent->UHS_SLEEP_MS(100)) return; // skip remainder if unplugged.
                                        }
                                }

                                break;
                        default:
                                printf_P(PSTR("HID-NOT_USED"));
                                break;
                }
                printf_P(PSTR("\r\n"));
        }

        void onPoll(UHS_HID_base *d, uint8_t *data, uint16_t length) {
                MOUSEINFO *squeek = (MOUSEINFO *)data;
                switch(d->driver) {
                        case UHS_HID_raw:
                                printf_P(PSTR("RAW input %d bytes interface %d, Subclass %02x, Protocol %02x Data:"), length, d->parent->bIface, d->parent->bSubClass, d->parent->bProtocol);
                                for(uint8_t i = 0; i < length; i++) {
                                        printf_P(PSTR(" %02x"), data[i]);
                                }
                                break;
                        case UHS_HID_mouse:
                                printf_P(PSTR("Mouse buttons left %s right %s mid %s fourth %s fifth %s motion (X,Y) %4d,%4d wheel %4d"), squeek->bmLeftButton == 1 ? "t" : "f", squeek->bmRightButton == 1 ? "t" : "f", squeek->bmMiddleButton == 1 ? "t" : "f", squeek->bmButton4 == 1 ? "t" : "f", squeek->bmButton5 == 1 ? "t" : "f", squeek->dX, squeek->dY, squeek->wheel1);
                                break;
                        case UHS_HID_keyboard:
                                printf_P(PSTR("keyboard input %d bytes interface %d, Subclass %02x, Protocol %02x Data:"), length, d->parent->bIface, d->parent->bSubClass, d->parent->bProtocol);
                                for(uint8_t i = 0; i < length; i++) {
                                        printf_P(PSTR(" %02x"), data[i]);
                                }
                                break;
                        default:
                                break;
                }
                printf_P(PSTR("\r\n"));
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
        printf_P(PSTR("\r\nHID RAW/KEYBOARD/MOUSE demo Begin.\r\n"));

}

void loop() {
        delay(1);
}
