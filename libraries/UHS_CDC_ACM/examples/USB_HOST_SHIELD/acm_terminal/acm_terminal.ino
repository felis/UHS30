// Load the USB Host System core
#define LOAD_USB_HOST_SYSTEM
// Load USB Host Shield
#define LOAD_USB_HOST_SHIELD
// Use USB hub
#define LOAD_UHS_HUB

// Patch printf so we can use it.
#define LOAD_UHS_PRINTF_HELPER
//#define DEBUG_PRINTF_EXTRA_HUGE 1
//#define DEBUG_PRINTF_EXTRA_HUGE_UHS_HOST 1
//#define DEBUG_PRINTF_EXTRA_HUGE_USB_HOST_SHIELD 1


// These all get combined under UHS_CDC_ACM multiplexer.
// Each should only add a trivial amount of code.
#define LOAD_UHS_CDC_ACM
#define LOAD_UHS_CDC_ACM_XR21B1411
// This needs testing.
#define LOAD_UHS_CDC_ACM_PROLIFIC
// This is coming soon!
//#define LOAD_UHS_CDC_ACM_FTDI

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

class MY_ACM : public UHS_CDC_ACM {
public:

        MY_ACM(UHS_USB_HOST_BASE *p) : UHS_CDC_ACM(p) {
        };
        uint8_t OnStart(void);
};

uint8_t MY_ACM::OnStart(void) {
        uint8_t rcode;
        // Set DTR = 1 RTS=1
        rcode = SetControlLineState(3);

        if(rcode) {
                ErrorMessage<uint8_t>(PSTR("SetControlLineState"), rcode);
                return rcode;
        }

        UHS_CDC_LINE_CODING lc;
        lc.dwDTERate = 9600;
        lc.bCharFormat = 0;
        lc.bParityType = 0;
        lc.bDataBits = 8;

        rcode = SetLineCoding(&lc);

        if(rcode)
                ErrorMessage<uint8_t>(PSTR("SetLineCoding"), rcode);

        return rcode;
}

MY_ACM Acm(&MAX3421E_Usb);

void setup() {
        while(!USB_HOST_SERIAL);
        USB_HOST_SERIAL.begin(115200);

        E_Notify(PSTR("\r\n\r\nStarting CDC-ACM test program...\r\n"), 0);
        while(MAX3421E_Usb.Init(1000) != 0);
        E_Notify(PSTR("\r\n\r\ngo!\r\n"), 0);

}

void loop() {

        if(Acm.isReady()) {
                uint8_t rcode;

                /* reading the keyboard */
                if(USB_HOST_SERIAL.available()) {
                        uint8_t data = USB_HOST_SERIAL.read();
                        /* sending to the phone */
                        rcode = Acm.Write(1, &data);
                        if(rcode)
                                ErrorMessage<uint8_t>(PSTR("SndData"), rcode);
                }//if(Serial.available()...


                /* reading the phone */
                /* buffer size must be greater or equal to max.packet size */
                /* it it set to 64 (largest possible max.packet size) here, can be tuned down
                for particular endpoint */
                uint8_t buf[64];
                uint16_t rcvd = 64;
                rcode = Acm.Read(&rcvd, buf);
                if(rcode && rcode != hrNAK)
                        ErrorMessage<uint8_t>(PSTR("Ret"), rcode);

                if(rcvd) { //more than zero bytes received
                        for(uint16_t i = 0; i < rcvd; i++) {
                                USB_HOST_SERIAL.print((char)buf[i]); //printing on the screen
                        }
                }
        }
}
