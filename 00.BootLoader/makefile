all: BootLoader.bin

BootLoader.bin: BootLoader.asm
	nasm -o $@ $<
	chmod +x $@
	mv $@ ../BinFiles/
