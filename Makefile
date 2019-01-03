FIND ?=find
DIRNAME ?=dirname

TOTEST ?=$(shell $(FIND) ./libraries -name Makefile -exec $(DIRNAME) \{\} \;)
$(info $(TOTEST))
all: build

squeeky: $(TOTEST)
	@for i in $? ; do (cd $$i && make squeeky ) ; done
	@echo FULL cleanup done

clean: $(TOTEST)
	@for i in $? ; do (cd $$i && make clean ) ; done
	@echo Cleanup done

build: $(TOTEST)
	@for i in $? ; do (cd $$i && make all ) ; done
	@echo all targets built

monitor:
	cu -l /dev/ttyUSB0 -s 115200 || cu -l /dev/ttyACM0 -s 115200 || cu -l /dev/ttyUSB1 -s 115200 || cu -l /dev/ttyACM1 -s 115200

.PHONY: all clean build squeeky monitor
