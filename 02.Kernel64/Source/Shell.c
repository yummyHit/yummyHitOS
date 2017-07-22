/*
 * Shell.c
 *
 *  Created on: 2017. 7. 23.
 *      Author: Yummy
 */

#include "Shell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Util.h"

// 커맨드 테이블 정의
SHELLENTRY gs_cmdTable[] = {
	{"help", "### What do u want?", csHelp},
	{"clear", "", csClear},
	{"tot_free", "### Your Total RAM :", csFree},
	{"strConvert", "### String To Number(Decimal or HexaDecimal)", csStrConvert},
	{"shutdown", "Shutdown & Reboot ...", csHalt},
};

// 셸 메인 루프
void startShell(void) {
	char buf[SHELL_MAXCOMMANDBUFFERCOUNT];
	int bufIdx = 0;
	BYTE key;
	int x, y;

	// 프롬프트 출력
	printF(SHELL_PROMPTMESSAGE);

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
			printF("%s", SHELL_PROMPTMESSAGE);
			memSet(buf, '\0', SHELL_MAXCOMMANDBUFFERCOUNT);
			bufIdx = 0;
		} else if((key == KEY_LSHIFT) || (key == KEY_RSHIFT) || (key == KEY_CAPSLOCK) || (key == KEY_NUMLOCK) || (key == KEY_SCROLLLOCK)) {
			;
		} else {
			// TAB 외 특수 키 무시 및 TAB 키 공백으로 전환
			if(key == KEY_TAB) key = ' ';

			// 버퍼에 공간이 남아있을 때만 가능
			if(bufIdx < SHELL_MAXCOMMANDBUFFERCOUNT) {
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
void csHelp(const char *buf) {
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
void csClear(const char *buf) {
	// 맨 윗줄은 디버깅 용으로 사용하므로 1번째 라인으로 커서 이동
	clearMonitor();
	setCursor(0, 1);
}

// 총 메모리 크기 출력
void csFree(const char *buf) {
	printF("    %d MB\n", getTotalMemSize());
}

// 문자열로 된 숫자를 숫자로 변환해 출력
void csStrConvert(const char *buf) {
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
void csHalt(const char *buf) {
	printF("Press any key on your keyboard for reboot PC !\n");
	getCh();
	reBoot();
}
