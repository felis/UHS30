#BOARD=arduino:samd:arduino_zero_edbg
#BOARD=arduino:sam:arduino_due_x_dbg
#BOARD=esp8266:esp8266:huzzah:CpuFrequency=80,UploadTool=esptool,UploadSpeed=115200,FlashSize=4M3M
#BOARD=chipkit-core:pic32:chipkit_uc32
#BOARD=teensy:avr:teensy30:usb=disable,speed=96,opt=o1std,keys=en-us
BOARD=teensy:avr:teensy31:usb=disable,speed=96,opt=o1std,keys=en-us
#BOARD=teensy:avr:teensy33:usb=disable,speed=96,opt=o1std,keys=en-us
#BOARD=teensy:avr:teensy34:usb=disable,speed=96,opt=o1std,keys=en-us
#BOARD=teensy:avr:teensy35:usb=disable,speed=96,opt=o1std,keys=en-us
#BOARD=teensy:avr:teensy36:usb=disable,speed=96,opt=o1std,keys=en-us
#BOARD=teensy:avr:teensypp2:usb=serial,speed=16,keys=en-us
#BOARD=arduino:avr:mega:cpu=atmega2560


PORT = /dev/ttyACM0

# And finally, the part that brings everything together for you.
HOMEDIR =$(HOME)
BUILD_PATH ?=$(HOMEDIR)/Arduino/build/$(SKETCH)/_$(build_aDjOkT_core)/_$(build_aDjOkT_board)/_$(build_aDjOkT_arch)/_$(build_aDjOkT_mcu)
include ~/Arduino/Arduino_Make/_Makefile.master
