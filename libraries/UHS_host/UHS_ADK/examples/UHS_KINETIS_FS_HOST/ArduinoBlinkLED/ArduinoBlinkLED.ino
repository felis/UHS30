// The source for the Android application can be found at the following link: https://github.com/Lauszus/ArduinoBlinkLED
// The code for the Android application is heavily based on this guide: http://allaboutee.com/2011/12/31/arduino-adk-board-blink-an-led-with-your-phone-code-and-explanation/ by Miguel

// Load the USB Host System core
#define LOAD_USB_HOST_SYSTEM
// Load the Kinetis core
#define LOAD_UHS_KINETIS_FS_HOST
#define LOAD_UHS_ADK

// Patch printf so we can use it.
#define LOAD_UHS_PRINTF_HELPER
//#define DEBUG_PRINTF_EXTRA_HUGE 1
//#define DEBUG_PRINTF_EXTRA_HUGE_UHS_HOST 1
//#define DEBUG_PRINTF_EXTRA_HUGE_USB_HUB 1
//#define DEBUG_PRINTF_EXTRA_HUGE_ADK_HOST 1
//#define UHS_DEBUG_USB_ADDRESS 1
// Redirect debugging and printf
//#define UHS_DEVICE_WINDOWS_USB_SPEC_VIOLATION_DESCRIPTOR_DEVICE 1
#define USB_HOST_SERIAL Serial1

#define UHS_ADK_MANUFACTURER "TKJElectronics"
#define UHS_ADK_MODEL        "ArduinoBlinkLED"
#define UHS_ADK_DESCRIPTION  "Example sketch for the USB Host Shield"
#define UHS_ADK_VERSION      "1.0"
#define UHS_ADK_URI          "http://www.tkjelectronics.dk/uploads/ArduinoBlinkLED.apk"
#define UHS_ADK_SERIAL       "123456789"


#include <Arduino.h>
#ifdef true
#undef true
#endif
#ifdef false
#undef false
#endif

#include <UHS_host.h>

#if defined(LED_BUILTIN)
#define LED LED_BUILTIN // Use built in LED
#else
#define LED 11 // Set to something here that makes sense for your board.
#endif

UHS_KINETIS_FS_HOST UsbHost;
UHS_ADK adk(&UsbHost);
uint32_t timer;
bool connected;

void setup() {
        USB_HOST_SERIAL.begin(115200);
        pinMode(LED, OUTPUT);
        USB_HOST_SERIAL.print("\r\nArduino Blink LED Started");
        while(UsbHost.Init(1000) != 0);
}

void loop() {
        if(adk.isReady()) {
                if(!connected) {
                        connected = true;
                        printf_P(PSTR("\r\nConnected to accessory"));
                }

                uint8_t msg[1];
                uint16_t len = sizeof (msg);
                uint8_t rcode = adk.Read(&len, msg);
                if(rcode && rcode != UHS_HOST_ERROR_NAK) {
                        printf_P(PSTR("\r\nData rcv: "));
                        printf("%X", rcode);
                } else if(len > 0) {
                        printf_P(PSTR("\r\nData Packet: "));
                        printf("%c", msg[0]);
                        digitalWrite(LED, msg[0] ? HIGH : LOW);
                }

                if(millis() - timer >= 1000) { // Send data every 1s
                        timer = millis();
                        rcode = adk.Write(sizeof (timer), (uint8_t*) & timer);
                        if(rcode && rcode != UHS_HOST_ERROR_NAK) {
                                printf_P(PSTR("\r\nData send: "));
                                printf("%X", rcode);
                        } else if(rcode != UHS_HOST_ERROR_NAK) {
                                printf_P(PSTR("\r\nTimer: "));
                                printf("%lu", timer);
                        }
                }
        } else {
                if(connected) {
                        connected = false;
                        printf_P(PSTR("\r\nDisconnected from accessory"));
                        digitalWrite(LED, LOW);
                }
        }
}
