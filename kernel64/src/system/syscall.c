/*
 * syscall.c
 *
 *  Created on: 2018. 6. 14.
 *      Author: Yummy
 */

#include <types.h>
#include <syscall.h>
#include <asmutil.h>
#include <descriptor.h>
#include <syslist.h>
#include <filesystem.h>
#include <dynmem.h>
#include <rtc.h>
#include <clitask.h>
#include <util.h>
#include <window.h>
#include <jpg.h>
#include <loader.h>

// 시스템 콜을 호출할 수 있도록 초기화
void kInitSysCall(void) {
	QWORD rdx, rax;

	// IA32_STAR MSR(0xC0000081) 설정. 상위 32bit : SYSRET CS/SS[63:48], CS/SS[47:32]
	rdx = ((GDT_KERNELDATA_SEGMENT | SELECTOR_RPL_3) << 16) | GDT_KERNELCODE_SEGMENT;
	// 하위 32bit : reserved
	rax = 0;
	kWriteMSR(0xC0000081, rdx, rax);

	// IA32_LSTAR MSR(0xC0000082) 설정. SysCallEntryPoint() 함수 주소로 지정
	kWriteMSR(0xC0000082, 0, (QWORD)kSysCallEntryPoint);

	// IA32_FMASK MSR(0xC0000084) 설정. RFLAGS 레지스터 값 변경하지 않으니 0x00 으로 유지
	kWriteMSR(0xC0000084, 0, 0x00);
}

// 시스템 콜 번호와 파라미터 자료구조를 이용해 시스템 콜 처리
QWORD kSysCallProc(QWORD num, PARAMTBL *param) {
	TCB *tcb;

	switch(num) {
		// 콘솔 IO 관련
		case SYSCALL_CSPRINT:
			return kCSPrint(PARAM(0));
		case SYSCALL_SETCURSOR:
			kSetCursor(PARAM(0), PARAM(1));
			return TRUE;
		case SYSCALL_GETCURSOR:
			kGetCursor((int*)PARAM(0), (int*)PARAM(1));
			return TRUE;
		case SYSCALL_CLEARMONITOR:
			kClearMonitor();
			return TRUE;
		case SYSCALL_GETCH:
			return kGetCh();

		// 동적 메모리 할당 및 해제 관련
		case SYSCALL_MALLOC:
			return (QWORD)kAllocMem(PARAM(0));
		case SYSCALL_FREE:
			return kFreeMem((void*)PARAM(0));

		// 파일 및 디렉터리 IO 관련
		case SYSCALL_FOPEN:
			return (QWORD)fopen((char*)PARAM(0), (char*)PARAM(1));
		case SYSCALL_FREAD:
			return fread((void*)PARAM(0), PARAM(1), PARAM(2), (FILE*)PARAM(3));
		case SYSCALL_FWRITE:
			return fwrite((void*)PARAM(0), PARAM(1), PARAM(2), (FILE*)PARAM(3));
		case SYSCALL_FSEEK:
			return fseek((FILE*)PARAM(0), PARAM(1), PARAM(2));
		case SYSCALL_FCLOSE:
			return fclose((FILE*)PARAM(0));
		case SYSCALL_FREMOVE:
			return fremove((char*)PARAM(0));
		case SYSCALL_DOPEN:
			return (QWORD)dopen((char*)PARAM(0));
		case SYSCALL_DREAD:
			return (QWORD)dread((DIR*)PARAM(0));
		case SYSCALL_DREWIND:
			drewind((DIR*)PARAM(0));
			return TRUE;
		case SYSCALL_DCLOSE:
			return dclose((DIR*)PARAM(0));
		case SYSCALL_ISFOPEN:
			return kIsFileOpen((DIRENTRY*)PARAM(0));

		// 하드 디스크 IO 관련
		case SYSCALL_READ_HDDSECTOR:
			return kReadHDDSector(PARAM(0), PARAM(1), PARAM(2), PARAM(3), (char*)PARAM(4));
		case SYSCALL_WRITE_HDDSECTOR:
			return kWriteHDDSector(PARAM(0), PARAM(1), PARAM(2), PARAM(3), (char*)PARAM(4));

		// 태스크 및 스케줄러 관련
		case SYSCALL_CREATETASK:
			tcb = kCreateTask(PARAM(0), (void*)PARAM(1), PARAM(2), PARAM(3), PARAM(4));
			if(tcb == NULL) return TASK_INVALID_ID;
			return tcb->link.id;
		case SYSCALL_SCHEDULE:
			return kSchedule();
		case SYSCALL_ALTERPRIORITY:
			return kAlterPriority(PARAM(0), PARAM(1));
		case SYSCALL_TASKFIN:
			return kTaskFin(PARAM(0));
		case SYSCALL_TASKEXIT:
			kTaskExit();
			return TRUE;
		case SYSCALL_GETTASKCNT:
			return kGetTaskCnt(PARAM(0));
		case SYSCALL_ISTASKEXIST:
			return kIsTaskExist(PARAM(0));
		case SYSCALL_GETPROCLOAD:
			return kGetProcLoad(PARAM(0));
		case SYSCALL_ALTERAFFINITY:
			return kAlterAffinity(PARAM(0), PARAM(1));
		case SYSCALL_EXECPROGRAM:
			return kExecFile((char*)PARAM(0), (char*)PARAM(1), (BYTE)PARAM(2));
		case SYSCALL_CREATETHREAD:
			return kCreateThread(PARAM(0), PARAM(1), (BYTE)PARAM(2), PARAM(3));

		// GUI 시스템 관련
		case SYSCALL_GETBACKGROUNDID:
			return kGetBackgroundID();
		case SYSCALL_GETMONAREA:
			kGetMonArea((RECT*)PARAM(0));
			return TRUE;
		case SYSCALL_CREATEWIN:
			return kCreateWin(PARAM(0), PARAM(1), PARAM(2), PARAM(3), PARAM(4), (char*)PARAM(5));
		case SYSCALL_DELWIN:
			return kDelWin(PARAM(0));
		case SYSCALL_SHOWWIN:
			return kShowWin(PARAM(0), PARAM(1));
		case SYSCALL_FINDWINPOINT:
			return kFindWinPoint(PARAM(0), PARAM(1));
		case SYSCALL_FINDWINTITLE:
			return kFindWinTitle((char*)PARAM(0));
		case SYSCALL_ISWINEXIST:
			return kIsWinExist(PARAM(0));
		case SYSCALL_GETTOPWIN:
			return kGetTopWin();
		case SYSCALL_MOVEWINTOP:
			return kMoveWinTop(PARAM(0));
		case SYSCALL_ISINTITLE:
			return kIsInTitle(PARAM(0), PARAM(1), PARAM(2));
		case SYSCALL_ISCLOSEBTN:
			return kIsCloseBtn(PARAM(0), PARAM(1), PARAM(2));
		case SYSCALL_MOVEWIN:
			return kMoveWin(PARAM(0), PARAM(1), PARAM(2));
		case SYSCALL_RESIZEWIN:
			return kResizeWin(PARAM(0), PARAM(1), PARAM(2), PARAM(3), PARAM(4));
		case SYSCALL_GETWINAREA:
			return kGetWinArea(PARAM(0), (RECT*)PARAM(1));
		case SYSCALL_UPDATEMONID:
			return kUpdateMonID(PARAM(0));
		case SYSCALL_UPDATEMONWINAREA:
			return kUpdateMonWinArea(PARAM(0), (RECT*)PARAM(1));
		case SYSCALL_UPDATEMONAREA:
			return kUpdateMonArea((RECT*)PARAM(0));
		case SYSCALL_EVENTTOWIN:
			return kEventToWin(PARAM(0), (EVENT*)PARAM(1));
		case SYSCALL_WINTOEVENT:
			return kWinToEvent(PARAM(0), (EVENT*)PARAM(1));
		case SYSCALL_DRAWWINFRAME:
			return kDrawWinFrame(PARAM(0));
		case SYSCALL_DRAWWINBACKGROUND:
			return kDrawWinBackground(PARAM(0));
		case SYSCALL_DRAWWINTITLE:
			return kDrawWinTitle(PARAM(0), (char*)PARAM(1), PARAM(2));
		case SYSCALL_DRAWBTN:
			return kDrawBtn(PARAM(0), (RECT*)PARAM(1), PARAM(2), (char*)PARAM(3), PARAM(4));
		case SYSCALL_DRAWPIXEL:
			return kDrawPixel(PARAM(0), PARAM(1), PARAM(2), PARAM(3));
		case SYSCALL_DRAWLINE:
			return kDrawLine(PARAM(0), PARAM(1), PARAM(2), PARAM(3), PARAM(4), PARAM(5));
		case SYSCALL_DRAWRECT:
			return kDrawRect(PARAM(0), PARAM(1), PARAM(2), PARAM(3), PARAM(4), PARAM(5), PARAM(6));
		case SYSCALL_DRAWCIRCLE:
			return kDrawCircle(PARAM(0), PARAM(1), PARAM(2), PARAM(3), PARAM(4), PARAM(5));
		case SYSCALL_DRAWTEXT:
			return kDrawText(PARAM(0), PARAM(1), PARAM(2), PARAM(3), PARAM(4), (char*)PARAM(5), PARAM(6));
		case SYSCALL_MOVECURSOR:
			kMoveCursor(PARAM(0), PARAM(1));
			return TRUE;
		case SYSCALL_GETWINCURSOR:
			kGetWinCursor((int*)PARAM(0), (int*)PARAM(1));
			return TRUE;
		case SYSCALL_BITBLT:
			return kBitBlt(PARAM(0), PARAM(1), PARAM(2), (COLOR*)PARAM(3), PARAM(4), PARAM(5));

		// JPEG 디코더 관련
		case SYSCALL_JPGINIT:
			return kJpgInit((JPEG*)PARAM(0), (char*)PARAM(1), PARAM(2));
		case SYSCALL_JPGDECODE:
			return kJpgDecode((JPEG*)PARAM(0), (COLOR*)PARAM(1));

		// RTC 관련
		case SYSCALL_READTIME:
			kReadTime((BYTE*)PARAM(0), (BYTE*)PARAM(1), (BYTE*)PARAM(2));
			return TRUE;
		case SYSCALL_READDATE:
			kReadDate((WORD*)PARAM(0), (BYTE*)PARAM(1), (BYTE*)PARAM(2), (BYTE*)PARAM(3));	
			return TRUE;

		// 시리얼 IO 관련
		case SYSCALL_SENDSERIALDATA:
			kSendSerialData((BYTE*)PARAM(0), PARAM(1));
			return TRUE;
		case SYSCALL_RECVSERIALDATA:
			return kRecvSerialData((BYTE*)PARAM(0), PARAM(1));
		case SYSCALL_CLEARSERIALFIFO:
			kClearSerialFIFO();
			return TRUE;

		// 유틸 관련
		case SYSCALL_GETTOTALMEMSIZE:
			return kGetTotalMemSize();
		case SYSCALL_GETTICKCNT:
			return kGetTickCnt();
		case SYSCALL_SLEEP:
			kSleep(PARAM(0));
			return TRUE;
		case SYSCALL_ISGUIMODE:
			return kIsGUIMode();
		case SYSCALL_TEST:
			kPrintf("\nSystem Call Test... System Call Success~!!\n");
			return TRUE;

		// 정의되지 않은 시스템 콜
		default:
			kPrintf("\nUndefined System Call!! Service Number: %d\n", num);
			return FALSE;
	}
}
