/*
 * main.c
 *
 *  Created on: 2017. 7. 6.
 *      Author: Yummy
 */

#include "types.h"
#include "page.h"
#include "modeswitch.h"

void kSetPrint(int x, int y, BYTE color, const char *str);
BOOL kInitArea(void);
BOOL kIsMemEnough(void);
void kCopyImage(void);

#define BSP_FLAGADDR	0x7C09

// main 함수
void main(void) {
	DWORD i, eax, ebx, ecx, edx;
	char cpuMaker[13] = {0,};

	// Application Processor이면 아래 코드 생략하고 64비트 모드로 전환
	if(*((BYTE*)BSP_FLAGADDR) == 0) kSwitchNExecKernel();

	// 최소 메모리 크기를 만족하는 지 검사
	kSetPrint(7, 3, 0x1F, "Minimum Memory Size Check ........................");
	if(kIsMemEnough() == FALSE) {
		kSetPrint(57, 3, 0x1C, "[  Err  ]");
		kSetPrint(7, 4, 0x1C, "Not Enough Memory. YummyHitOS Requires Over 64Mbyte Memory!");
		while(1);
	} else kSetPrint(57, 3, 0x1A, "[  Hit  ]");

	// IA-32e 모드의 커널 영역을 초기화
	kSetPrint(7, 3, 0x1F, "IA-32e Kernel Area Initialize .....................");
	if(kInitArea() == FALSE) {
		kSetPrint(57, 3, 0x1C, "[  Err  ]");
		kSetPrint(7, 4, 0x1C, "Kernel Area Initialization Fail !!");
		while(1);
	} else kSetPrint(57, 3, 0x1A, "[  Hit  ]");

	// IA-32e 모드 커널을 위한 페이지 테이블 생성
	kSetPrint(7, 3, 0x1F, "IA-32e Page Tables Initialize ....................");
	kInitPageTbl();
	kSetPrint(57, 3, 0x1A, "[  Hit  ]");

	// 프로세서 제조사 정보 읽기
	kReadCPUID(0x00000000, &eax, &ebx, &ecx, &edx);
	*(DWORD*)cpuMaker = ebx;
	*((DWORD*)cpuMaker + 1) = edx;
	*((DWORD*)cpuMaker + 2) = ecx;
	kSetPrint(7, 3, 0x1F, "CPU maker check ..................................[              ]");
	kSetPrint(59, 3, 0x1C, cpuMaker);

	// 64비트 지원 유무 확인
	kReadCPUID(0x80000001, &eax, &ebx, &ecx, &edx);
	kSetPrint(7, 4, 0x1F, "64bit Mode Support Check .........................");
	if(edx & (1 << 29)) kSetPrint(57, 4, 0x1A, "[  Hit  ]");
	else {
		kSetPrint(57, 4, 0x1C, "[  Err  ]");
		kSetPrint(7, 5, 0x1C, "This processor doesn't support 64bit mode !!");
		while(1);
	}

	// IA-32e 모드 커널을 0x200000(2Mbyte) 어드레스로 이동
	kSetPrint(7, 5, 0x1F, "Copy IA-32e Kernel To 2M byte Address ............");
	kCopyImage();
	kSetPrint(57, 5, 0x1A, "[  Hit  ]");

	// IA-32e 모드로 전환
	kSetPrint(7, 5, 0x1F, "Switching from 32bit to 64bit ....................");
	kSetPrint(57, 5, 0x11, "         ");
	kSwitchNExecKernel();
}

// 문자열 출력 함수
void kSetPrint(int x, int y, BYTE color, const char *str) {
	CHARACTER *p = (CHARACTER*)0xB8000;
	int i;

	// X, Y좌표를 이용해 문자열 출력 어드레스 계산
	p += (y * 80) + x;

	// NULL이 나올때까지 문자열 출력
	for(i = 0; str[i] != 0; i++) {
		p[i].character = str[i];
		p[i].color = color;
	}
}

// IA-32e 모드용 커널 영역을 0으로 초기화
BOOL kInitArea(void) {
	DWORD *addr = (DWORD*)0x100000;

	// 초기화를 시작할 어드레스인 0x100000(1MB)부터 마지막 어드레스인 0x600000(6MB)까지 루프를 돌면서 4바이트씩 0으로 채움
	while((DWORD) addr < 0x600000) {
		*addr = 0x00;

		// 0으로 저장한 후 다시 읽었을 때 0이 나오지 않으면 해당 어드레스를 사용하는데 문제가 생긴 것으로 더이상 진행하지 않고 종료
		if(*addr != 0) return FALSE;

		// 다음 어드레스로 이동
		addr++;
	}
	return TRUE;
}

// OS를 실행하기에 충분한 메모리를 가지고 있는지 체크
BOOL kIsMemEnough(void) {
	DWORD *addr = (DWORD*)0x100000;

	// 0x100000(1MB)부터 0x4000000(64MB)까지 루프를 돌며 검사
	while((DWORD)addr < 0x4000000) {
		*addr = 0x11111111;

		// 0x12345678로 저장한 후 다시 읽었을 때 0x12345678이 나오지 않으면 해당 어드레스를 사용하는데 문제가 생긴 것이므로 더이상 진행하지 않고 종료
		if(*addr != 0x11111111) return FALSE;

		// 1MB씩 이동하면서 확인
		addr += (0x100000 / 4);
	}
	return TRUE;
}

// IA-32e 모드 커널을 0x200000(2Mbyte) 어드레스에 복사
void kCopyImage(void) {
	int i;

	// 0x7C05에 총 커널 섹터 수, 0x7C07에 보호 모드 커널 섹터 수가 들어있음
	WORD totalSectorCnt = *((WORD*) 0x7C05);
	WORD sectorCnt = *((WORD*) 0x7C07);
	DWORD *srcAddr = (DWORD*)(0x10000 + (sectorCnt * 512));
	DWORD *dstAddr = (DWORD*)0x200000;

	// IA-32e 모드 커널 섹터 크기만큼 복사
	for(i = 0; i < 512 * (totalSectorCnt - sectorCnt) / 4; i++) {
		*dstAddr = *srcAddr;
		dstAddr++;
		srcAddr++;
	}
}
