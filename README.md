# UHS30
Pre-release of USB Host Library version 3.0. No technical support offered at this time.

__This set of libraries is EXTERMELY ALPHA!__

# Arduino version 1.6.9 OR BETTER ONLY! DO NOT USE 1.6.10, it is broken!
* This library set does all USB host operations in an interrupt. This means that calling the Task() method no longer does anything, unless you disable the interrupt mode in your sketch.
* This library set provides interrupt safe heap for AVR, ARM, and PIC32 based boards.<BR>
* Circuitsathome USB Host Shield support (of course!)
* Freescale Kinetis native full-speed host support!
    * NOTE! To use the Teensy 3.0/3.1/3.2/3.4/3.5 and LC in host mode, select "No USB" from the tools menu for USB Type.

# Current working interface drivers
* MIDI
* Hub
* Mass storage
* CDC-ACM serial
* XR21B1411 serial
* PROLIFIC serial
* FTDI serial

## AVR MEGA ADK
No special modifications are required!
### Thank you Olayinka Babalola for donating an Arduino Mega ADK board. It would not have been possible without your support!

## AVR Arduino 
### Software tools
Arduino AVR boards by Arduino 1.6.11.
Newer versions are broken.
### hardware instructions
On AVR based Arduino boards and clones, you must do one of the following:

Connect pin 9 to pin 3 with a jumper wire.

OR

Cut the INT solder bridge. Connect from the INT thru-hole to pin 3 thru-hole with a jumper wire.

OR

Cut the INT solder bridge. Install right angle headers and use a female jumper wire.

## ARM Arduino hardware instructions
No special modifications are required!
ARM boards have interrupt capabilities on nearly every pin.

## PIC32
UNO32, uC32, WF32, Wi-Fire

Please note that the UNO32 really lacks enough flash to be very useful. Some demos will be too big to fit into flash.

Digilent messed up on the ICSP connector. You need to modify your shield. Pull the 2x3 header plastic cover, and break off pin recepticle contacts for pins 2 and 5. Run a patch wire from +5V to pin 2. Run a patch wire from RESET to pin 5. Put plastic cover back on. Don't worry, the board will still work on all other development boards. Detailed photos are in the images directory. Alternatly, modify the Digilent board. More recent boards have an option jumper which you could use to do this fix as well, however you still need to patch 5v onto pin 2. 

Then you must do one of the following:

Connect pin 9 to pin 7 with a jumper wire.

OR

Cut the INT solder bridge. Connect from the INT thru-hole to pin 7 thru-hole with a jumper wire.

OR

Cut the INT solder bridge. Install right angle headers and use a female jumper wire.


### Installation
Copy each of the directories in the libraries directory into your USER LIBRARIES directory. DO NOT copy these to your system-wide installation. (you should never do that anyway!)
