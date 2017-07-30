#########################################
# 빌드 환경 및 규칙 설정
#########################################
NASM64 = nasm -f elf64
GCC64 = gcc -g -std=c99 -C -m64 -ffreestanding -I ../Source/Headers -Wall -w
LD64 = ld -elf_x86_64 -T ../Linker64.x -nostdlib -e Main -Ttext 0x200000
OBJCOPY64 = objcopy -j .text -j .data -j .rodata -j .bss -S -O binary

OBJDIR = Temp
SRCDIR = Source

#########################################
# 빌드 항목 및 빌드 방법 설정
#########################################
all: prepare Kernel64.bin

prepare:
	mkdir -p $(OBJDIR)
	
dep:
	@echo === Make Dependency File ===
	make -C $(OBJDIR) -f ../makefile InternalDependency
	@echo === Dependency Search Complete ===
	
ExecuteInternalBuild: dep
	make -C $(OBJDIR) -f ../makefile Kernel64.elf
	
Kernel64.bin: ExecuteInternalBuild
	$(OBJCOPY64) $(OBJDIR)/Kernel64.elf $@
	rm -f $(SRCDIR)/Headers/*.gch
	
clean:
	rm -f *.bin
	rm -f $(OBJDIR)/*.*
	
#########################################
# Make에 의해 다시 호출되는 부분, Temp 디렉터리를 기준으로 수행됨
#########################################
# 빌드할 어셈블리어 엔트리 포인트 소스 파일 정의, Temp 디렉터리를 기준으로 설정
EPSRCFILE = ../$(SRCDIR)/EntryPoint.s
EPOBJFILE = EntryPoint.o
# 빌드할 C소스 파일 정의, Temp 디렉터리를 기준으로 설정
CSRCFILES = $(shell find ../$(SRCDIR)/ -name *.c)
ASMSRCFILES = $(shell find ../$(SRCDIR)/ -name *.asm)
COBJFILES = $(notdir $(patsubst %.c,%.o,$(CSRCFILES)))
ASMOBJFILES = $(notdir $(patsubst %.asm,%.o,$(ASMSRCFILES)))

# 어셈블리어 엔트리 포인트 빌드
$(EPOBJFILE): $(EPSRCFILE)
	$(NASM64) -o $@ $<
	
# .c 파일을 .o 파일로 바꾸는 규칙 정의
$(COBJFILES): $(CSRCFILES)
	$(GCC64) -c $^
	
# .asm 파일을 .o 파일로 바꾸는 규칙 정의
$(ASMOBJFILES): $(ASMSRCFILES)
	$(foreach var, $(ASMSRCFILES), $(NASM64) $(var) -o $(notdir $(patsubst %.asm, %.o, $(var)));)
	
InternalDependency:
	$(GCC64) -MM $(CSRCFILES) > Dependency.dep
	
Kernel64.elf: $(EPOBJFILE) $(COBJFILES) $(ASMOBJFILES)
	$(LD64) -o $@ $^
	
ifeq (Dependency.dep, $(wildcard Dependency.dep))
include Dependency.dep
endif
