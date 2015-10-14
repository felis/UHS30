FIND ?=find
DIRNAME ?=dirname

TOTEST=$(shell $(FIND) ./libraries -name Makefile -exec $(DIRNAME) \{\} \;)
$(info $(TOTEST))
all: build

squeeky:
	$(foreach thing,$(TOTEST),cd $(thing); make squeeky) 

clean:
	$(foreach thing,$(TOTEST),cd $(thing); make clean) 

build:
	$(foreach thing,$(TOTEST),cd $(thing); make build) 

.PHONY: all clean build
