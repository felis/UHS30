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


MAX3421E_HOST UHS_Usb;
UHS_USBHub hub_1(&UHS_Usb);

void setup() {
        USB_HOST_SERIAL.begin(115200);
        while(UHS_Usb.Init(1000) != 0);


}

void loop() {
        delay(1);
}
