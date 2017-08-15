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

// 아래 함수는 C언어 커널 시작 부분
void Main(void) {
	int x, y;

	// 콘솔 초기화 후 작업 수행
	initConsole(0, 5);

	printXY(57, 5, 0x1A, "[  Hit  ]");

	getCursor(&x, &y);	y++;
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
	setCursor(58, y++);
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
	}	y++;

	printXY(7, 7, 0x1F, "PIC Controller And Interrupt Initialize ..........");
	initPIC();
	maskPIC(0);
	onInterrupt();
	printXY(57, 7, 0x1A, "[  Hit  ]");

	printXY(7, 7, 0x1F, "File System Initialize ...........................");
	if(initFileSystem() == TRUE) printXY(57, 7, 0x1A, "[  Hit  ]");
	else printXY(57, 7, 0x1C, "[  Err  ]");

	printXY(12, 8, 0x1B, "### Welcome to YummyHitOS !! Please enjoy this !! ###");
	setCursor(0, ++y);

	createTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD | TASK_FLAGS_SYSTEM | TASK_FLAGS_IDLE, 0, 0, (QWORD)idleTask);
	startShell();
}
