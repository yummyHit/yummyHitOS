/*
 * Console.c
 *
 *  Created on: 2017. 7. 23.
 *      Author: Yummy
 */

#include <stdarg.h>
#include <Console.h>
#include <Keyboard.h>
#include <FileSystem.h>

// 콘솔의 정보를 관리하는 자료구조
CONSOLEMANAGER gs_csManager = {0,};

// 그래픽 모드로 시작했을 때 사용하는 화면 버퍼 영역
static CHARACTER gs_monBuf[CONSOLE_WIDTH * CONSOLE_HEIGHT];

// 그래픽 모드로 시작했을 때 GUI 콘솔 셸 윈도우로 전달된 키 이벤트 콘솔 셸 태스크로 전달하는 큐 버퍼
static KEYDATA gs_keyQBuf[CONSOLE_GUIKEYQ_MAXCNT];

// 콘솔 초기화
void initConsole(int x, int y) {
	// 자료구조를 모두 0으로 초기화
	memSet(&gs_csManager, 0, sizeof(gs_csManager));
	// 화면 버퍼 초기화
	memSet(&gs_monBuf, 0, sizeof(gs_monBuf));

	if(isGUIMode() == FALSE) gs_csManager.monBuf = (CHARACTER*)CONSOLE_VIDEOMEMADDR;
	else {
		// 그래픽 모드이면 그래픽 모드 화면 버퍼 설정
		gs_csManager.monBuf = gs_monBuf;

		// 그래픽 모드에서 사용할 키 큐와 뮤텍스 초기화
		initQueue(&(gs_csManager.keyQForGUI), gs_keyQBuf, CONSOLE_GUIKEYQ_MAXCNT, sizeof(KEYDATA));
		initMutex(&(gs_csManager.lock));
	}

	// 커서 위치 설정
	setCursor(x, y);
}

// 커서 위치 설정. 문자 출력 위치도 같이 설정
void setCursor(int x, int y) {
	int line, oldX, oldY, i;

	// 커서 위치 계산
	line = y * CONSOLE_WIDTH + x;

	// 텍스트 모드로 시작했으면 CRT 컨트롤러로 커서 위치 전송
	if(isGUIMode() == FALSE) {
		// CRTC 컨트롤 어드레스 레지스터(포트 0x3D4)에 0x0E를 전송해 상위 커서 위치 레지스터 선택
		outByte(VGA_PORT_INDEX, VGA_INDEX_HIGHCURSOR);
		// CRTC 컨트롤 데이터 레지스터(포트 0x3D5)에 커서의 상위 바이트 출력
		outByte(VGA_PORT_DATA, line >> 8);

		// CRTC 컨트롤 어드레스 레지스터(포트 0x3D4)에 0x0F를 전송해 하위 커서 위치 레지스터 선택
		outByte(VGA_PORT_INDEX, VGA_INDEX_LOWCURSOR);
		// CRTC 컨트롤 데이터 레지스터(포트 0x3D5)에 커서의 하위 바이트 출력
		outByte(VGA_PORT_DATA, line & 0xFF);
	} else {
		// 그래픽 모드로 시작했으면 화면 버퍼에 출력한 커서 위치 옮겨줌
		// 이전 커서가 있던 위치가 그대로 커서로 남아있으면 지움
		for(i = 0; i < CONSOLE_WIDTH * CONSOLE_HEIGHT; i++) {
			// 커서 있으면 삭제
			if((gs_csManager.monBuf[i].character == '_') && (gs_csManager.monBuf[i].color == 0x0A)) {
				gs_csManager.monBuf[i].character = ' ';
				gs_csManager.monBuf[i].color = CONSOLE_DEFAULTTEXTCOLOR;
				break;
			}
		}

		// 새로운 위치에 커서 출력
		gs_csManager.monBuf[line].character = '_';
		gs_csManager.monBuf[line].color = 0x0A;
	}

	// 문자를 출력할 위치 업데이트
	gs_csManager.nowPrintOffset = line;
}

// 현재 커서 위치 반환
void getCursor(int *x, int *y) {
	// 저장된 위치 콘솔 화면 너비로 나눈 나머지로 X 좌표를 구할 수 있고 화면 너비로 나누면 Y 좌표를 구할 수 있음
	*x = gs_csManager.nowPrintOffset % CONSOLE_WIDTH;
	*y = gs_csManager.nowPrintOffset / CONSOLE_WIDTH;
}

// printf 함수 내부 구현
int printF(const char *format, ...) {
	va_list v;
	char buf[1024];
	int nextPrintOffset, tmp = 0;

	// 가변 인자 리스트를 사용해 vsprintf()로 처리
	va_start(v, format);
	tmp = vsprintF(buf, format, v);
	va_end(v);

	// 포맷 문자열을 화면에 출력
	nextPrintOffset = csPrint(buf);

	// 커서 위치 업데이트
	setCursor(nextPrintOffset % CONSOLE_WIDTH, nextPrintOffset / CONSOLE_WIDTH);

	return tmp;
}

// 개행문자가 포함된 문자열 출력 후 화면상 다음 출력 위치 반환
int csPrint(const char *buf) {
	CHARACTER *mon;
	int i, j, len, printOffset;

	// 화면 버퍼 설정
	mon = gs_csManager.monBuf;

	// 문자열 출력 위치 저장
	printOffset = gs_csManager.nowPrintOffset;

	// 문자열 길이만큼 화면에 출력
	len = strLen(buf);
	for(i = 0; i < len; i++) {
		// 줄바꿈 처리
		if(buf[i] == '\n') printOffset += (CONSOLE_WIDTH - (printOffset % CONSOLE_WIDTH));
		else if(buf[i] == '\t') printOffset += (8 - (printOffset % 8));
		else {
			mon[printOffset].character = buf[i];
			mon[printOffset].color = CONSOLE_DEFAULTTEXTCOLOR;
			printOffset++;
		}

		// 출력 위치가 화면의 최대값(80 * 25)을 벗어났으면 스크롤 처리
		if(printOffset >= (CONSOLE_HEIGHT * CONSOLE_WIDTH)) {
			// 가장 윗줄 제외 나머지 한 줄 위로 복사
			memCpy(mon, mon + CONSOLE_WIDTH, (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH * sizeof(CHARACTER));

			// 가장 마지막 라인은 공백
			for(j = (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH; j < (CONSOLE_HEIGHT * CONSOLE_WIDTH); j++) {
				// 공백 출력
				mon[j].character = ' ';
				mon[j].color = CONSOLE_DEFAULTTEXTCOLOR;
			}

			// 출력할 위치를 가장 아래쪽 라인의 처음으로 설정
			printOffset = (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH;
		}
	}
	return printOffset;
}

// 전체 화면 삭제
void clearMonitor(void) {
	CHARACTER *mon;
	int i;

	// 화면 버퍼 설정
	mon = gs_csManager.monBuf;

	// 화면 전체를 공백으로 채우고 커서 위치를 0, 0으로 이동
	for(i = 0; i < CONSOLE_WIDTH * CONSOLE_HEIGHT; i++) {
		mon[i].character = ' ';
		mon[i].color = CONSOLE_DEFAULTTEXTCOLOR;
	}
}

// 매트릭스용 모니터
void clearMatrix(void) {
	CHARACTER *mon = (CHARACTER*)CONSOLE_VIDEOMEMADDR;
	int i;

	for(i = 0; i < CONSOLE_WIDTH * CONSOLE_HEIGHT; i++) {
		mon[i].character = ' ';
		mon[i].color = MATRIX_COLOR;
	}
}

// getch() 함수 구현
BYTE getCh(void) {
	KEYDATA data;

	// 키가 눌러질 때까지 대기, 키 큐에 데이터가 수신되면 키가 눌렸다는 데이터 수신시 ASCII 코드 반환
	while(1) {
		// 그래픽 모드가 아니면 커널 키 큐에서 값 가져옴
		if(isGUIMode() == FALSE) {
			// 키 큐에 데이터 수신될 때까지 대기
			while(getKeyData(&data) == FALSE) schedule();
		} else {
			// 그래픽 모드면 그래픽 모드용 키 큐에서 값 가져옴
			while(rmGUIKeyQ(&data) == FALSE) {
				// 그래픽 모드에서 동작하는 중 셸 태스크 종료해야 될 경우 루프 종료
				if(gs_csManager.exit == TRUE) return 0xFF;
				schedule();
			}
		}

		if(data.flag & KEY_FLAGS_DOWN) return data.ascii;
	}
}

// 문자열을 X, Y 위치에 출력
void printXY(int x, int y, BYTE color, const char *str) {
	CHARACTER *mon;
	int i;

	// 화면 버퍼 설정
	mon = gs_csManager.monBuf;

	// 비디오 메모리 어드레스에서 현재 출력할 위치 계산
	mon += (y * CONSOLE_WIDTH) + x;

	// 문자열 길이만큼 루프 돌면서 문자와 속성 저장
	for(i = 0; str[i] != 0; i++) {
		mon[i].character = str[i];
		mon[i].color = color;
	}
}

// 콘솔 관리 자료구조 반환
CONSOLEMANAGER *getConsoleManager(void) {
	return &gs_csManager;
}

// 그래픽 모드용 키 큐에서 키 데이터 제거
BOOL rmGUIKeyQ(KEYDATA *data) {
	BOOL res;

	// 큐에 데이터 없으면 실패
	if(isQEmpty(&(gs_csManager.keyQForGUI)) == TRUE) return FALSE;

	// 동기화 처리
	_lock(&(gs_csManager.lock));

	// 큐에서 데이터 제거
	res = rmQData(&(gs_csManager.keyQForGUI), data);

	// 동기화 처리
	_unlock(&(gs_csManager.lock));

	return res;
}

// 그래픽 모드용 키 큐에 데이터 삽입
BOOL addGUIKeyQ(KEYDATA *data) {
	BOOL res;

	// 큐에 데이터 가득 차면 실패
	if(isQFull(&(gs_csManager.keyQForGUI)) == TRUE) return FALSE;

	// 동기화 처리
	_lock(&(gs_csManager.lock));

	// 큐에 데이터 삽입
	res = addQData(&(gs_csManager.keyQForGUI), data);

	// 동기화 처리
	_unlock(&(gs_csManager.lock));

	return res;
}

// 콘솔 셸 태스크 종료 플래그 설정
void setShellExitFlag(BOOL flag) {
	gs_csManager.exit = flag;
}

// YummyHitOS ascii art
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
