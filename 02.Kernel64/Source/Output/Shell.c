/*
 * Shell.c
 *
 *  Created on: 2017. 7. 23.
 *      Author: Yummy
 */

#include <Shell.h>
#include <Console.h>
#include <Keyboard.h>
#include <Util.h>
#include <PIT.h>
#include <RTC.h>
#include <AsmUtil.h>
#include <Task.h>
#include <Synchronize.h>
#include <DynMem.h>
#include <HardDisk.h>
#include <FileSystem.h>
#include <SerialPort.h>
#include <MPConfig.h>
#include <LocalAPIC.h>
#include <MP.h>
#include <IOAPIC.h>
#include <InterruptHandler.h>
#include <VBE.h>

// 커맨드 테이블 정의
SHELLENTRY gs_cmdTable[] = {
	{"help", "### Show Commands ###", csHelp},
	{"clear", "### Clear mon ###", csClear},
	{"tot_free", "### Show your total memory ###", csFree},
	{"strConvert", "### String To Number(Decimal or HexaDecimal) ###", csStrConvert},
	{"shutdown", "### Shutdown & Reboot ###", csHalt},
	{"setTime", "### Set Timer. ex)setTime 10(ms) 1(term) ###", csSetTime},
	{"wait", "### Wait a few millisecond. ex)wait 100(ms) ###", csWait},
	{"rdtsc", "### Read Time Stamp Counter ###", csRTSC},
	{"cpuSpeed", "### Measure Processor Speed ###", csCPUSpeed},
	{"date", "### Show Date & Time ###", csDate},
	{"createTask", "### Create Task. ex)createTask 1(type) 10(count) ###", csCreateTask},
	{"changePriority", "### Change Task Priority. ex)changePriority 1(ID) 2(Priority) ###", csChangePriority},
	{"tasklist", "### Show Task List ###", csTaskList},
	{"taskill", "### Task Kill. ex)taskill 1(ID) or 0xffffffff(ALL Task) ###", csTaskill},
	{"cpuload", "### Show Processor Load ###", csCPULoad},
	{"mutexTest", "### Mutex Test Command ###", csMutexTest},
	{"threadTest", "### Thread And Process Test ###", csThreadTest},
	{"matrix", "### Show Matrix on your Monitor ###", csMatrix},
	{"getPIE", "### Get PIE(3.14) Calculation ###", csGetPIE},
	{"dynamicInfo", "### Show Dynamic Memory Information ###", csDynMemInfo},
	{"seqAlloc", "### Sequential Allocation & Free Test ###", csSeqAllocTest},
	{"randAlloc", "### Random Allocation & Free Test ###", csRandAllocTest},
	{"HDDInfo", "### Show HDD Information ###", csHDDInfo},
	{"readSector", "### Read HDD Sector, ex)readSector 0(LBA) 10(Count) ###", csReadSector},
	{"writeSector", "### Write HDD Sector, ex)writeSector 0(LBA) 10(Count) ###", csWriteSector},
	{"HDDMount", "### Mount HDD ###", csMountHDD},
	{"HDDFormat", "### Format HDD ###", csFormatHDD},
	{"fileSystemInfo", "### Show File System Information ###", csFileSystemInfo},
	{"mkFile", "### Make File. ex)mkFile a.txt", csMakeFile},
	{"rmFile", "### Remove File. ex)rmFile a.txt", csRemoveFile},
	{"ls", "### Show Directory ###", csRootDir},
	{"fwrite", "### Write Data to File. ex)fwrite a.txt", csFileWrite},
	{"fread", "### Read Data to File. ex)fread a.txt", csFileRead},
	{"fileIOTest", "### File I/O Test Function ###", csFileIOTest},
	{"cacheTest", "### Cache's Read & Write Party ###", csCacheTest},
	{"flush", "### Flush File System Cache", csCacheFlush},
	{"download", "### Download Data Using Serial. ex)download a.txt", csDownload},
	{"mpInfo", "### Show MP Configuration Table Information", csMPConfigInfo},
	{"startAP", "### Start Application Processor ###", csStartAP},
	{"symmetricIO", "### Start Symmetric I/O Mode ###", csSymmetricIOMode},
	{"irqINTINMapInfo", "### Show IRQ->INTIN Mapping Table ###", csIRQMapTbl},
	{"intProcCnt", "### Show Interrupt Processing Count ###", csInterruptProcCnt},
	{"intLoadBalancing", "### Start Interrupt Load Balancing ###", csInterruptLoadBalancing},
	{"taskLoadBalancing", "### Start Task Load Balancing ###", csTaskLoadBalancing},
	{"changeAffinity", "### Change Task Affinity. ex)changeAffinity 1(ID) 0xFF(Affinity)", csChangeAffinity},
	{"vbeModeInfo", "### Show VBE Mode Information ###", csVBEModeInfo},
};

// 셸 메인 루프
void startShell(void) {
	char buf[SHELL_MAXCMDBUFCNT];
	int bufIdx = 0;
	BYTE key;
	int x, y;

	// 프롬프트 출력
	printF(SHELL_PROMPTMSG);

	while(1) {
		// 키가 수신될 때까지 대기
		key = getCh();

		// Backspace 키 처리
		if(key == KEY_BACKSPACE) {
			if(bufIdx > 0) {
				// 현재 커서 위치를 얻어 한 문자 앞으로 이동한 다음 공백을 출력, 커맨드 버퍼에서 마지막 문자 삭제
				getCursor(&x, &y);
				printXY(x - 1, y, CONSOLE_DEFAULTTEXTCOLOR, " ");
				setCursor(x - 1, y);
				bufIdx--;
			}
		} else if(key == KEY_ENTER) {
			printF("\n");
			if(bufIdx > 0) {
				// 커맨드 버퍼에 있는 명령 실행
				buf[bufIdx] = '\0';
				execCMD(buf);
			}

			// 프롬프트 출력 및 커맨드 버퍼 초기화
			printF("%s", SHELL_PROMPTMSG);
			memSet(buf, '\0', SHELL_MAXCMDBUFCNT);
			bufIdx = 0;
		} else if((key == KEY_LSHIFT) || (key == KEY_RSHIFT) || (key == KEY_CAPSLOCK) || (key == KEY_NUMLOCK) || (key == KEY_SCROLLLOCK)) {
			;
		} else {
			// TAB 외 특수 키 무시 및 TAB 키 공백으로 전환
			if(key == KEY_TAB) key = ' ';

			// 버퍼에 공간이 남아있을 때만 가능
			if(bufIdx < SHELL_MAXCMDBUFCNT) {
				buf[bufIdx++] = key;
				printF("%c", key);
			}
		}
	}
}

// 커맨드 버퍼에 있는 커맨드를 비교해 해당 커맨드 처리하는 함수 수행
void execCMD(const char *buf) {
	int i, idx, bufLen, len, cnt;

	// 공백으로 구분된 커맨드 추출
	bufLen = strLen(buf);
	for(idx = 0; idx < bufLen; idx++) if(buf[idx] == ' ') break;

	// 커맨드 테이블 검사 후 일치하는 커맨드 확인
	cnt = sizeof(gs_cmdTable) / sizeof(SHELLENTRY);
	for(i = 0; i < cnt; i++) {
		len = strLen(gs_cmdTable[i].cmd);
		// 커맨드 길이와 내용 일치 검사
		if((len == idx) && (memCmp(gs_cmdTable[i].cmd, buf, idx) == 0)) {
			gs_cmdTable[i].func(buf + idx + 1);
			break;
		}
	}

	// 리스트에서 못 찾으면 에러 출력
	if(i >= cnt) printF("'%s' is not found.\n", buf);
}

// 파라미터 자료구조 초기화
void initParam(PARAMLIST *list, const char *param) {
	list->buf = param;
	list->len = strLen(param);
	list->nowPosition = 0;
}

// 공백으로 구분된 파라미터 내용과 길이 반환
int getNextParam(PARAMLIST *list, char *param) {
	int i, len;

	// 더 이상 파라미터가 없으면 나감
	if(list->len <= list->nowPosition) return 0;

	// 버퍼 길이만큼 이동하며 공백 검색
	for(i = list->nowPosition; i < list->len; i++) if(list->buf[i] == ' ') break;

	// 파라미터 복사 후 길이 반환
	memCpy(param, list->buf + list->nowPosition, i);
	len = i - list->nowPosition;
	param[len] = '\0';

	// 파라미터 위치 업데이트
	list->nowPosition += len + 1;
	return len;
}

// 도움말 출력
static void csHelp(const char *buf) {
	int i, cnt, x, y, len, maxLen = 0;

	printF("   ============================================================\n");
	printF("   |                    YummyHitOS Shell Help                 |\n");
	printF("   ============================================================\n");
	
	cnt = sizeof(gs_cmdTable) / sizeof(SHELLENTRY);

	// 가장 긴 커맨드 길이 계산
	for(i = 0; i < cnt; i++) {
		len = strLen(gs_cmdTable[i].cmd);
		if(len > maxLen) maxLen = len;
	}

	// 도움말 출력
	for(i = 0; i < cnt; i++) {
		printF("%s", gs_cmdTable[i].cmd);
		getCursor(&x, &y);
		setCursor(maxLen, y);
		printF("  - %s\n", gs_cmdTable[i].help);

		// 목록이 많을 경우 나눠서 보여중
		if((i != 0) && ((i % 20) == 0)) {
			printF("Press any key to continue... ('q' is exit) : ");
			if(getCh() == 'q') {
				printF("\n");
				break;
			}
			printF("\n");
		}
	}
}

// 화면 지움
static void csClear(const char *buf) {
	// 1번째 라인은 디버깅, 미관을 위해 2번째 라인으로 커서 이동
	clearMonitor();
	setCursor(0, 2);
}

// 총 메모리 크기 출력
static void csFree(const char *buf) {
	printF("    %d MB\n", getTotalMemSize());
}

// 문자열로 된 숫자를 숫자로 변환해 출력
static void csStrConvert(const char *buf) {
	char param[100];
	int len, cnt = 0;
	PARAMLIST list;
	long v;

	// 파라미터 초기화
	initParam(&list, buf);

	while(1) {
		// 다음 파라미터 구하고 파라미터 길이가 0이면 없다는 뜻이므로 종료
		len = getNextParam(&list, param);
		if(len == 0) break;

		// 파라미터에 대한 정보 출력 후 16진수|10진수 판단해 변환하여 출력
		printF("Parameter %d = '%s', Length = %d, ", cnt + 1, param, len);

		// 0x로 시작하면 16진수, 그 외에는 10진수로 판단
		if(memCmp(param, "0x", 2) == 0) {
			v = aToi(param + 2, 16);
			printF("HexaDecimale Value = %q\n", v);
		} else {
			v = aToi(param, 10);
			printF("Decimal Value = %d\n", v);
		}
		cnt++;
	}
}

// PC 재부팅
static void csHalt(const char *buf) {
	printF("Cache Flushing ...");
	if(flushFileSystemCache() == TRUE) printF("[  Hit  ]\n");
	else printF("[  Err  ]\n");
	printF("Press any key on your keyboard for reboot PC !\n");
	getCh();
	reBoot();
}

// PIT 컨트롤러 카운터 0 설정
static void csSetTime(const char *buf) {
	char param[100];
	PARAMLIST list;
	long v;
	BOOL term;

	// 파라미터 초기화
	initParam(&list, buf);

	// milli second 추출
	if(getNextParam(&list, param) == 0) {
		printF("ex) setTime 10(ms) 1(term)\n");
		return;
	}
	v = aToi(param, 10);

	// Periodic 추출
	if(getNextParam(&list, param) == 0) {
		printF("ex) setTime 10(ms) 1(term)\n");
		return;
	}
	term = aToi(param, 10);

	initPIT(MSTOCNT(v), term);
	printF("Time = %d ms, Term = %d Change Complete\n", v, term);
}

// PIT 컨트롤러를 직접 사용해 ms 동안 대기
static void csWait(const char *buf) {
	char param[100];
	int len, i;
	PARAMLIST list;
	long ms;

	// 파라미터 초기화
	initParam(&list, buf);
	if(getNextParam(&list, param) == 0) {
		printF("ex) wait 100(ms)\n");
		return;
	}
	ms = aToi(buf, 10);
	printF("### %d ms Sleep Start... ###\n", ms);

	// 인터럽트 비활성화 후 PIT 컨트롤러를 통해 직접 시간 측정
	offInterrupt();
	for(i = 0; i < ms / 30; i++) waitPIT(MSTOCNT(30));
	waitPIT(MSTOCNT(ms % 30));
	onInterrupt();
	printF("### %d ms Sleep Complete ###\n", ms);

	// 타이머 복원
	initPIT(MSTOCNT(1), TRUE);
}

// 타임 스탬프 카운터 읽음
static void csRTSC(const char *buf) {
	QWORD TSC;
	
	TSC = readTSC();
	printF("Time Stamp Counter = %q\n", TSC);
}

// 프로세서 속도 측정
static void csCPUSpeed(const char *buf) {
	int i;
	QWORD last, total = 0;

	printF("### Now Measuring.");

	// 10초 동안 변화한 타임 스탬프 카운터를 이용해 프로세서 속도 측정
	offInterrupt();
	for(i = 0; i < 200; i++) {
		last = readTSC();
		waitPIT(MSTOCNT(50));
		total += readTSC() - last;
		printF(".");
	}
	// 타이머 복원
	initPIT(MSTOCNT(1), TRUE);
	onInterrupt();
	printF("\n### CPU Speed = %d MHz ###\n", total / 10 / 1000 / 1000);
}

// RTC 컨트롤러에 저장된 일자 및 시간 정보 표시
static void csDate(const char *buf) {
	BYTE sec, min, hour, week, day, month;
	WORD year;

	// RTC 컨트롤러에서 시간 및 일자 읽음
	readTime(&hour, &min, &sec);
	readDate(&year, &month, &day, &week);

	printF("Date : %d/%d/%d %s, ", year, month, day, convertWeek(week));
	printF("Time : %d:%d:%d\n", hour, min, sec);
}

// TCB 자료구조와 스택 정의
static TCB gs_task[2] = {0,};
static QWORD gs_stack[1024] = {0,};

// 태스크 전환 테스트 태스크
static void taskTest1(void) {
	BYTE data;
	int i = 0, x = 0, y = 0, margin, j;
	CHARACTER *mon = (CHARACTER*)CONSOLE_VIDEOMEMADDR;
	TCB *runningTask;

	// 자신의 ID 얻어서 화면 오프셋으로 사용
	runningTask = getRunningTask(getAPICID());
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
	taskExit();
}

static void taskTest2(void) {
	int i = 0, offset;
	CHARACTER *mon = (CHARACTER*)CONSOLE_VIDEOMEMADDR;
	TCB *runningTask;
	char data[4] = {'-', '\\', '|', '/'};

	// 자신의 ID를 얻어 화면 오프셋으로 사용
	runningTask = getRunningTask(getAPICID());
	offset = (runningTask->link.id & 0xFFFFFFFF) * 2;
	offset = CONSOLE_WIDTH * CONSOLE_HEIGHT - (offset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));

	while(1) {
		mon[offset].character = data[i % 4];
		mon[offset].color = (offset % 15) + 1;
		i++;
	}
}

// 자신이 수행되는 코어의 ID가 변경될 때마다 자신의 태스크 ID와 코어 ID 출력
static void taskTest3(void) {
	QWORD id, lastTick;
	TCB *runningTask;
	BYTE _id;

	// 자신의 태스크 자료구조 저장
	runningTask = getRunningTask(getAPICID());
	id = runningTask->link.id;
	printF("Test Task 3 Started. Task ID = 0x%q, Local APIC ID = 0x%x\n", id, getAPICID());

	// 현재 수행 중 로컬 APIC ID 저장하고 태스크가 부하 분산되어 다른 코어로 옮겨갔을 때 메시지 출력
	_id = getAPICID();
	while(1) {
		// 이전에 수행되었던 코어와 현재 수행하는 코어가 다르면 메시지 출력
		if(_id != getAPICID()) {
			printF("Core Changed! Task ID = 0x%q, Prev Local APIC ID = 0x%x, Current = 0x%x\n", id, _id, getAPICID());

			// 현재 수행 중인 코어 ID 변경
			_id = getAPICID();
		}

		schedule();
	}
}			

// 태스크 생성하여 멀티태스킹
static void csCreateTask(const char *buf) {
	PARAMLIST list;
	char type[30], cnt[30];
	int i;

	// 파라미터 추출
	initParam(&list, buf);
	getNextParam(&list, type);
	getNextParam(&list, cnt);

	switch(aToi(type, 10)) {
	case 1:
		for(i = 0; i < aToi(cnt, 10); i++) if(createTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)taskTest1, TASK_LOADBALANCING_ID) == NULL) break;
		printF("Task Test 1 %d Created.\n", i);
		break;

	case 2:
	default:
		for(i = 0; i < aToi(cnt, 10); i++) if(createTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)taskTest2, TASK_LOADBALANCING_ID) == NULL) break;
		printF("Task Test 2 %d Created.\n", i);
		break;

	case 3:
		for(i = 0; i < aToi(cnt, 10); i++) {
			if(createTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)taskTest3, TASK_LOADBALANCING_ID) == NULL) break;
			schedule();
		}
		printF("Task3 %d Created\n", i);
		break;
	}
}

// 태스크 우선순위 변경
static void csChangePriority(const char *buf) {
	PARAMLIST list;
	char id[30], priority[30];
	QWORD _id;
	BYTE _priority;

	// 파라미터 추출
	initParam(&list, buf);
	getNextParam(&list, id);
	getNextParam(&list, priority);

	// 태스크 우선순위 변경
	if(memCmp(id, "0x", 2) == 0) _id = aToi(id + 2, 16);
	else _id = aToi(id, 10);

	_priority = aToi(priority, 10);

	printF("### Change Task Priority ID [0x%q] Priority[%d] ", _id, _priority);
	if(alterPriority(_id, _priority) == TRUE) printF("Success !!\n");
	else printF("Fail...\n");
}

// 현재 생성된 모든 태스크 정보 출력
static void csTaskList(const char *buf) {
	int i, cnt = 0, totalCnt = 0, len, processorCnt;
	char _buf[20];
	TCB *tcb;

	// 코어 수만큼 루프를 돌며 각 스케줄러에 있는 태스크 수 더함
	processorCnt = getProcessorCnt();

	for(i = 0; i < processorCnt; i++) totalCnt += getTaskCnt(i);

	printF("   ================== Task Total Count [%d] ===================\n", totalCnt);

	// 코어가 두 개 이상이면 각 스케줄러별 개수 출력
	if(processorCnt > 1) {
		for(i = 0; i < processorCnt; i++) {
			if((i != 0) && ((i % 4) == 0)) printF("\n");
			sprintF(_buf, "Core %d : %d", i, getTaskCnt(i));
			printF(_buf);

			// 출력하고 남은 공간 모두 스페이스바로 채움
			len = 19 - strLen(_buf);
			memSet(_buf, ' ', len);
			_buf[len] = '\0';
			printF(_buf);
		}

		printF("\nPress any key to continue... ('q' is exit) : ");
		if(getCh() == 'q') {
			printF("\n");
			return;
		}
		printF("\n\n");
	}

	for(i = 0; i < TASK_MAXCNT; i++) {
		// TCB 구해서 TCB가 사용 중이면 ID 출력
		tcb = getTCB(i);
		if((tcb->link.id >> 32) != 0) {
			// 태스크가 6개 출력될 때마다 태스크 정보 표시할지 여부 확인
			if((cnt != 0) && ((cnt % 6) == 0)) {
				printF("Press any key to continue... ('q' is exit) : ");
				if(getCh() == 'q') {
					printF("\n");
					break;
				}
				printF("\n");
			}
			printF("[%d] Task ID[0x%Q], Priority[%d], Flags[0x%Q], Thread[%d]\n", 1 + cnt++, tcb->link.id, GETPRIORITY(tcb->flag), tcb->flag, getListCnt(&(tcb->childThreadList)));
			printF("     Core ID[0x%X] CPU Affinity[0x%x]\n", tcb->_id, tcb->affinity);
			printF("     Parent PID[0x%Q], Memory Address[0x%Q], Size[0x%Q]\n", tcb->parentProcID, tcb->memAddr, tcb->memSize);
		}
	}
}

// 태스크 종료
static void csTaskill(const char *buf) {
	PARAMLIST list;
	char id[30];
	QWORD _id;
	TCB *tcb;
	int i;

	// 파라미터 추출
	initParam(&list, buf);
	getNextParam(&list, id);

	// 태스크 종료
	if(memCmp(id, "0x", 2) == 0) _id = aToi(id + 2, 16);
	else _id = aToi(id, 10);

	// 특정 ID만 종료하는 경우
	if(_id != 0xFFFFFFFF) {
		tcb = getTCB(GETTCBOFFSET(_id));
		_id = tcb->link.id;

		// 시스템 테스트는 제외
		if(((_id >> 32) != 0) && ((tcb->flag & TASK_FLAGS_SYSTEM) == 0x00)) {
			printF("Kill Task ID [0x%q] ", _id);
			if(taskFin(_id) == TRUE) printF("Success !!\n");
			else printF("Fail...\n");
		} else printF("Task does not exist or task is system task.\n");
	} else { // 콘솔 셸과 유후 태스크 제외 모든 태스크 종료
		for(i = 0; i < TASK_MAXCNT; i++) {
			tcb = getTCB(i);
			_id = tcb->link.id;

			// 시스템 테스트는 삭제 목록에서 제외
			if(((_id >> 32) != 0) && ((tcb->flag & TASK_FLAGS_SYSTEM) == 0x00)) {
				printF("Kill Task ID [0x%q] ", _id);
				if(taskFin(_id) == TRUE) printF("Success !!\n");
				else printF("Fail...\n");
			}
		}
	}
}

// 프로세서 사용률 표시
static void csCPULoad(const char *buf) {
	int i, len;
	char _buf[50];

	printF("   ====================== Processor Load ======================\n");

	// 각 코어별 부하 출력
	for(i = 0; i < getProcessorCnt(); i++) {
		if((i != 0) && ((i % 4) == 0)) printF("\n");
		sprintF(_buf, "Core %d : %d%%", i, getProcessorLoad(i));
		printF("%s", _buf);

		// 출력하고 남은 공간 모두 스페이스바로 채움
		len = 19 - strLen(_buf);
		memSet(_buf, ' ', len);
		_buf[len] = '\0';
		printF(_buf);
	}
	printF("\n");
}

// 뮤텍스 테스트용 뮤텍스와 변수
static MUTEX gs_mut;
static volatile QWORD gs_add;

// 뮤텍스 테스트 태스크
static void printNumTask(void) {
	int i, j;
	QWORD tickCnt;

	// 50ms 정도 대기하여 콘솔 셸이 출력하는 메시지와 겹치지 않도록 함
	tickCnt = getTickCnt();
	while((getTickCnt() - tickCnt) < 50) schedule();

	// 루프 돌면서 숫자 출력
	for(i = 0; i < 5; i++) {
		_lock(&gs_mut);
		printF("Task ID [0x%Q] Value[%d]\n", getRunningTask(getAPICID())->link.id, gs_add);
		gs_add += 1;
		_unlock(&gs_mut);

		// 프로세서 소모를 늘리려고 추가한 테스트 코드
		for(j = 0; j < 30000; j++);
	}

	// 모든 태스크가 종료할 때까지 1초(100ms) 정도 대기
	tickCnt = getTickCnt();
	while((getTickCnt() - tickCnt) < 1000) schedule();

	// 태스크 종료
	taskExit();
}

// 뮤텍스 테스트하는 태스크 생성
static void csMutexTest(const char *buf) {
	int i;
	gs_add = 1;

	// 뮤텍스 초기화
	initMutex(&gs_mut);

	for(i = 0; i < 3; i++) createTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)printNumTask, getAPICID());
	printF("Wait Util %d Task Finishing...\n", i);
	getCh();
}

// 태스크 2를 자신의 쓰레드로 생성하는 태스크
static void createThreadTask(void) {
	int i;

	for(i = 0; i < 3; i++) createTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)taskTest2, TASK_LOADBALANCING_ID);
	while(1) _sleep(1);
}

// 쓰레드를 테스트하는 태스크 생성
static void csThreadTest(const char *buf) {
	TCB *proc;

	proc = createTask(TASK_FLAGS_LOW | TASK_FLAGS_PROC, (void*)0xEEEEEEEE, 0x1000, (QWORD)createThreadTask, TASK_LOADBALANCING_ID);

	if(proc != NULL) printF("Process [0x%Q] Create Success !\n", proc->link.id);
	else printF("Process Create Fail...\n");
}

// 난수 발생을 위한 변수
static volatile QWORD gs_matrixValue = 0;

// 임의의 난수 반환
QWORD _rand(void) {
	gs_matrixValue = (gs_matrixValue * 412153 + 5571031) >> 16;
	return gs_matrixValue;
}

// 철자를 흘러내리게 하는 쓰레드
static void dropMatrixChar(void) {
	int x, y, i;
	char txt[2] = {0,};

	x = _rand() % CONSOLE_WIDTH;

	while(1) {
		_sleep(_rand() % 20);
		if((_rand() % 20) < 15) {
			txt[0] = ' ';
			for(i = 0; i < CONSOLE_HEIGHT - 1; i++) {
				printXY(x, i, 0x0A, txt);
				_sleep(50);
			}
		} else {
			for(i = 0; i < CONSOLE_HEIGHT - 1; i++) {
				txt[0] = i + _rand();
				printXY(x, i, 0x0A, txt);
				_sleep(50);
			}
		}
	}
}

// 쓰레드 생성으로 매트릭스 화면 구성 프로세스
static void matrixProc(void) {
	int i;

	for(i = 0; i < 300; i++) {
		if(createTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)dropMatrixChar, TASK_LOADBALANCING_ID) == NULL) break;
		_sleep(_rand() % 5 + 5);
	}
	getCh();
}

// 매트릭스 명령
static void csMatrix(const char *buf) {
	TCB *proc;

	proc = createTask(TASK_FLAGS_LOW | TASK_FLAGS_PROC, (void*)0xE00000, 0xE00000, (QWORD)matrixProc, TASK_LOADBALANCING_ID);

	if(proc != NULL) {
		clearMatrix();

		// 태스크 종료시까지 대기
		while((proc->link.id >> 32) != 0) _sleep(100);
	} else 	printXY(0, 0, 0x0A, "Matrix Process Create Fail...\n");
}

// FPU를 테스트하는 태스크
static void fpuTest(void) {
	double dV1, dV2;
	TCB *runningTask;
	QWORD cnt = 0, randValue;
	int i, offset;
	char data[4] = { '-', '\\', '|', '/' };
	CHARACTER *mon = (CHARACTER*)CONSOLE_VIDEOMEMADDR;

	runningTask = getRunningTask(getAPICID());

	// 자신의 ID를 얻어 화면 오프셋으로 사용
	offset = (runningTask->link.id & 0xFFFFFFFF) * 2;
	offset = (CONSOLE_WIDTH * CONSOLE_HEIGHT) - (offset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));

	// 루프를 무한히 반복해 동일 계산 수행
	while(1) {
		dV1 = 1;
		dV2 = 1;
		// 테스트를 위해 동일 계산 2번 반복 실행
		for(i = 0; i < 100; i++) {
			randValue = _rand();
			dV1 *= (double)randValue;
			dV2 *= (double)randValue;

			_sleep(1);

			randValue = _rand();
			dV1 /= (double)randValue;
			dV2 /= (double)randValue;
		}

		if(dV1 != dV2) {
			printF("Value is Different !! [%f] != [%f]\n", dV1, dV2);
			break;
		}

		cnt++;

		// 회전하는 바람개비 표시
		mon[offset].character = data[cnt % 4];
		mon[offset].color = (offset % 15) + 1;
	}
}

// 원주율 계산
static void csGetPIE(const char *buf) {
	double res;
	int i;
	printF("PIE Calculation Test\n");
	printF("Result: 355 / 113 = ");
	res = (double)355 / 113;

	printF("%d.%d%d\n", (QWORD)res, ((QWORD)(res * 10) % 10), ((QWORD)(res * 100) % 10));

	// 실수를 계산하는 태스크 생성
	for(i = 0; i < 100; i++) createTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)fpuTest, TASK_LOADBALANCING_ID);
}

// 동적 메모리 정보 표시
static void csDynMemInfo(const char *buf) {
	QWORD startAddr, totalSize, metaSize, usedSize;

	getDynMemInfo(&startAddr, &totalSize, &metaSize, &usedSize);

	printF("   ================ Dynamic Memory Information ================\n");
	printF("Start Address:	[0x%Q]\n", startAddr);
	printF("Total Size:	[0x%Q]byte, [%d]MB\n", totalSize, totalSize / 1024 / 1024);
	printF("Meta Size:	[0x%Q]byte, [%d]KB\n", metaSize, metaSize / 1024);
	printF("Used Size:	[0x%Q]byte, [%d]KB\n", usedSize, usedSize / 1204);\
}

// 모든 블록 리스트의 블록을 순차적으로 할당하고 해제하는 테스트
static void csSeqAllocTest(const char *buf) {
	DYNMEM *mem;
	long i, j, k;
	QWORD *_buf;

	printF("   ================ Dynamic Memory Information ================\n");
	mem = getDynMemManager();

	for(i = 0; i < mem->maxLvCnt; i++) {
		printF("Block List [%d] Test Start\n", i);
		printF("Allocation And Compare: ");

		// 모든 블록을 할당받아 값을 채운 후 검사
		for(j = 0; j < (mem->smallBlockCnt >> i); j++) {
			_buf = allocMem(DYNMEM_MIN_SIZE << i);
			if(_buf == NULL) {
				printF("\nAllocation Fail...\n");
				return;
			}

			// 값을 채운 후 다시 검사
			for(k = 0; k < (DYNMEM_MIN_SIZE << i) / 8; k++) _buf[k] = k;

			for(k = 0; k < (DYNMEM_MIN_SIZE << i) / 8; k++) if(_buf[k] != k) {
				printF("\nCompare Fail...\n");
				return;
			}
			// 진행 과정을 . 으로 표시
			printF(".");
		}

		printF("\nFree: ");
		// 할당 받은 블록 모두 반환
		for(j = 0; j < (mem->smallBlockCnt >> i); j++) {
			if(freeMem((void*)(mem->startAddr + (DYNMEM_MIN_SIZE << i) * j)) == FALSE) {
				printF("\nFree Fail...\n");
				return;
			}
			// 진행 과정을 . 으로 표시
			printF(".");
		}
		printF("\n");
	}
	printF("Sequential Allocation Test is Finished Successfully !!\n");
}

// 임의로 메모리 할당 후 해제하는 것을 반복하는 태스크
static void randAllocTask(void) {
	TCB *task;
	QWORD memSize;
	char _buf[CONSOLE_WIDTH - 13];	// 가로길이 총 80에서(CONSOLE_WIDTH == 80) print할 x좌표만큼 빼준다!
	BYTE *allocBuf;
	int i, j, y;

	task = getRunningTask(getAPICID());
	y = (task->link.id) % 15 + 9;

	for(j = 0; j < 10; j++) { // 1KB ~ 32M까지 할당
		do {
			memSize = ((_rand() % (32 * 1024)) + 1) * 1024;
			allocBuf = allocMem(memSize);

			// 만일 버퍼를 할당받지 못하면 다른 태스크가 메모리를 사용할 수 있으니 잠시 대기 후 재시도
			if(allocBuf == 0) _sleep(1);
		} while(allocBuf == 0);

		sprintF(_buf, "|Address: [0x%Q] Size: [0x%Q] Allocation Success", allocBuf, memSize);
		// 자신의 ID를 Y좌표로 하여 데이터 출력
		printXY(12, y, CONSOLE_DEFAULTTEXTCOLOR, _buf);
		_sleep(200);

		// 버퍼를 반으로 나눠 랜덤한 데이터를 똑같이 채움
		sprintF(_buf, "|Address: [0x%Q] Size: [0x%Q] Data Write...     ", allocBuf, memSize);
		printXY(12, y, CONSOLE_DEFAULTTEXTCOLOR, _buf);
		for(i = 0; i < memSize / 2; i++) {
			allocBuf[i] = _rand() & 0xFF;
			allocBuf[i + (memSize / 2)] = allocBuf[i];
		}
		_sleep(200);

		// 채운 데이터가 정상적인지 다시 확인
		sprintF(_buf, "|Address: [0x%Q] Size: [0x%Q] Data Verify...    ", allocBuf, memSize);
		printXY(12, y, CONSOLE_DEFAULTTEXTCOLOR, _buf);
		for(i = 0; i < memSize / 2; i++) if(allocBuf[i] != allocBuf[i + (memSize / 2)]) {
			printF("Task ID[0x%Q] Verify Fail...\n", task->link.id);
			taskExit();
		}
		freeMem(allocBuf);
		_sleep(200);
	}
	taskExit();
}

// 태스크를 여러 개 생성해 임의의 메모리 할당 후 해제하는 것 반복하는 테스트
static void csRandAllocTest(const char *buf) {
	int i;
		// Ram Disk를 이용해도 에러나는 부분. 동적 메모리가 0 Byte여야하는데, 비어있지 않아 오류나는 것 같음.
	for(i = 0; i < 1000; i++) createTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD, 0, 0, (QWORD)randAllocTask, TASK_LOADBALANCING_ID);
}

// 하드 디스크 정보 표시
static void csHDDInfo(const char *buf) {
	HDDINFO hdd;
	char _buf[80];

	// 하드 디스크 정보 읽음
	if(getHDDInfo(&hdd) == FALSE) {
		printF("HDD Information Read Fail...\n");
		return;
	}

	printF("   ============== Primary Master HDD Information ==============\n");

	// 모델 번호 출력
	memCpy(_buf, hdd.modelNum, sizeof(hdd.modelNum));
	_buf[sizeof(hdd.modelNum) - 1] = '\0';
	printF("Model Number:\t %s\n", _buf);

	// 시리얼 번호 출력
	memCpy(_buf, hdd.serialNum, sizeof(hdd.serialNum));
	_buf[sizeof(hdd.serialNum) - 1] = '\0';
	printF("Serial Number:\t %s\n", _buf);

	// 헤드, 실린더, 실린더 당 섹터 수 출력
	printF("Head Count:\t %d\n", hdd.headNum);
	printF("Cylinder Count:\t %d\n", hdd.cylinderNum);
	printF("Sector Count:\t %d\n", hdd.perSectorNum);

	// 총 섹터 수 출력
	printF("Total Sector:\t %d Secotr, %dMB\n", hdd.totalSector, hdd.totalSector / 2 / 1024);
}

// 하드 디스크에서 파라미터로 넘어온 LBA 어드레스부터 섹터 수만큼 읽음
static void csReadSector(const char *buf) {
	PARAMLIST list;
	char lba[50], sectorCnt[50];
	DWORD _lba;
	int _sectorCnt, i, j;
	char *_buf;
	BYTE data;
	BOOL _exit = FALSE;

	// 파라미터 리스트 초기화 후 LBA 어드레스와 섹터 수 추출
	initParam(&list, buf);
	if((getNextParam(&list, lba) == 0) || (getNextParam(&list, sectorCnt) == 0)) {
		printF("ex)readSector 0(LBA) 10(Count)\n");
		return;
	}
	_lba = aToi(lba, 10);
	_sectorCnt = aToi(sectorCnt, 10);

	// 섹터 수만큼 메모리 할당받아 읽기 수행
	_buf = allocMem(_sectorCnt * 512);
	if(readHDDSector(TRUE, TRUE, _lba, _sectorCnt, _buf) == _sectorCnt) {
		printF("LBA [%d], [%d] Sector Read Success !!", _lba, _sectorCnt);
		// 데이터 버퍼 내용 출력
		for(j = 0; j < _sectorCnt; j++) {
			for(i = 0; i < 512; i++) {
				if(!((j == 0) && (i == 0)) && ((i % 256) == 0)) {
					printF("\nPress any key to continue... ('q' is exit) : ");
					if(getCh() == 'q') {
						_exit = TRUE;
						break;
					}
				}
				if((i % 16) == 0) printF("\n[LBA:%d, Offset:%d]\t| ", _lba + j, i);

				// 모두 두 자리로 표시하려고 16보다 작은 경우 0추가
				data = _buf[j * 512 + i] & 0xFF;
				if(data < 16) printF("0");
				printF("%X ", data);
			}
			if(_exit == TRUE) break;
		}
		printF("\n");
	} else printF("Read Fail...\n");

	freeMem(_buf);
}

// 하드 디스크에서 파라미터로 넘어온 LBA 어드레스부터 섹터 수만큼 씀
static void csWriteSector(const char *buf) {
	PARAMLIST list;
	char lba[50], sectorCnt[50];
	DWORD _lba;
	int _sectorCnt, i, j;
	char *_buf;
	BOOL _exit = FALSE;
	BYTE data;
	static DWORD ls_writeCnt = 0;

	// 파라미터 리스트를 초기화해 LBA 어드레스와 섹터 수 추출
	initParam(&list, buf);
	if((getNextParam(&list, lba) == 0) || (getNextParam(&list, sectorCnt) == 0)) {
		printF("ex)writeSector 0(LBA) 10(Count)\n");
		return;
	}
	_lba = aToi(lba, 10);
	_sectorCnt = aToi(sectorCnt, 10);

	ls_writeCnt++;
	// 버퍼를 할당받아 데이터 채움. 패턴은 4바이트의 LBA 어드레스와 4바이트 쓰기가 수행된 횟수로 생성
	_buf = allocMem(_sectorCnt * 512);
	for(j = 0; j < _sectorCnt; j++) for(i = 0; i < 512; i += 8) {
		*(DWORD*)&(_buf[j * 512 + i]) = _lba + j;
		*(DWORD*)&(_buf[j * 512 + i + 4]) = ls_writeCnt;
	}

	// 쓰기 수행
	if(writeHDDSector(TRUE, TRUE, _lba, _sectorCnt, _buf) != _sectorCnt) {
		printF("Write Fail...\n");
		return;
	}
	printF("LBA [%d], [%d] Sector Write Success !!\n", _lba, _sectorCnt);

	// 데이터 버퍼의 내용 출력
	for(j = 0; j < _sectorCnt; j++) {
		for(i = 0; i < 512; i++) {
			if(!((j == 0) && (i == 0)) && ((i % 256) == 0)) {
				printF("\nPress any key to continue... ('q' is exit) : ");
				if(getCh() == 'q') {
					_exit = TRUE;
					break;
				}
			}
			if((i % 16) == 0) printF("\n[LBA:%d, Offset:%d]\t| ", _lba + j, i);

			// 모두 두 자리로 표시하기위해 16보다 작은 경우 0 추가
			data = _buf[j * 512 + i] & 0xFF;
			if(data < 16) printF("0");
			printF("%X ", data);
		}
		if(_exit == TRUE) break;
	}
	printF("\n");
	freeMem(_buf);
}

// 하드 디스크 연결
static void csMountHDD(const char *buf) {
	if(_mount() == FALSE) {
		printF("HDD Mount Fail...\n");
		return;
	}
	printF("HDD Mount Success !!\n");
}

// 하드 디스크에 파일 시스템 생성(포맷)
static void csFormatHDD(const char *buf) {
	if(_format() == FALSE) {
		printF("HDD Format Fail...\n");
		return;
	}
	printF("HDD Format Success !!\n");
}

// 파일 시스템 정보 표시
static void csFileSystemInfo(const char *buf) {
	FILESYSTEMMANAGER manager;

	getFileSystemInfo(&manager);

	printF("   ================= File System Information ==================\n");
	printF("Mounted:\t\t\t\t %d\n", manager.mnt);
	printF("Reserved Sector Count:\t\t\t %d Sector\n", manager.reserved_sectorCnt);
	printF("Cluster Link Table Start Address:\t %d Sector\n", manager.linkStartAddr);
	printF("Cluster Link Table Size:\t\t %d Sector\n", manager.linkAreaSize);
	printF("Data Area Start Address:\t\t %d Sector\n", manager.dataStartAddr);
	printF("Total Cluster Count:\t\t\t %d Cluster\n", manager.totalClusterCnt);
}

// 루트 디렉터리에 빈 파일 생성
static void csMakeFile(const char *buf) {
	PARAMLIST list;
	char name[50];
	int len, i;
	DWORD cluster;
	FILE *file;

	// 파라미터 리스트 초기화해 파일 이름 추출
	initParam(&list, buf);
	len = getNextParam(&list, name);
	name[len] = '\0';
	if((len > (FILESYSTEM_MAXFILENAMELEN - 1)) || (len == 0)) {
		printF("Too Long or Too Short File Name\n");
		return;
	}

	file = fopen(name, "w");
	if(file == NULL) {
		printF("Make File Fail...\n");
		return;
	}
	fclose(file);
	printF("Make a File Success !!\n");
}

// 루트 디렉터리에서 파일 삭제
static void csRemoveFile(const char *buf) {
	PARAMLIST list;
	char name[50];
	int len;

	// 파라미터 리스트 초기화해 파일 이름 추출
	initParam(&list, buf);
	len = getNextParam(&list, name);
	name[len] = '\0';

	if((len > (FILESYSTEM_MAXFILENAMELEN -1)) || (len == 0)) {
		printF("Too Long or Too Short File Name\n");
		return;
	}

	if(fremove(name) != 0) {
		printF("File Not Found or File Opened...\n");
		return;
	}

	printF("Remove a File Success !!\n");
}

// 루트 디렉터리 파일 목록 표시
static void csRootDir(const char *buf) {
	DIR *dir;
	int cnt = 0, totalCnt = 0;
	struct dirent *entry;
	char *_buf[CONSOLE_WIDTH - 5], tmp[50];	// _buf의 크기가 CONSOLE_WIDTH - 5인 이유는 현재 우리의 OS는 총 80만큼의 가로길이이고
						// 아래 코드를 보면 미관을 위해 4칸을 띄어준 후 %s를 통해 출력하므로 5만큼 빼준다!
	DWORD totalByte = 0, usedClusterCnt = 0;
	FILESYSTEMMANAGER manager;

	// 파일 시스템 정보를 얻음
	getFileSystemInfo(&manager);

	// 루트 디렉터리 오픈
	dir = dirOpen("/");
	if(dir == NULL) {
		printF("Root Directory Open Fail...\n");
		return;
	}

	// 먼저 루프 돌며 디렉터리에 있는 파일 개수와 전체 파일이 사용한 크기 계산
	while(1) {
		// 디렉터리에서 엔트리 하나 읽음
		entry = dirRead(dir);
		// 더 이상 파일이 없으면 나감
		if(entry == NULL) break;
		totalCnt++;
		totalByte += entry->size;

		// 실제로 사용된 클러스터 개수를 계산
		if(entry->size == 0) usedClusterCnt++;	// 크기가 0이라도 클러스터 1개는 할당되어 있음
		else usedClusterCnt += (entry->size + (FILESYSTEM_CLUSTER_SIZE - 1)) / FILESYSTEM_CLUSTER_SIZE;
	}

	// 실제 파일의 내용을 표시하는 루프
	dirRewind(dir);
	while(1) {
		// 디렉터리에서 엔트리 하나를 읽음
		entry = dirRead(dir);
		// 더 이상 파일이 없으면 나감
		if(entry == NULL) break;

		// 전부 공백으로 초기화한 후 각 위치에 값을 대입
		memSet(_buf, ' ', CONSOLE_WIDTH - 6);
		_buf[CONSOLE_WIDTH - 6] = '\0';

		// 파일 이름 삽입
		memCpy(_buf, entry->d_name, strLen(entry->d_name));
		// 파일 길이 삽입
		sprintF(tmp, "%d Byte", entry->size);
		memCpy(_buf + 3, tmp, strLen(tmp));
		// 파일 시작 클러스터 삽입
		sprintF(tmp, "0x%X Cluster", entry->startClusterIdx);
		memCpy(_buf + 6, tmp, strLen(tmp));
		printF("    %s\n", _buf);

		if((cnt != 0) && (cnt % 20) == 0) {
			printF("Press any key to continue... ('q' is exit) : ");
			if(getCh() == 'q') {
				printF("\n");
				break;
			}
		}
		cnt++;
	}

	// 총 파일 개수와 파일 총 크기 출력
	printF("\t\tTotal File Count: %d\n", totalCnt);
	printF("\t\tTotal File Size: %d KByte (%d Cluster)\n", totalByte, usedClusterCnt);

	// 남은 클러스터 수를 이용해 여유 공간 출력
	printF("\t\tFree Space: %d KByte (%d Cluster)\n", (manager.totalClusterCnt - usedClusterCnt) * FILESYSTEM_CLUSTER_SIZE / 1024, manager.totalClusterCnt - usedClusterCnt);

	// 디렉터리를 닫음
	dirClose(dir);
}

// 파일을 생성해 키보드로 입력된 데이터 씀
static void csFileWrite(const char *buf) {
	PARAMLIST list;
	char name[50];
	int len, enterCnt = 0;
	FILE *fp;
	BYTE key;

	// 파라미터 리스트를 초기화하여 파일 이름을 추출
	initParam(&list, buf);
	len = getNextParam(&list, name);
	name[len] = '\0';
	if((len > (FILESYSTEM_MAXFILENAMELEN - 1)) || (len == 0)) {
		printF("Too Long or Too Short File Name\n");
		return;
	}

	// 파일 생성
	fp = fopen(name, "w");
	if(fp == NULL) {
		printF("%s File Open Fail...\n", name);
		return;
	}

	// 엔터 키가 연속 3번 눌러질 때까지 내용을 파일에 씀
	while(1) {
		key = getCh();
		// 엔터 키면 연속 3번 눌러졌는가 확인 후 루프를 빠져나감
		if(key == KEY_ENTER) {
			enterCnt++;
			if(enterCnt >= 3) break;
		} else enterCnt = 0;	// 엔터 키가 아니면 엔터 키 입력 횟수 초기화
		printF("%c", key);
		if(fwrite(&key, 1, 1, fp) != 1) {
			printF("File Write Fail...\n");
			break;
		}
	}

	printF("Write File Success !!\n");
	fclose(fp);
}

// 파일을 열어 데이터를 읽음
static void csFileRead(const char *buf) {
	PARAMLIST list;
	char name[50];
	int len, enterCnt = 0;
	FILE *fp;
	BYTE key;

	// 파라미터 리스트를 초기화해 파일 이름을 추출
	initParam(&list, buf);
	len = getNextParam(&list, name);
	name[len] = '\0';
	if((len > (FILESYSTEM_MAXFILENAMELEN - 1)) || (len == 0)) {
		printF("Too Long or Too Short File Name\n");
		return;
	}

	// 파일 생성
	fp = fopen(name, "r");
	if(fp == NULL) {
		printF("%s File Open Fail...\n", name);
		return;
	}

	// 파일 끝까지 출력하는 것 반복
	while(1) {
		if(fread(&key, 1, 1, fp) != 1) break;
		printF("%c", key);

		// 만약 엔터 키면 엔터 키 횟수를 증가시키고 20라인까지 출력했으면 더 출력할지 여부 물어봄
		if(key == KEY_ENTER) {
			enterCnt++;
			if((enterCnt != 0) && ((enterCnt % 20) == 0)) {
				printF("Press any key to continue... ('q' is exit) : ");
				if(getCh() == 'q') {
					printF("\n");
					break;
				}
				printF("\n");
				enterCnt = 0;
			}
		}
	}
	fclose(fp);
}

// 파일 I/O 관련된 기능을 테스트
static void csFileIOTest(const char *buf) {
	FILE *file;
	BYTE *_buf, _tmpBuf[1024];
	int i, j;
	DWORD randOffset, byteCnt, maxSize;

	printF("   ================== File I/O Function Test ==================\n");

	// 4MB의 버퍼 할당
	maxSize = 4 * 1024 * 1024;
	_buf = allocMem(maxSize);
	if(_buf == NULL) {
		printF("Memory Allocation Fail...\n");
		return;
	}
	// 테스트용 파일 삭제
	fremove("fileIOTest.bin");

	// 파일 열기 테스트
	printF("1. File Open Fail Test ....");
	// r 옵션은 파일을 생성하지 않으니 테스트 파일이 없으면 NULL이 되어야 함
	file = fopen("fileIOTest.bin", "r");
	if(file == NULL) printF("[  Hit  ]\n");
	else {
		printF("[  Err  ]\n");
		fclose(file);
	}

	// 파일 생성 테스트
	printF("2. Make a File Test .......");
	// w 옵션은 파일을 생성하므로 정상적으로 핸들이 반환되어야 함
	file = fopen("fileIOTest.bin", "w");
	if(file != NULL) {
		printF("[  Hit  ]\n");
		printF("    File Handle [0x%Q]\n", file);
	} else printF("[  Err  ]\n");

	// 순차적 영역 쓰기 테스트
	printF("3. Sequential Write Test(Cluster Size) .............");
	// 열린 핸들로 쓰기 수행
	for(i = 0; i < 100; i++) {
		memSet(_buf, i, FILESYSTEM_CLUSTER_SIZE);
		if(fwrite(_buf, 1, FILESYSTEM_CLUSTER_SIZE, file) != FILESYSTEM_CLUSTER_SIZE) {
			printF("[  Err  ]\n");
			printF("    %d Cluster Error\n", i);
			break;
		}
	}
	if(i >= 100) printF("[  Hit  ]\n");

	// 순차적인 영역 읽기 테스트
	printF("4. Sequential Read and Verify Test(Cluster Size) ...");
	// 파일 처음으로 이동
	fseek(file, -100 * FILESYSTEM_CLUSTER_SIZE, SEEK_END);

	// 열린 핸들로 읽기 수행 후 데이터 검증
	for(i = 0; i < 100; i++) {
		// 파일 읽음
		if(fread(_buf, 1, FILESYSTEM_CLUSTER_SIZE, file) != FILESYSTEM_CLUSTER_SIZE) {
			printF("[  Err  ]\n");
			return;
		}

		// 데이터 검사
		for(j = 0; j < FILESYSTEM_CLUSTER_SIZE; j++) if(_buf[j] != (BYTE)i) {
			printF("[  Err  ]\n");
			printF("    %d Cluster Error. [%X] != [%X]\n", i, _buf[j], (BYTE)i);
			break;
		}
	}
	if(i >= 100) printF("[  Hit  ]\n");

	// 임의의 영역 쓰기 테스트
	printF("5. Random Write Test ...............\n");

	// 버퍼를 모두 0으로 채움
	memSet(_buf, 0, maxSize);
	// 여기 저기에 옮겨다니면서 데이터를 쓰고 검증
	// 파일의 내용을 읽어 버퍼로 복사
	fseek(file, -100 * FILESYSTEM_CLUSTER_SIZE, SEEK_CUR);
	fread(_buf, 1, maxSize, file);

	// 임의의 위치로 옮기며 데이터를 파일과 버퍼에 씀
	for(i = 0; i < 100; i ++) {
		byteCnt = (_rand() % (sizeof(_tmpBuf) - 1)) + 1;
		randOffset = _rand() % (maxSize - byteCnt);
		printF("    [%d] Offset [%d] Byte [%d] ...................", i, randOffset, byteCnt);

		// 파일 포인터 이동
		fseek(file, randOffset, SEEK_SET);
		memSet(_tmpBuf, i, byteCnt);

		// 데이터 씀
		if(fwrite(_tmpBuf, 1, byteCnt, file) != byteCnt) {
			printF("[  Hit  ]\n");
			break;
		} else printF("[  Err  ]\n");

		memSet(_buf + randOffset, i, byteCnt);
	}

	// 맨 마지막으로 이동해 1바이트 써서 파일 크기를 4MB로 만듬
	fseek(file, maxSize - 1, SEEK_SET);
	fwrite(&i, 1, 1, file);
	_buf[maxSize - 1] = (BYTE)i;

	// 임의의 영역 읽기 테스트
	printF("6. Random Read And Verify Test .....\n");
	// 임의의 위치로 옮기며 파일에서 데이터를 읽어 버퍼 내용과 비교
	for(i = 0; i < 100; i++) {
		byteCnt = (_rand() % (sizeof(_tmpBuf) -  1)) + 1;
		randOffset = _rand() % ((maxSize) - byteCnt);
		printF("    [%d] Offset [%d] Byte [%d] ...................", i, randOffset, byteCnt);

		// 파일 포인터 이동
		fseek(file, randOffset, SEEK_SET);

		// 데이터 읽음
		if(fread(_tmpBuf, 1, byteCnt, file) != byteCnt) {
			printF("[  Err  ]\n");
			printF("    %d Read Fail...\n", randOffset);
			break;
		}

		// 버퍼와 비교
		if(memCmp(_buf + randOffset, _tmpBuf, byteCnt) != 0) {
			printF("[  Err  ]\n");
			printF("    %d Compare Fail\n", randOffset);
			break;
		}
		printF("[  Hit  ]\n");
	}

	// 다시 순차적 영역 읽기 테스
	printF("7. Sequential Write, Read and Verify Test(1024 Byte)\n");
	// 파일의 처음으로 이동
	fseek(file, -100 * maxSize, SEEK_CUR);

	// 열린 핸들로 쓰기 수행. 앞부분 2MB만 씀
	for(i = 0; i < (2 * 1024 * 1024 / 1024) ; i++) {
		printF("    [%d] Offset [%d] Byte [%d] Write .............", i, i * 1024, 1024);

		// 1024바이트씩 파일 읽음
		if(fread(_tmpBuf, 1, 1024, file) != 1024) {
			printF("[  Err  ]\n");
			return;
		}

		if(memCmp(_buf + (i * 1024), _tmpBuf, 1024) != 0) {
			printF("[  Err  ]\n");
			break;
		} else printF("[  Hit  ]\n");
	}

	// 파일 삭제 실패 테스트
	printF("8. File Remove Fail Test ..");
	// 파일이 열려있는 상태이므로 지울 수 없으니 실패
	if(fremove("fileIOTest.bin" != 0)) printF("[  Hit  ]\n");
	else printF("[  Err  ]\n");

	// 파일 닫기 테스트
	printF("9. File Close Test ........");
	// 파일이 정상적으로 닫혀야 함
	if(fclose(file) == 0) printF("[  Hit  ]\n");
	else printF("[  Err  ]\n");

	// 파일 삭제 테스트
	printF("10. File Remove Test ......");
	// 파일이 닫혔으니 정상 삭제
	if(fremove("fileIOTest.bin") == 0) printF("[  Hit  ]\n");
	else printF("[  Err  ]\n");

	freeMem(_buf);
}

// 파일 읽고 쓰는 속도 측정
static void csCacheTest(const char *buf) {
	FILE *file;
	DWORD clusterSize, oneByteSize, i;
	QWORD lastTickCnt;
	BYTE *_buf;

	// 클러스터는 1MB까지 파일 테스트
	clusterSize = 1024 * 1024;
	// 1바이트씩 읽고 쓰는 테스트는 시간이 많이 걸리므로 16KB만 테스트
	oneByteSize = 16 * 1024;

	// 테스트용 버퍼 메모리 할당
	_buf = allocMem(clusterSize);
	if(_buf == NULL) {
		printF("Memory Allocation Fail...\n");
		return;
	} else printF("Allocation Memory : 0x%Q\n", _buf);

	// 버퍼 초기화
	memSet(_buf, 0, FILESYSTEM_CLUSTER_SIZE);

	printF("   ==================== File I/O Cache Test ===================\n");

	// 클러스터 단위 파일을 순차적으로 쓰는 테스트
	printF("1. Sequential Read/Write Test(Cluster Size)\n");

	// 기존 테스트 파일 제거 후 새로 만듬
	fremove("performance.txt");
	file = fopen("performance.txt", "w");
	if(file == NULL) {
		printF("File Open Fail...\n");
		freeMem(_buf);
		return;
	}

	lastTickCnt = getTickCnt();
	// 클러스터 단위로 쓰는 테스트
	for(i = 0; i < (clusterSize / FILESYSTEM_CLUSTER_SIZE); i++) if(fwrite(_buf, 1, FILESYSTEM_CLUSTER_SIZE, file) != FILESYSTEM_CLUSTER_SIZE) {
		printF("Write Fail...\n");
		// 파일 닫고 메모리 해제
		fclose(file);
		freeMem(_buf);
		return;
	}
	// 시간 출력
	printF("   Sequential Write(Cluster Size): %d ms\n", getTickCnt() - lastTickCnt);

	// 클러스터 단위로 파일 순차적 읽기 테스트, 파일의 처음으로 이동
	fseek(file, 0, SEEK_SET);

	lastTickCnt = getTickCnt();
	// 클러스터 단위로 읽는 테스트
	for(i = 0; i < (clusterSize / FILESYSTEM_CLUSTER_SIZE); i++) if(fread(_buf, 1, FILESYSTEM_CLUSTER_SIZE, file) != FILESYSTEM_CLUSTER_SIZE) {
		printF("Read Fail...\n");
		// 파일 닫고 메모리 해제
		fclose(file);
		freeMem(_buf);
		return;
	}
	// 시간 출력
	printF("   Sequential Read(Cluster Size): %d ms\n", getTickCnt() - lastTickCnt);

	// 1바이트 단위로 파일 순차적으로 쓰는 테스트
	printF("2. Sequential Read/Write Test(1 Byte)\n");

	// 기존 테스트 파일 제거 후 새로 만듬
	fremove("performance.txt");
	file = fopen("performance.txt", "w");
	if(file == NULL) {
		printF("File Open Fail...\n");
		freeMem(_buf);
		return;
	}

	lastTickCnt = getTickCnt();
	// 1 바이트 단위로 쓰는 테스트
	for(i = 0; i < oneByteSize; i++) if(fwrite(_buf, 1, 1, file) != 1) {
		printF("Write Fail...\n");
		// 파일 닫고 메모리 해제
		fclose(file);
		freeMem(_buf);
		return;
	}
	// 시간 출력
	printF("   Sequential Write(1 Byte): %d ms\n", getTickCnt() - lastTickCnt);

	// 1 바이트 단위로 파일을 순차적이게 읽는 테스트, 파일의 처음으로 이동
	fseek(file, 0, SEEK_SET);

	lastTickCnt = getTickCnt();
	// 1 바이트 단위로 읽는 테스트
	for(i = 0; i < oneByteSize; i++) if(fread(_buf, 1, 1, file) != 1) {
		printF("Read Fail...\n");
		// 파일 닫고 메모리 해제
		fclose(file);
		freeMem(_buf);
		return;
	}
	// 시간 출력
	printF("   Sequential Read(1 Byte): %d ms\n", getTickCnt() - lastTickCnt);

	// 파일 닫고 메모리 해제
	fclose(file);
	freeMem(_buf);
}

// 파일 시스템 캐시 버퍼의 데이터 모두 하드 디스크에 씀
static void csCacheFlush(const char *buf) {
	QWORD tickCnt;

	tickCnt = getTickCnt();
	printF("Cache Flushing ...");
	if(flushFileSystemCache() == TRUE) printF("[  Hit  ]\n");
	else printF("[  Err  ]\n");
	printF("Total Time = %d ms\n", getTickCnt() - tickCnt);
}

// 시리얼 포트로부터 데이터 수신해 파일로 저장
static void csDownload(const char *buf) {
	PARAMLIST list;
	char name[50];
	int nameLen;
	DWORD dataLen, recvSize = 0, tmpSize;
	FILE *fp;
	BYTE _buf[SERIAL_FIFO_MAXSIZE];
	QWORD lastTickCnt;

	// 파라미터 리스트 초기화해 파일 이름 추출
	initParam(&list, buf);
	nameLen = getNextParam(&list, name);
	name[nameLen] = '\0';
	if((nameLen > (FILESYSTEM_MAXFILENAMELEN - 1)) || (nameLen == 0)) {
		printF("Too Long or Too Short File Name\n");
		printF("ex)download a.txt\n");
		return;
	}

	// 시리얼 포트의 FIFO 클리어
	clearSerialFIFO();

	// 데이터 길이 수신될 때까지 기다린 후 4바이트 수신하면 ACK 전송
	printF("Waiting For Data Length ...");
	lastTickCnt = getTickCnt();
	while(recvSize < 4) {
		// 남은 수만큼 데이터 수신
		tmpSize = recvSerialData(((BYTE*)&dataLen) + recvSize, 4 - recvSize);
		recvSize += tmpSize;

		// 수신된 데이터가 없다면 잠시 대기
		if(tmpSize == 0) {
			_sleep(0);

			// 대기한 시간이 30초 이상이면 Time Out
			if((getTickCnt() - lastTickCnt) > 30000) {
				printF("Time Out Occur(30s) !!\n");
				return;
			}
		} else lastTickCnt = getTickCnt();	// 마지막으로 데이터 수신한 시간 갱신
	}
	printF("[%d] Byte\n", dataLen);

	// 정상적으로 데이터 길이 수신했으니 ACK 송신
	sendSerialData("A", 1);

	// 파일 생성 후 시리얼로부터 데이터 수신해 파일로 저장
	fp = fopen(name, "w");
	if(fp == NULL) {
		printF("%s File Open Fail...\n", name);
		return;
	}

	// 데이터 수신
	printF("Data Receive Start: ");
	recvSize = 0;
	lastTickCnt = getTickCnt();
	while(recvSize < dataLen) {
		// 버퍼에 담아 데이터 씀
		tmpSize = recvSerialData(_buf, SERIAL_FIFO_MAXSIZE);
		recvSize += tmpSize;

		// 이번에 데이터 수신된 것이 있다면 ACK 또는 파일 쓰기 수행
		if(tmpSize != 0) {
			// 수신하는 쪽은 데이터 마지막까지 수신했거나 FIFO 크기인 16바이트마다 한 번씩 ACK 전송
			if(((recvSize % SERIAL_FIFO_MAXSIZE) == 0) || (recvSize == dataLen)) {
				sendSerialData("A", 1);
				printF("#");
			}

			// 쓰기 중에 문제가 생기면 바로 종료
			if(fwrite(_buf, 1, tmpSize, fp) != tmpSize) {
				printF("File Write Error Occur !!\n");
				break;
			}

			// 마지막으로 데이터 수신한 시간 갱신
			lastTickCnt = getTickCnt();
		} else {	// 수신된 데이터가 없다면 잠시 대기
			_sleep(0);

			// 대기한 시간이 10초 이상이면 Time Out
			if((getTickCnt() - lastTickCnt) > 10000) {
				printF("Time Out Occur(10s) !!\n");
				break;
			}
		}
	}

	// 전체 데이터 크기와 실제 수신된 데이터 크기를 비교해 성공 여부 출력 후 파일을 닫고 캐시 비우기, 수신된 길이 비교 후 문제 발생 여부 표시
	if(recvSize != dataLen) printF("\nError Occur. Total Size [%d] Received Size [%d]\n", recvSize, dataLen);
	else printF("\nReceive Complete. Total Size [%d] Byte\n", recvSize);

	// 파일 닫고 캐시 비움
	fclose(fp);
	flushFileSystemCache();
}

// MP 설정 테이블 정보 출력
static void csMPConfigInfo(const char *buf) {
	printMPConfig();
}

// AP(Application Processor) 시작
static void csStartAP(const char *buf) {
	// AP(Application Processor) 깨움
	if(startUpAP() == FALSE) {
		printF("Application Processor Start Fail...\n");
		return;
	}
	printF("Application Processor Start Success !!\n");

	// BSP(Bootstrap Processor)의 APIC ID 출력
	printF("Bootstrap Processor[APIC ID: %d] Start Application Processor\n", getAPICID());
}

// 대칭 IO 모드로 전환
static void csSymmetricIOMode(const char *buf) {
	MPCONFIGMANAGER *manager;
	BOOL interruptFlag;

	// MP 설정 테이블 분석
	if(analysisMPConfig() == FALSE) {
		printF("Analyze MP Configuration Table Fail...\n");
		return;
	}

	// MP 설정 매니저를 찾아 PIC 모드 여부 확인
	manager = getMPConfigManager();
	if(manager->usePICMode == TRUE) {
		// PIC 모드이면 IO 포트 어드레스 0x22에 0x70을 먼저 전송, IO 포트 어드레스 0x23에 0x01을 전송하는 방법으로 IMCR 레지스터에 접근해 PIC 모드 비활성화
		outByte(0x22, 0x70);
		outByte(0x23, 0x01);
	}

	// PIC 컨트롤러의 인터럽트 모두 마스크해 인터럽트가 발생할 수 없도록 함
	printF("Mask All PIC Controller Interrupt\n");
	maskPIC(0xFFFF);

	// 프로세서 전체의 로컬 APIC 활성화
	printF("Enable Global Local APIC\n");
	onLocalAPIC();

	// 현재 코어 로컬 APIC 활성화
	printF("Enable Software Local APIC\n");
	onSWLocalAPIC();

	// 인터럽트 불가로 설정
	printF("Disable CPU Interrupt Flag\n");
	interruptFlag = setInterruptFlag(FALSE);

	// 모든 인터럽트를 수신할 수 있도록 태스크 우선순위 레지스터를 0으로 설정
	setTaskPriority(0);

	// 로컬 APIC의 로컬 벡터 테이블 초기화
	initLocalVecTbl();

	// 대칭 IO 모드로 변경되었음을 설정
	setSymmetricIOMode(TRUE);

	// IO APIC 리다이렉트 테이블 초기화
	printF("Initialize I/O Redirection Table\n");
	initIORedirect();

	// 이전 인터럽트 플래그 복원
	printF("Restore CPU Interrupt Flag\n");
	setInterruptFlag(interruptFlag);

	printF("Change Symmetric I/O Mode Complete !!\n");
}

// IRQ와 IO APIC의 인터럽트 입력 핀(INTIN)의 관계를 저장한 테이블 표시
static void csIRQMapTbl(const char *buf) {
	// IO APIC 관리하는 자료구조의 출력 함수 호출
	printIRQMap();
}

// 코어별 인터럽트 처리 횟수 출력
static void csInterruptProcCnt(const char *buf) {
	INTERRUPTMANAGER *manager;
	int i, j, procCnt, len, lineCnt;
	char _buf[20];

	printF("   ====================== Interrupt Count =====================\n");

	// MP 설정 테이블에 저장된 코어 개수 읽음
	procCnt = getProcessorCnt();

	// 프로세서 수만큼 Column 출력. 한 줄에 코어 4개씩 출력하고 한 Column당 15칸 할당
	for(i = 0; i < procCnt; i++) {
		if(i == 0) printF("IRQ Num\t\t");
		else if((i % 4) == 0) printF("\n      \t\t");
		sprintF(_buf, "Core %d", i);
		printF(_buf);

		// 출력하고 남은 공간을 모두 스페이스로 채움
		len = 15 - strLen(_buf);
		memSet(_buf, ' ', len);
		_buf[len] = '\0';
		printF(_buf);
	}
	printF("\n");

	// Row와 인터럽트 처리 횟수 출력. 총 인터럽트 횟수와 코어별 인터럽트 처리 횟수 출력
	lineCnt = 0;
	manager = getInterruptManager();
	for(i = 0; i < INTERRUPT_MAX_VECCNT; i++) {
		for(j = 0; j < procCnt; j++) {
			// ROW 출력. 한 줄에 코어 4개씩 출력하고 한 Column당 15칸 할당
			if(j == 0) {
				// 20 라인마다 화면 정지
				if((lineCnt != 0) && (lineCnt > 10)) {
					printF("\nPress any key to continue... ('q' is exit) : ");
					if(getCh() == 'q') {
						printF("\n");
						return;
					}
					lineCnt = 0;
					printF("\n");
				}
				printF("---------------------------------------------------------------\n");
				printF("IRQ %d\t\t", i);
				lineCnt += 2;
			} else if((j % 4) == 0) {
				printF("\n      \t\t");
				lineCnt++;
			}

			sprintF(_buf, "0x%Q", manager->coreInterruptCnt[j][i]);
			// 출력하고 남은 영역을 모두 스페이스로 채움
			printF(_buf);
			len = 15 - strLen(_buf);
			memSet(_buf, ' ', len);
			_buf[len] = '\0';
			printF(_buf);
		}
		printF("\n");
	}
}

// 인터럽트 부하 분산 기능 시작
static void csInterruptLoadBalancing(const char *buf) {
	printF("Start Interrupt Load Balancing...\n");
	setInterruptLoadBalancing(TRUE);
}

// 태스크 부하 분산 기능 시작
static void csTaskLoadBalancing(const char *buf) {
	int i;

	printF("Start Task Load Balancing...\n");
	for(i = 0; i < MAXPROCESSORCNT; i++) setTaskLoadBalancing(i, TRUE);
}

// 태스크 프로세서 친화도 변경
static void csChangeAffinity(const char *buf) {
	PARAMLIST list;
	char id[30], affinity[30];
	QWORD _id;
	BYTE _affinity;

	// 파라미터 추출
	initParam(&list, buf);
	getNextParam(&list, id);
	getNextParam(&list, affinity);

	// 태스크 ID 필드 추출
	if(memCmp(id, "0x", 2) == 0) _id = aToi(id + 2, 16);
	else _id = aToi(id, 10);

	// 프로세서 친화도 추출
	if(memCmp(id, "0x", 2) == 0) _affinity = aToi(affinity + 2, 16);
	else _affinity = aToi(affinity, 10);

	printF("Change Task Affinity ID [0x%q] Affinity[0x%x] ", _id, _affinity);
	if(alterAffinity(_id, _affinity) == TRUE) printF("Success !!\n");
	else printF("Fail...\n");
}

// VBE 모드 정보 블록 출력
static void csVBEModeInfo(const char *buf) {
	VBEMODEINFO *mode;

	// VBE 모드 정보 블록 반환
	mode = getVBEModeInfo();
	printF("VESA %x\n", mode->winWeighting);
	printF("X Pixel: %d\n", mode->xPixel);
	printF("Y Pixel: %d\n", mode->yPixel);
	printF("Bits Per Pixel: %d\n", mode->perPixelBit);

	// 해상도와 색 정보 위주 출력
	printF("Red Mask Size: %d, Field Position: %d\n", mode->redMaskSize, mode->redPosition);
	printF("Green Mask Size: %d, Field Position: %d\n", mode->greenMaskSize, mode->greenPosition);
	printF("Blue Mask Size: %d, Field Position: %d\n", mode->blueMaskSize, mode->bluePosition);
	printF("Linear Frame Base Address: 0x%X\n", mode->linearBaseAddr);
	printF("Linear Red Mask Size: %d, Field Position: %d\n", mode->linearRedMaskSize, mode->linearRedPosition);
	printF("Linear Green Mask Size: %d, Field Position: %d\n", mode->linearGreenMaskSize, mode->linearGreenPosition);
	printF("Linear Blue Mask Size: %d, Field Position: %d\n", mode->linearBlueMaskSize, mode->linearBluePosition);
}
