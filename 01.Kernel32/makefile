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
GCC32 = gcc -c -m32 -ffreestanding
LD32 = ld -melf_i386 -T ../elf_i386.x -nostdlib -e Main -Ttext 0x10200
OBJCOPY32 = objcopy -j .text -j .data -j .rodata -j .bss -S -O binary

OBJECTDIRECTORY = Temp
SOURCEDIRECTORY = Source

#########################################
# 빌드 항목 및 빌드 방법 설정
#########################################
all: prepare Kernel32.bin

prepare:
	mkdir -p $(OBJECTDIRECTORY)
	
$(OBJECTDIRECTORY)/EntryPoint.bin: $(SOURCEDIRECTORY)/EntryPoint.s
	$(NASM32) -o $@ $<
	
dep:
	@echo === Make Dependency File ===
	make -C $(OBJECTDIRECTORY) -f ../makefile InternalDependency
	@echo === Dependency Search Complete ===
	
ExecuteInternalBuild: dep
	make -C $(OBJECTDIRECTORY) -f ../makefile Kernel32.elf
	
$(OBJECTDIRECTORY)/Kernel32.elf.bin: ExecuteInternalBuild
	$(OBJCOPY32) $(OBJECTDIRECTORY)/Kernel32.elf $@
	
Kernel32.bin: $(OBJECTDIRECTORY)/EntryPoint.bin $(OBJECTDIRECTORY)/Kernel32.elf.bin
	cat $^ > $@
	
clean:
	rm -f *.bin
	rm -f $(OBJECTDIRECTORY)/*.*
	
#########################################
# Make에 의해 다시 호출되는 부분, Temp 디렉토리 기준 수행
#########################################
CENTRYPOINTOBJECTFILE = Main.o
CSOURCEFILES = $(wildcard ../$(SOURCEDIRECTORY)/*.c)
ASSEMBLYSOURCEFILES = $(wildcard ../$(SOURCEDIRECTORY)/*.asm)
COBJECTFILES = $(subst Main.o, , $(notdir $(patsubst %.c,%.o,$(CSOURCEFILES))))
ASSEMBLYOBJECTFILES = $(notdir $(patsubst %.asm,%.o,$(ASSEMBLYSOURCEFILES)))

# .c 파일을 .o 파일로 바꾸는 규칙 정의
%.o: ../$(SOURCEDIRECTORY)/%.c
	$(GCC32) -c $<
	
# .asm 파일을 .o 파일로 바꾸는 규칙 정의
%.o: ../$(SOURCEDIRECTORY)/%.asm
	$(NASM32) -f elf32 -o $@ $<
	
InternalDependency:
	$(GCC32) -MM $(CSOURCEFILES) > Dependency.dep
	
Kernel32.elf: $(CENTRYPOINTOBJECTFILE) $(COBJECTFILES) $(ASSEMBLYOBJECTFILES)
	$(LD32) -o $@ $^
	
ifeq (Dependency.dep, $(wildcard Dependency.dep))
include Dependency.dep
endif
