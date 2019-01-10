#
# makefile
#
#  Created on: 2017. 7. 4.
#      Author: Yummy
#

NASM32 = nasm
GCC32 = gcc -g -std=c99 -C -m32 -I ../../kernel64/src/includes -ffreestanding
LD32 = ld -elf_i386 -T ../linker32.x -nostdlib -e main -Ttext 0x10200
OBJCOPY32 = objcopy -j .text -j .data -j .rodata -j .bss -S -O binary

OBJDIR = tmp
SRCDIR = src

all: prepare kernel32.bin

prepare:
	mkdir -p $(OBJDIR)
	
$(OBJDIR)/ep.bin: $(SRCDIR)/ep.s
	$(NASM32) -o $@ $<
	
dep:
	@echo === Make Dependency File ===
	make -C $(OBJDIR) -f ../makefile InternalDependency
	@echo === Dependency Search Complete ===
	
ExecuteInternalBuild: dep
	make -C $(OBJDIR) -f ../makefile kernel32.elf
	
$(OBJDIR)/kernel32.elf.bin: ExecuteInternalBuild
	$(OBJCOPY32) $(OBJDIR)/kernel32.elf $@
	
kernel32.bin: $(OBJDIR)/ep.bin $(OBJDIR)/kernel32.elf.bin
	cat $^ > $@
	chmod +x $@
	mv $@ ../binfiles/
	rm -f $(OBJDIR)/*.*
	
EPOBJFILE = main32.o
CSRCFILES = $(wildcard ../$(SRCDIR)/*.c)
ASMSRCFILES = $(wildcard ../$(SRCDIR)/*.asm)
COBJFILES = $(subst Main.o, , $(notdir $(patsubst %.c,%.o,$(CSRCFILES))))
ASMOBJFILES = $(notdir $(patsubst %.asm,%.o,$(ASMSRCFILES)))

%.o: ../$(SRCDIR)/%.c
	$(GCC32) -c $<
	
%.o: ../$(SRCDIR)/%.asm
	$(NASM32) -f elf32 -o $@ $<
	
InternalDependency:
	$(GCC32) -MM $(CSRCFILES) > Dependency.dep
	
kernel32.elf: $(EPOBJFILE) $(COBJFILES) $(ASMOBJFILES)
	$(LD32) -o $@ $^
	
ifeq (Dependency.dep, $(wildcard Dependency.dep))
include Dependency.dep
endif
