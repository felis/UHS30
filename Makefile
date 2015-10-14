FIND ?=find
DIRNAME ?=dirname

TOTEST ?=$(shell $(FIND) ./libraries -name Makefile -exec $(DIRNAME) \{\} \;)

all: build

squeeky: $(TOTEST)
	@for i in $? ; do (cd $$i && $(MAKE) ) ; done
	@echo FULL cleanup done
	
clean: $(TOTEST)
	@for i in $? ; do (cd $$i && $(MAKE) ) ; done
	@echo Cleanup done

build: $(TOTEST)
	@for i in $? ; do (cd $$i && make all ) ; done
	@echo all targets built

.PHONY: all clean build squeeky
