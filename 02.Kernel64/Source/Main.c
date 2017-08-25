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

void forAP(void);
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
		changeLED(FALSE, FALSE, FALSE);
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

	createTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD | TASK_FLAGS_SYSTEM | TASK_FLAGS_IDLE, 0, 0, (QWORD)idleTask);
	startShell();
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

/*
	// 1초마다 한 번씩 메시지 출력
	tickCnt = getTickCnt();
	while(1) if(getTickCnt() - tickCnt > 1000) {
		tickCnt = getTickCnt();
		printF("Application Processor[APIC ID: %d] is Activated\n", getAPICID());
	}
*/
	idleTask();
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
