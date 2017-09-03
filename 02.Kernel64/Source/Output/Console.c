/*
 * Console.c
 *
 *  Created on: 2017. 7. 23.
 *      Author: Yummy
 */

#include <stdarg.h>
#include <Console.h>
#include <Keyboard.h>

// 콘솔의 정보를 관리하는 자료구조
CONSOLEMANAGER gs_csManager = {0,};

// 콘솔 초기화
void initConsole(int x, int y) {
	// 자료구조를 모두 0으로 초기화
	memSet(&gs_csManager, 0, sizeof(gs_csManager));

	// 커서 위치 설정
	setCursor(x, y);
}

// 커서 위치 설정. 문자 출력 위치도 같이 설정
void setCursor(int x, int y) {
	int line;

	// 커서 위치 계산
	line = y * CONSOLE_WIDTH + x;

	// CRTC 컨트롤 어드레스 레지스터(포트 0x3D4)에 0x0E를 전송해 상위 커서 위치 레지스터 선택
	outByte(VGA_PORT_INDEX, VGA_INDEX_HIGHCURSOR);
	// CRTC 컨트롤 데이터 레지스터(포트 0x3D5)에 커서의 상위 바이트 출력
	outByte(VGA_PORT_DATA, line >> 8);

	// CRTC 컨트롤 어드레스 레지스터(포트 0x3D4)에 0x0F를 전송해 하위 커서 위치 레지스터 선택
	outByte(VGA_PORT_INDEX, VGA_INDEX_LOWCURSOR);
	// CRTC 컨트롤 데이터 레지스터(포트 0x3D5)에 커서의 하위 바이트 출력
	outByte(VGA_PORT_DATA, line & 0xFF);

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
	CHARACTER *mon = (CHARACTER*)CONSOLE_VIDEOMEMADDR;
	int i, j, len, printOffset;

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
			memCpy(CONSOLE_VIDEOMEMADDR, CONSOLE_VIDEOMEMADDR + CONSOLE_WIDTH * sizeof(CHARACTER), (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH * sizeof(CHARACTER));

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
	CHARACTER *mon = (CHARACTER*)CONSOLE_VIDEOMEMADDR;
	int i;

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
		while(getKeyData(&data) == FALSE) schedule();
		if(data.flag & KEY_FLAGS_DOWN) return data.ascii;
	}
}

// 문자열을 X, Y 위치에 출력
void printXY(int x, int y, BYTE color, const char *str) {
	CHARACTER *mon = (CHARACTER*)CONSOLE_VIDEOMEMADDR;
	int i;

	// 비디오 메모리 어드레스에서 현재 출력할 위치 계산
	mon += (y * 80) + x;

	// 문자열 길이만큼 루프 돌면서 문자와 속성 저장
	for(i = 0; str[i] != 0; i++) {
		mon[i].character = str[i];
		mon[i].color = color;
	}
}
