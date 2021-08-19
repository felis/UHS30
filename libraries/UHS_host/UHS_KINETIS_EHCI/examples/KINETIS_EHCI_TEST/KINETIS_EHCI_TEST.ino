// Send an 's' to print out the INTEN and CTL registers.
// Send a 'p' to print out status of the interface.

// inline library loading
// Patch printf so we can use it.
#define LOAD_UHS_PRINTF_HELPER
// Loads the Kinetis core
#define LOAD_UHS_KINETIS_EHCI
// Load the USB Host System core
#define LOAD_USB_HOST_SYSTEM

// enable testing output
#define LED_STATUS 1
#define DEBUG_PRINTF_EXTRA_HUGE 1
#define DEBUG_PRINTF_EXTRA_HUGE_UHS_HOST 1

#include <Arduino.h>
#ifdef true
#undef true
#endif
#ifdef false
#undef false
#endif

// Bring in all the libraries that we requested above.
#include <UHS_host.h>

UHS_KINETIS_EHCI UHS_Usb;
uint8_t current_state = 128;
uint8_t last_state = 255;

uint8_t d;
void poopOutStatus() {
        uint32_t n;

        n = USBHS_PORTSC1;
        if(n & USBHS_PORTSC_PR) {
                printf("reset ");
        }
        if(n & USBHS_PORTSC_PP) {
                printf("on ");
        } else {
                printf("off ");
        }
        if(n & USBHS_PORTSC_PHCD) {
                printf("phyoff ");
        }
        if(n & USBHS_PORTSC_PE) {
                if(n & USBHS_PORTSC_SUSP) {
                        printf("suspend ");
                } else {
                        printf("enable ");
                }
        } else {
                printf("disable ");
        }
        printf("speed=");
        switch(((n >> 26) & 3)) {
                case 0: printf("12 Mbps FS ");
                        break;
                case 1: printf("1.5 Mbps LS ");
                        break;
                case 2: printf("480 Mbps HS ");
                        break;
                default: printf("(undefined) ");
        }
        if(n & USBHS_PORTSC_HSP) {
                printf("high-speed ");
        }
        if(n & USBHS_PORTSC_OCA) {
                printf("overcurrent ");
        }
        if(n & USBHS_PORTSC_CCS) {
                printf("connected ");
        } else {
                printf("not-connected ");
        }

        printf(" run=%i", (USBHS_USBCMD & 1) ? 1 : 0); // running mode

        // print info about the EHCI status
        n = USBHS_USBSTS;
        printf(",SUSP=%i", n & USBHS_USBSTS_HCH ? 1 : 0);
        printf(",RECL=%i", n & USBHS_USBSTS_RCL ? 1 : 0);
        printf(",PSRUN=%i", n & USBHS_USBSTS_PS ? 1 : 0);
        printf(",ASRUN=%i", n & USBHS_USBSTS_AS ? 1 : 0);
        printf(",USBINT=%i", n & USBHS_USBSTS_UI ? 1 : 0);
        printf(",USBERRINT=%i", n & USBHS_USBSTS_UEI ? 1 : 0);
        printf(",PCHGINT=%i", n & USBHS_USBSTS_PCI ? 1 : 0);
        printf(",FRAMEINT=%i", n & USBHS_USBSTS_FRI ? 1 : 0);
        printf(",errINT=%i", n & USBHS_USBSTS_SEI ? 1 : 0);
        printf(",AAINT=%i", n & USBHS_USBSTS_AAI ? 1 : 0);
        printf(",RSTINT=%i", n & USBHS_USBSTS_URI ? 1 : 0);
        printf(",SOFINT=%i", n & USBHS_USBSTS_SRI ? 1 : 0);
        printf(",NAKINT=%i", n & USBHS_USBSTS_NAKI ? 1 : 0);
        printf(",ASINT=%i", n & USBHS_USBSTS_UAI ? 1 : 0);
        printf(",PSINT=%i", n & USBHS_USBSTS_UPI ? 1 : 0);
        printf(",T0INT=%i", n & USBHS_USBSTS_TI0 ? 1 : 0);
        printf(",T1INT=%i", n & USBHS_USBSTS_TI1 ? 1 : 0);

        printf(",index=%lu\r\n", USBHS_FRINDEX); // periodic index
}
void setup() {
#if !defined(USB_HOST_SERIAL_NOWAIT)
        while(!USB_HOST_SERIAL);
#endif
        delay(5000); //wait 5 seconds for user to bring up a terminal
        USB_HOST_SERIAL.begin(115200);
        USB_HOST_SERIAL.println("Start.");
        while(UHS_Usb.Init(1000) != 0);
        // printf may be used after at least 1 host init
        printf("\r\n\r\nSWI_IRQ_NUM %i\r\n", SWI_IRQ_NUM);
        printf("\r\n\r\nUSB HOST READY.\r\n");

        pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
        // "I'm Alive" signal for Oscilloscope/logic probe.
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

        current_state = UHS_Usb.getUsbTaskState();
        if(current_state != last_state) {
                last_state = current_state;
                printf("USB HOST state %2.2x\r\n", current_state);
        }

        if (USB_HOST_SERIAL.available() > 0) {

                d = USB_HOST_SERIAL.read();
                if(d=='s') {
                        printf("USB0_INTEN: 0x%x ", USB0_INTEN);
                        printf("USB0_CTL: 0x%x\r\n", USB0_CTL);
                } else if(d=='p') {
                        poopOutStatus();
                }
        }

}
