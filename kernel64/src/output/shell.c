/*
 * shell.c
 *
 *  Created on: 2017. 7. 23.
 *      Author: Yummy
 */

#include <shell.h>
#include <console.h>
#include <keyboard.h>
#include <util.h>
#include <pit.h>
#include <rtc.h>
#include <asmutil.h>
#include <clitask.h>
#include <sync.h>
#include <dynmem.h>
#include <hdd.h>
#include <filesystem.h>
#include <rs232.h>
#include <mpconfig.h>
#include <localapic.h>
#include <mp.h>
#include <ioapic.h>
#include <interrupt.h>
#include <vbe.h>
#include <syscall.h>

// 커맨드 테이블 정의
SHELLENTRY gs_cmdTable[] = {
	{"help", "### Show Commands ###", kCSHelp},
	{"clear", "### Clear mon ###", kCSClear},
	{"tot_free", "### Show your total memory ###", kCSFree},
	{"strConvert", "### String To Number(Decimal or HexaDecimal) ###", kCSStrConvert},
	{"exit", "### EXIT ###", kCSExit},
	{"shutdown", "### Shutdown ###", kCSHalt},
	{"reboot", "### Reboot ###", kCSReboot},
	{"setTime", "### Set Timer. ex)setTime 10(ms) 1(term) ###", kCSSetTime},
	{"wait", "### Wait a few millisecond. ex)wait 100(ms) ###", kCSWait},
	{"rdtsc", "### Read Time Stamp Counter ###", kCSRTSC},
	{"cpuSpeed", "### Measure Processor Speed ###", kCSCPUSpeed},
	{"date", "### Show Date & Time ###", kCSDate},
	{"createTask", "### Create Task. ex)createTask 1(type) 10(count) ###", kCSCreateTask},
	{"changePriority", "### Change Task Priority. ex)changePriority 1(ID) 2(Priority) ###", kCSChangePriority},
	{"tasklist", "### Show Task List ###", kCSTaskList},
	{"kill", "### Task Kill. ex)kill 1(ID) or 0xffffffff(ALL Task) ###", kCSTaskill},
	{"cpuload", "### Show Processor Load ###", kCSCPULoad},
	{"mutexTest", "### Mutex Test Command ###", kCSMutexTest},
	{"threadTest", "### Thread And Process Test ###", kCSThreadTest},
	{"matrix", "### Show Matrix on your Monitor ###", kCSMatrix},
	{"getPIE", "### Get PIE(3.14) Calculation ###", kCSGetPIE},
	{"dynamicInfo", "### Show Dynamic Memory Information ###", kCSDynMemInfo},
	{"seqAlloc", "### Sequential Allocation & Free Test ###", kCSSeqAllocTest},
	{"randAlloc", "### Random Allocation & Free Test ###", kCSRandAllocTest},
	{"HDDInfo", "### Show HDD Information ###", kCSHDDInfo},
	{"readSector", "### Read HDD Sector, ex)readSector 0(LBA) 10(Count) ###", kCSReadSector},
	{"writeSector", "### Write HDD Sector, ex)writeSector 0(LBA) 10(Count) ###", kCSWriteSector},
	{"HDDMount", "### Mount HDD ###", kCSMountHDD},
	{"HDDFormat", "### Format HDD ###", kCSFormatHDD},
	{"fileSystemInfo", "### Show File System Information ###", kCSFileSystemInfo},
	{"touch", "### Make File. ex)touch a.txt", kCSMakeFile},
	{"rm", "### Remove File. ex)rm a.txt", kCSRemoveFile},
	{"ls", "### Show Directory ###", kCSRootDir},
	{"fwrite", "### Write Data to File. ex)fwrite a.txt", kCSFileWrite},
	{"fread", "### Read Data to File. ex)fread a.txt", kCSFileRead},
	{"fileIOTest", "### File I/O Test Function ###", kCSFileIOTest},
	{"cacheTest", "### Cache's Read & Write Party ###", kCSCacheTest},
	{"flush", "### Flush File System Cache", kCSCacheFlush},
	{"download", "### Download Data Using Serial. ex)download a.txt", kCSDownload},
	{"mpInfo", "### Show MP Configuration Table Information", kCSMPConfigInfo},
	{"startAP", "### Start Application Processor ###", kCSStartAP},
	{"symmetricIO", "### Start Symmetric I/O Mode ###", kCSSymmetricIOMode},
	{"irqINTINMapInfo", "### Show IRQ->INTIN Mapping Table ###", kCSIRQMapTbl},
	{"intProcCnt", "### Show Interrupt Processing Count ###", kCSInterruptProcCnt},
	{"intLoadBalancing", "### Start Interrupt Load Balancing ###", kCSInterruptLoadBalancing},
	{"taskLoadBalancing", "### Start Task Load Balancing ###", kCSTaskLoadBalancing},
	{"changeAffinity", "### Change Task Affinity. ex)changeAffinity 1(ID) 0xFF(Affinity)", kCSChangeAffinity},
	{"vbeModeInfo", "### Show VBE Mode Information ###", kCSVBEModeInfo},
	{"syscallTest", "### System Call Test ###", kCSSysCall},
	{"exec", "### Execute Application Program, ex)exec a.elf argument", kCSExecApp },
};

// 셸 메인 루프
void kStartShell(void) {
	char buf[SHELL_MAXCMDBUFCNT];
	int bufIdx = 0;
	BYTE key;
	int x, y;
	CONSOLEMANAGER *csManager;

	// 콘솔 관리 자료구조 반환
	csManager = kGetConsoleManager();

	// 프롬프트 출력
	kPrintf(SHELL_PROMPTMSG);

	// 콘솔 셸 종료 플래그가 TRUE 일 때 까지 반복
	while(1) {
		// 키가 수신될 때까지 대기
		key = kGetCh();

		if(csManager->exit == TRUE) break;

		// Backspace 키 처리
		if(key == KEY_BACKSPACE) {
			if(bufIdx > 0) {
				// 현재 커서 위치를 얻어 한 문자 앞으로 이동한 다음 공백을 출력, 커맨드 버퍼에서 마지막 문자 삭제
				kGetCursor(&x, &y);
				kPrintXY(x - 1, y, CONSOLE_DEFAULTTEXTCOLOR, " ");
				kSetCursor(x - 1, y);
				bufIdx--;
			}
		} else if(key == KEY_ENTER) {
			kPrintf("\n");
			if(bufIdx > 0) {
				// 커맨드 버퍼에 있는 명령 실행
				buf[bufIdx] = '\0';
				kExecCMD(buf);
			}

			// 프롬프트 출력 및 커맨드 버퍼 초기화
			kPrintf("%s", SHELL_PROMPTMSG);
			kMemSet(buf, '\0', SHELL_MAXCMDBUFCNT);
			bufIdx = 0;
		} else if((key == KEY_LSHIFT) || (key == KEY_RSHIFT) || (key == KEY_CAPSLOCK) || (key == KEY_NUMLOCK) || (key == KEY_SCROLLLOCK)) {
			;
		} else if(key < 128) {
			// TAB 외 특수 키 무시 및 TAB 키 공백으로 전환
			if(key == KEY_TAB) key = ' ';

			// 버퍼에 공간이 남아있을 때만 가능
			if(bufIdx < SHELL_MAXCMDBUFCNT) {
				buf[bufIdx++] = key;
				kPrintf("%c", key);
			}
		}
	}
}

// 커맨드 버퍼에 있는 커맨드를 비교해 해당 커맨드 처리하는 함수 수행
void kExecCMD(const char *buf) {
	int i, idx, bufLen, len, cnt;

	// 공백으로 구분된 커맨드 추출
	bufLen = kStrLen(buf);
	for(idx = 0; idx < bufLen; idx++) if(buf[idx] == ' ') break;

	// 커맨드 테이블 검사 후 일치하는 커맨드 확인
	cnt = sizeof(gs_cmdTable) / sizeof(SHELLENTRY);
	for(i = 0; i < cnt; i++) {
		len = kStrLen(gs_cmdTable[i].cmd);
		// 커맨드 길이와 내용 일치 검사
		if((len == idx) && (kMemCmp(gs_cmdTable[i].cmd, buf, idx) == 0)) {
			gs_cmdTable[i].func(buf + idx + 1);
			break;
		}
	}

	// 리스트에서 못 찾으면 에러 출력
	if(i >= cnt) kPrintf("'%s' is not found.\n", buf);
}

// 파라미터 자료구조 초기화
void kInitParam(PARAMLIST *list, const char *param) {
	list->buf = param;
	list->len = kStrLen(param);
	list->nowPosition = 0;
}

// 공백으로 구분된 파라미터 내용과 길이 반환
int kGetNextParam(PARAMLIST *list, char *param) {
	int i, len;

	// 더 이상 파라미터가 없으면 나감
	if(list->len <= list->nowPosition) return 0;

	// 버퍼 길이만큼 이동하며 공백 검색
	for(i = list->nowPosition; i < list->len; i++) if(list->buf[i] == ' ') break;

	// 파라미터 복사 후 길이 반환
	kMemCpy(param, list->buf + list->nowPosition, i);
	len = i - list->nowPosition;
	param[len] = '\0';

	// 파라미터 위치 업데이트
	list->nowPosition += len + 1;
	return len;
}

// 도움말 출력
static void kCSHelp(const char *buf) {
	int i, cnt, x, y, len, maxLen = 0;

	kPrintf("   ============================================================\n");
	kPrintf("   |                    YummyHitOS Shell Help                 |\n");
	kPrintf("   ============================================================\n");
	
	cnt = sizeof(gs_cmdTable) / sizeof(SHELLENTRY);

	// 가장 긴 커맨드 길이 계산
	for(i = 0; i < cnt; i++) {
		len = kStrLen(gs_cmdTable[i].cmd);
		if(len > maxLen) maxLen = len;
	}

	// 도움말 출력
	for(i = 0; i < cnt; i++) {
		kPrintf("%s", gs_cmdTable[i].cmd);
		kGetCursor(&x, &y);
		kSetCursor(maxLen, y);
		kPrintf("  - %s\n", gs_cmdTable[i].help);

		// 목록이 많을 경우 나눠서 보여중
		if((i != 0) && ((i % 20) == 0)) {
			kPrintf("Press any key to continue... ('q' is exit) : ");
			if(kGetCh() == 'q') {
				kPrintf("\n");
				break;
			}
			kPrintf("\n");
		}
	}
}

// 화면 지움
static void kCSClear(const char *buf) {
	// 1번째 라인은 디버깅, 미관을 위해 2번째 라인으로 커서 이동
	kClearMonitor();
	kSetCursor(0, 2);
}

// 총 메모리 크기 출력
static void kCSFree(const char *buf) {
	kPrintf("    %d MB\n", kGetTotalMemSize());
}

// 문자열로 된 숫자를 숫자로 변환해 출력
static void kCSStrConvert(const char *buf) {
	char param[100];
	int len, cnt = 0;
	PARAMLIST list;
	long v;

	// 파라미터 초기화
	kInitParam(&list, buf);

	while(1) {
		// 다음 파라미터 구하고 파라미터 길이가 0이면 없다는 뜻이므로 종료
		len = kGetNextParam(&list, param);
		if(len == 0) break;

		// 파라미터에 대한 정보 출력 후 16진수|10진수 판단해 변환하여 출력
		kPrintf("Parameter %d = '%s', Length = %d, ", cnt + 1, param, len);

		// 0x로 시작하면 16진수, 그 외에는 10진수로 판단
		if(kMemCmp(param, "0x", 2) == 0) {
			v = kAtoI(param + 2, 16);
			kPrintf("HexaDecimale Value = %q\n", v);
		} else {
			v = kAtoI(param, 10);
			kPrintf("Decimal Value = %d\n", v);
		}
		cnt++;
	}
}

// Shell 종료
static void kCSExit(const char *buf) {
	// 그래픽 모드 판단
	if(kIsGUIMode() == FALSE) {
		kPrintf("It is GUI Task. You must execute GUI Mode.\n");
		return;
	}
	kPrintf("Cache Flushing ...");
	if(kFlushFileSystemCache() == TRUE) kPrintf("[  Hit  ]\n");
	else kPrintf("[  Err  ]\n");
//	setShellExitFlag(TRUE);		// how to exit GUI terminal? not OS only terminal!
}

// PC 종료
static void kCSHalt(const char *buf) {
	kPrintf("Cache Flushing ...");
	if(kFlushFileSystemCache() == TRUE) kPrintf("[  Hit  ]\n");
	else kPrintf("[  Err  ]\n");
	kPrintf("Press any key on your keyboard for shutdown PC !\n");
	kGetCh();
	kShutDown();
}

// PC 재부팅
static void kCSReboot(const char *buf) {
	kPrintf("Cache Flushing ...");
	if(kFlushFileSystemCache() == TRUE) kPrintf("[  Hit  ]\n");
	else kPrintf("[  Err  ]\n");
	kPrintf("Press any key on your keyboard for reboot PC !\n");
	kGetCh();
	kReBoot();
}

// PIT 컨트롤러 카운터 0 설정
static void kCSSetTime(const char *buf) {
	char param[100];
	PARAMLIST list;
	long v;
	BOOL term;

	// 파라미터 초기화
	kInitParam(&list, buf);

	// milli second 추출
	if(kGetNextParam(&list, param) == 0) {
		kPrintf("ex)setTime 10(ms) 1(term)\n");
		return;
	}
	v = kAtoI(param, 10);

	// Periodic 추출
	if(kGetNextParam(&list, param) == 0) {
		kPrintf("ex)setTime 10(ms) 1(term)\n");
		return;
	}
	term = kAtoI(param, 10);

	kInitPIT(MSTOCNT(v), term);
	kPrintf("Time = %d ms, Term = %d Change Complete\n", v, term);
}

// PIT 컨트롤러를 직접 사용해 ms 동안 대기
static void kCSWait(const char *buf) {
	char param[100];
	int len, i;
	PARAMLIST list;
	long ms;

	// 파라미터 초기화
	kInitParam(&list, buf);
	if(kGetNextParam(&list, param) == 0) {
		kPrintf("ex)wait 100(ms)\n");
		return;
	}
	ms = kAtoI(buf, 10);
	kPrintf("### %d ms Sleep Start... ###\n", ms);

	// 인터럽트 비활성화 후 PIT 컨트롤러를 통해 직접 시간 측정
	kOffInterrupt();
	for(i = 0; i < ms / 30; i++) kWaitPIT(MSTOCNT(30));
	kWaitPIT(MSTOCNT(ms % 30));
	kOnInterrupt();
	kPrintf("### %d ms Sleep Complete ###\n", ms);

	// 타이머 복원
	kInitPIT(MSTOCNT(1), TRUE);
}

// 타임 스탬프 카운터 읽음
static void kCSRTSC(const char *buf) {
	QWORD TSC;
	
	TSC = kReadTSC();
	kPrintf("Time Stamp Counter = %q\n", TSC);
}

// 프로세서 속도 측정
static void kCSCPUSpeed(const char *buf) {
	int i;
	QWORD last, total = 0;

	kPrintf("### Now Measuring.");

	// 10초 동안 변화한 타임 스탬프 카운터를 이용해 프로세서 속도 측정
	kOffInterrupt();
	for(i = 0; i < 200; i++) {
		last = kReadTSC();
		kWaitPIT(MSTOCNT(50));
		total += kReadTSC() - last;
		kPrintf(".");
	}
	// 타이머 복원
	kInitPIT(MSTOCNT(1), TRUE);
	kOnInterrupt();
	kPrintf("\n### CPU Speed = %d MHz ###\n", total / 10 / 1000 / 1000);
}

// RTC 컨트롤러에 저장된 일자 및 시간 정보 표시
static void kCSDate(const char *buf) {
	BYTE sec, min, hour, week, day, month;
	WORD year;

	// RTC 컨트롤러에서 시간 및 일자 읽음
	kReadTime(&hour, &min, &sec);
	kReadDate(&year, &month, &day, &week);

	kPrintf("Date : %d/%d/%d %s, ", year, month, day, kConvertWeek(week));
	kPrintf("Time : %d:%d:%d\n", hour, min, sec);
}

// TCB 자료구조와 스택 정의
static TCB gs_task[2] = {0,};
static QWORD gs_stack[1024] = {0,};

// 태스크 전환 테스트 태스크
static void kTaskTest1(void) {
	BYTE data;
	int i = 0, x = 0, y = 0, margin, j;
	CHARACTER *mon = (CHARACTER*)CONSOLE_VIDEOMEMADDR;
	TCB *runningTask;

	// 자신의 ID 얻어서 화면 오프셋으로 사용
	runningTask = kGetRunningTask(kGetAPICID());
	margin = (runningTask->link.id & 0xFFFFFFFF) % 10;

	// 화면 네 귀퉁이를 돌며 문자 출력
	for(j = 0; j < 20000; j++) {
		switch(i) {
		case 0:
			x++;
			if(x >= (CONSOLE_WIDTH - margin)) i = 1;
			break;

		case 1:
			y++;
			if(y >= (CONSOLE_HEIGHT - margin)) i = 2;
			break;

		case 2:
			x--;
			if(x < margin) i = 3;
			break;

		case 3:
			y--;
			if(y < margin) i = 4;
			break;
		}

		mon[y * CONSOLE_WIDTH + x].character = data;
		mon[y * CONSOLE_WIDTH + x].color = data & 0x0F;
		data++;
	}
	kTaskExit();
}

static void kTaskTest2(void) {
	int i = 0, offset;
	CHARACTER *mon = (CHARACTER*)CONSOLE_VIDEOMEMADDR;
	TCB *runningTask;
	char data[4] = {'-', '\\', '|', '/'};

	// 자신의 ID를 얻어 화면 오프셋으로 사용
	runningTask = kGetRunningTask(kGetAPICID());
	offset = (runningTask->link.id & 0xFFFFFFFF) * 2;
	offset = CONSOLE_WIDTH * CONSOLE_HEIGHT - (offset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));

	while(1) {
		mon[offset].character = data[i % 4];
		mon[offset].color = (offset % 15) + 1;
		i++;
	}
}

// 자신이 수행되는 코어의 ID가 변경될 때마다 자신의 태스크 ID와 코어 ID 출력
static void kTaskTest3(void) {
	QWORD id, lastTick;
	TCB *runningTask;
	BYTE _id;

	// 자신의 태스크 자료구조 저장
	runningTask = kGetRunningTask(kGetAPICID());
	id = runningTask->link.id;
	kPrintf("Test Task 3 Started. Task ID = 0x%q, Local APIC ID = 0x%x\n", id, kGetAPICID());

	// 현재 수행 중 로컬 APIC ID 저장하고 태스크가 부하 분산되어 다른 코어로 옮겨갔을 때 메시지 출력
	_id = kGetAPICID();
	while(1) {
		// 이전에 수행되었던 코어와 현재 수행하는 코어가 다르면 메시지 출력
		if(_id != kGetAPICID()) {
			kPrintf("Core Changed! Task ID = 0x%q, Prev Local APIC ID = 0x%x, Current = 0x%x\n", id, _id, kGetAPICID());

			// 현재 수행 중인 코어 ID 변경
			_id = kGetAPICID();
		}

		kSchedule();
	}
}			

// 태스크 생성하여 멀티태스킹
static void kCSCreateTask(const char *buf) {
	PARAMLIST list;
	char type[30], cnt[30];
	int i;

	// 파라미터 추출
	kInitParam(&list, buf);
	kGetNextParam(&list, type);
	kGetNextParam(&list, cnt);

	switch(kAtoI(type, 10)) {
	case 1:
		for(i = 0; i < kAtoI(cnt, 10); i++) if(kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kTaskTest1, TASK_LOADBALANCING_ID) == NULL) break;
		kPrintf("Task Test 1 %d Created.\n", i);
		break;

	case 2:
	default:
		for(i = 0; i < kAtoI(cnt, 10); i++) if(kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kTaskTest2, TASK_LOADBALANCING_ID) == NULL) break;
		kPrintf("Task Test 2 %d Created.\n", i);
		break;

	case 3:
		for(i = 0; i < kAtoI(cnt, 10); i++) {
			if(kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kTaskTest3, TASK_LOADBALANCING_ID) == NULL) break;
			kSchedule();
		}
		kPrintf("Task3 %d Created\n", i);
		break;
	}
}

// 태스크 우선순위 변경
static void kCSChangePriority(const char *buf) {
	PARAMLIST list;
	char id[30], priority[30];
	QWORD _id;
	BYTE _priority;

	// 파라미터 추출
	kInitParam(&list, buf);
	kGetNextParam(&list, id);
	kGetNextParam(&list, priority);

	// 태스크 우선순위 변경
	if(kMemCmp(id, "0x", 2) == 0) _id = kAtoI(id + 2, 16);
	else _id = kAtoI(id, 10);

	_priority = kAtoI(priority, 10);

	kPrintf("### Change Task Priority ID [0x%q] Priority[%d] ", _id, _priority);
	if(kAlterPriority(_id, _priority) == TRUE) kPrintf("Success !!\n");
	else kPrintf("Fail...\n");
}

// 현재 생성된 모든 태스크 정보 출력
static void kCSTaskList(const char *buf) {
	int i, cnt = 0, totalCnt = 0, len, procCnt;
	char _buf[20];
	TCB *tcb;

	// 코어 수만큼 루프를 돌며 각 스케줄러에 있는 태스크 수 더함
	procCnt = kGetProcCnt();

	for(i = 0; i < procCnt; i++) totalCnt += kGetTaskCnt(i);

	kPrintf("   ================== Task Total Count [%d] ===================\n", totalCnt);

	// 코어가 두 개 이상이면 각 스케줄러별 개수 출력
	if(procCnt > 1) {
		for(i = 0; i < procCnt; i++) {
			if((i != 0) && ((i % 4) == 0)) kPrintf("\n");
			kSprintf(_buf, "Core %d: %d", i, kGetTaskCnt(i));
			kPrintf(_buf);

			// 출력하고 남은 공간 모두 스페이스바로 채움
			len = 19 - kStrLen(_buf);
			kMemSet(_buf, ' ', len);
			_buf[len] = '\0';
			kPrintf(_buf);
		}

		kPrintf("\nPress any key to continue... ('q' is exit) : ");
		if(kGetCh() == 'q') {
			kPrintf("\n");
			return;
		}
		kPrintf("\n\n");
	}

	for(i = 0; i < TASK_MAXCNT; i++) {
		// TCB 구해서 TCB가 사용 중이면 ID 출력
		tcb = kGetTCB(i);
		if((tcb->link.id >> 32) != 0) {
			// 태스크가 6개 출력될 때마다 태스크 정보 표시할지 여부 확인
			if((cnt != 0) && ((cnt % 6) == 0)) {
				kPrintf("Press any key to continue... ('q' is exit) : ");
				if(kGetCh() == 'q') {
					kPrintf("\n");
					break;
				}
				kPrintf("\n");
			}
			kPrintf("[%d] Task ID[0x%Q], Priority[%d], Flags[0x%Q], Thread[%d]\n", 1 + cnt++, tcb->link.id, GETPRIORITY(tcb->flag), tcb->flag, kGetListCnt(&(tcb->childThreadList)));
			kPrintf("     Core ID[0x%X] CPU Affinity[0x%x]\n", tcb->_id, tcb->affinity);
			kPrintf("     Parent PID[0x%Q], Memory Address[0x%Q], Size[0x%Q]\n", tcb->parentProcID, tcb->memAddr, tcb->memSize);
		}
	}
}

// 태스크 종료
static void kCSTaskill(const char *buf) {
	PARAMLIST list;
	char id[30];
	QWORD _id;
	TCB *tcb;
	int i;

	// 파라미터 추출
	kInitParam(&list, buf);
	kGetNextParam(&list, id);

	// 태스크 종료
	if(kMemCmp(id, "0x", 2) == 0) _id = kAtoI(id + 2, 16);
	else _id = kAtoI(id, 10);

	// 특정 ID만 종료하는 경우
	if(_id != 0xFFFFFFFF) {
		tcb = kGetTCB(GETTCBOFFSET(_id));
		_id = tcb->link.id;

		// 시스템 테스트는 제외
		if(((_id >> 32) != 0) && ((tcb->flag & TASK_FLAGS_SYSTEM) == 0x00)) {
			kPrintf("Kill Task ID [0x%q] ", _id);
			if(kTaskFin(_id) == TRUE) kPrintf("Success !!\n");
			else kPrintf("Fail...\n");
		} else kPrintf("Task does not exist or task is system task.\n");
	} else { // 콘솔 셸과 유후 태스크 제외 모든 태스크 종료
		for(i = 0; i < TASK_MAXCNT; i++) {
			tcb = kGetTCB(i);
			_id = tcb->link.id;

			// 시스템 테스트는 삭제 목록에서 제외
			if(((_id >> 32) != 0) && ((tcb->flag & TASK_FLAGS_SYSTEM) == 0x00)) {
				kPrintf("Kill Task ID [0x%q] ", _id);
				if(kTaskFin(_id) == TRUE) kPrintf("Success !!\n");
				else kPrintf("Fail...\n");
			}
		}
	}
}

// 프로세서 사용률 표시
static void kCSCPULoad(const char *buf) {
	int i, len;
	char _buf[50];

	kPrintf("   ====================== Processor Load ======================\n");

	// 각 코어별 부하 출력
	for(i = 0; i < kGetProcCnt(); i++) {
		if((i != 0) && ((i % 4) == 0)) kPrintf("\n");
		kSprintf(_buf, "Core %d : %d%%", i, kGetProcLoad(i));
		kPrintf("%s", _buf);

		// 출력하고 남은 공간 모두 스페이스바로 채움
		len = 19 - kStrLen(_buf);
		kMemSet(_buf, ' ', len);
		_buf[len] = '\0';
		kPrintf(_buf);
	}
	kPrintf("\n");
}

// 뮤텍스 테스트용 뮤텍스와 변수
static MUTEX gs_mut;
static volatile QWORD gs_add;

// 뮤텍스 테스트 태스크
static void kPrintNumTask(void) {
	int i, j;
	QWORD tickCnt;

	// 50ms 정도 대기하여 콘솔 셸이 출력하는 메시지와 겹치지 않도록 함
	tickCnt = kGetTickCnt();
	while((kGetTickCnt() - tickCnt) < 50) kSchedule();

	// 루프 돌면서 숫자 출력
	for(i = 0; i < 5; i++) {
		kLock(&gs_mut);
		kPrintf("Task ID [0x%Q] Value[%d]\n", kGetRunningTask(kGetAPICID())->link.id, gs_add);
		gs_add += 1;
		kUnlock(&gs_mut);

		// 프로세서 소모를 늘리려고 추가한 테스트 코드
		for(j = 0; j < 30000; j++);
	}

	// 모든 태스크가 종료할 때까지 1초(100ms) 정도 대기
	tickCnt = kGetTickCnt();
	while((kGetTickCnt() - tickCnt) < 1000) kSchedule();

	// 태스크 종료
	kTaskExit();
}

// 뮤텍스 테스트하는 태스크 생성
static void kCSMutexTest(const char *buf) {
	int i;
	gs_add = 1;

	// 뮤텍스 초기화
	kInitMutex(&gs_mut);

	for(i = 0; i < 3; i++) kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kPrintNumTask, kGetAPICID());
	kPrintf("Wait Util %d Task Finishing...\n", i);
	kGetCh();
}

// 태스크 2를 자신의 쓰레드로 생성하는 태스크
static void kCreateThreadTask(void) {
	int i;

	for(i = 0; i < 3; i++) kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kTaskTest2, TASK_LOADBALANCING_ID);
	while(1) kSleep(1);
}

// 쓰레드를 테스트하는 태스크 생성
static void kCSThreadTest(const char *buf) {
	TCB *proc;

	proc = kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_PROC, (void*)0xEEEEEEEE, 0x1000, (QWORD)kCreateThreadTask, TASK_LOADBALANCING_ID);

	if(proc != NULL) kPrintf("Process [0x%Q] Create Success !\n", proc->link.id);
	else kPrintf("Process Create Fail...\n");
}

// 난수 발생을 위한 변수
static volatile QWORD gs_matrixValue = 0;

// 임의의 난수 반환
QWORD kRand(void) {
	gs_matrixValue = (gs_matrixValue * 412153 + 5571031) >> 16;
	return gs_matrixValue;
}

// 철자를 흘러내리게 하는 쓰레드
static void dropMatrixChar(void) {
	int x, y, i;
	char txt[2] = {0,};

	x = kRand() % CONSOLE_WIDTH;

	while(1) {
		kSleep(kRand() % 20);
		if((kRand() % 20) < 16) {
			txt[0] = ' ';
			for(i = 0; i < CONSOLE_HEIGHT - 1; i++) {
				kPrintXY(x, i, 0x0A, txt);
				kSleep(50);
			}
		} else {
			for(i = 0; i < CONSOLE_HEIGHT - 1; i++) {
				txt[0] = (i + kRand()) % 128;
				kPrintXY(x, i, 0x0A, txt);
				kSleep(50);
			}
		}
	}
}

// 쓰레드 생성으로 매트릭스 화면 구성 프로세스
static void matrixProc(void) {
	int i;

	for(i = 0; i < 300; i++) {
		if(kCreateTask(TASK_FLAGS_THREAD | TASK_FLAGS_LOW, 0, 0, (QWORD)dropMatrixChar, TASK_LOADBALANCING_ID) == NULL) break;
		kSleep(kRand() % 5 + 5);
	}
	kGetCh();
}

// 매트릭스 명령
static void kCSMatrix(const char *buf) {
	TCB *proc;

	proc = kCreateTask(TASK_FLAGS_PROC | TASK_FLAGS_LOW, (void*)0xE00000, 0xE00000, (QWORD)matrixProc, TASK_LOADBALANCING_ID);

	if(proc != NULL) {
		kClearMatrix();

		// 태스크 종료시까지 대기
		while((proc->link.id >> 32) != 0) kSleep(100);
	} else 	kPrintXY(0, 0, 0x0A, "Matrix Process Create Fail...\n");
}

// FPU를 테스트하는 태스크
static void fpuTest(void) {
	double dV1, dV2;
	TCB *runningTask;
	QWORD cnt = 0, randValue;
	int i, offset;
	char data[4] = { '-', '\\', '|', '/' };
	CHARACTER *mon = (CHARACTER*)CONSOLE_VIDEOMEMADDR;

	runningTask = kGetRunningTask(kGetAPICID());

	// 자신의 ID를 얻어 화면 오프셋으로 사용
	offset = (runningTask->link.id & 0xFFFFFFFF) * 2;
	offset = (CONSOLE_WIDTH * CONSOLE_HEIGHT) - (offset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));

	// 루프를 무한히 반복해 동일 계산 수행
	while(1) {
		dV1 = 1;
		dV2 = 1;
		// 테스트를 위해 동일 계산 2번 반복 실행
		for(i = 0; i < 100; i++) {
			randValue = kRand();
			dV1 *= (double)randValue;
			dV2 *= (double)randValue;

			kSleep(1);

			randValue = kRand();
			dV1 /= (double)randValue;
			dV2 /= (double)randValue;
		}

		if(dV1 != dV2) {
			kPrintf("Value is Different !! [%f] != [%f]\n", dV1, dV2);
			break;
		}

		cnt++;

		// 회전하는 바람개비 표시
		mon[offset].character = data[cnt % 4];
		mon[offset].color = (offset % 15) + 1;
	}
}

// 원주율 계산
static void kCSGetPIE(const char *buf) {
	double res;
	int i;
	kPrintf("PIE Calculation Test\n");
	kPrintf("Result: 355 / 113 = ");
	res = (double)355 / 113;

	kPrintf("%d.%d%d\n", (QWORD)res, ((QWORD)(res * 10) % 10), ((QWORD)(res * 100) % 10));

	// 실수를 계산하는 태스크 생성
	for(i = 0; i < 100; i++) kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)fpuTest, TASK_LOADBALANCING_ID);
}

// 동적 메모리 정보 표시
static void kCSDynMemInfo(const char *buf) {
	QWORD startAddr, totalSize, metaSize, usedSize;

	kGetDynMemInfo(&startAddr, &totalSize, &metaSize, &usedSize);

	kPrintf("   ================ Dynamic Memory Information ================\n");
	kPrintf("Start Address:	[0x%Q]\n", startAddr);
	kPrintf("Total Size:	[0x%Q]byte, [%d]MB\n", totalSize, totalSize / 1024 / 1024);
	kPrintf("Meta Size:	[0x%Q]byte, [%d]KB\n", metaSize, metaSize / 1024);
	kPrintf("Used Size:	[0x%Q]byte, [%d]KB\n", usedSize, usedSize / 1204);\
}

// 모든 블록 리스트의 블록을 순차적으로 할당하고 해제하는 테스트
static void kCSSeqAllocTest(const char *buf) {
	DYNMEM *mem;
	long i, j, k;
	QWORD *_buf;

	kPrintf("   ================ Dynamic Memory Information ================\n");
	mem = kGetDynMemManager();

	for(i = 0; i < mem->maxLvCnt; i++) {
		kPrintf("Block List [%d] Test Start\n", i);
		kPrintf("Allocation And Compare: ");

		// 모든 블록을 할당받아 값을 채운 후 검사
		for(j = 0; j < (mem->smallBlockCnt >> i); j++) {
			_buf = kAllocMem(DYNMEM_MIN_SIZE << i);
			if(_buf == NULL) {
				kPrintf("\nAllocation Fail...\n");
				return;
			}

			// 값을 채운 후 다시 검사
			for(k = 0; k < (DYNMEM_MIN_SIZE << i) / 8; k++) _buf[k] = k;

			for(k = 0; k < (DYNMEM_MIN_SIZE << i) / 8; k++) if(_buf[k] != k) {
				kPrintf("\nCompare Fail...\n");
				return;
			}
			// 진행 과정을 . 으로 표시
			kPrintf(".");
		}

		kPrintf("\nFree: ");
		// 할당 받은 블록 모두 반환
		for(j = 0; j < (mem->smallBlockCnt >> i); j++) {
			if(kFreeMem((void*)(mem->startAddr + (DYNMEM_MIN_SIZE << i) * j)) == FALSE) {
				kPrintf("\nFree Fail...\n");
				return;
			}
			// 진행 과정을 . 으로 표시
			kPrintf(".");
		}
		kPrintf("\n");
	}
	kPrintf("Sequential Allocation Test is Finished Successfully !!\n");
}

// 임의로 메모리 할당 후 해제하는 것을 반복하는 태스크
static void randAllocTask(void) {
	TCB *task;
	QWORD memSize;
	char _buf[CONSOLE_WIDTH - 13];	// 가로길이 총 80에서(CONSOLE_WIDTH == 80) print할 x좌표만큼 빼준다!
	BYTE *allocBuf;
	int i, j, y;

	task = kGetRunningTask(kGetAPICID());
	y = (task->link.id) % 15 + 9;

	for(j = 0; j < 10; j++) { // 1KB ~ 32M까지 할당
		do {
			memSize = ((kRand() % (32 * 1024)) + 1) * 1024;
			allocBuf = kAllocMem(memSize);

			// 만일 버퍼를 할당받지 못하면 다른 태스크가 메모리를 사용할 수 있으니 잠시 대기 후 재시도
			if(allocBuf == 0) kSleep(1);
		} while(allocBuf == 0);

		kSprintf(_buf, "|Address: [0x%Q] Size: [0x%Q] Allocation Success", allocBuf, memSize);
		// 자신의 ID를 Y좌표로 하여 데이터 출력
		kPrintXY(12, y, CONSOLE_DEFAULTTEXTCOLOR, _buf);
		kSleep(200);

		// 버퍼를 반으로 나눠 랜덤한 데이터를 똑같이 채움
		kSprintf(_buf, "|Address: [0x%Q] Size: [0x%Q] Data Write...     ", allocBuf, memSize);
		kPrintXY(12, y, CONSOLE_DEFAULTTEXTCOLOR, _buf);
		for(i = 0; i < memSize / 2; i++) {
			allocBuf[i] = kRand() & 0xFF;
			allocBuf[i + (memSize / 2)] = allocBuf[i];
		}
		kSleep(200);

		// 채운 데이터가 정상적인지 다시 확인
		kSprintf(_buf, "|Address: [0x%Q] Size: [0x%Q] Data Verify...    ", allocBuf, memSize);
		kPrintXY(12, y, CONSOLE_DEFAULTTEXTCOLOR, _buf);
		for(i = 0; i < memSize / 2; i++) if(allocBuf[i] != allocBuf[i + (memSize / 2)]) {
			kPrintf("Task ID[0x%Q] Verify Fail...\n", task->link.id);
			kTaskExit();
		}
		kFreeMem(allocBuf);
		kSleep(200);
	}
	kTaskExit();
}

// 태스크를 여러 개 생성해 임의의 메모리 할당 후 해제하는 것 반복하는 테스트
static void kCSRandAllocTest(const char *buf) {
	int i;
		// Ram Disk를 이용해도 에러나는 부분. 동적 메모리가 0 Byte여야하는데, 비어있지 않아 오류나는 것 같음.
	for(i = 0; i < 1000; i++) kCreateTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD, 0, 0, (QWORD)randAllocTask, TASK_LOADBALANCING_ID);
}

// 하드 디스크 정보 표시
static void kCSHDDInfo(const char *buf) {
	HDDINFO hdd;
	char _buf[80];

	// 하드 디스크 정보 읽음
	if(kGetHDDInfo(&hdd) == FALSE) {
		kPrintf("HDD Information Read Fail...\n");
		return;
	}

	kPrintf("   ============== Primary Master HDD Information ==============\n");

	// 모델 번호 출력
	kMemCpy(_buf, hdd.modelNum, sizeof(hdd.modelNum));
	_buf[sizeof(hdd.modelNum) - 1] = '\0';
	kPrintf("Model Number:\t %s\n", _buf);

	// 시리얼 번호 출력
	kMemCpy(_buf, hdd.serialNum, sizeof(hdd.serialNum));
	_buf[sizeof(hdd.serialNum) - 1] = '\0';
	kPrintf("Serial Number:\t %s\n", _buf);

	// 헤드, 실린더, 실린더 당 섹터 수 출력
	kPrintf("Head Count:\t %d\n", hdd.headNum);
	kPrintf("Cylinder Count:\t %d\n", hdd.cylinderNum);
	kPrintf("Sector Count:\t %d\n", hdd.perSectorNum);

	// 총 섹터 수 출력
	kPrintf("Total Sector:\t %d Secotr, %dMB\n", hdd.totalSector, hdd.totalSector / 2 / 1024);
}

// 하드 디스크에서 파라미터로 넘어온 LBA 어드레스부터 섹터 수만큼 읽음
static void kCSReadSector(const char *buf) {
	PARAMLIST list;
	char lba[50], sectorCnt[50];
	DWORD _lba;
	int _sectorCnt, i, j;
	char *_buf;
	BYTE data;
	BOOL _exit = FALSE;

	// 파라미터 리스트 초기화 후 LBA 어드레스와 섹터 수 추출
	kInitParam(&list, buf);
	if((kGetNextParam(&list, lba) == 0) || (kGetNextParam(&list, sectorCnt) == 0)) {
		kPrintf("ex)readSector 0(LBA) 10(Count)\n");
		return;
	}
	_lba = kAtoI(lba, 10);
	_sectorCnt = kAtoI(sectorCnt, 10);

	// 섹터 수만큼 메모리 할당받아 읽기 수행
	_buf = kAllocMem(_sectorCnt * 512);
	if(kReadHDDSector(TRUE, TRUE, _lba, _sectorCnt, _buf) == _sectorCnt) {
		kPrintf("LBA [%d], [%d] Sector Read Success !!", _lba, _sectorCnt);
		// 데이터 버퍼 내용 출력
		for(j = 0; j < _sectorCnt; j++) {
			for(i = 0; i < 512; i++) {
				if(!((j == 0) && (i == 0)) && ((i % 256) == 0)) {
					kPrintf("\nPress any key to continue... ('q' is exit) : ");
					if(kGetCh() == 'q') {
						_exit = TRUE;
						break;
					}
				}
				if((i % 16) == 0) kPrintf("\n[LBA:%d, Offset:%d]\t| ", _lba + j, i);

				// 모두 두 자리로 표시하려고 16보다 작은 경우 0추가
				data = _buf[j * 512 + i] & 0xFF;
				if(data < 16) kPrintf("0");
				kPrintf("%X ", data);
			}
			if(_exit == TRUE) break;
		}
		kPrintf("\n");
	} else kPrintf("Read Fail...\n");

	kFreeMem(_buf);
}

// 하드 디스크에서 파라미터로 넘어온 LBA 어드레스부터 섹터 수만큼 씀
static void kCSWriteSector(const char *buf) {
	PARAMLIST list;
	char lba[50], sectorCnt[50];
	DWORD _lba;
	int _sectorCnt, i, j;
	char *_buf;
	BOOL _exit = FALSE;
	BYTE data;
	static DWORD ls_writeCnt = 0;

	// 파라미터 리스트를 초기화해 LBA 어드레스와 섹터 수 추출
	kInitParam(&list, buf);
	if((kGetNextParam(&list, lba) == 0) || (kGetNextParam(&list, sectorCnt) == 0)) {
		kPrintf("ex)writeSector 0(LBA) 10(Count)\n");
		return;
	}
	_lba = kAtoI(lba, 10);
	_sectorCnt = kAtoI(sectorCnt, 10);

	ls_writeCnt++;
	// 버퍼를 할당받아 데이터 채움. 패턴은 4바이트의 LBA 어드레스와 4바이트 쓰기가 수행된 횟수로 생성
	_buf = kAllocMem(_sectorCnt * 512);
	for(j = 0; j < _sectorCnt; j++) for(i = 0; i < 512; i += 8) {
		*(DWORD*)&(_buf[j * 512 + i]) = _lba + j;
		*(DWORD*)&(_buf[j * 512 + i + 4]) = ls_writeCnt;
	}

	// 쓰기 수행
	if(kWriteHDDSector(TRUE, TRUE, _lba, _sectorCnt, _buf) != _sectorCnt) {
		kPrintf("Write Fail...\n");
		return;
	}
	kPrintf("LBA [%d], [%d] Sector Write Success !!\n", _lba, _sectorCnt);

	// 데이터 버퍼의 내용 출력
	for(j = 0; j < _sectorCnt; j++) {
		for(i = 0; i < 512; i++) {
			if(!((j == 0) && (i == 0)) && ((i % 256) == 0)) {
				kPrintf("\nPress any key to continue... ('q' is exit) : ");
				if(kGetCh() == 'q') {
					_exit = TRUE;
					break;
				}
			}
			if((i % 16) == 0) kPrintf("\n[LBA:%d, Offset:%d]\t| ", _lba + j, i);

			// 모두 두 자리로 표시하기위해 16보다 작은 경우 0 추가
			data = _buf[j * 512 + i] & 0xFF;
			if(data < 16) kPrintf("0");
			kPrintf("%X ", data);
		}
		if(_exit == TRUE) break;
	}
	kPrintf("\n");
	kFreeMem(_buf);
}

// 하드 디스크 연결
static void kCSMountHDD(const char *buf) {
	if(kMount() == FALSE) {
		kPrintf("HDD Mount Fail...\n");
		return;
	}
	kPrintf("HDD Mount Success !!\n");
}

// 하드 디스크에 파일 시스템 생성(포맷)
static void kCSFormatHDD(const char *buf) {
	if(kFormat() == FALSE) {
		kPrintf("HDD Format Fail...\n");
		return;
	}
	kPrintf("HDD Format Success !!\n");
}

// 파일 시스템 정보 표시
static void kCSFileSystemInfo(const char *buf) {
	FILESYSTEMMANAGER manager;

	kGetFileSystemInfo(&manager);

	kPrintf("   ================= File System Information ==================\n");
	kPrintf("Mounted:\t\t\t\t %d\n", manager.mnt);
	kPrintf("Reserved Sector Count:\t\t\t %d Sector\n", manager.reserved_sectorCnt);
	kPrintf("Cluster Link Table Start Address:\t %d Sector\n", manager.linkStartAddr);
	kPrintf("Cluster Link Table Size:\t\t %d Sector\n", manager.linkAreaSize);
	kPrintf("Data Area Start Address:\t\t %d Sector\n", manager.dataStartAddr);
	kPrintf("Total Cluster Count:\t\t\t %d Cluster\n", manager.totalClusterCnt);
}

// 루트 디렉터리에 빈 파일 생성
static void kCSMakeFile(const char *buf) {
	PARAMLIST list;
	char name[50];
	int len, i;
	DWORD cluster;
	FILE *file;

	// 파라미터 리스트 초기화해 파일 이름 추출
	kInitParam(&list, buf);
	len = kGetNextParam(&list, name);
	name[len] = '\0';
	if((len > (FILESYSTEM_FILENAME_MAXLEN - 1)) || (len == 0)) {
		kPrintf("Too Long or Too Short File Name\n");
		return;
	}

	file = fopen(name, "w");
	if(file == NULL) {
		kPrintf("Make File Fail...\n");
		return;
	}
	fclose(file);
	kPrintf("Make a File Success !!\n");
}

// 루트 디렉터리에서 파일 삭제
static void kCSRemoveFile(const char *buf) {
	PARAMLIST list;
	char name[50];
	int len;

	// 파라미터 리스트 초기화해 파일 이름 추출
	kInitParam(&list, buf);
	len = kGetNextParam(&list, name);
	name[len] = '\0';

	if((len > (FILESYSTEM_FILENAME_MAXLEN -1)) || (len == 0)) {
		kPrintf("Too Long or Too Short File Name\n");
		return;
	}

	if(fremove(name) != 0) {
		kPrintf("File Not Found or File Opened...\n");
		return;
	}

	kPrintf("Remove a File Success !!\n");
}

// 루트 디렉터리 파일 목록 표시
static void kCSRootDir(const char *buf) {
	DIR *dir;
	int cnt = 0, totalCnt = 0;
	DIRENTRY *entry;
	char *_buf[CONSOLE_WIDTH - 5], tmp[50];	// _buf의 크기가 CONSOLE_WIDTH - 5인 이유는 현재 우리의 OS는 총 80만큼의 가로길이이고
						// 아래 코드를 보면 미관을 위해 4칸을 띄어준 후 %s를 통해 출력하므로 5만큼 빼준다!
	DWORD totalByte = 0, usedClusterCnt = 0;
	FILESYSTEMMANAGER manager;

	// 파일 시스템 정보를 얻음
	kGetFileSystemInfo(&manager);

	// 루트 디렉터리 오픈
	dir = dopen("/");
	if(dir == NULL) {
		kPrintf("Root Directory Open Fail...\n");
		return;
	}

	// 먼저 루프 돌며 디렉터리에 있는 파일 개수와 전체 파일이 사용한 크기 계산
	while(1) {
		// 디렉터리에서 엔트리 하나 읽음
		entry = dread(dir);
		// 더 이상 파일이 없으면 나감
		if(entry == NULL) break;
		totalCnt++;
		totalByte += entry->size;

		// 실제로 사용된 클러스터 개수를 계산
		if(entry->size == 0) usedClusterCnt++;	// 크기가 0이라도 클러스터 1개는 할당되어 있음
		else usedClusterCnt += (entry->size + (FILESYSTEM_CLUSTER_SIZE - 1)) / FILESYSTEM_CLUSTER_SIZE;
	}

	// 실제 파일의 내용을 표시하는 루프
	drewind(dir);
	while(1) {
		// 디렉터리에서 엔트리 하나를 읽음
		entry = dread(dir);
		// 더 이상 파일이 없으면 나감
		if(entry == NULL) break;

		// 전부 공백으로 초기화한 후 각 위치에 값을 대입
		kMemSet(_buf, ' ', CONSOLE_WIDTH - 6);
		_buf[CONSOLE_WIDTH - 6] = '\0';

		// 파일 이름 삽입
		kMemCpy(_buf, entry->d_name, kStrLen(entry->d_name));
		// 파일 길이 삽입
		kSprintf(tmp, "%d Byte", entry->size);
		kMemCpy(_buf + 3, tmp, kStrLen(tmp));
		// 파일 시작 클러스터 삽입
		kSprintf(tmp, "0x%X Cluster", entry->startClusterIdx);
		kMemCpy(_buf + 6, tmp, kStrLen(tmp));
		kPrintf("    %s\n", _buf);

		if((cnt != 0) && (cnt % 20) == 0) {
			kPrintf("Press any key to continue... ('q' is exit) : ");
			if(kGetCh() == 'q') {
				kPrintf("\n");
				break;
			}
		}
		cnt++;
	}

	// 총 파일 개수와 파일 총 크기 출력
	kPrintf("\t\tTotal File Count: %d\n", totalCnt);
	kPrintf("\t\tTotal File Size: %d Byte (%d Cluster)\n", totalByte, usedClusterCnt);

	// 남은 클러스터 수를 이용해 여유 공간 출력
	kPrintf("\t\tFree Space: %d KByte (%d Cluster)\n", (manager.totalClusterCnt - usedClusterCnt) * FILESYSTEM_CLUSTER_SIZE / 1024, manager.totalClusterCnt - usedClusterCnt);

	// 디렉터리를 닫음
	dclose(dir);
}

// 파일을 생성해 키보드로 입력된 데이터 씀
static void kCSFileWrite(const char *buf) {
	PARAMLIST list;
	char name[50];
	int len, enterCnt = 0;
	FILE *fp;
	BYTE key;

	// 파라미터 리스트를 초기화하여 파일 이름을 추출
	kInitParam(&list, buf);
	len = kGetNextParam(&list, name);
	name[len] = '\0';
	if((len > (FILESYSTEM_FILENAME_MAXLEN - 1)) || (len == 0)) {
		kPrintf("Too Long or Too Short File Name\n");
		return;
	}

	// 파일 생성
	fp = fopen(name, "w");
	if(fp == NULL) {
		kPrintf("%s File Open Fail...\n", name);
		return;
	}

	// 엔터 키가 연속 3번 눌러질 때까지 내용을 파일에 씀
	while(1) {
		key = kGetCh();
		// 엔터 키면 연속 3번 눌러졌는가 확인 후 루프를 빠져나감
		if(key == KEY_ENTER) {
			enterCnt++;
			if(enterCnt >= 3) break;
		} else enterCnt = 0;	// 엔터 키가 아니면 엔터 키 입력 횟수 초기화
		kPrintf("%c", key);
		if(fwrite(&key, 1, 1, fp) != 1) {
			kPrintf("File Write Fail...\n");
			break;
		}
	}

	kPrintf("Write File Success !!\n");
	fclose(fp);
}

// 파일을 열어 데이터를 읽음
static void kCSFileRead(const char *buf) {
	PARAMLIST list;
	char name[50];
	int len, enterCnt = 0;
	FILE *fp;
	BYTE key;

	// 파라미터 리스트를 초기화해 파일 이름을 추출
	kInitParam(&list, buf);
	len = kGetNextParam(&list, name);
	name[len] = '\0';
	if((len > (FILESYSTEM_FILENAME_MAXLEN - 1)) || (len == 0)) {
		kPrintf("Too Long or Too Short File Name\n");
		return;
	}

	// 파일 생성
	fp = fopen(name, "r");
	if(fp == NULL) {
		kPrintf("%s File Open Fail...\n", name);
		return;
	}

	// 파일 끝까지 출력하는 것 반복
	while(1) {
		if(fread(&key, 1, 1, fp) != 1) break;
		kPrintf("%c", key);

		// 만약 엔터 키면 엔터 키 횟수를 증가시키고 20라인까지 출력했으면 더 출력할지 여부 물어봄
		if(key == KEY_ENTER) {
			enterCnt++;
			if((enterCnt != 0) && ((enterCnt % 20) == 0)) {
				kPrintf("Press any key to continue... ('q' is exit) : ");
				if(kGetCh() == 'q') {
					kPrintf("\n");
					break;
				}
				kPrintf("\n");
				enterCnt = 0;
			}
		}
	}
	fclose(fp);
}

// 파일 I/O 관련된 기능을 테스트
static void kCSFileIOTest(const char *buf) {
	FILE *file;
	BYTE *_buf, _tmpBuf[1024];
	int i, j;
	DWORD randOffset, byteCnt, maxSize;

	kPrintf("   ================== File I/O Function Test ==================\n");

	// 4MB의 버퍼 할당
	maxSize = 4 * 1024 * 1024;
	_buf = kAllocMem(maxSize);
	if(_buf == NULL) {
		kPrintf("Memory Allocation Fail...\n");
		return;
	}
	// 테스트용 파일 삭제
	fremove("fileIOTest.bin");

	// 파일 열기 테스트
	kPrintf("1. File Open Fail Test ....");
	// r 옵션은 파일을 생성하지 않으니 테스트 파일이 없으면 NULL이 되어야 함
	file = fopen("fileIOTest.bin", "r");
	if(file == NULL) kPrintf("[  Hit  ]\n");
	else {
		kPrintf("[  Err  ]\n");
		fclose(file);
	}

	// 파일 생성 테스트
	kPrintf("2. Make a File Test .......");
	// w 옵션은 파일을 생성하므로 정상적으로 핸들이 반환되어야 함
	file = fopen("fileIOTest.bin", "w");
	if(file != NULL) {
		kPrintf("[  Hit  ]\n");
		kPrintf("    File Handle [0x%Q]\n", file);
	} else kPrintf("[  Err  ]\n");

	// 순차적 영역 쓰기 테스트
	kPrintf("3. Sequential Write Test(Cluster Size) .............");
	// 열린 핸들로 쓰기 수행
	for(i = 0; i < 100; i++) {
		kMemSet(_buf, i, FILESYSTEM_CLUSTER_SIZE);
		if(fwrite(_buf, 1, FILESYSTEM_CLUSTER_SIZE, file) != FILESYSTEM_CLUSTER_SIZE) {
			kPrintf("[  Err  ]\n");
			kPrintf("    %d Cluster Error\n", i);
			break;
		}
	}
	if(i >= 100) kPrintf("[  Hit  ]\n");

	// 순차적인 영역 읽기 테스트
	kPrintf("4. Sequential Read and Verify Test(Cluster Size) ...");
	// 파일 처음으로 이동
	fseek(file, -100 * FILESYSTEM_CLUSTER_SIZE, SEEK_END);

	// 열린 핸들로 읽기 수행 후 데이터 검증
	for(i = 0; i < 100; i++) {
		// 파일 읽음
		if(fread(_buf, 1, FILESYSTEM_CLUSTER_SIZE, file) != FILESYSTEM_CLUSTER_SIZE) {
			kPrintf("[  Err  ]\n");
			return;
		}

		// 데이터 검사
		for(j = 0; j < FILESYSTEM_CLUSTER_SIZE; j++) if(_buf[j] != (BYTE)i) {
			kPrintf("[  Err  ]\n");
			kPrintf("    %d Cluster Error. [%X] != [%X]\n", i, _buf[j], (BYTE)i);
			break;
		}
	}
	if(i >= 100) kPrintf("[  Hit  ]\n");

	// 임의의 영역 쓰기 테스트
	kPrintf("5. Random Write Test ...............\n");

	// 버퍼를 모두 0으로 채움
	kMemSet(_buf, 0, maxSize);
	// 여기 저기에 옮겨다니면서 데이터를 쓰고 검증
	// 파일의 내용을 읽어 버퍼로 복사
	fseek(file, -100 * FILESYSTEM_CLUSTER_SIZE, SEEK_CUR);
	fread(_buf, 1, maxSize, file);

	// 임의의 위치로 옮기며 데이터를 파일과 버퍼에 씀
	for(i = 0; i < 100; i ++) {
		byteCnt = (kRand() % (sizeof(_tmpBuf) - 1)) + 1;
		randOffset = kRand() % (maxSize - byteCnt);
		kPrintf("    [%d] Offset [%d] Byte [%d] ...................", i, randOffset, byteCnt);

		// 파일 포인터 이동
		fseek(file, randOffset, SEEK_SET);
		kMemSet(_tmpBuf, i, byteCnt);

		// 데이터 씀
		if(fwrite(_tmpBuf, 1, byteCnt, file) != byteCnt) {
			kPrintf("[  Hit  ]\n");
			break;
		} else kPrintf("[  Err  ]\n");

		kMemSet(_buf + randOffset, i, byteCnt);
	}

	// 맨 마지막으로 이동해 1바이트 써서 파일 크기를 4MB로 만듬
	fseek(file, maxSize - 1, SEEK_SET);
	fwrite(&i, 1, 1, file);
	_buf[maxSize - 1] = (BYTE)i;

	// 임의의 영역 읽기 테스트
	kPrintf("6. Random Read And Verify Test .....\n");
	// 임의의 위치로 옮기며 파일에서 데이터를 읽어 버퍼 내용과 비교
	for(i = 0; i < 100; i++) {
		byteCnt = (kRand() % (sizeof(_tmpBuf) -  1)) + 1;
		randOffset = kRand() % ((maxSize) - byteCnt);
		kPrintf("    [%d] Offset [%d] Byte [%d] ...................", i, randOffset, byteCnt);

		// 파일 포인터 이동
		fseek(file, randOffset, SEEK_SET);

		// 데이터 읽음
		if(fread(_tmpBuf, 1, byteCnt, file) != byteCnt) {
			kPrintf("[  Err  ]\n");
			kPrintf("    %d Read Fail...\n", randOffset);
			break;
		}

		// 버퍼와 비교
		if(kMemCmp(_buf + randOffset, _tmpBuf, byteCnt) != 0) {
			kPrintf("[  Err  ]\n");
			kPrintf("    %d Compare Fail\n", randOffset);
			break;
		}
		kPrintf("[  Hit  ]\n");
	}

	// 다시 순차적 영역 읽기 테스
	kPrintf("7. Sequential Write, Read and Verify Test(1024 Byte)\n");
	// 파일의 처음으로 이동
	fseek(file, -100 * maxSize, SEEK_CUR);

	// 열린 핸들로 쓰기 수행. 앞부분 2MB만 씀
	for(i = 0; i < (2 * 1024 * 1024 / 1024) ; i++) {
		kPrintf("    [%d] Offset [%d] Byte [%d] Write .............", i, i * 1024, 1024);

		// 1024바이트씩 파일 읽음
		if(fread(_tmpBuf, 1, 1024, file) != 1024) {
			kPrintf("[  Err  ]\n");
			return;
		}

		if(kMemCmp(_buf + (i * 1024), _tmpBuf, 1024) != 0) {
			kPrintf("[  Err  ]\n");
			break;
		} else kPrintf("[  Hit  ]\n");
	}

	// 파일 삭제 실패 테스트
	kPrintf("8. File Remove Fail Test ..");
	// 파일이 열려있는 상태이므로 지울 수 없으니 실패
	if(fremove("fileIOTest.bin" != 0)) kPrintf("[  Hit  ]\n");
	else kPrintf("[  Err  ]\n");

	// 파일 닫기 테스트
	kPrintf("9. File Close Test ........");
	// 파일이 정상적으로 닫혀야 함
	if(fclose(file) == 0) kPrintf("[  Hit  ]\n");
	else kPrintf("[  Err  ]\n");

	// 파일 삭제 테스트
	kPrintf("10. File Remove Test ......");
	// 파일이 닫혔으니 정상 삭제
	if(fremove("fileIOTest.bin") == 0) kPrintf("[  Hit  ]\n");
	else kPrintf("[  Err  ]\n");

	kFreeMem(_buf);
}

// 파일 읽고 쓰는 속도 측정
static void kCSCacheTest(const char *buf) {
	FILE *file;
	DWORD clusterSize, oneByteSize, i;
	QWORD lastTickCnt;
	BYTE *_buf;

	// 클러스터는 1MB까지 파일 테스트
	clusterSize = 1024 * 1024;
	// 1바이트씩 읽고 쓰는 테스트는 시간이 많이 걸리므로 16KB만 테스트
	oneByteSize = 16 * 1024;

	// 테스트용 버퍼 메모리 할당
	_buf = kAllocMem(clusterSize);
	if(_buf == NULL) {
		kPrintf("Memory Allocation Fail...\n");
		return;
	} else kPrintf("Allocation Memory : 0x%Q\n", _buf);

	// 버퍼 초기화
	kMemSet(_buf, 0, FILESYSTEM_CLUSTER_SIZE);

	kPrintf("   ==================== File I/O Cache Test ===================\n");

	// 클러스터 단위 파일을 순차적으로 쓰는 테스트
	kPrintf("1. Sequential Read/Write Test(Cluster Size)\n");

	// 기존 테스트 파일 제거 후 새로 만듬
	fremove("performance.txt");
	file = fopen("performance.txt", "w");
	if(file == NULL) {
		kPrintf("File Open Fail...\n");
		kFreeMem(_buf);
		return;
	}

	lastTickCnt = kGetTickCnt();
	// 클러스터 단위로 쓰는 테스트
	for(i = 0; i < (clusterSize / FILESYSTEM_CLUSTER_SIZE); i++) if(fwrite(_buf, 1, FILESYSTEM_CLUSTER_SIZE, file) != FILESYSTEM_CLUSTER_SIZE) {
		kPrintf("Write Fail...\n");
		// 파일 닫고 메모리 해제
		fclose(file);
		kFreeMem(_buf);
		return;
	}
	// 시간 출력
	kPrintf("   Sequential Write(Cluster Size): %d ms\n", kGetTickCnt() - lastTickCnt);

	// 클러스터 단위로 파일 순차적 읽기 테스트, 파일의 처음으로 이동
	fseek(file, 0, SEEK_SET);

	lastTickCnt = kGetTickCnt();
	// 클러스터 단위로 읽는 테스트
	for(i = 0; i < (clusterSize / FILESYSTEM_CLUSTER_SIZE); i++) if(fread(_buf, 1, FILESYSTEM_CLUSTER_SIZE, file) != FILESYSTEM_CLUSTER_SIZE) {
		kPrintf("Read Fail...\n");
		// 파일 닫고 메모리 해제
		fclose(file);
		kFreeMem(_buf);
		return;
	}
	// 시간 출력
	kPrintf("   Sequential Read(Cluster Size): %d ms\n", kGetTickCnt() - lastTickCnt);

	// 1바이트 단위로 파일 순차적으로 쓰는 테스트
	kPrintf("2. Sequential Read/Write Test(1 Byte)\n");

	// 기존 테스트 파일 제거 후 새로 만듬
	fremove("performance.txt");
	file = fopen("performance.txt", "w");
	if(file == NULL) {
		kPrintf("File Open Fail...\n");
		kFreeMem(_buf);
		return;
	}

	lastTickCnt = kGetTickCnt();
	// 1 바이트 단위로 쓰는 테스트
	for(i = 0; i < oneByteSize; i++) if(fwrite(_buf, 1, 1, file) != 1) {
		kPrintf("Write Fail...\n");
		// 파일 닫고 메모리 해제
		fclose(file);
		kFreeMem(_buf);
		return;
	}
	// 시간 출력
	kPrintf("   Sequential Write(1 Byte): %d ms\n", kGetTickCnt() - lastTickCnt);

	// 1 바이트 단위로 파일을 순차적이게 읽는 테스트, 파일의 처음으로 이동
	fseek(file, 0, SEEK_SET);

	lastTickCnt = kGetTickCnt();
	// 1 바이트 단위로 읽는 테스트
	for(i = 0; i < oneByteSize; i++) if(fread(_buf, 1, 1, file) != 1) {
		kPrintf("Read Fail...\n");
		// 파일 닫고 메모리 해제
		fclose(file);
		kFreeMem(_buf);
		return;
	}
	// 시간 출력
	kPrintf("   Sequential Read(1 Byte): %d ms\n", kGetTickCnt() - lastTickCnt);

	// 파일 닫고 메모리 해제
	fclose(file);
	kFreeMem(_buf);
}

// 파일 시스템 캐시 버퍼의 데이터 모두 하드 디스크에 씀
static void kCSCacheFlush(const char *buf) {
	QWORD tickCnt;

	tickCnt = kGetTickCnt();
	kPrintf("Cache Flushing ...");
	if(kFlushFileSystemCache() == TRUE) kPrintf("[  Hit  ]\n");
	else kPrintf("[  Err  ]\n");
	kPrintf("Total Time = %d ms\n", kGetTickCnt() - tickCnt);
}

// 시리얼 포트로부터 데이터 수신해 파일로 저장
static void kCSDownload(const char *buf) {
	PARAMLIST list;
	char name[50];
	int nameLen;
	DWORD dataLen, recvSize = 0, tmpSize;
	FILE *fp;
	BYTE _buf[SERIAL_FIFO_MAXSIZE];
	QWORD lastTickCnt;

	// 파라미터 리스트 초기화해 파일 이름 추출
	kInitParam(&list, buf);
	nameLen = kGetNextParam(&list, name);
	name[nameLen] = '\0';
	if((nameLen > (FILESYSTEM_FILENAME_MAXLEN - 1)) || (nameLen == 0)) {
		kPrintf("Too Long or Too Short File Name\n");
		kPrintf("ex)download a.txt\n");
		return;
	}

	// 시리얼 포트의 FIFO 클리어
	kClearSerialFIFO();

	// 데이터 길이 수신될 때까지 기다린 후 4바이트 수신하면 ACK 전송
	kPrintf("Waiting For Data Length ...");
	lastTickCnt = kGetTickCnt();
	while(recvSize < 4) {
		// 남은 수만큼 데이터 수신
		tmpSize = kRecvSerialData(((BYTE*)&dataLen) + recvSize, 4 - recvSize);
		recvSize += tmpSize;

		// 수신된 데이터가 없다면 잠시 대기
		if(tmpSize == 0) {
			kSleep(0);

			// 대기한 시간이 30초 이상이면 Time Out
			if((kGetTickCnt() - lastTickCnt) > 30000) {
				kPrintf("Time Out Occur(30s) !!\n");
				return;
			}
		} else lastTickCnt = kGetTickCnt();	// 마지막으로 데이터 수신한 시간 갱신
	}
	kPrintf("[%d] Byte\n", dataLen);

	// 정상적으로 데이터 길이 수신했으니 ACK 송신
	kSendSerialData("A", 1);

	// 파일 생성 후 시리얼로부터 데이터 수신해 파일로 저장
	fp = fopen(name, "w");
	if(fp == NULL) {
		kPrintf("%s File Open Fail...\n", name);
		return;
	}

	// 데이터 수신
	kPrintf("Data Receive Start: ");
	recvSize = 0;
	lastTickCnt = kGetTickCnt();
	while(recvSize < dataLen) {
		// 버퍼에 담아 데이터 씀
		tmpSize = kRecvSerialData(_buf, SERIAL_FIFO_MAXSIZE);
		recvSize += tmpSize;

		// 이번에 데이터 수신된 것이 있다면 ACK 또는 파일 쓰기 수행
		if(tmpSize != 0) {
			// 수신하는 쪽은 데이터 마지막까지 수신했거나 FIFO 크기인 16바이트마다 한 번씩 ACK 전송
			if(((recvSize % SERIAL_FIFO_MAXSIZE) == 0) || (recvSize == dataLen)) {
				kSendSerialData("A", 1);
				kPrintf("#");
			}

			// 쓰기 중에 문제가 생기면 바로 종료
			if(fwrite(_buf, 1, tmpSize, fp) != tmpSize) {
				kPrintf("File Write Error Occur !!\n");
				break;
			}

			// 마지막으로 데이터 수신한 시간 갱신
			lastTickCnt = kGetTickCnt();
		} else {	// 수신된 데이터가 없다면 잠시 대기
			kSleep(0);

			// 대기한 시간이 10초 이상이면 Time Out
			if((kGetTickCnt() - lastTickCnt) > 10000) {
				kPrintf("Time Out Occur(10s) !!\n");
				break;
			}
		}
	}

	// 전체 데이터 크기와 실제 수신된 데이터 크기를 비교해 성공 여부 출력 후 파일을 닫고 캐시 비우기, 수신된 길이 비교 후 문제 발생 여부 표시
	if(recvSize != dataLen) kPrintf("\nError Occur. Total Size [%d] Received Size [%d]\n", recvSize, dataLen);
	else kPrintf("\nReceive Complete. Total Size [%d] Byte\n", recvSize);

	// 파일 닫고 캐시 비움
	fclose(fp);
	kFlushFileSystemCache();
}

// MP 설정 테이블 정보 출력
static void kCSMPConfigInfo(const char *buf) {
	kPrintMPConfig();
}

// AP(Application Processor) 시작
static void kCSStartAP(const char *buf) {
	// AP(Application Processor) 깨움
	if(kStartUpAP() == FALSE) {
		kPrintf("Application Processor Start Fail...\n");
		return;
	}
	kPrintf("Application Processor Start Success !!\n");

	// BSP(Bootstrap Processor)의 APIC ID 출력
	kPrintf("Bootstrap Processor[APIC ID: %d] Start Application Processor\n", kGetAPICID());
}

// 대칭 IO 모드로 전환
static void kCSSymmetricIOMode(const char *buf) {
	MPCONFIGMANAGER *manager;
	BOOL interruptFlag;

	// MP 설정 테이블 분석
	if(kAnalysisMPConfig() == FALSE) {
		kPrintf("Analyze MP Configuration Table Fail...\n");
		return;
	}

	// MP 설정 매니저를 찾아 PIC 모드 여부 확인
	manager = kGetMPConfigManager();
	if(manager->usePICMode == TRUE) {
		// PIC 모드이면 IO 포트 어드레스 0x22에 0x70을 먼저 전송, IO 포트 어드레스 0x23에 0x01을 전송하는 방법으로 IMCR 레지스터에 접근해 PIC 모드 비활성화
		kOutByte(0x22, 0x70);
		kOutByte(0x23, 0x01);
	}

	// PIC 컨트롤러의 인터럽트 모두 마스크해 인터럽트가 발생할 수 없도록 함
	kPrintf("Mask All PIC Controller Interrupt\n");
	kMaskPIC(0xFFFF);

	// 프로세서 전체의 로컬 APIC 활성화
	kPrintf("Enable Global Local APIC\n");
	kOnLocalAPIC();

	// 현재 코어 로컬 APIC 활성화
	kPrintf("Enable Software Local APIC\n");
	kOnSWLocalAPIC();

	// 인터럽트 불가로 설정
	kPrintf("Disable CPU Interrupt Flag\n");
	interruptFlag = kSetInterruptFlag(FALSE);

	// 모든 인터럽트를 수신할 수 있도록 태스크 우선순위 레지스터를 0으로 설정
	kSetTaskPriority(0);

	// 로컬 APIC의 로컬 벡터 테이블 초기화
	kInitLocalVecTbl();

	// 대칭 IO 모드로 변경되었음을 설정
	kSetSymmetricIOMode(TRUE);

	// IO APIC 리다이렉트 테이블 초기화
	kPrintf("Initialize I/O Redirection Table\n");
	kInitIORedirect();

	// 이전 인터럽트 플래그 복원
	kPrintf("Restore CPU Interrupt Flag\n");
	kSetInterruptFlag(interruptFlag);

	kPrintf("Change Symmetric I/O Mode Complete !!\n");
}

// IRQ와 IO APIC의 인터럽트 입력 핀(INTIN)의 관계를 저장한 테이블 표시
static void kCSIRQMapTbl(const char *buf) {
	// IO APIC 관리하는 자료구조의 출력 함수 호출
	kPrintIRQMap();
}

// 코어별 인터럽트 처리 횟수 출력
static void kCSInterruptProcCnt(const char *buf) {
	INTERRUPTMANAGER *manager;
	int i, j, procCnt, len, lineCnt;
	char _buf[20];

	kPrintf("   ====================== Interrupt Count =====================\n");

	// MP 설정 테이블에 저장된 코어 개수 읽음
	procCnt = kGetProcCnt();

	// 프로세서 수만큼 Column 출력. 한 줄에 코어 4개씩 출력하고 한 Column당 15칸 할당
	for(i = 0; i < procCnt; i++) {
		if(i == 0) kPrintf("IRQ Num\t\t");
		else if((i % 4) == 0) kPrintf("\n      \t\t");
		kSprintf(_buf, "Core %d", i);
		kPrintf(_buf);

		// 출력하고 남은 공간을 모두 스페이스로 채움
		len = 15 - kStrLen(_buf);
		kMemSet(_buf, ' ', len);
		_buf[len] = '\0';
		kPrintf(_buf);
	}
	kPrintf("\n");

	// Row와 인터럽트 처리 횟수 출력. 총 인터럽트 횟수와 코어별 인터럽트 처리 횟수 출력
	lineCnt = 0;
	manager = kGetInterruptManager();
	for(i = 0; i < INTERRUPT_MAX_VECCNT; i++) {
		for(j = 0; j < procCnt; j++) {
			// ROW 출력. 한 줄에 코어 4개씩 출력하고 한 Column당 15칸 할당
			if(j == 0) {
				// 20 라인마다 화면 정지
				if((lineCnt != 0) && (lineCnt > 10)) {
					kPrintf("\nPress any key to continue... ('q' is exit) : ");
					if(kGetCh() == 'q') {
						kPrintf("\n");
						return;
					}
					lineCnt = 0;
					kPrintf("\n");
				}
				kPrintf("---------------------------------------------------------------\n");
				kPrintf("IRQ %d\t\t", i);
				lineCnt += 2;
			} else if((j % 4) == 0) {
				kPrintf("\n      \t\t");
				lineCnt++;
			}

			kSprintf(_buf, "0x%Q", manager->coreInterruptCnt[j][i]);
			// 출력하고 남은 영역을 모두 스페이스로 채움
			kPrintf(_buf);
			len = 15 - kStrLen(_buf);
			kMemSet(_buf, ' ', len);
			_buf[len] = '\0';
			kPrintf(_buf);
		}
		kPrintf("\n");
	}
}

// 인터럽트 부하 분산 기능 시작
static void kCSInterruptLoadBalancing(const char *buf) {
	kPrintf("Start Interrupt Load Balancing...\n");
	kSetInterruptLoadBalancing(TRUE);
}

// 태스크 부하 분산 기능 시작
static void kCSTaskLoadBalancing(const char *buf) {
	int i;

	kPrintf("Start Task Load Balancing...\n");
	for(i = 0; i < MAXPROCESSORCNT; i++) kSetTaskLoadBalancing(i, TRUE);
}

// 태스크 프로세서 친화도 변경
static void kCSChangeAffinity(const char *buf) {
	PARAMLIST list;
	char id[30], affinity[30];
	QWORD _id;
	BYTE _affinity;

	// 파라미터 추출
	kInitParam(&list, buf);
	kGetNextParam(&list, id);
	kGetNextParam(&list, affinity);

	// 태스크 ID 필드 추출
	if(kMemCmp(id, "0x", 2) == 0) _id = kAtoI(id + 2, 16);
	else _id = kAtoI(id, 10);

	// 프로세서 친화도 추출
	if(kMemCmp(id, "0x", 2) == 0) _affinity = kAtoI(affinity + 2, 16);
	else _affinity = kAtoI(affinity, 10);

	kPrintf("Change Task Affinity ID [0x%q] Affinity[0x%x] ", _id, _affinity);
	if(kAlterAffinity(_id, _affinity) == TRUE) kPrintf("Success !!\n");
	else kPrintf("Fail...\n");
}

// VBE 모드 정보 블록 출력
static void kCSVBEModeInfo(const char *buf) {
	VBEMODEINFO *mode;

	// VBE 모드 정보 블록 반환
	mode = kGetVBEModeInfo();
	kPrintf("VESA %x\n", mode->winWeighting);
	kPrintf("X Pixel: %d\n", mode->xPixel);
	kPrintf("Y Pixel: %d\n", mode->yPixel);
	kPrintf("Bits Per Pixel: %d\n", mode->perPixelBit);

	// 해상도와 색 정보 위주 출력
	kPrintf("Red Mask Size: %d, Field Position: %d\n", mode->redMaskSize, mode->redPosition);
	kPrintf("Green Mask Size: %d, Field Position: %d\n", mode->greenMaskSize, mode->greenPosition);
	kPrintf("Blue Mask Size: %d, Field Position: %d\n", mode->blueMaskSize, mode->bluePosition);
	kPrintf("Linear Frame Base Address: 0x%X\n", mode->linearBaseAddr);
	kPrintf("Linear Red Mask Size: %d, Field Position: %d\n", mode->linearRedMaskSize, mode->linearRedPosition);
	kPrintf("Linear Green Mask Size: %d, Field Position: %d\n", mode->linearGreenMaskSize, mode->linearGreenPosition);
	kPrintf("Linear Blue Mask Size: %d, Field Position: %d\n", mode->linearBlueMaskSize, mode->linearBluePosition);
}

// 시스템 콜 테스트 유저 레벨 태스크 생성
static void kCSSysCall(const char *buf) {
	BYTE *userMem;

	// 동적 할당 영역에 4Kbyte 메모리 할당받아 유저 레벨 태스크를 생성할 준비
	userMem = kAllocMem(0x1000);
	if(userMem == NULL) return;

	// 유저 레벨 태스크로 사용할 kSysCallTaskTest() 함수 코드를 할당받은 메모리에 복사
	kMemCpy(userMem, kSysCallTaskTest, 0x1000);

	// 유저 레벨 프로세스 생성
	kCreateTask(TASK_FLAGS_USERLV | TASK_FLAGS_PROC, userMem, 0x1000, (QWORD)userMem, TASK_LOADBALANCING_ID);
}

// 응용프로그램 실행
static void kCSExecApp(const char *buf) {
	PARAMLIST list;
	char fileName[512], argv[1024];
//	QWORD id;

	// 파라미터 추출
	kInitParam(&list, buf);
	// 형식에 맞지 않으면 도움말 출력 후 종료
	if(kGetNextParam(&list, fileName) == 0) {
		kPrintf("ex)exec a.elf argument\n");
		return;
	}

	// 두 번째 인자 문자열은 옵션으로 처리
	if(kGetNextParam(&list, argv) == 0) argv[0] = '\0';

	kPrintf("Execute Program... File [%s], Argument [%s]\n", fileName, argv);

	// 태스크 생성, return 시 8바이트 크기(QWORD)의 task->link.id 가 4바이트가 짤려 1바이트만 출력됨. 0x700000006 이면 0x6 으로 나옴.
//	id = kExecFile(fileName, argv, TASK_LOADBALANCING_ID);
	kExecFile(fileName, argv, TASK_LOADBALANCING_ID);
//	kPrintf("Task ID = 0x%Q\n", id);
}
