all: bootloader.bin

bootloader.bin: bootloader.asm
	nasm -o $@ $<
	chmod +x $@
	mv $@ ../binfiles/
