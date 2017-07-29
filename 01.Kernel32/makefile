#
# makefile
#
#  Created on: 2017. 7. 4.
#      Author: Yummy
#

#########################################
####### 가상 OS 디스크 이미지 만들 때
#########################################
#all: VirtualOS.bin
#
#VirtualOS.bin: VirtualOS.asm
#	nasm -o VirtualOS.bin VirtualOS.asm
#	
#clean:
#	rm -f VirtualOS.bin

#########################################
####### 32비트 보호 모드 소스 파일 만들 때
#########################################
#all: Kernel32.bin
#
#Kernel32.bin: Source/EntryPoint.s
#	nasm -o Kernel32.bin $<	# $<는 Dependency(: 의 왼쪽)의 첫 번째 파일을 의미하는 매크로, 즉 Source/EntryPoint.s 로 치
#	
#clean:
#	rm -f Kernel32.bin

#########################################
# 빌드 환경 및 규칙 설정
#########################################
NASM32 = nasm
GCC32 = gcc -g -std=c99 -C -m32 -ffreestanding
LD32 = ld -elf_i386 -T ../Linker32.x -nostdlib -e Main -Ttext 0x10200
OBJCOPY32 = objcopy -j .text -j .data -j .rodata -j .bss -S -O binary

OBJDIR = Temp
SRCDIR = Source

#########################################
# 빌드 항목 및 빌드 방법 설정
#########################################
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
	
clean:
	rm -f *.bin
	rm -f $(OBJDIR)/*.*
	
#########################################
# Make에 의해 다시 호출되는 부분, Temp 디렉토리 기준 수행
#########################################
EPOBJFILE = Main.o
CSRCFILES = $(wildcard ../$(SRCDIR)/*.c)
ASMSRCFILES = $(wildcard ../$(SRCDIR)/*.asm)
COBJFILES = $(subst Main.o, , $(notdir $(patsubst %.c,%.o,$(CSRCFILES))))
ASMOBJFILES = $(notdir $(patsubst %.asm,%.o,$(ASMSRCFILES)))

# .c 파일을 .o 파일로 바꾸는 규칙 정의
%.o: ../$(SRCDIR)/%.c
	$(GCC32) -c $<
	
# .asm 파일을 .o 파일로 바꾸는 규칙 정의
%.o: ../$(SRCDIR)/%.asm
	$(NASM32) -f elf32 -o $@ $<
	
InternalDependency:
	$(GCC32) -MM $(CSRCFILES) > Dependency.dep
	
Kernel32.elf: $(EPOBJFILE) $(COBJFILES) $(ASMOBJFILES)
	$(LD32) -o $@ $^
	
ifeq (Dependency.dep, $(wildcard Dependency.dep))
include Dependency.dep
endif
