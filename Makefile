FIND ?=find
DIRNAME ?=dirname

TOTEST=$(shell $(FIND) ./libraries -name Makefile -exec $(DIRNAME) \{\} \;)

all: build

squeeky: $(TOTEST)
	for i in $? ; do (cd $$i && $(MAKE) ) ; done

clean: $(TOTEST)
	for i in $? ; do (cd $$i && $(MAKE) ) ; done

build: $(TOTEST)
	for i in $? ; do (cd $$i && $(MAKE) ) ; done

.PHONY: all clean build squeeky
