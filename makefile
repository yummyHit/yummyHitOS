#
# makefile
#
#  Created on: 2017. 6. 30.
#      Author: Yummy
#

BINFILES = binfiles

all: boot.loader kernel.32 kernel.64 app.list util.list disk.img

boot.loader:
	@echo
	@echo =============== Build Boot Loader ===============
	@echo
	
	make -C bootloader
	
	@echo
	@echo =============== Build Complete ===============
	@echo
	
kernel.32:
	@echo
	@echo =============== Build 32bit Kernel ===============
	@echo
	
	make -C kernel32
	
	@echo
	@echo =============== Build Complete ===============
	@echo

kernel.64:
	@echo
	@echo =============== Build 64bit Kernel ===============
	@echo
	
	make -C kernel64
	
	@echo
	@echo =============== Build Complete ===============
	@echo

app.list:
	@echo 
	@echo =========== Application Build Start ===========
	@echo 
	
	make -C app
	
	@echo 
	@echo =========== Application Build Complete ===========
	@echo 

util.list:
	@echo 
	@echo =============== Utility Build Start ===============
	@echo 
	
	make -C util
	
	@echo 
	@echo =============== Utility Build Complete ===============
	@echo 

disk.img: $(BINFILES)/bootloader.bin $(BINFILES)/kernel32.bin $(BINFILES)/kernel64.bin
	@echo
	@echo =============== Disk Image Build Start ===============
	@echo
	
	$(BINFILES)/ImageMaker.exe $^
	
	@echo
	@echo =============== All Build Complete ===============
	@echo

clean:
	rm -f ./binfiles/*.*
	rm -f disk.img
