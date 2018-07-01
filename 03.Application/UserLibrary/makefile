NASM64 = nasm -f elf64
GCC64 = gcc -g -std=c99 -C -m64 -ffreestanding -I ../../../02.Kernel64/Source/Headers -Wall -w
AR64 = ar rcs

OBJDIR = Temp
SRCDIR = Source

all: prepare libyummyHit

prepare:
	mkdir -p $(OBJDIR)

dep:
	@echo === Make Dependancy File ===
	make -C $(OBJDIR) -f ../makefile InternalDependency
	@echo === Dependancy Search Complete ===

libyummyHit: dep
	make -C $(OBJDIR) -f ../makefile libyummyHit.a
	cp $(OBJDIR)/libyummyHit.a ./
	rm -f $(SRCDIR)/*.gch
	rm -f $(OBJDIR)/*.*
	rm -f ../../02.Kernel64/Source/Headers/*.gch
	rm -f ./libyummyHit.a

CSRCFILES = $(shell find ../ -name *.c)
ASMSRCFILES = $(shell find ../ -name *.asm)
COBJFILES = $(notdir $(patsubst %.c,%.o,$(CSRCFILES)))
ASMOBJFILES = $(notdir $(patsubst %.asm,%.o,$(ASMSRCFILES)))

$(COBJFILES): $(CSRCFILES)
	$(GCC64) -c $^

$(ASMOBJFILES): $(ASMSRCFILES)
	$(foreach var, $(ASMSRCFILES), $(NASM64) $(var) -o $(notdir $(patsubst %.asm, %.o, $(var)));)

InternalDependency:
	$(GCC64) -MM $(CSRCFILES) > Dependency.dep

libyummyHit.a: $(ASMOBJFILES) $(COBJFILES)
	$(AR64) $@ $^

clean:
	rm -f $(SRCDIR)/*.gch
	rm -f $(OBJDIR)/*.*

ifeq (Dependency.dep, $(wildcard Dependency.dep))
include Dependency.dep
endif
