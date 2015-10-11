# UHS30
Pre-release of USB Host Library version 3.0. No technical support offered at this time.

__This set of libraries is EXTERMELY ALPHA!__

# Arduino version 1.6.1 OR BETTER ONLY!
* This library set does all USB host operations in an interrupt. This means that calling the Task() method no longer does anything, unless you disable the interrupt mode in your sketch.
* This library set provides interrupt safe heap for AVR and ARM based boards.<BR>
* Circuitsathome USB Host Shield support (of course!)
* Freescale Kinetis native full-speed host support!
    * NOTE! To use the Teensy 3.0/3.1/LC in host mode, select "No USB" from the tools menu for USB Type.

## AVR Arduino hardware instructions
On AVR based Arduino boards and clones, you must do one of the following:

Connect pin 9 to pin 3 with a jumper wire.

OR

Cut the INT solder bridge. Connect from the INT thru-hole to pin 3 thru-hole with a jumper wire.

OR

Cut the INT solder bridge. Install right angle headers and use a female jumper wire.

## NON-AVR Arduino hardware instructions
No special modifications are required!
ARM boards have interrupt capabilities on nearly every pin.

### Installation
Copy each of the directories in the libraries directory into your USER LIBARIES directories. DO NOT copy these to your system-wide installation. (you should never do that anyway!)
