// Load the USB Host System core
#define LOAD_USB_HOST_SYSTEM
// Load USB Host Shield
#define LOAD_USB_HOST_SHIELD
// Use USB hub
#define LOAD_UHS_HUB

// Patch printf so we can use it.
#define LOAD_UHS_PRINTF_HELPER

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

void setup() {
        USB_HOST_SERIAL.begin(115200);
        while(MAX3421E_Usb.Init(1000) != 0);


}

void loop() {
        delay(1);
}
