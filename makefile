#
# makefile
#
#  Created on: 2017. 6. 30.
#      Author: Yummy
#

all: BootLoader Kernel32 Kernel64 Disk.img Utility

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
	
Disk.img: 00.BootLoader/BootLoader.bin 01.Kernel32/Kernel32.bin 02.Kernel64/Kernel64.bin # BootLoader Kernel32 Kernel 64
	@echo
	@echo =============== Disk Image Build Start ===============
	@echo
	
#	cat 00.BootLoader/BootLoader.bin 01.Kernel32/VirtualOS.bin > Disk.img
#	cat $^ > Disk.img	# $^는 Dependency(: 의 오른쪽)에 나열된 전체 파일을 의미함
	./ImageMaker.exe $^
	
	@echo
	@echo =============== All Build Complete ===============
	@echo
	
# 유틸리티 빌드
Utility: 
	@echo 
	@echo =============== Utility Build Start ===============
	@echo 

	make -C 04.Utility

	@echo 
	@echo =============== Utility Build Complete ===============
	@echo 
	
clean:
	make -C 00.BootLoader clean
	make -C 01.Kernel32 clean
	make -C 02.Kernel64 clean
	make -C 04.Utility clean
	rm -f Disk.img
