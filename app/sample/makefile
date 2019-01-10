NASM64 = nasm -f elf64
GCC64 = gcc -g -std=c99 -C -m64 -ffreestanding -Wall -w -mcmodel=large -fno-common
LD64 = ld -elf_x86_64 -T ../linker64.x -nostdlib -e Start -Ttext 0x0000
LDFLAGS = -lyummyHit -L../../custom_lib/tmp
CFLAGS = -I../../custom_lib/src
OBJCOPY64 = objcopy -j .text -j .data -j .rodata -j .bss -S #O binary

OBJDIR = tmp
SRCDIR = src

all: prepare sample.elf

prepare:
	mkdir -p $(OBJDIR)
	
dep:
	@echo === Make Dependency File ===
	make -C $(OBJDIR) -f ../makefile InternalDependency
	@echo === Dependency Search Complete ===
	
ExecuteInternalBuild: dep
	make -C $(OBJDIR) -f ../makefile app.elf
	
sample.elf: ExecuteInternalBuild
	$(OBJCOPY64) $(OBJDIR)/app.elf $@
	chmod +x $@
	mv $@ ../../util/SerialTransfer/
	rm -f $(OBJDIR)/*.*
	rm -f ../custom_lib/src/*.gch
	
CSRCFILES = $(shell find ../ -name *.c)
ASMSRCFILES = $(shell find ../ -name *.asm)
COBJFILES = $(notdir $(patsubst %.c,%.o,$(CSRCFILES)))
ASMOBJFILES = $(notdir $(patsubst %.asm,%.o,$(ASMSRCFILES)))

$(COBJFILES): $(CSRCFILES)
	$(GCC64) -c $^ $(CFLAGS)
	
$(ASMOBJFILES): $(ASMSRCFILES)
	$(foreach var, $(ASMSRCFILES), $(NASM64) $(var) -o $(notdir $(patsubst %.asm, %.o, $(var)));)
	
InternalDependency:
	$(GCC64) -MM $(CSRCFILES) > Dependency.dep $(CFLAGS)
	
app.elf: $(COBJFILES) $(ASMOBJFILES)
	$(LD64) -r -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OBJDIR)/*.*
	
ifeq (Dependency.dep, $(wildcard Dependency.dep))
include Dependency.dep
endif
