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

void forAP(void);
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
	printF("Application Processor[APIC ID: %d] is Activated\n", getAPICID());

	// 유휴 태스크 실행
	idleTask();
}

// x를 절대값으로 변환하는 매크로
#define ABS(x)	(((x) >= 0) ? (x) : -(x))

// 임의의 X, Y좌표 반환
void getRandXY(int *x, int *y) {
	int rand;

	// X좌표 계산
	rand = _rand();
	*x = ABS(rand) % 1000;

	// Y좌표 계산
	rand = _rand();
	*y = ABS(rand) % 700;
}

// 임의의 색 반환
COLOR getRandColor(void) {
	int r, g, b, rand;

	rand = _rand();
	r = ABS(rand) % 256;

	rand = _rand();
	g = ABS(rand) % 256;

	rand = _rand();
	b = ABS(rand) % 256;

	return RGB(r, g, b);
}

// 윈도우 프레임 그림
void drawWinFrame(int x, int y, int width, int height, const char *title) {
	char *str1 = "### This is YummyHitOS's GUI Testing Version ###";
	char *str2 = "###       YummyHitOs'll be back soon !!      ###";

	// 윈도우 프레임의 가장자리를 그림. 2픽셀 두께
	drawRect(x, y, x + width, y + height, RGB(109, 218, 22), FALSE);
	drawRect(x + 1, y + 1, x + width - 1, y + height - 1, RGB(109, 218, 22), FALSE);

	// 제목 표시줄 채움
	drawRect(x, y + 3, x + width - 1, y + 21, RGB(102, 0, 255), TRUE);

	// 윈도우 제목 표시
	drawText(x + 6, y + 3, RGB(255, 255, 255), RGB(102, 0, 255), title, strLen(title));

	// 제목 표시줄을 입체로 보이게 위쪽의 선 그림. 2픽셀 두께
	drawLine(x + 1, y + 1, x + width - 1, y + 1, RGB(102, 102, 255));
	drawLine(x + 1, y + 2, x + width - 1, y + 2, RGB(102, 102, 204));

	drawLine(x + 1, y + 2, x + 1, y + 20, RGB(102, 102, 255));
	drawLine(x + 2, y + 2, x + 2, y + 20, RGB(102, 102, 204));

	// 제목 표시줄의 아래쪽에 선 그림
	drawLine(x + 2, y + 19, x + width - 2, y + 19, RGB(85, 255, 85));
	drawLine(x + 2, y + 20, x + width - 2, y + 20, RGB(85, 255, 85));

	// 닫기 버튼을 그림. 오른쪽 상단에 표시
	drawRect(x + width - 2 - 18, y + 1, x + width - 2, y + 19, RGB(255, 255, 255), TRUE);

	// 닫기 버튼을 입체로 보이게 선 그림. 2픽셀 두께
	drawRect(x + width - 2, y + 1, x + width - 2, y + 19 - 1, RGB(86, 86, 86), TRUE);
	drawRect(x + width - 2 -  1, y + 1, x + width - 2 - 1, y + 19 - 1, RGB(86, 86, 86), TRUE);
	drawRect(x + width - 2 - 18 + 1, y + 19, x + width - 2, y + 19, RGB(86, 86, 86), TRUE);
	drawRect(x + width - 2 - 18 + 1, y + 19 - 1, x + width - 2, y + 19 - 1, RGB(86, 86, 86), TRUE);

	drawLine(x + width - 2 - 18, y + 1, x + width - 2 - 1, y + 1, RGB(229, 229, 229));
	drawLine(x + width - 2 - 18, y + 1 + 1, x + width - 2 - 2, y + 1 + 1, RGB(229, 229, 229));
	drawLine(x + width - 2 - 18, y + 1, x + width - 2 - 18, y + 19, RGB(229, 229, 229));
	drawLine(x + width - 2 - 18 + 1, y + 1, x + width - 2 - 18 + 1, y + 19 - 1, RGB(229, 229, 229));

	// 대각선 X 그림. 3픽셀
	drawLine(x + width - 2 - 18 + 4, y + 1 + 4, x + width - 2 - 4, y + 19 - 4, RGB(102, 0, 255));
	drawLine(x + width - 2 - 18 + 5, y + 1 + 4, x + width - 2 - 4, y + 19 - 5, RGB(102, 0, 255));
	drawLine(x + width - 2 - 18 + 4, y + 1 + 5, x + width - 2 - 5, y + 19 - 4, RGB(102, 0, 255));
	drawLine(x + width - 2 - 18 + 4, y + 19 - 4, x + width - 2 - 4, y + 1 + 4, RGB(102, 0, 255));
	drawLine(x + width - 2 - 18 + 5, y + 19 - 4, x + width - 2 - 4, y + 1 + 5, RGB(102, 0, 255));
	drawLine(x + width - 2 - 18 + 4, y + 19 - 5, x + width - 2 - 5, y + 1 + 4, RGB(102, 0, 255));

	// 내부 그림
	drawRect(x + 2, y + 21, x + width - 2, y + height - 2, RGB(255, 255, 255), TRUE);

	// 문자 출력
	drawText(x + 10, y + 30, RGB(102, 0, 204), RGB(255, 255, 255), str1, strLen(str1));
	drawText(x + 10, y + 50, RGB(119, 68, 255), RGB(255, 255, 255), str2, strLen(str2));
}

// Graphic Mode Test
void startGUI() {
	VBEMODEINFO *mode;
	int x1, y1, x2, y2, i;
	COLOR color1, color2;
	char *str[] = {"Pixel", "Line", "Rectangle", "Circle", "### YummyHitOS ###"};

	// 점, 선, 사각형, 원, 문자 간단히 출력
	// (0, 0)에 Pixel이란 문자열과 픽셀을 검은바탕 흰색 출력
	drawText(0, 0, RGB(255, 255, 255), RGB(0, 0, 0), str[0], strLen(str[0]));
	drawPixel(1, 20, RGB(255, 255, 255));
	drawPixel(2, 20, RGB(255, 255, 255));

	// (0, 25)에 Line이란 문자열과 라인을 검은바탕 노란색 출력
	drawText(0, 25, RGB(255, 255, 80), RGB(0, 0, 0), str[1], strLen(str[1]));
	drawLine(20, 50, 1000, 50, RGB(255, 255, 80));
	drawLine(20, 50, 1000, 100, RGB(255, 255, 80));
	drawLine(20, 50, 1000, 150, RGB(255, 255, 80));
	drawLine(20, 50, 1000, 200, RGB(255, 255, 80));
	drawLine(20, 50, 1000, 250, RGB(255, 255, 80));

	// (0, 180)에 Rectangle이란 문자열과 사각형을 검은바탕 녹색 출력
	drawText(0, 180, RGB(85, 255, 85), RGB(0, 0, 0), str[2], strLen(str[2]));
	drawRect(20, 200, 70, 250, RGB(85, 255, 85), FALSE);
	drawRect(120, 200, 220, 300, RGB(85, 255, 85), TRUE);
	drawRect(270, 200, 420, 350, RGB(85, 255, 85), FALSE);
	drawRect(470, 200, 670, 400, RGB(85, 255, 85), TRUE);

	// (0, 550)에 Circle이란 문자열과 원을 검은바탕 보라색 출력
	drawText(0, 550, RGB(102, 0, 255), RGB(0, 0, 0), str[3], strLen(str[3]));
	drawCircle(45, 600, 25, RGB(102, 0, 255), FALSE);
	drawCircle(170, 600, 50, RGB(102, 0, 255), TRUE);
	drawCircle(345, 600, 75, RGB(102, 0, 255), FALSE);
	drawCircle(570, 600, 100, RGB(102, 0, 255), TRUE);

	// 키 입력 대기
	getCh();

	// 점, 선, 사각형, 원, 문자 무작위 출력
	do {
		// 점 그리기
		for(i = 0; i < 100; i++) {
			// 임의의 X좌표와 색 반환
			getRandXY(&x1, &y1);
			color1 = getRandColor();

			drawPixel(x1, y1, color1);
		}

		// 선 그리기
		for(i = 0; i < 100; i++) {
			// 임의의 X좌표와 색 반환
			getRandXY(&x1, &y1);
			getRandXY(&x2, &y2);
			color1 = getRandColor();

			drawLine(x1, y1, x2, y2, color1);
		}

		// 사각형 그리기
		for(i = 0; i < 20; i++) {
			// 임의의 X좌표와 색 반환
			getRandXY(&x1, &y1);
			getRandXY(&x2, &y2);
			color1 = getRandColor();

			drawRect(x1, y1, x2, y2, color1, _rand() % 2);
		}

		// 원 그리기
		for(i = 0; i < 100; i ++) {
			// 임의의 X좌표와 색 반환
			getRandXY(&x1, &y1);
			color1 = getRandColor();

			drawCircle(x1, y1, ABS(_rand() % 50 + 1), color1, _rand() % 2);
		}

		// 텍스트 표시
		for(i = 0; i < 100; i ++) {
			// 임의의 X좌표와 색 반환
			getRandXY(&x1, &y1);
			color1 = getRandColor();
			color2 = getRandColor();

			// 텍스트 출력
			drawText(x1, y1, color1, color2, str[4], strLen(str[4]));
		}
	} while(getCh() != 'q');

	// 윈도우 프레임 출력
	while(1) {
		// 배경 출력
		drawRect(0, 0, 1024, 768, RGB(183, 147, 255), TRUE);

		// 윈도우 프레임 3개 그림
		for(i = 0; i < 3; i++) {
			getRandXY(&x1, &y1);
			drawWinFrame(x1, y1, 400, 200, "First GUI Frame");
		}

		getCh();
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
