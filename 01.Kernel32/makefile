#
# makefile
#
#  Created on: 2017. 7. 4.
#      Author: Yummy
#

NASM32 = nasm
GCC32 = gcc -g -std=c99 -C -m32 -I ../../02.Kernel64/Source/Headers -ffreestanding
LD32 = ld -elf_i386 -T ../Linker32.x -nostdlib -e Main -Ttext 0x10200
OBJCOPY32 = objcopy -j .text -j .data -j .rodata -j .bss -S -O binary

OBJDIR = Temp
SRCDIR = Source

all: prepare Kernel32.bin

prepare:
	mkdir -p $(OBJDIR)
	
$(OBJDIR)/EntryPoint.bin: $(SRCDIR)/EntryPoint.s
	$(NASM32) -o $@ $<
	
dep:
	@echo === Make Dependency File ===
	make -C $(OBJDIR) -f ../makefile InternalDependency
	@echo === Dependency Search Complete ===
	
ExecuteInternalBuild: dep
	make -C $(OBJDIR) -f ../makefile Kernel32.elf
	
$(OBJDIR)/Kernel32.elf.bin: ExecuteInternalBuild
	$(OBJCOPY32) $(OBJDIR)/Kernel32.elf $@
	
Kernel32.bin: $(OBJDIR)/EntryPoint.bin $(OBJDIR)/Kernel32.elf.bin
	cat $^ > $@
	chmod +x $@
	mv $@ ../BinFiles/
	rm -f $(OBJDIR)/*.*
	
EPOBJFILE = Main.o
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
	
Kernel32.elf: $(EPOBJFILE) $(COBJFILES) $(ASMOBJFILES)
	$(LD32) -o $@ $^
	
ifeq (Dependency.dep, $(wildcard Dependency.dep))
include Dependency.dep
endif
