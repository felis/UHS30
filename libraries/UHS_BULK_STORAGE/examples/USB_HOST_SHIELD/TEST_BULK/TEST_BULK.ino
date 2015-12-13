#define LOAD_USB_HOST_SYSTEM
#define LOAD_USB_HOST_SHIELD
#define LOAD_UHS_BULK_STORAGE
#define _USE_MAX3421E_HOST 1
#define LOAD_UHS_HUB
#define USB_HOST_SHIELD_USE_ISR 1

//#define ENABLE_UHS_DEBUGGING 1
//#define DEBUG_PRINTF_EXTRA_HUGE 1
//#define USB_HOST_SHIELD_USE_ISR 0
//#define USB_HOST_SERIAL Serial1

#include <Arduino.h>
#ifdef true
#undef true
#endif
#ifdef false
#undef false
#endif

#include <UHS_host.h>

MAX3421E_HOST MAX3421E_Usb;
UHS_USBHub hub_MAX3421E(&MAX3421E_Usb);
UHS_Bulk_Storage Storage_MAX3421E(&MAX3421E_Usb);
uint8_t usbstate;
uint8_t laststate;
boolean tested;
boolean notified;
boolean lastEnable = false;

#if DEBUG_PRINTF_EXTRA_HUGE
static FILE mystdout;
static int my_putc(char c, FILE *t) {
        USB_HOST_SERIAL.write(c);
        return 0;
}
#endif

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
                rcode = Storage_MAX3421E.Read(lun, loops, 512, 1, buf);
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
        while(!Serial);
        Serial.begin(115200);
        //USB_HOST_SERIAL.begin(115200);

#if DEBUG_PRINTF_EXTRA_HUGE
        mystdout.put = my_putc;
        mystdout.get = NULL;
        mystdout.flags = _FDEV_SETUP_WRITE;
        mystdout.udata = 0;
        stdout = &mystdout;
#endif
        Init_dyn_SWI();
        while(MAX3421E_Usb.Init(1000) !=0);
        E_Notify(PSTR("\r\n\r\ngo!\r\n"), 0);
        laststate = 0xff;
        tested = false;
        notified = false;
        E_Notify(PSTR("\r\nPlug in a storage device now..."), 0);
}

void loop() {
#if !USB_HOST_SHIELD_USE_ISR
        // This is broken
        MAX3421E_Usb.Task();
#endif
        usbstate = MAX3421E_Usb.getUsbTaskState();
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
                                USB_HOST_SERIAL.print(MAX3421E_Usb.usb_error, HEX);
                        break;
                        case UHS_USB_HOST_STATE_RUNNING:
                                if(hub_MAX3421E.bPollEnable) {
                                        E_Notify(PSTR("\r\nHub Connected..."),0);
                                        E_Notify(PSTR("\r\nPlug in a storage device now..."), 0);
                                }
                                break;
                        default:
                                //E_Notify(PSTR("."),0);
                                break;

                }
        }
        boolean pbe = Storage_MAX3421E.bPollEnable;
        boolean en = (pbe == lastEnable);
        if(!en) {
                lastEnable = pbe;
                if(pbe) {
                        E_Notify(PSTR("\r\nStorage is polling..."),0);
                } else {
                        E_Notify(PSTR("\r\nStorage is not polling..."),0);
                }
        }
        if((usbstate == UHS_USB_HOST_STATE_RUNNING) && Storage_MAX3421E.bPollEnable && !tested) {
                if(!notified) {
                        E_Notify(PSTR("\r\nWaiting for device to become ready..."),0);
                        notified = true;
                }
                for(uint8_t i = 0; i < MASS_MAX_SUPPORTED_LUN; i++) {
                        if(Storage_MAX3421E.LUNIsGood(i)) {
                                test_bulk(i);
                        }
                }
                //if(tested) Storage_MAX3421E.bPollEnable = false;
        }

        if(!Storage_MAX3421E.bPollEnable && tested) {
                           tested = false;
                           notified = false;
                           E_Notify(PSTR("\r\nPlug in a storage device now..."), 0);
        }
}

