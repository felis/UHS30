/*
 *******************************************************************************
 * USB-MIDI dump utility
 * Copyright 2013-2017 Yuuichi Akagawa
 *
 * based on USBH_MIDI for USB Host Shield 2.0.
 * https://github.com/YuuichiAkagawa/USBH_MIDI
 *
 * This is sample program. Do not expect perfect behavior.
 *
 * Note: This driver is support for MIDI Streaming class only.
 *       If your MIDI Controler is not work, probably you needs its vendor specific driver. *******************************************************************************
 */

// Load the USB Host System core
#define LOAD_USB_HOST_SYSTEM
// Load USB Host Shield
#define LOAD_USB_HOST_SHIELD
#define LOAD_UHS_MIDI

// Patch printf so we can use it.
#define LOAD_UHS_PRINTF_HELPER
//#define DEBUG_PRINTF_EXTRA_HUGE 1
//#define DEBUG_PRINTF_EXTRA_HUGE_UHS_HOST 1
//#define DEBUG_PRINTF_EXTRA_HUGE_USB_HUB 1
//#define DEBUG_PRINTF_EXTRA_HUGE_MIDI_HOST 1
//#define UHS_DEBUG_USB_ADDRESS 1
// Redirect debugging and printf
//#define UHS_DEVICE_WINDOWS_USB_SPEC_VIOLATION_DESCRIPTOR_DEVICE 1
#define USB_HOST_SERIAL Serial

#include <Arduino.h>
#ifdef true
#undef true
#endif
#ifdef false
#undef false
#endif

#include <UHS_host.h>

MAX3421E_HOST UsbHost;
UHS_MIDI Midi(&UsbHost);
bool connected;
char buf[20];


void setup() {
  // USB data switcher, PC -> device. (test jig, this can be ignored for regular use)
  pinMode(5, OUTPUT);
  digitalWrite(5, HIGH);
  connected = false;
  USB_HOST_SERIAL.begin(115200);
  while (UsbHost.Init(1000) != 0);
  printf_P(PSTR("\r\nHost initialized.\r\n"));

}

void loop() {
  if (Midi.isReady()) {
    if (!connected) {
      connected = true;
      printf_P(PSTR("Connected to MIDI\r\n"));
      sprintf(buf, "VID:%04X, PID:%04X", Midi.vid, Midi.pid);
      Serial.println(buf);

    }
    MIDI_poll();
  } else {
    if (connected) {
      connected = false;
      printf_P(PSTR("\r\nDisconnected from MIDI\r\n"));
    }
  }
}

// Poll USB MIDI Controler and send to serial MIDI
void MIDI_poll()
{
  uint8_t bufMidi[64];
  uint16_t  rcvd;
  if (Midi.RecvData( &rcvd,  bufMidi) == 0 ) {
    sprintf(buf, "%08lX: ", (uint32_t)millis());
    Serial.print(buf);
    Serial.print(rcvd);
    Serial.print(':');
    for (int i = 0; i < 64; i++) {
      sprintf(buf, " %02X", bufMidi[i]);
      Serial.print(buf);
    }
    Serial.println("");
  }
}