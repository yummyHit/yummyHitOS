#########################################
# 빌드 환경 및 규칙 설정
#########################################
NASM64 = nasm -f elf64
GCC64 = gcc -g -std=c99 -C -m64 -ffreestanding -I ../Source/ -Wall -w
LD64 = ld -elf_x86_64 -T ../Linker64.x -nostdlib -e Main -Ttext 0x200000
OBJCOPY64 = objcopy -j .text -j .data -j .rodata -j .bss -S -O binary

OBJECTDIRECTORY = Temp
SOURCEDIRECTORY = Source

#########################################
# 빌드 항목 및 빌드 방법 설정
#########################################
all: prepare Kernel64.bin

prepare:
	mkdir -p $(OBJECTDIRECTORY)
	
dep:
	@echo === Make Dependency File ===
	make -C $(OBJECTDIRECTORY) -f ../makefile InternalDependency
	@echo === Dependency Search Complete ===
	
ExecuteInternalBuild: dep
	make -C $(OBJECTDIRECTORY) -f ../makefile Kernel64.elf
	
Kernel64.bin: ExecuteInternalBuild
	$(OBJCOPY64) $(OBJECTDIRECTORY)/Kernel64.elf $@
	
clean:
	rm -f *.bin
	rm -f $(OBJECTDIRECTORY)/*.*
	
#########################################
# Make에 의해 다시 호출되는 부분, Temp 디렉터리를 기준으로 수행됨
#########################################
# 빌드할 어셈블리어 엔트리 포인트 소스 파일 정의, Temp 디렉터리를 기준으로 설정
ENTRYPOINTSOURCEFILE = ../$(SOURCEDIRECTORY)/EntryPoint.s
ENTRYPOINTOBJECTFILE = EntryPoint.o
# 빌드할 C소스 파일 정의, Temp 디렉터리를 기준으로 설정
CSOURCEFILES = $(wildcard ../$(SOURCEDIRECTORY)/*.c)
ASSEMBLYSOURCEFILES = $(wildcard ../$(SOURCEDIRECTORY)/*.asm)
COBJECTFILES = $(notdir $(patsubst %.c,%.o,$(CSOURCEFILES)))
ASSEMBLYOBJECTFILES = $(notdir $(patsubst %.asm,%.o,$(ASSEMBLYSOURCEFILES)))

# 어셈블리어 엔트리 포인트 빌드
$(ENTRYPOINTOBJECTFILE): $(ENTRYPOINTSOURCEFILE)
	$(NASM64) -o $@ $<
	
# .c 파일을 .o 파일로 바꾸는 규칙 정의
%.o: ../$(SOURCEDIRECTORY)/%.c
	$(GCC64) -c $<
	
# .asm 파일을 .o 파일로 바꾸는 규칙 정의
%.o: ../$(SOURCEDIRECTORY)/%.asm
	$(NASM64) -o $@ $<
	
InternalDependency:
	$(GCC64) -MM $(CSOURCEFILES) > Dependency.dep
	
Kernel64.elf: $(ENTRYPOINTOBJECTFILE) $(COBJECTFILES) $(ASSEMBLYOBJECTFILES)
	$(LD64) -o $@ $^
	
ifeq (Dependency.dep, $(wildcard Dependency.dep))
include Dependency.dep
endif
