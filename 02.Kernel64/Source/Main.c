/*
 * Main.c
 *
 *  Created on: 2017. 7. 18.
 *      Author: Yummy
 */

#include <Types.h>
#include <Keyboard.h>
#include <Descriptor.h>
#include <PIC.h>
#include <PIT.h>
#include <Console.h>
#include <Shell.h>
#include <Task.h>
#include <DynMem.h>
#include <HardDisk.h>
#include <FileSystem.h>
#include <SerialPort.h>
#include <MPConfig.h>
#include <LocalAPIC.h>
#include <MP.h>
#include <IOAPIC.h>
#include <VBE.h>
#include <BaseGraph.h>
#include <Mouse.h>

void forAP(void);
BOOL multiCoreMode(void);
void startGUI();
void yummy_ascii_art(const char *buf);

// BSP용 C언어 커널 엔트리 포인트, 아래 함수는 C언어 커널 시작 부분
void Main(void) {
	int x, y;

	// 부트로더에 있는 BSP 플래그를 읽어 Application Processor이면 해당 코어용 초기화 함수로 이동
	if(*((BYTE*)BSP_FLAGADDR) == 0) forAP();

	// Bootstrap Processor가 부팅을 완료했으므로 0x7C09에 있는 BSP 플래그를 0으로 설정해 AP용으로 코드 실행 경로 변경
	*((BYTE*)BSP_FLAGADDR) = 0;

	// 콘솔 초기화 후 작업 수행
	initConsole(0, 5);

	printXY(57, 5, 0x1A, "[  Hit  ]");

	getCursor(&x, &y);
	printXY(7, 6, 0x1F, "GDT Initialize And Switch For IA-32e Mode ........");
	initGDTNTSS();
	loadGDTR(GDTR_STARTADDR);
	printXY(57, 6, 0x1A, "[  Hit  ]");

	printXY(7, 6, 0x1F, "TSS Segment Load .................................");
	loadTSS(GDT_TSSSEGMENT);
	printXY(57, 6, 0x1A, "[  Hit  ]");

	printXY(7, 6, 0x1F, "IDT Initialize ...................................");
	initIDT();
	loadIDTR(IDTR_STARTADDR);
	printXY(57, 6, 0x1A, "[  Hit  ]");

	printXY(7, 6, 0x1F, "Memory Size Check ................................[     MB]");
	chkTotalMemSize();
	setCursor(58, ++y);
	printF("%d", getTotalMemSize());

	printXY(7, 7, 0x1F, "TCB Pool And Scheduler Initialize ................");
	initScheduler();

	printXY(7, 7, 0x1F, "Dynamic Memory Initialize ........................");
	initDynMem();

	initPIT(MSTOCNT(1),1);	// 1ms 당 한 번씩 인터럽트 발생
	printXY(57, 7, 0x1A, "[  Hit  ]");

	printXY(7, 7, 0x1F, "Keyboard Activate And Queue Initialize ...........");
	if(initKeyboard() == TRUE) {
		printXY(57, 7, 0x1A, "[  Hit  ]");
		alterLED(FALSE, FALSE, FALSE);
	} else {
		printXY(57, 7, 0x1C, "[  Err  ]");
		printXY(7, 8, 0x1C, "Keyboard is Not active ! Check your keyboard port ...");
		while(1);
	}

	printXY(7, 7, 0x1F, "Mouse Activate And Queue Initialize ..............");
	if(initMouse() == TRUE) {
		onMouseInterrupt();
		printXY(57, 7, 0x1A, "[  Hit  ]");
	} else {
		printXY(57, 7, 0x1C, "[  Err  ]");
		printXY(7, 8, 0x1C, "Mouse is Not active ! Check your mouse port ...");
		while(1);
	}

	printXY(7, 7, 0x1F, "PIC Controller And Interrupt Initialize ..........");
	initPIC();
	maskPIC(0);
	onInterrupt();
	printXY(57, 7, 0x1A, "[  Hit  ]");

	printXY(7, 7, 0x1F, "File System Initialize ...........................");
	if(initFileSystem() == TRUE) printXY(57, 7, 0x1A, "[  Hit  ]");
	else printXY(57, 7, 0x1C, "[  Err  ]");

	printXY(7, 8, 0x1F, "Serial Port Initialize ...........................");
	initSerial();
	printXY(57, 8, 0x1A, "[  Hit  ]");

	// 멀티코어 프로세서 모드로 전환. Application Processor 활성화, I/O 모드 활성화, 인터럽트 및 태스크 부하 분산 기능 활성화
	printXY(7, 8, 0x1F, "Change To MultiCore Processor Mode ...............");
	if(multiCoreMode() == TRUE) printXY(57, 8, 0x1A, "[  Hit  ]");
	else printXY(57, 8, 0x1C, "[  Err  ]");

	clearMonitor();
	setCursor(0, 1);
	yummy_ascii_art("yummy_ascii.txt");

	printXY(12, 9, 0x1B, "### Welcome to YummyHitOS !! Please enjoy this !! ###");
	y += 4;
	setCursor(0, y);

	createTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD | TASK_FLAGS_SYSTEM | TASK_FLAGS_IDLE, 0, 0, (QWORD)idleTask, getAPICID());

	// 그래픽 모드가 아니면 콘솔 셸 실행, 그래픽 모드면 그래픽 모드 실행
	if(*(BYTE*)VBE_GRAPHICMODE_STARTADDR == 0) startShell();
	else startGUI();
}

// Application Processor용 C언어 커널 엔트리 포인트. 코어에 설정하는 작업만 수행
void forAP(void) {
	QWORD tickCnt;

	// GDT 테이블 설정
	loadGDTR(GDTR_STARTADDR);

	// TSS 디스크립터 설정. APIC ID를 이용해 TSS 디스크립터 할당
	loadTSS(GDT_TSSSEGMENT + (getAPICID() * sizeof(GDTENTRY16)));

	// IDT 테이블 설정
	loadIDTR(IDTR_STARTADDR);

	// 스케줄러 초기화
	initScheduler();

	// 현재 코어의 로컬 APIC 활성화
	onSWLocalAPIC();

	// 모든 인터럽트를 수신할 수 있도록 태스크 우선순위 레지스터 0으로 설정
	setTaskPriority(0);

	// 로컬 APIC 로컬 벡터 테이블 초기화
	initLocalVecTbl();

	// 인터럽트 활성화
	onInterrupt();

	// 대칭 IO 모드 테스트를 위해 AP가 시작된 후 한 번만 출력
	//printF("Application Processor[APIC ID: %d] is Activated\n", getAPICID());

	// 유휴 태스크 실행
	idleTask();
}

// 멀티코어 프로세서 또는 멀티프로세서 모드로 전환하는 함수
BOOL multiCoreMode(void) {
	MPCONFIGMANAGER *manager;
	BOOL interruptFlag;
	int i;

	// Application Processor 활성화
	if(startUpAP() == FALSE) return FALSE;

	// 대칭 IO 모드 전환. MP 설정 매니저를 찾아 PIC 모드인가 확인
	manager = getMPConfigManager();
	if(manager->usePICMode == TRUE) {
		// PIC 모드이면 IO 포트 어드레스 0x22에 0x70을 먼저 전송, IO 포트 어드레스 0x23에 0x01을 전송하는 방법으로 IMCR 레지스터에 접근해 PIC 모드 비활성화
		outByte(0x22, 0x70);
		outByte(0x23, 0x01);
	}

	// PIC 컨트롤러의 인터럽트를 모두 마스크해 인터럽트 발생할 수 없도록 함
	maskPIC(0xFFFF);

	// 프로세서 전체 로컬 APIC 활성화
	onLocalAPIC();

	// 현재 코어 로컬 APIC 활성화
	onSWLocalAPIC();

	// 인터럽트 불가로 설정
	interruptFlag = setInterruptFlag(FALSE);

	// 모든 인터럽트 수신할 수 있도록 태스크 우선순위 레지스터 0으로 설정
	setTaskPriority(0);

	// 로컬 APIC의 로컬 벡터 테이블 초기화
	initLocalVecTbl();

	// 대칭 IO 모드로 변경되었음을 설정
	setSymmetricIOMode(TRUE);

	// IO APIC 초기화
	initIORedirect();

	// 이전 인터럽트 플래그 복원
	setInterruptFlag(interruptFlag);

	// 인터럽트 부하 분산 기능 활성화
	setInterruptLoadBalancing(TRUE);

	// 태스크 부하 분산 기능 활성화
	for(i = 0; i < MAXPROCESSORCNT; i++) setTaskLoadBalancing(i, TRUE);

	return TRUE;
}

// 윈도우 프레임 그림
void drawWinFrame(int x, int y, int width, int height, const char *title) {
	char *str1 = "### This is YummyHitOS's GUI Testing Version ###";
	char *str2 = "###       YummyHitOs'll be back soon !!      ###";
	VBEMODEINFO *mode;
	COLOR *mem;
	RECT mon;

	// VBE 모드 정보 블록 반환
	mode = getVBEModeInfo();

	// 화면 영역 설정
	mon.x1 = 0;
	mon.y1 = 0;
	mon.x2 = mode->xPixel - 1;
	mon.y2 = mode->yPixel - 1;

	// 그래픽 메모리 어드레스 설정
	mem = (COLOR*)mode->linearBaseAddr;

	// 윈도우 프레임의 가장자리를 그림. 2픽셀 두께
	inDrawRect(&mon, mem, x, y, x + width, y + height, RGB(109, 218, 22), FALSE);
	inDrawRect(&mon, mem, x + 1, y + 1, x + width - 1, y + height - 1, RGB(109, 218, 22), FALSE);

	// 제목 표시줄 채움
	inDrawRect(&mon, mem, x, y + 3, x + width - 1, y + 21, RGB(102, 0, 255), TRUE);

	// 윈도우 제목 표시
	inDrawText(&mon, mem, x + 6, y + 3, RGB(255, 255, 255), RGB(102, 0, 255), title, strLen(title));

	// 제목 표시줄을 입체로 보이게 위쪽의 선 그림. 2픽셀 두께
	inDrawLine(&mon, mem, x + 1, y + 1, x + width - 1, y + 1, RGB(102, 102, 255));
	inDrawLine(&mon, mem, x + 1, y + 2, x + width - 1, y + 2, RGB(102, 102, 204));

	inDrawLine(&mon, mem, x + 1, y + 2, x + 1, y + 20, RGB(102, 102, 255));
	inDrawLine(&mon, mem, x + 2, y + 2, x + 2, y + 20, RGB(102, 102, 204));

	// 제목 표시줄의 아래쪽에 선 그림
	inDrawLine(&mon, mem, x + 2, y + 19, x + width - 2, y + 19, RGB(85, 255, 85));
	inDrawLine(&mon, mem, x + 2, y + 20, x + width - 2, y + 20, RGB(85, 255, 85));

	// 닫기 버튼을 그림. 오른쪽 상단에 표시
	inDrawRect(&mon, mem, x + width - 2 - 18, y + 1, x + width - 2, y + 19, RGB(255, 255, 255), TRUE);

	// 닫기 버튼을 입체로 보이게 선 그림. 2픽셀 두께
	inDrawRect(&mon, mem, x + width - 2, y + 1, x + width - 2, y + 19 - 1, RGB(86, 86, 86), TRUE);
	inDrawRect(&mon, mem, x + width - 2 -  1, y + 1, x + width - 2 - 1, y + 19 - 1, RGB(86, 86, 86), TRUE);
	inDrawRect(&mon, mem, x + width - 2 - 18 + 1, y + 19, x + width - 2, y + 19, RGB(86, 86, 86), TRUE);
	inDrawRect(&mon, mem, x + width - 2 - 18 + 1, y + 19 - 1, x + width - 2, y + 19 - 1, RGB(86, 86, 86), TRUE);

	inDrawLine(&mon, mem, x + width - 2 - 18, y + 1, x + width - 2 - 1, y + 1, RGB(229, 229, 229));
	inDrawLine(&mon, mem, x + width - 2 - 18, y + 1 + 1, x + width - 2 - 2, y + 1 + 1, RGB(229, 229, 229));
	inDrawLine(&mon, mem, x + width - 2 - 18, y + 1, x + width - 2 - 18, y + 19, RGB(229, 229, 229));
	inDrawLine(&mon, mem, x + width - 2 - 18 + 1, y + 1, x + width - 2 - 18 + 1, y + 19 - 1, RGB(229, 229, 229));

	// 대각선 X 그림. 3픽셀
	inDrawLine(&mon, mem, x + width - 2 - 18 + 4, y + 1 + 4, x + width - 2 - 4, y + 19 - 4, RGB(102, 0, 255));
	inDrawLine(&mon, mem, x + width - 2 - 18 + 5, y + 1 + 4, x + width - 2 - 4, y + 19 - 5, RGB(102, 0, 255));
	inDrawLine(&mon, mem, x + width - 2 - 18 + 4, y + 1 + 5, x + width - 2 - 5, y + 19 - 4, RGB(102, 0, 255));
	inDrawLine(&mon, mem, x + width - 2 - 18 + 4, y + 19 - 4, x + width - 2 - 4, y + 1 + 4, RGB(102, 0, 255));
	inDrawLine(&mon, mem, x + width - 2 - 18 + 5, y + 19 - 4, x + width - 2 - 4, y + 1 + 5, RGB(102, 0, 255));
	inDrawLine(&mon, mem, x + width - 2 - 18 + 4, y + 19 - 5, x + width - 2 - 5, y + 1 + 4, RGB(102, 0, 255));

	// 내부 그림
	inDrawRect(&mon, mem, x + 2, y + 21, x + width - 2, y + height - 2, RGB(255, 255, 135), TRUE);

	// 문자 출력
	inDrawText(&mon, mem, x + 10, y + 30, RGB(102, 0, 204), RGB(255, 255, 135), str1, strLen(str1));
	inDrawText(&mon, mem, x + 10, y + 50, RGB(119, 68, 255), RGB(255, 255, 135), str2, strLen(str2));
}

// 마우스 너비와 높이
#define MOUSE_CURSOR_WIDTH	20
#define MOUSE_CURSOR_HEIGHT	20

// 마우스 커서 이미지 저장 데이터
BYTE gs_mouseBuf[MOUSE_CURSOR_WIDTH * MOUSE_CURSOR_HEIGHT] = {
1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
1, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
1, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 1, 2, 3, 3, 3, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 1, 2, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0,
0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 0, 0, 0, 0, 0,
0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 0, 0, 0, 0,
0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 0, 0,
0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2,
0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 0, 0, 0, 0, 0,
0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 2, 2, 0, 0, 0, 0, 0,
0, 0, 0, 0, 1, 2, 3, 3, 3, 2, 2, 3, 3, 3, 2, 2, 0, 0, 0, 0,
0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 1, 2, 3, 3, 2, 2, 2, 0, 0, 0,
0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 1, 2, 2, 2, 2, 2, 2, 0, 0,
0, 0, 0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 1, 2, 2, 2, 2, 2, 2, 0,
0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1,
0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 2, 2, 2, 1, 0,
0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 2, 1, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,};

// 커서 이미지 색
#define MOUSE_CURSOR_OUTLINE	RGB(102, 102, 255)
#define MOUSE_CURSOR_OUTER	RGB(102, 102, 204)
#define MOUSE_CURSOR_INNER	RGB(102, 0, 255)

// X, Y 위치에 마우스 커서 출력
void drawCursor(RECT *area, COLOR *mem, int x, int y) {
	int i, j;
	BYTE *position;

	// 커서 데이터 시작 위치 설정
	position = gs_mouseBuf;

	// 커서 너비와 높이만큼 루프를 돌며 픽셀을 화면에 출력
	for(j = 0; j < MOUSE_CURSOR_HEIGHT; j++) for(i = 0; i < MOUSE_CURSOR_WIDTH; i++) {
		switch(*position) {
		// 0은 출력하지 않음
		case 0:
			break;
		case 1:
			inDrawPixel(area, mem, i + x, j + y, MOUSE_CURSOR_OUTLINE);
			break;
		case 2:
			inDrawPixel(area, mem, i + x, j + y, MOUSE_CURSOR_OUTER);
			break;
		case 3:
			inDrawPixel(area, mem, i + x, j + y, MOUSE_CURSOR_INNER);
			break;
		}

		// 커서 픽셀이 표시됨에 따라 커서 데이터 위치도 같이 이동
		position++;
	}
}

// Graphic Mode Test
void startGUI() {
	VBEMODEINFO *mode;
	int x, y, mouseX, mouseY;
	COLOR *mem;
	RECT area;
	BYTE click;

	// VBE 모드 정보 블록 반환
	mode = getVBEModeInfo();

	// 화면 영역 설정
	area.x1 = 0;
	area.y1 = 0;
	area.x2 = mode->xPixel - 1;
	area.y2 = mode->yPixel - 1;

	// 그래픽 메모리 어드레스 설정
	mem = (COLOR*)mode->linearBaseAddr;

	// 마우스 초기 위치를 화면 가운데로 설정
	x = mode->xPixel / 2;
	y = mode->yPixel / 2;

	// 마우스 커서를 출력하고 마우스 이동 처리. 배경 출력
	inDrawRect(&area, mem, area.x1, area.y1, area.x2, area.y2, RGB(255, 255, 135), TRUE);

	// 현재 위치에 마우스 커서 출력
	drawCursor(&area, mem, x, y);

	while(1) {
		// 마우스 데이터가 수신되기를 기다림
		if(rmMouseData(&click, &mouseX, &mouseY) == FALSE) {
			_sleep(0);
			continue;
		}

		// 이전 마우스 커서가 있던 위치에 배경 출력
		inDrawRect(&area, mem, x, y, x + MOUSE_CURSOR_WIDTH, y + MOUSE_CURSOR_HEIGHT, RGB(255, 255, 135), TRUE);

		// 마우스가 움직인 거리를 이전 커서 위치에 더해 현재 좌표 계산
		x += mouseX;
		y += mouseY;

		// 마우스 커서가 화면을 벗어나지 못하도록 보정
		if(x < area.x1) x = area.x1;
		else if(x > area.x2) x = area.x2;
		if(y < area.y1) y = area.y1;
		else if(y > area.y2) y = area.y2;

		// 왼쪽 버튼이 눌러지면 윈도우 프레임 표시
		if(click & MOUSE_LCLICK) drawWinFrame(x - 200, y - 120, 400, 200, "Mouse GUI Frame");
		else if(click & MOUSE_RCLICK) inDrawRect(&area, mem, area.x1, area.y1, area.x2, area.y2, RGB(255, 255, 135), TRUE);

		// 변경된 마우스 위치에 마우스 커서 출력 이미지 출력
		drawCursor(&area, mem, x, y);
	}
}

void yummy_ascii_art(const char *buf) {
	FILE *fp;
	BYTE key;

	// 파일 생성
	fp = fopen(buf, "r");

	// 파일 끝까지 출력하는 것 반복
	while(1) {
		if(fread(&key, 1, 1, fp) != 1) break;
		printF("%c", key);
	}
	fclose(fp);
}
