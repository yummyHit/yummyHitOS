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
	{"taskill", "### Task Kill. ex)taskill 1(ID) ###", csTaskill},
	{"cpuload", "### Show Processor Load ###", csCPULoad},
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
	runningTask = getRunningTask();
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
	runningTask = getRunningTask();
	offset = (runningTask->link.id & 0xFFFFFFFF) * 2;
	offset = CONSOLE_WIDTH * CONSOLE_HEIGHT - (offset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));

	while(1) {
		mon[offset].character = data[i % 4];
		mon[offset].color = (offset % 15) + 1;
		i++;
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
		for(i = 0; i < aToi(cnt, 10); i++) if(createTask(TASK_FLAGS_LOW, (QWORD)taskTest1) == NULL) break;
		printF("Task Test 1 %d Created.\n", i);
		break;

	case 2:
	default:
		for(i = 0; i < aToi(cnt, 10); i++) if(createTask(TASK_FLAGS_LOW, (QWORD)taskTest2) == NULL) break;
		printF("Task Test 2 %d Created.\n", i);
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
	if(changePriority(_id, _priority) == TRUE) printF("Success !!\n");
	else printF("Fail...\n");
}

// 현재 생성된 모든 태스크 정보 출력
static void csTaskList(const char *buf) {
	int i, cnt = 0;
	TCB *tcb;

	printF("   ===================Task Total Count [%d]====================\n", getTaskCnt());
	for(i = 0; i < TASK_MAXCNT; i++) {
		// TCB 구해서 TCB가 사용 중이면 ID 출력
		tcb = getTCB(i);
		if((tcb->link.id >> 32) != 0) {
			// 태스크가 10개 출력될 때마다 태스크 정보 표시할지 여부 확인
			if((cnt != 0) && ((cnt % 10) == 0)) {
				printF("Press any key to continue... ('q' is exit) : ");
				if(getCh() == 'q') {
					printF("\n");
					break;
				}
				printF("\n");
			}
			printF("[%d] Task ID[0x%Q], Priority[%d], Flags[0x%Q]\n", 1 + cnt++, tcb->link.id, GETPRIORITY(tcb->flag), tcb->flag);
		}
	}
}

// 태스크 종료
static void csTaskill(const char *buf) {
	PARAMLIST list;
	char id[30];
	QWORD _id;

	// 파라미터 추출
	initParam(&list, buf);
	getNextParam(&list, id);

	// 태스크 종료
	if(memCmp(id, "0x", 2) == 0) _id = aToi(id + 2, 16);
	else _id = aToi(id, 10);

	printF("Kill Task ID [0x%q] ", _id);
	if(taskFin(_id) == TRUE) printF("Success !!\n");
	else printF("Fail...\n");
}

// 프로세서 사용률 표시
static void csCPULoad(const char *buf) {
	printF("Processor Load : %d%%\n", getProcessorLoad());
}
