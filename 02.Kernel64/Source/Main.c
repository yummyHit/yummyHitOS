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
#include <CLITask.h>
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
#include <WinManager.h>

void forAP(void);
BOOL multiCoreMode(void);
//void startGUI();

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
	if(*(BYTE*)VBE_GUIMODE_STARTADDR == 0) startShell();
	else startWinManager();
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
