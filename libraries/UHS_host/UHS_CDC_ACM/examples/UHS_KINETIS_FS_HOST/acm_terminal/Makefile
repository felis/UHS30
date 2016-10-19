#BOARD=teensy:avr:teensy30:usb=serial,speed=96opt,keys=en-us
BOARD=teensy:avr:teensy31:usb=serial,speed=96opt,keys=en-us
#BOARD=teensy:avr:teensy33:usb=serial,speed=96opt,keys=en-us
#BOARD=teensy:avr:teensy34:usb=serial,speed=96opt,keys=en-us
#BOARD=teensy:avr:teensy35:usb=serial,speed=96opt,keys=en-us
#BOARD=teensy:avr:teensy36:usb=serial,speed=96opt,keys=en-us

PORT = /dev/ttyUSB0

# And finally, the part that brings everything together for you.
HOMEDIR =$(HOME)
BUILD_PATH ?=$(HOMEDIR)/Arduino/build/$(SKETCH)/_$(build_aDjOkT_core)/_$(build_aDjOkT_board)/_$(build_aDjOkT_arch)/_$(build_aDjOkT_mcu)
include ~/Arduino/Arduino_Make/_Makefile.master
