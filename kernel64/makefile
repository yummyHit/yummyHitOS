NASM64 = nasm -f elf64
GCC64 = gcc -g -std=c99 -C -m64 -ffreestanding -I ../src/includes -Wall -w
LD64 = ld -elf_x86_64 -T ../linker64.x -nostdlib -e main -Ttext 0x200000
OBJCOPY64 = objcopy -j .text -j .data -j .rodata -j .bss -S -O binary

OBJDIR = tmp
SRCDIR = src

all: prepare kernel64.bin

prepare:
	mkdir -p $(OBJDIR)

dep:
	@echo === Make Dependency File ===
	make -C $(OBJDIR) -f ../makefile InternalDependency
	@echo === Dependency Search Complete ===

ExecuteInternalBuild: dep
	make -C $(OBJDIR) -f ../makefile kernel64.elf

kernel64.bin: ExecuteInternalBuild
	$(OBJCOPY64) $(OBJDIR)/kernel64.elf $@
	chmod +x $@
	mv $@ ../binfiles/
	rm -f $(SRCDIR)/includes/*.gch
	rm -f $(OBJDIR)/*.*
	
EPSRCFILE = ../$(SRCDIR)/ep.s
EPOBJFILE = ep.o
CSRCFILES = $(shell find ../ -name *.c)
ASMSRCFILES = $(shell find ../ -name *.asm)
COBJFILES = $(notdir $(patsubst %.c,%.o,$(CSRCFILES)))
ASMOBJFILES = $(notdir $(patsubst %.asm,%.o,$(ASMSRCFILES)))

$(EPOBJFILE): $(EPSRCFILE)
	$(NASM64) -o $@ $<
	
$(COBJFILES): $(CSRCFILES)
	$(GCC64) -c $(CSRCFILES)
	
$(ASMOBJFILES): $(ASMSRCFILES)
	$(foreach var, $(ASMSRCFILES), $(NASM64) $(var) -o $(notdir $(patsubst %.asm, %.o, $(var)));)
	
InternalDependency:
	$(GCC64) -MM $(CSRCFILES) > Dependency.dep
	
kernel64.elf: $(EPOBJFILE) $(COBJFILES) $(ASMOBJFILES)
	$(LD64) -o $@ $^

clean:
	rm -f $(SRCDIR)/includes/*.gch
	rm -f $(OBJDIR)/*.*
	
ifeq (Dependency.dep, $(wildcard Dependency.dep))
include Dependency.dep
endif
