// Patch printf so we can use it.
#define LOAD_UHS_PRINTF_HELPER
// Load the USB Host System core
#define LOAD_USB_HOST_SYSTEM
// Load USB Host Shield
#define LOAD_USB_HOST_SHIELD
// Load PRINTER class driver
#define LOAD_UHS_PRINTER
#define LOAD_UHS_HUB
#define DEBUG_PRINTF_EXTRA_HUGE_USB_HUB 0
#define ENABLE_UHS_DEBUGGING 0
#define DEBUG_PRINTF_EXTRA_HUGE 0
#define DEBUG_PRINTF_EXTRA_HUGE_UHS_HOST 0
#define DEBUG_PRINTF_EXTRA_HUGE_USB_HOST_SHIELD 0
#define DEBUG_PRINTF_EXTRA_HUGE_PRINTER_HOST 0
#define UHS_DEVICE_WINDOWS_USB_SPEC_VIOLATION_DESCRIPTOR_DEVICE 1

#include <Arduino.h>
#ifdef true
#undef true
#endif
#ifdef false
#undef false
#endif

#include <UHS_host.h>

//MAX3421E_HOST *UHS_Usb;
//UHS_PRINTER *my_printer;
MAX3421E_HOST UHS_Usb;
UHS_USBHub hub1(&UHS_Usb);
UHS_PRINTER my_printer(&UHS_Usb);
UHS_PRINTER my_printer2(&UHS_Usb);
bool connected;
String s;
bool isReady1;

void setup() {
        connected = false;
        while(!Serial) {
                yield();
        }
        Serial.begin(115200);
        delay(100);
        Serial.println(F("USB PRINTER example."));
        //UHS_Usb = new MAX3421E_HOST();
        //my_printer = new UHS_PRINTER(UHS_Usb);
        while(UHS_Usb.Init(1000) != 0);
}

char message[] = "Hello World!\r\n";

void loop() {
        if(my_printer.isReady()) {
                if(!connected) {
                        connected = true;
                        printf_P(PSTR("Connected to PRINTER\r\n"));
                        my_printer.select_printer();
                        my_printer.write(strlen(message), (uint8_t *)message);
                }
        } else {
                if(connected) {
                        connected = false;
                        printf_P(PSTR("\r\nDisconnected from PRINTER\r\n"));
                }
        }
}
