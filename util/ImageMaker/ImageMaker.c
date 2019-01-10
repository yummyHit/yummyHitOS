/*
 * ImageMaker.c
 *
 *  Created on: 2017. 7. 12.
 *      Author: Yummy
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/uio.h>    // Mac에서는 sys/uio.h 타 플렛폼에서는 io.h일 수도 있음
#include <unistd.h>     // Unix 기반에서만 include
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define BYTESOFSECTOR 512

// 함수 선언
int sectorSize(int fd, int srcSize);
void writeInfo(int targetFd, int totalSectorCnt, int sectorCnt); // 170718 수정
int copyFile(int srcFd, int targerFd);

// Main 함수
int main(int argc, char* argv[]) {
	int srcFd, targetFd, bootSize, lowSectorCnt, highSectorCnt, srcSize;

	// 커맨드 라인 옵션 검사
	if(argc < 4) {	// argc 수 및 kernel64.bin 추가
		fprintf(stderr, "[ERROR] ImageMaker.exe bootloader.bin kernel32.bin kernel64.bin\n");
		exit(-1);
	}

	// disk.img 파일을 생성
	if((targetFd = open("disk.img", O_RDWR | O_CREAT | O_TRUNC)) == -1) { // , S_IREAD | S_IWRITE
		fprintf(stderr, "[ERROR] disk.img open fail.\n");
		exit(-1);
	}

	//----------------------------------------------------------------
	// 부트 로더 파일을 열어서 모든 내용을 디스크 이지미 파일로 복사
	//----------------------------------------------------------------
	printf("[INFO] Copy boot loader to image file\n");
	if((srcFd = open(argv[1], O_RDONLY)) == -1) {
		fprintf(stderr, "[ERROR] %s open fail\n", argv[1]);
		exit(-1);
	}

	srcSize = copyFile(srcFd, targetFd);
	close(srcFd);

	// 파일 크기를 섹터 크기인 512바이트로 맞추기 위해 나머지 부분을 0x00으로 채움
	bootSize = sectorSize(targetFd, srcSize);
	printf("[INFO] %s size = [%d] and sector count = [%d]\n", argv[1], srcSize, bootSize);

	//----------------------------------------------------------------
	// 32비트 커널 파일을 열어서 모든 내용을 디스크 이미지  파일로 복사
	//----------------------------------------------------------------
	printf("[INFO] Copy protected mode kernel to image file\n");
	if((srcFd = open(argv[2], O_RDONLY)) == -1) {
		fprintf(stderr, "[ERROR] %s open fail\n", argv[2]);
		exit(-1);
	}

	srcSize = copyFile(srcFd, targetFd);
	close(srcFd);

	lowSectorCnt = sectorSize(targetFd, srcSize);
	printf("[INFO] %s size = [%d] and sector count = [%d]\n", argv[2], srcSize, lowSectorCnt);

	//----------------------------------------------------------------
	// 64비트 커널 파일을 열어서 모든 내용을 디스크 이미지 파일로 복사
	//----------------------------------------------------------------
	printf("[INFO] Copy IA-32e mode kernel to image file\n");
	if((srcFd = open(argv[3], O_RDONLY)) == -1) {
		fprintf(stderr, "[ERROR] %s open fail\n", argv[3]);
		exit(-1);
	}

	srcSize = copyFile(srcFd, targetFd);
	close(srcFd);

	// 파일 크기를 섹터 크기인 512바이트로 맞추기 위해 나머지 부분을 0x000 으로 채움
	highSectorCnt = sectorSize(targetFd, srcSize);
	printf("[INFO] %s size = [%d] and sector count = [%d]\n", argv[3], srcSize, highSectorCnt);

	//----------------------------------------------------------------
	// 디스크 이미지에 커널 정보를 갱신
	//----------------------------------------------------------------
	printf("[INFO] Start to write kernel information\n");
	// 부트섹터의 5번째 바이트부터 커널에 대한 정보를 넣음
	writeInfo(targetFd, lowSectorCnt + highSectorCnt, lowSectorCnt);	// 170718 수정
	printf("[INFO] Image file create complete\n");

	close(targetFd);
	return 0;
}

// 현재 위치부터 512바이트 배수 위치까지 맞추어 0x00으로 채움
int sectorSize(int fd, int srcSize) {
	int i, sector_size, ch, sectorCnt;

	sector_size = srcSize % BYTESOFSECTOR;
	ch = 0x00;

	if(sector_size != 0) {
		sector_size = 512 - sector_size;
		printf("[INFO] File size [%u] and fill [%u] byte\n", srcSize, sector_size);
		for(i = 0; i < sector_size; i++) write(fd, &ch, 1);
	}
	else printf("[INFO] File size is aligned 512 byte\n");

	// 섹터 수를 되돌려줌
	sectorCnt = (srcSize + sector_size) / BYTESOFSECTOR;
	return sectorCnt;
}

// 부트 로더에 커널에 대한 정보를 삽입
void writeInfo(int targetFd, int totalSectorCnt, int lowSectorCnt) {
	unsigned short usData;
	long position;

	// 파일의 시작에서 5바이트 떨어진 위치가 커널의 총 섹터 수 정보를 나타냄
	position = lseek(targetFd, 5, SEEK_SET);
	if(position == -1) {
		fprintf(stderr, "lseek fail. Return value = %lu, errno = %d, %d\n", position, errno, SEEK_SET);
		exit(-1);
	}

	usData = (unsigned short) totalSectorCnt;
	write(targetFd, &usData, 2);
	usData = (unsigned short) lowSectorCnt;
	write(targetFd, &usData, 2);

	printf("[INFO] Total sector count except boot loader [%d]\n", totalSectorCnt);
	printf("[INFO] Total sector count of protected mode kernel [%d]\n", lowSectorCnt);
}

// 소스 파일(Source FD)의 내용을 목표 파일(Target FD)에 복사하고 그 크기를 되돌려줌
int copyFile(int srcFd, int targetFd) {
	int srcSize, r, w;
	char buf[BYTESOFSECTOR];

	srcSize = 0;
	while(1) {
		r = read(srcFd, buf, sizeof(buf));
		w = write(targetFd, buf, r);

		if(r != w) {
			fprintf(stderr, "[ERROR] iRead != iWrite.. \n");
			exit(-1);
		}
		srcSize += r;

		if(r != sizeof(buf)) break;
	}
	return srcSize;
}

