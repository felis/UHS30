/*
 *******************************************************************************
 * USB-MIDI dump utility
 * Copyright 2013-2018 Yuuichi Akagawa
 *
 * based on USBH_MIDI for USB Host Shield 2.0.
 * https://github.com/YuuichiAkagawa/USBH_MIDI
 *
 * This is sample program. Do not expect perfect behavior.
 *
 * Note: This driver is support for MIDI Streaming class only.
 *       If your MIDI Controler is not work, probably you needs its vendor specific driver. *******************************************************************************
 */

// Patch printf so we can use it.
#define LOAD_UHS_PRINTF_HELPER
// Load the USB Host System core
#define LOAD_USB_HOST_SYSTEM
// Load USB Host Shield
#define LOAD_USB_HOST_SHIELD
// Load MIDI class driver
#define LOAD_UHS_MIDI

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

MAX3421E_HOST *UHS_Usb;
UHS_MIDI *Midi;
bool connected;

void setup() {
  connected = false;
  USB_HOST_SERIAL.begin(115200);
  delay(100);

  UHS_Usb = new MAX3421E_HOST();
  Midi = new UHS_MIDI(UHS_Usb);

  while (UHS_Usb->Init(1000) != 0);
  printf_P(PSTR("\r\nHost initialized.\r\n"));
}

void loop() {
  if (Midi->isReady()) {
    if (!connected) {
      connected = true;
      printf_P(PSTR("Connected to MIDI\r\n"));
      printf_P(PSTR("VID:%04X, PID:%04X\r\n"), Midi->idVendor(), Midi->idProduct());

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
  if (Midi->RecvData( &rcvd,  bufMidi) == 0 ) {
    printf_P(PSTR("%08lX:%d:"),  (uint32_t)millis(), rcvd);
    for (int i = 0; i < 64; i++) {
      printf_P(PSTR(" %02X"), bufMidi[i]);
    }
    printf_P(PSTR("\r\n"));
  }
}
