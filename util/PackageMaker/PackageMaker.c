/*
 * PackageMaker.c
 *
 *  Created on: 2019. 2. 23.
 *      Author: Yummy
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/uio.h>    // Mac에서는 sys/uio.h 타 플렛폼에서는 io.h일 수도 있음
#include <unistd.h>     // Unix 기반에서만 include
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

// 한 섹터 당 바이트 수
#define SECTOR_BYTES		512
// 패키지 시그니처
#define PACKAGE_SIGNATURE	"YUMMYHITOS_PACKAGE"
// 파일 이름 최대 길이. 커널의 FILESYSTEM_MAXFILENAMELENGTH 와 같음
#define FILENAME_LEN		24
#define DWORD				unsigned int

#pragma pack(push, 1)

// 패키지 헤더 내부 각 파일 정보 구성
typedef struct PackageItems {
	// 파일 명
	char name[FILENAME_LEN];

	// 파일 크기
	DWORD size;
} PACKAGEITEM;

// 패키지 헤더
typedef struct PackageHeader {
	// 시그니처
	char sign[16];

	// 패키지 헤더 전체 크기
	DWORD size;

	// 패키지 아이템 시작 위치
	PACKAGEITEM item[0];
} PACKAGEHEADER;

#pragma pack(pop)

int sectorPadding(int fd, int size);
int copyFile(int srcFd, int dstFd);

int main(int argc, char *argv[]) {
	int srcFd, dstFd, size, i;
	struct stat fileData;
	PACKAGEHEADER header;
	PACKAGEITEM item;

	if(argc < 2) {
		fprintf(stderr, "Usage: %s test1.elf test2.elf sample.txt ...\n", argv[0]);
		exit(-1);
	}

	// pack.img 파일 생성
	if((dstFd = open("pack.img", O_RDWR | O_CREAT | O_TRUNC)) == -1) {	// O_BINARY, S_IREAD, S_IWRITE 제외
		fprintf(stderr, "[Error] pack.img open failed.\n");
		exit(-1);
	}

	printf("[Info] Create package header...\n");

	// 시그니처 복사 후 헤더 크기 계산
	memcpy(header.sign, PACKAGE_SIGNATURE, sizeof(header.sign));
	header.size = sizeof(PACKAGEHEADER) + (argc - 1) * sizeof(PACKAGEITEM);
	// 파일에 저장
	if(write(dstFd, &header, sizeof(header)) != sizeof(header)) {
		fprintf(stderr, "[Error] Data write failed.\n");
		exit(-1);
	}

	// 인자 돌며 패키지 헤더 정보 채워 넣음
	for(i = 1; i < argc; i++) {
		// 파일 정보 확인
		if(stat(argv[i], &fileData) != 0) {
			fprintf(stderr, "[Error] %s file open failed.\n", argv[i]);	// %s 포맷에 일치하는 변수가 안들어감. 아마 argv[i] 일 듯.
			exit(-1);
		}

		// 파일 이름과 크기 저장
		memset(item.name, 0, sizeof(item.name));
		strncpy(item.name, argv[i], sizeof(item.name));
		item.name[sizeof(item.name) - 1] = '\0';
		item.size = fileData.st_size;

		// 파일에 씀
		if(write(dstFd, &item, sizeof(item)) != sizeof(item)) {
			fprintf(stderr, "[Error] Data write failed.\n");
			exit(-1);
		}
		printf("[%d] file: %s, size: %ld bytes\n", i, argv[i], fileData.st_size);
	}
	printf("[Info] Create complete!\n");

	printf("[Info] Copy data file to package...\n");
	// 인자 돌며 파일 채워 넣음
	for(i = 1, size = 0; i < argc; i++) {
		// 데이터 파일 열기
		if((srcFd = open(argv[i], O_RDONLY)) == -1) {		// O_BINARY 생략
			fprintf(stderr, "[Error] %s open failed\n", argv[i]);		// argv[1] 이 아니라 argv[i] 일 듯.
			exit(-1);
		}

		// 파일 내용을 패키지 파일에 쓴 후 파일 닫음
		size += copyFile(srcFd, dstFd);
		close(srcFd);
	}

	// 파일 크기를 섹터 크기인 512바이트로 맞추기 위해 나머지 부분 0x00 으로 채움
	sectorPadding(dstFd, size + header.size);

	printf("[Info] Total %d bytes copy complete!\n", size);
	printf("[Info] Package file create success!\n");

	close(dstFd);
	return 0;
}

// 현재 위치부터 512바이트 배수 위치까지 맞춰 0x00 으로 채움
int sectorPadding(int fd, int size) {	
	int i, sectorSize, cnt;
	char ch;

	sectorSize = size % SECTOR_BYTES;
	ch = 0x00;

	if(sectorSize != 0) {
		sectorSize = 512 - sectorSize;
		for(i = 0; i < sectorSize; i++) write(fd, &ch, 1);
	} else printf("[Info] File size is aligned 512 bytes\n");
	
	// 섹터 수 반환
	cnt = (size + sectorSize) / SECTOR_BYTES;
	return cnt;
}

// 소스 파일의 내용을 목표 파일에 복사 후 그 크기 반환
int copyFile(int srcFd, int dstFd) {
	int size, r, w;
	char buf[SECTOR_BYTES];

	size = 0;
	while(1) {
		r = read(srcFd, buf, sizeof(buf));
		w = write(dstFd, buf, r);

		if(r != w) {
			fprintf(stderr, "[Error] r != w..\n");
			exit(-1);
		}
		size += r;

		if(r != sizeof(buf)) break;
	}
	return size;
}
