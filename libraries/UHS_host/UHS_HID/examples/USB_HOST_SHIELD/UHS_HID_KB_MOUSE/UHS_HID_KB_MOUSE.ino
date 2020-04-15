// Load the USB Host System core
#define LOAD_USB_HOST_SYSTEM
// Load USB Host Shield
#define LOAD_USB_HOST_SHIELD
// Use USB hub, you might need this even for a combo dongle.
#define LOAD_UHS_HUB

// Patch printf so we can use it.
#define LOAD_UHS_PRINTF_HELPER
#define DEBUG_PRINTF_EXTRA_HUGE 0
#define DEBUG_PRINTF_EXTRA_HUGE_UHS_HOST 0
#define DEBUG_PRINTF_EXTRA_HUGE_USB_HID 0
#define DEBUG_PRINTF_EXTRA_HUGE_USB_HUB  0

#define LOAD_UHS_HID
#define LOAD_UHS_HIDRAWBOOT_KEYBOARD
#define LOAD_UHS_HIDRAWBOOT_MOUSE
#define UHS_DEVICE_WINDOWS_USB_SPEC_VIOLATION_DESCRIPTOR_DEVICE 0

#include <Arduino.h>
#ifdef true
#undef true
#endif
#ifdef false
#undef false
#endif

#include <UHS_host.h>


/*
 * capslock 0xf0
 * SCROLLLOCK 0xfe
 * NUMLOCK 0xff
 * F1-F12 0xf1-0xfc
 *
 * print-screen 0xfd
 *
 * PAUSE    0x13/0x11 (control S, shifted control Q)
 *
 * INSERT   INST
 *   HOME   HOME
 * PAGEUP   PGUP
 *
 * DELETE   0x7f (DEL)
 * END      0x04 (EOT)
 * PAGEDOWN 0x0c (form feed)
 *
 * RIGHT    CRRT
 * LEFT     CRLT
 * DOWN     CRDN
 * UP       CRUP
 *
 * enter    0x0d
 * padenter 0x0a
 *
 * 0x00 in the table means not assigned.
 * You can assign these if you really want to, except the first four.
 *
 */

// these are control chars, assigned as to not interfere
#define KAPP 0x01 // SOH
#define HOME 0x02 // STX
#define PGUP 0x05 // ENQ
#define POWR 0x06 // ACK
#define INST 0x0F // SI
#define CRRT 0x10 // DLE
#define CRLT 0x12 // DC2
#define CRDN 0x14 // DC4
#define CRUP 0x15 // NAK

const uint8_t scantoascii[] PROGMEM = {
        /*        0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f */
        /* 0 */ 0x00, 0x00, 0x00, 0x00, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
        /* 1 */ 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '1', '2',
        /* 2 */ '3', '4', '5', '6', '7', '8', '9', '0', 0x0d, 0x1b, 0x08, 0x09, ' ', '-', '=', '[',
        /* 3 */ ']', 0x5c, 0xa3, ';', 0x27, '`', ',', '.', '/', 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6,
        /* 4 */ 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0x03, INST, HOME, PGUP, 0x7f, 0x04, 0x0c, CRRT,
        /* 5 */ CRLT, CRDN, CRUP, 0xff, '/', '*', '-', '+', 0x0a, 0x04, CRDN, 0x0c, CRLT, 0x00, CRRT, HOME,
        /* 6 */ CRUP, PGUP, INST, 0x7f, 0x27, KAPP, POWR, '=', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* 7 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* 8 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* 9 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* a */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* b */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* c */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* d */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* e */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* f */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

        // shift
        /*        0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f */
        /* 0 */ 0x00, 0x00, 0x00, 0x00, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
        /* 1 */ 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '!', '@',
        /* 2 */ '#', '$', '%', '^', '&', '*', '(', ')', 0x0d, 0x1b, 0x08, 0x89, ' ', '_', '+', '{',
        /* 3 */ '}', '|', 0xa3, ':', '"', '~', '<', '>', '?', 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6,
        /* 4 */ 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0x03, INST, HOME, PGUP, 0x7f, 0x04, 0x0c, CRRT,
        /* 5 */ CRLT, CRDN, CRUP, 0xff, '/', '*', '-', '+', 0x0a, 0x04, CRDN, 0x0c, CRLT, 0x00, CRRT, HOME,
        /* 6 */ CRUP, PGUP, INST, 0x7f, 0x27, KAPP, POWR, '=', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* 7 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* 8 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* 9 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* a */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* b */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* c */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* d */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* e */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* f */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,


        // numlock-noshift
        /*        0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f */
        /* 0 */ 0x00, 0x00, 0x00, 0x00, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
        /* 1 */ 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '1', '2',
        /* 2 */ '3', '4', '5', '6', '7', '8', '9', '0', 0x0d, 0x1b, 0x08, 0x09, ' ', '-', '=', '[',
        /* 3 */ ']', 0x5c, 0xa3, ';', 0x27, '`', ',', '.', '/', 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6,
        /* 4 */ 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0x03, INST, HOME, PGUP, 0x7f, 0x04, 0x0c, CRRT,
        /* 5 */ CRLT, CRDN, CRUP, 0xff, '/', '*', '-', '+', 0x0a, '1', '2', '3', '4', '5', '6', '7',
        /* 6 */ '8', '9', '0', '.', 0x27, KAPP, POWR, '=', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* 7 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* 8 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* 9 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* a */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* b */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* c */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* d */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* e */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* f */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

        // numlock shift
        /*        0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f */
        /* 0 */ 0x00, 0x00, 0x00, 0x00, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
        /* 1 */ 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '!', '@',
        /* 2 */ '#', '$', '%', '^', '&', '*', '(', ')', 0x0d, 0x1b, 0x08, 0x89, ' ', '_', '+', '{',
        /* 3 */ '}', '|', 0xa3, ':', '"', '~', '<', '>', '?', 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6,
        /* 4 */ 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0x03, INST, HOME, PGUP, 0x7f, 0x04, 0x0c, CRRT,
        /* 5 */ CRLT, CRDN, CRUP, 0xff, '/', '*', '-', '+', 0x0a, '1', '2', '3', '4', '5', '6', '7',
        /* 6 */ '8', '9', '0', '.', '|', KAPP, POWR, '=', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* 7 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* 8 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* 9 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* a */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* b */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* c */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* d */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* e */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* f */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

        // control - take key uppercase ascii value, and mask 0x1f
        // caps-lock - only flips a-z A-Z ranges, shift unflips them
};

class myHID_processor : public UHS_HID_PROCESSOR {
public:

        bool numlock = false;
        bool capslock = false;
        bool scrolllock = false;
        uint8_t last_keys[8];
        UHS_ByteBuffer keybuffer;

        myHID_processor(void) {
        }

        void onRelease(UHS_HID_base *d) {
                printf_P(PSTR("HID device unplugged driver type %d no longer available.\r\n"), d->driver);
        }

        void onStart(UHS_HID_base *d) {
                printf_P(PSTR("HID driver type %d started, Subclass %02x, Protocol %02x "), d->driver, d->parent->bSubClass, d->parent->bProtocol);
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
                                numlock = false;
                                capslock = false;
                                scrolllock = false;
                                last_keys[0] = 0x00;
                                last_keys[1] = 0x00;
                                last_keys[2] = 0x00;
                                last_keys[3] = 0x00;
                                last_keys[4] = 0x00;
                                last_keys[5] = 0x00;
                                last_keys[6] = 0x00;
                                keybuffer.clear();
                                for(uint8_t i = 0, led = 0x40U, rv = 0; i < 10; i++) {
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

        void processKB(UHS_HID_base *d, uint8_t *data, uint16_t length) {

                // normally 8 bytes
                // 0 Modifier mask CTRL, SHIFT, ALT, META -- we only deal with the first 2
                // 1 reserved
                // 2 KEY
                // 3 KEY
                // 4 KEY
                // 5 KEY
                // 6 KEY
                // 7 KEY
                if(length != 8) return; // error, ignore
                if(data[2] == 0x01u || data[2] == 0x02u) return; // error, ignore
                uint8_t modmask = ((data[0] & 0xf0u) >> 4) | (data[0] &0x0fu);
                const uint8_t *p = scantoascii;
                const uint8_t *q;
                if(numlock) p += 512; // points to numlock table
                if((modmask & 0x02u) == 0x02u) {
                        p += 256; // offset by one group to shift
                }
                bool updateLEDs = false;
                for(uint16_t i = 2; i < 8; i++) {
                        if(data[i] == 0x00u) break;
                        bool found = false;
                        for(uint16_t j = 2; j < 8; j++) {
                                if(data[i] == last_keys[j]) {
                                        // ignore it, we could implement repeat here
                                        found = true;
                                        break;
                                }
                        }
                        if(!found) {
                                // new key pressed, process it.
                                q = p + data[i];
                                uint8_t c = pgm_read_byte(q);
                                switch(c) {
                                        case 0x00u:
                                                break;
                                        case 0xf0u:
                                                // caps lock
                                                updateLEDs = true;
                                                capslock = !capslock;
                                                break;
                                        case 0xfeu:
                                                // scroll lock
                                                updateLEDs = true;
                                                scrolllock = !scrolllock;
                                                break;
                                        case 0xffu:
                                                // numlock
                                                updateLEDs = true;
                                                numlock = !numlock;
                                                break;
                                        default:
                                                // apply modifiers
                                                if(capslock) {
                                                        // flip a-z A-Z
                                                        c = toupper(c);
                                                        if((modmask & 0x02u) == 0x02u) {
                                                                // shifted capslock
                                                                c = tolower(c);
                                                        }
                                                }
                                                if((modmask & 0x01u) == 0x01u) {
                                                        // control pressed
                                                        c = c & 0x1f;
                                                }
                                                // stuff decoded key into fifo
                                                keybuffer.put(c);
                                                break;
                                }
                        }
                }
                // copy...
                for(uint16_t i = 0; i < 8; i++) last_keys[i] = data[i];
                if(updateLEDs) {
                        uint8_t led = 0x00u;
                        if(numlock) led |= 0x01;
                        if(capslock) led |= 0x02;
                        if(scrolllock) led |= 0x04;
                        uint8_t rv = ((UHS_HIDBOOT_keyboard *)d)->SetLEDs(led);
                        if(rv != 0) return; // skip onStart if unplugged.

                }
        }

        void onPoll(UHS_HID_base *d, uint8_t *data, uint16_t length) {
                MOUSEINFO *squeek = (MOUSEINFO *)data;
                switch(d->driver) {
                        case UHS_HID_raw:
                                printf_P(PSTR("RAW input %d bytes interface %d, Subclass %02x, Protocol %02x Data:"), length, d->parent->bIface, d->parent->bSubClass, d->parent->bProtocol);
                                for(uint8_t i = 0; i < length; i++) {
                                        printf_P(PSTR(" %02x"), data[i]);
                                }
                                printf_P(PSTR("\r\n"));
                                break;
                        case UHS_HID_mouse:
                                printf_P(PSTR("Mouse buttons left %s right %s mid %s fourth %s fifth %s motion (X,Y) %4d,%4d wheel %4d\r\n"), squeek->bmLeftButton == 1 ? "t" : "f", squeek->bmRightButton == 1 ? "t" : "f", squeek->bmMiddleButton == 1 ? "t" : "f", squeek->bmButton4 == 1 ? "t" : "f", squeek->bmButton5 == 1 ? "t" : "f", squeek->dX, squeek->dY, squeek->wheel1);
                                break;
                        case UHS_HID_keyboard:
                                processKB(d, data, length);
                                break;
                        default:
                                break;
                }
        }
};

myHID_processor HID_processor1;
myHID_processor HID_processor2;
MAX3421E_HOST UHS_Usb;
UHS_USBHub hub1(&UHS_Usb);
UHS_HID hid1(&UHS_Usb, &HID_processor1);
UHS_HID hid2(&UHS_Usb, &HID_processor2);

void setup() {
        USB_HOST_SERIAL.begin(115200);
        while(UHS_Usb.Init(1000) != 0);
        printf_P(PSTR("\r\nHID RAW/KEYBOARD/MOUSE demo Begin.\r\n"));

}

void check_keyboards() {
        int available = HID_processor1.keybuffer.getSize();
        for(; available > 0; available--) {
                uint8_t q = HID_processor1.keybuffer.get();
                printf_P(PSTR("%c"), q);
        }

        available = HID_processor2.keybuffer.getSize();
        for(; available > 0; available--) {
                uint8_t q = HID_processor2.keybuffer.get();
                printf_P(PSTR("%c"), q);
        }
}

uint8_t stat0 = 128;
uint8_t stat1 = 128;
uint8_t stat2 = 128;

void loop() {
        check_keyboards();
        if(hub1.GetAddress() != stat0) {
                stat0 = hub1.GetAddress();
                printf_P(PSTR("Hub address %u\r\n"), stat0);
        }
        if(hid1.GetAddress() != stat1) {
                stat1 = hid1.GetAddress();
                printf_P(PSTR("hid1 address %u\r\n"), stat1);
        }
        if(hid2.GetAddress() != stat2) {
                stat2 = hid2.GetAddress();
                printf_P(PSTR("hid2 address %u\r\n"), stat2);
        }

}
