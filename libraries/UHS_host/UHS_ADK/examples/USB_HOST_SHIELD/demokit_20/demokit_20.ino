// The source for the Android application can be found at the following link: https://github.com/Lauszus/ArduinoBlinkLED
// The code for the Android application is heavily based on this guide: http://allaboutee.com/2011/12/31/arduino-adk-board-blink-an-led-with-your-phone-code-and-explanation/ by Miguel

// Load the USB Host System core
#define LOAD_USB_HOST_SYSTEM
// Load USB Host Shield
#define LOAD_USB_HOST_SHIELD
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
#define USB_HOST_SERIAL Serial

char demokit_MANUFACTURER[] = "Google, Inc.";
char demokit_MODEL[] = "DemoKit";
char demokit_DESCRIPTION[] = "DemoKit Arduino Board";
char demokit_VERSION[] = "1.0";
char demokit_URI[] = "http://www.android.com";
char demokit_SERIAL[] = "0000000012345678";


#include <Arduino.h>
#ifdef true
#undef true
#endif
#ifdef false
#undef false
#endif

#include <UHS_host.h>

MAX3421E_HOST UsbHost;
UHS_ADK adk(&UsbHost);
uint8_t b, b1;


#define  LED1_RED       LED_BUILTIN
#define  BUTTON1        2

void init_buttons() {
        pinMode(BUTTON1, INPUT);

        // enable the internal pullups
        digitalWrite(BUTTON1, HIGH);
}

void init_leds() {
        digitalWrite(LED1_RED, 0);

        pinMode(LED1_RED, OUTPUT);
}

void setup() {
        USB_HOST_SERIAL.begin(115200);
        printf_P(PSTR("\r\nADK demo start"));
        // USB data switcher, PC -> device. (test jig, this can be ignored for regular use)
        pinMode(5, OUTPUT);
        digitalWrite(5, HIGH);

        // This must be executed before host init.
        adk.SetHints(demokit_MANUFACTURER, demokit_MODEL, demokit_DESCRIPTION, demokit_VERSION, demokit_URI, demokit_SERIAL);

        while(UsbHost.Init(1000) != 0);
        printf_P(PSTR("\r\nHost initialized.\r\n"));

        init_leds();
        init_buttons();
        b1 = digitalRead(BUTTON1);
}

void loop() {
        uint8_t rcode;
        uint8_t msg[3] = {0x00};

        if(adk.isReady() == false) {
                analogWrite(LED1_RED, 255);
        } else {
                uint16_t len = sizeof (msg);

                rcode = adk.Read(&len, msg);
                if(rcode) {
                        printf_P(PSTR("Data rcv. : %2.2X\r\n"), rcode);
                }
                if(len > 0) {
                        printf_P(PSTR("Data Packet.\r\n"));
                        // assumes only one command per packet
                        if(msg[0] == 0x2) {
                                switch(msg[1]) {
                                        case 0:
                                                analogWrite(LED1_RED, 255 - msg[2]);
                                                break;
                                }//switch( msg[1]...
                        }//if (msg[0] == 0x2...
                }//if( len > 0...

                msg[0] = 0x1;

                b = digitalRead(BUTTON1);
                if(b != b1) {
                        printf_P(PSTR("Button state changed\r\n"));
                        msg[1] = 0;
                        msg[2] = b ? 0 : 1;
                        rcode = adk.Write(3, msg);
                        if(rcode) {
                                printf_P(PSTR("Button send: %2.2X\r\n"), rcode);
                        }
                        b1 = b;
                }//if (b != b1...


                delay(10);
        }
}
