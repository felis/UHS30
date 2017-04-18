// Redirect debugging and printf
#define USB_HOST_SERIAL Serial1
// inline library loading
// Patch printf so we can use it.
#define LOAD_UHS_PRINTF_HELPER
// Loads the Kinetis core
#define LOAD_UHS_KINETIS_FS_HOST
// Load the USB Host System core
#define LOAD_USB_HOST_SYSTEM
// Load Bulk storage
#define LOAD_UHS_BULK_STORAGE


#include <Arduino.h>
#ifdef true
#undef true
#endif
#ifdef false
#undef false
#endif

// Bring in all the libraries that we requested above.
#include <UHS_host.h>

UHS_KINETIS_FS_HOST KINETIS_Usb;
UHS_Bulk_Storage Storage_KINETIS(&KINETIS_Usb);

uint8_t usbstate;
uint8_t laststate;
uint8_t d;
boolean tested;
boolean notified;
boolean lastEnable = false;

uint8_t buf[512]; // WARNING! Assumes a sector is 512bytes!

void test_bulk(uint8_t lun) {
        // USB data switcher, PC -> device.
        pinMode(5,OUTPUT),
        digitalWriteFast(5, HIGH);

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
                printf("\r\nRead passed, Read 2048 sectors (1024K) in ");
                USB_HOST_SERIAL.print(finish - xnow);
                printf("ms ");
        } else {
                printf("\r\nERROR: Read Failed");

        }
        //USB_HOST_SERIAL.flush();

}

void setup() {
        //while(!Serial1);
        Serial1.begin(115200);
        Serial1.println("Waiting...");
        delay(10000);
        Serial1.println("Start.");

        while(KINETIS_Usb.Init(1000) != 0);
        // printf may be used after atleast 1 host init
        printf("\r\n\r\nSWI_IRQ_NUM %i\r\n", SWI_IRQ_NUM);
        printf("\r\n\r\nUSB HOST READY.\r\n");

        pinMode(LED_BUILTIN, OUTPUT);

        laststate = 0xff;
        tested = false;
        notified = false;
        E_Notify(PSTR("\r\nPlug in a storage device now..."), 0);
}

void loop() {

        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

        if (Serial1.available() > 0) {

                d = Serial1.read();
                if(d=='s') {
                        Serial1.print("USB0_INTEN: ");
                        Serial1.println(USB0_INTEN, HEX);
                        Serial1.print("USB0_CTL: ");
                        Serial1.println(USB0_CTL, HEX);
                } else if(d=='p') { //
                }
        }

        usbstate = KINETIS_Usb.getUsbTaskState();
        if(usbstate != laststate) {
                printf("USB HOST state %2.2x\r\n", usbstate);
                laststate = usbstate;
                switch(usbstate) {
                        case UHS_USB_HOST_STATE_IDLE:
                                // E_Notify(PSTR("\r\nPlug in a storage device now..."), 0);
                                break;
                        case UHS_USB_HOST_STATE_ERROR:
                                E_Notify(PSTR("\r\nUSB state machine reached error state 0x"),0);
                                USB_HOST_SERIAL.print(KINETIS_Usb.usb_error, HEX);
                        break;
                        case UHS_USB_HOST_STATE_RUNNING:
                                /*if(hub_MAX3421E.bPollEnable) {
                                        E_Notify(PSTR("\r\nHub Connected..."),0);
                                        E_Notify(PSTR("\r\nPlug in a storage device now..."), 0);
                                }*/
                                break;
                        default:
                                //E_Notify(PSTR("."),0);
                                break;

                }
        }
        boolean pbe = Storage_KINETIS.bPollEnable;
        boolean en = (pbe == lastEnable);
        if(!en) {
                lastEnable = pbe;
                if(pbe) {
                        E_Notify(PSTR("\r\nStorage is polling..."),0);
                } else {
                        E_Notify(PSTR("\r\nStorage is not polling..."),0);
                }
        }
        if((usbstate == UHS_USB_HOST_STATE_RUNNING) && Storage_KINETIS.bPollEnable && !tested) {
                if(!notified) {
                        E_Notify(PSTR("\r\nWaiting for device to become ready..."),0);
                        notified = true;
                }
                for(uint8_t i = 0; i < MASS_MAX_SUPPORTED_LUN; i++) {
                        if(Storage_KINETIS.LUNIsGood(i)) {
                                test_bulk(i);
                        }
                }
                //if(tested) Storage_KINETIS.bPollEnable = false;
        }

        if(!Storage_KINETIS.bPollEnable && tested) {
                           tested = false;
                           notified = false;
                           E_Notify(PSTR("\r\nPlug in a storage device now..."), 0);
        }
}

