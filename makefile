#
# makefile
#
#  Created on: 2017. 6. 30.
#      Author: Yummy
#

BINFILES = BinFiles

all: BootLoader Kernel32 Kernel64 Application Utility Disk.img

BootLoader:
	@echo
	@echo =============== Build Boot Loader ===============
	@echo
	
	make -C 00.BootLoader
	
	@echo
	@echo =============== Build Complete ===============
	@echo
	
Kernel32:
	@echo
	@echo =============== Build 32bit Kernel ===============
	@echo
	
	make -C 01.Kernel32
	
	@echo
	@echo =============== Build Complete ===============
	@echo
	
Kernel64:
	@echo
	@echo =============== Build 64bit Kernel ===============
	@echo
	
	make -C 02.Kernel64
	
	@echo
	@echo =============== Build Complete ===============
	@echo

Application:
	@echo 
	@echo =========== Application Build Start ===========
	@echo 

	make -C 03.Application

	@echo 
	@echo =========== Application Build Complete ===========
	@echo 


Utility: 
	@echo 
	@echo =============== Utility Build Start ===============
	@echo 

	make -C 04.Utility

	@echo 
	@echo =============== Utility Build Complete ===============
	@echo 
	
Disk.img: $(BINFILES)/BootLoader.bin $(BINFILES)/Kernel32.bin $(BINFILES)/Kernel64.bin
	@echo
	@echo =============== Disk Image Build Start ===============
	@echo
	
	$(BINFILES)/ImageMaker.exe $^
	
	@echo
	@echo =============== All Build Complete ===============
	@echo
	
clean:
	rm -f ./BinFiles/*.*
	rm -f Disk.img
