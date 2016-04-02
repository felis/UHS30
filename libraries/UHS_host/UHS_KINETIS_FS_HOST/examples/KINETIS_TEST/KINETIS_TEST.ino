// Redirect debugging and printf
#define USB_HOST_SERIAL Serial1

// inline library loading
// Patch printf so we can use it.
#define LOAD_UHS_PRINTF_HELPER
// Loads the Kinetis core
#define LOAD_UHS_KINETIS_FS_HOST
// Load the USB Host System core
#define LOAD_USB_HOST_SYSTEM


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
uint8_t current_state = 128;
uint8_t last_state = 255;

uint8_t d;


void setup() {
        USB_HOST_SERIAL.begin(115200);
        delay(10000);
        USB_HOST_SERIAL.println("Start.");
        while(KINETIS_Usb.Init(1000) != 0);
        // printf may be used after atleast 1 host init
        printf("\r\n\r\nSWI_IRQ_NUM %i\r\n", SWI_IRQ_NUM);
        printf("\r\n\r\nUSB HOST READY.\r\n");

        pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

        current_state = KINETIS_Usb.getUsbTaskState();
        if(current_state != last_state) {
                last_state = current_state;
                printf("USB HOST state %2.2x\r\n", current_state);
        }

        if (Serial1.available() > 0) {

                d = Serial1.read();
                if(d=='s') {
                        printf("USB0_INTEN: ");
                        USB_HOST_SERIAL.println(USB0_INTEN, HEX);
                        printf("USB0_CTL: ");
                        USB_HOST_SERIAL.println(USB0_CTL, HEX);
                } else if(d=='p') { //
                }
        }

}
