// Load the USB Host System core
#define LOAD_USB_HOST_SYSTEM
// Load the Kinetis core
#define LOAD_UHS_KINETIS_FS_HOST
// Use USB hub
#define LOAD_UHS_HUB

// Patch printf so we can use it.
#define LOAD_UHS_PRINTF_HELPER
//#define DEBUG_PRINTF_EXTRA_HUGE 1
//#define DEBUG_PRINTF_EXTRA_HUGE_UHS_HOST 1
//#define DEBUG_PRINTF_EXTRA_HUGE_USB_HUB 1
//#define DEBUG_PRINTF_EXTRA_HUGE_USB_HOST_SHIELD 1

// Redirect debugging and printf
#define USB_HOST_SERIAL Serial1


// These all get combined under UHS_CDC_ACM multiplexer.
// Each should only add a trivial amount of code.
// XR21B1411 can run in a pure CDC-ACM mode, as can PROLIFIC.
// FTDI has a large code and data footprint. Avoid this chip if you can.
#define LOAD_UHS_CDC_ACM
#define LOAD_UHS_CDC_ACM_XR21B1411
// This needs testing.
#define LOAD_UHS_CDC_ACM_PROLIFIC
// This needs testing.
#define LOAD_UHS_CDC_ACM_FTDI

#include <Arduino.h>
#ifdef true
#undef true
#endif
#ifdef false
#undef false
#endif

#include <UHS_host.h>

class MY_ACM : public UHS_CDC_ACM {
public:

        MY_ACM(UHS_USB_HOST_BASE *p) : UHS_CDC_ACM(p) {
        };
        void OnRelease(void);
        uint8_t OnStart(void);
};

void MY_ACM::OnRelease(void) {
        // Tell the user that the device has disconnected
        if(bAddress) printf("\r\n\r\nDisconnected.\r\n\r\n");
}

uint8_t MY_ACM::OnStart(void) {
        uint8_t rcode;
        // Set DTR = 1 RTS=1
        rcode = SetControlLineState(3);

        if(rcode) {
                printf_P(PSTR("SetControlLineState %x\r\n"), rcode);
                return rcode;
        }

        UHS_CDC_LINE_CODING lc;
        lc.dwDTERate = 9600;
        lc.bCharFormat = 0;
        lc.bParityType = 0;
        lc.bDataBits = 8;

        rcode = SetLineCoding(&lc);

        if(rcode) {
                printf_P(PSTR("SetLineCoding %x\r\n"), rcode);
                return rcode;
        }
        // Tell the user that the device has connected
        printf("\r\n\r\nConnected.\r\n\r\n");
        return 0;
}

// declare EVERYTHING here AFTER all classes are described.
// TO-DO: use new instancing instead.

UHS_KINETIS_FS_HOST KINETIS_Usb;
UHS_USBHub hub_KINETIS1(&KINETIS_Usb);
UHS_USBHub hub_KINETIS2(&KINETIS_Usb);
MY_ACM Acm(&KINETIS_Usb);

void setup() {
        while(!USB_HOST_SERIAL);
        USB_HOST_SERIAL.begin(115200);

        printf_P(PSTR("\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\nStarting CDC-ACM test program...\r\n"));
        while(KINETIS_Usb.Init(1000) != 0);
        printf_P(PSTR("\r\n\r\nWaiting for Connection...\r\n"));
}

void loop() {

        if(Acm.isReady()) {
                uint8_t rcode;

                /* read the keyboard */
                if(USB_HOST_SERIAL.available()) {
                        uint8_t data = USB_HOST_SERIAL.read();
                        /* send to client */
                        rcode = Acm.Write(1, &data);
                        if(rcode) {
                                printf_P(PSTR("\r\nError %i on write\r\n"), rcode);
                                return;
                        }
                }


                /* read from client
                 * buffer size must be greater or equal to max.packet size
                 * it is set to the largest possible maximum packet size here.
                 * It must not be set less than 3.
                 */
                uint8_t buf[64];
                uint16_t rcvd = 64;
                rcode = Acm.Read(&rcvd, buf);
                if(rcode && rcode != hrNAK) {
                        printf_P(PSTR("\r\nError %i on read\r\n"), rcode);
                        return;
                }

                if(rcvd) {
                        // More than zero bytes received, display the text.
                        for(uint16_t i = 0; i < rcvd; i++) {
                                putc((char)buf[i], stdout);
                                //printf_P(PSTR("%c"),(char)buf[i]);
                        }
                        fflush(stdout);
                }
        }
}
