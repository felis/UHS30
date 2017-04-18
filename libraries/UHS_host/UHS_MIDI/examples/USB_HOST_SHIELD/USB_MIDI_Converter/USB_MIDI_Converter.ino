/*
 *******************************************************************************
 * USB-MIDI to Legacy Serial MIDI converter with SysEx support
 * Copyright 2017 Yuuichi Akagawa
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
// Load MIDI class driver
#define LOAD_UHS_MIDI

//#define UHS_DEVICE_WINDOWS_USB_SPEC_VIOLATION_DESCRIPTOR_DEVICE 1

#ifdef USBCON
#define _MIDI_SERIAL_PORT Serial1
#else
#define _MIDI_SERIAL_PORT Serial
#endif

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

void setup() {
  // USB data switcher, PC -> device. (test jig, this can be ignored for regular use)
  pinMode(5, OUTPUT);
  digitalWrite(5, HIGH);
  connected = false;
  USB_HOST_SERIAL.begin(115200);
  while (UsbHost.Init(1000) != 0);
}

void loop() {
  //   UsbHost.getUsbTaskState();
  uint32_t t1 = (uint32_t)micros();
  if (Midi.isReady()) {
    if (!connected) {
      connected = true;
      printf_P(PSTR("Connected to MIDI\r\n"));
    }
    MIDI_poll();
  } else {
    if (connected) {
      connected = false;
      printf_P(PSTR("\r\nDisconnected from MIDI\r\n"));
    }
  }
  //delay(1ms)
  doDelay(t1, (uint32_t)micros(), 1000);

}
// Poll USB MIDI Controler and send to serial MIDI
void MIDI_poll()
{
  uint8_t size;
  uint8_t outBuf[4];
  uint8_t sysexBuf[3];

  do {
    if ( (size = Midi.RecvRawData(outBuf)) > 0 ) {
      //MIDI Output
      uint8_t rc = Midi.extractSysExData(outBuf, sysexBuf);
      if ( rc != 0 ) { //SysEx
        _MIDI_SERIAL_PORT.write(sysexBuf, rc);
      } else { //SysEx
        _MIDI_SERIAL_PORT.write(outBuf + 1, size);
      }
    }
  } while (size > 0);
}

// Delay time (max 16383 us)
void doDelay(uint32_t t1, uint32_t t2, uint32_t delayTime)
{
  uint32_t t3;

  if ( t1 > t2 ) {
    t3 = (0xFFFFFFFF - t1 + t2);
  } else {
    t3 = t2 - t1;
  }

  if ( t3 < delayTime ) {
    delayMicroseconds(delayTime - t3);
  }
}
