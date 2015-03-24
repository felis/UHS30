# UHS30
Pre-release of USB Host Library version 3.0. No technical support offered at this time.

<H2>This set of libraries is EXTERMELY ALPHA!<H2>
<H1>Arduino version 1.6.1 OR BETTER ONLY!</H1>
* This library set does all USB host operations in an interrupt. This means that calling the Task() method no longer does anything, unless you disable the interrupt mode in your sketch.<BR>
* This library set provides interrupt safe heap for AVR and ARM based boards.<BR>
* Circuitsathome USB Host Shield support (of course!)<BR>
* Freescale(tm) Kinetis(tm) native full-speed host support!<BR>

<HR><H2>AVR Arduino(tm) hardware instructions</H2>
On AVR based Arduino(tm) boards and clones, you must do one of the following:<BR>
Connect pin 9 to pin 3 with a jumper wire.<BR>
OR<BR>
Cut the INT solder bridge. Connect from the INT thruhole to pin 3 thruhole with a jumper wire.<BR>
OR<BR>
Cut the INT solder bridge. Install right angle headers and use a female jumper wire.<BR>

<HR><H2>NON-AVR Arduino(tm) hardware instructions</H2>
No special modifications are required!<BR>
ARM boards have interrupt capabilities on nearly every pin.
<HR><H3>Installation</H3>
Copy each of the directories in the libraries directory into your USER LIBARIES directories. DO NOT copy these to your system-wide installation. (you should never do that anyway!)

Have fun! -- xxxajk
