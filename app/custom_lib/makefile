NASM64 = nasm -f elf64
GCC64 = gcc -g -std=c99 -C -m64 -ffreestanding -I ../../../kernel64/src/includes -Wall -w -mcmodel=large -fno-common
AR64 = ar rcs

OBJDIR = tmp
SRCDIR = src

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
	rm -f ../../kernel64/src/includes/*.gch
	mv ./libyummyHit.a $(OBJDIR)/libyummyHit.a

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
