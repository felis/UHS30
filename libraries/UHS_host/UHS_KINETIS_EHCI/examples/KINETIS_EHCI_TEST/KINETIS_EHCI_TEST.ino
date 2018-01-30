// Note: to use Serial 1 for debugging
// Attach FTDI or similar to pins 0 and 1
// N/8/1 @ 115200
// and uncomment the next two lines.
//#define USB_HOST_SERIAL Serial1
//#define USB_HOST_SERIAL_NOWAIT
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
#define EHCI_TEST_DEV
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

UHS_KINETIS_EHCI KINETIS_EHCI_Usb;
uint8_t current_state = 128;
uint8_t last_state = 255;

uint8_t d;


void setup() {
#if !defined(USB_HOST_SERIAL_NOWAIT)
        while(!USB_HOST_SERIAL);
#endif
        delay(5000); //wait 5 seconds for user to bring up a terminal
        USB_HOST_SERIAL.begin(115200);
        USB_HOST_SERIAL.println("Start.");
        while(KINETIS_EHCI_Usb.Init(1000) != 0);
        KINETIS_EHCI_Usb.vbusPower(vbus_on);
        // printf may be used after at least 1 host init
        printf("\r\n\r\nSWI_IRQ_NUM %i\r\n", SWI_IRQ_NUM);
        printf("\r\n\r\nUSB HOST READY.\r\n");

        pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
        // "I'm Alive" signal for Oscilloscope/logic probe.
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

        current_state = KINETIS_EHCI_Usb.getUsbTaskState();
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
                        KINETIS_EHCI_Usb.poopOutStatus();
                }
        }

}
