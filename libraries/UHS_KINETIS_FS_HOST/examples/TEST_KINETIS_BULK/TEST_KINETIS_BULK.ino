#define LOAD_USB_HOST_SYSTEM
#define LOAD_UHS_KINETIS_FS_HOST
#define LOAD_UHS_BULK_STORAGE

// Redirect debugging
#define USB_HOST_SERIAL Serial1

#include <Arduino.h>
#ifdef true
#undef true
#endif
#ifdef false
#undef false
#endif


#include <dyn_SWI.h>


#include <stdio.h>
#include <UHS_host.h>
#include <UHS_KINETIS_FS_HOST.h>
//#include <UHS_HUB.h>
#include <UHS_BULK_STORAGE.h>

UHS_KINETIS_FS_HOST KINETIS_Usb;
UHS_Bulk_Storage Storage_KINETIS(&KINETIS_Usb);

#if defined(CORE_TEENSY)
extern "C" {

        int _write(int fd, const char *ptr, int len) {
                int j;
                for(j = 0; j < len; j++) {
                        if(fd == 1)
                                Serial1.write(*ptr++);
                        else if(fd == 2)
                                USB_HOST_SERIAL.write(*ptr++);
                }
                return len;
        }

        int _read(int fd, char *ptr, int len) {
                if(len > 0 && fd == 0) {
                        while(!Serial1.available());
                        *ptr = Serial1.read();
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
// Else we are using CMSIS DAP (possibly) or debug output
#endif // TEENSY_CORE

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
                rcode = Storage_KINETIS.Read(lun, loops, 512, 1, buf);
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
        //while(!Serial1);
        Serial1.begin(115200);
        delay(10000);
        Serial1.println("Start.");
        Init_dyn_SWI();

        printf("\r\n\r\nSWI_IRQ_NUM %i\r\n", SWI_IRQ_NUM);
        while(KINETIS_Usb.Init(1000) != 0);
        printf("\r\n\r\nUSB HOST READY.\r\n");

        pinMode(LED_BUILTIN, OUTPUT);

        laststate = 0xff;
        tested = false;
        notified = false;
        E_Notify(PSTR("\r\nPlug in a storage device now..."), 0);
}

uint8_t d;

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

