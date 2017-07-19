/*
 * Main.c
 *
 *  Created on: 2017. 7. 6.
 *      Author: Yummy
 */


#include "Types.h"
#include "Page.h"
#include "ModeSwitch.h"

void setPrint(int x, int y, BYTE color, const char *str);
BOOL initializeArea(void);
BOOL isMemEnough(void);
void copyImageTo2Mbyte(void);

#define BOOTSTRAPPROCESSOR_FLAGADDRESS  0x7C09

// Main 함수
void Main( void ) {
	DWORD i;
	DWORD eax, ebx, ecx, edx;
	char cpuMaker[13] = {0,};

	if(*((BYTE*)BOOTSTRAPPROCESSOR_FLAGADDRESS) == 0) {
		SwitchNExecKernel();
		while(1) ;
	}

	// 최소 메모리 크기를 만족하는 지 검사
	setPrint(3, 5, 0x0F, "Minimum Memory Size Check.........................");
	if(isMemEnough() == FALSE) {
		setPrint(53, 5, 0x0C, "[  Err  ]");
		setPrint(3, 6, 0x0C, "Not Enough Memory. MINT64 OS Requires Over 64Mbyte Memory!");
		while(1);
	} else setPrint(53, 5, 0x0A, "[  Hit  ]");

	// IA-32e 모드의 커널 영역을 초기화
	setPrint(3, 6, 0x0F, "IA-32 Kernel Area Initialize......................");
	if(initializeArea() == FALSE) {
		setPrint(53, 6, 0x0C, "[  Err  ]");
		setPrint(3, 7, 0x0C, "Kernel Area Initialization Fail!!");
		while(1);
	} else setPrint(53, 6, 0x0A, "[  Hit  ]");

	// IA-32e 모드 커널을 위한 페이지 테이블 생성
	setPrint(3, 7, 0x0F, "IA-32e Page Tables Initialize.....................");
	initializePageTables();
	setPrint(53, 7, 0x0A, "[  Hit  ]");

	// 프로세서 제조사 정보 읽기
	ReadCPUID(0x00000000, &eax, &ebx, &ecx, &edx);
	*(DWORD*)cpuMaker = ebx;
	*((DWORD*)cpuMaker + 1) = edx;
	*((DWORD*)cpuMaker + 2) = ecx;
	setPrint(3, 8, 0x0F, "CPU maker check...................................[              ]");
	setPrint(55, 8, 0x0C, cpuMaker);

	// 64비트 지원 유무 확인
	ReadCPUID(0x80000001, &eax, &ebx, &ecx, &edx);
	setPrint(3, 9, 0x0F, "64bit Mode Support Check..........................");
	if(edx & ( 1 << 29 )) setPrint(53, 9, 0x0A, "[  Hit  ]");
	else {
		setPrint(53, 9, 0x0C, "[  Err  ]");
		setPrint(3, 10, 0x0C, "This processor doesn't support 64bit mode!!");
		while(1);
	}

	// IA-32e 모드 커널을 0x200000(2Mbyte) 어드레스로 이동
	setPrint(3, 10, 0x0F, "Copy IA-32e Kernel To 2M byte Address.............");
	copyImageTo2Mbyte();
	setPrint(53, 10, 0x0A, "[  Hit  ]");

	// IA-32e 모드로 전환
	setPrint(3, 11, 0x0F, "Switch To IA-32e Mode.............................");
	SwitchNExecKernel();

	while(1);
}

// 문자열 출력 함수
void setPrint(int x, int y, BYTE color, const char *str) {
	CHARACTER *p = (CHARACTER*) 0xB8000;
	int i;

	// X, Y좌표를 이용해 문자열 출력 어드레스 계산
	p += (y * 80) + x;

	// NULL이 나올때까지 문자열 출력
	for(i = 0; str[i] != 0; i++ ) {
		p[i].character = str[i];
		p[i].color = color;
	}
}

// IA-32e 모드용 커널 영역을 0으로 초기화
BOOL initializeArea(void) {
	DWORD *addr;

	// 초기화를 시작할 어드레스인 0x100000(1MB)을 설정
	addr = (DWORD*) 0x100000;

	// 마지막 어드레스인 0x600000(6MB)까지 루프를 돌면서 4바이트씩 0으로 채움
	while((DWORD) addr < 0x600000) {
		*addr = 0x00;

		// 0으로 저장한 후 다시 읽었을 때 0이 나오지 않으면 해당 어드레스를
		// 사용하는데 문제가 생긴 것으로 더이상 진행하지 않고 종료
		if(*addr != 0) return FALSE;

		// 다음 어드레스로 이동
		addr++;
	}

	return TRUE;
}

// MINT64 OS를 실행하기에 충분한 메모리를 가지고 있는지 체크
BOOL isMemEnough(void) {
	DWORD *addr;

	// 0x100000(1MB)부터 검사 시작
	addr = (DWORD*) 0x100000;

	// 0x4000000(64MB)까지 루프를 돌며 확인
	while((DWORD)addr < 0x4000000) {
		*addr = 0x12345678;

		// 0x12345678로 저장한 후 다시 읽었을 때 0x12345678이 나오지 않으면 해당
		// 어드레스를 사용하는데 문제가 생긴 것이므로 더이상 진행하지 않고 종료
		if(*addr != 0x12345678) return FALSE;

		// 1MB씩 이동하면서 확인
		addr += (0x100000 / 4);
	}

	return TRUE;
}

// IA-32e 모드 커널을 0x200000(2Mbyte) 어드레스에 복사
void copyImageTo2Mbyte(void) {
	WORD sectorCnt, totalSectorCnt;
	DWORD *srcAddr, *dstAddr;
	int i;

	// 0x7C05에 총 커널 섹터 수, 0x7C07에 보호 모드 커널 섹터 수가 들어있음
	totalSectorCnt = *((WORD*) 0x7C05);
	sectorCnt = *((WORD*) 0x7C07);

	srcAddr = (DWORD*)(0x10000 + (sectorCnt * 512));
	dstAddr = (DWORD*)0x200000;

	// IA-32e 모드 커널 섹터 크기만큼 복사
	for(i = 0; i < 512 * (totalSectorCnt - sectorCnt) / 4; i++) {
		*dstAddr = *srcAddr;
		dstAddr++;
		srcAddr++;
	}
}

