/*
 * Keyboard.c
 *
 *  Created on: 2017. 7. 20.
 *      Author: Yummy
 */


#include "Types.h"
#include "Port.h"
#include "Keyboard.h"
#include "Queue.h"

/*	키보드 컨트롤러와 키보드 제어에 관련된 함수	*/

// 출력 버퍼(포트 0x60)에 수신된 데이터가 있는지 여부 반환
BOOL outputBufCheck(void) {
	// 상태 레지스터(포트 0x64)에서 읽은 값에 출력 버퍼 상태 비트(비트 0)가 1이면 출력 버퍼에 키보드가 전송한 데이터 존재
	if(inByte(0x64) & 0x01) return TRUE;
	return FALSE;
}

// 입력 버퍼(포트 0x60)에 프로세서가 쓴 데이터가 남아있는지 여부 반환
BOOL inputBufCheck(void) {
	// 상태 레지스터(포트 0x64)에서 읽은 값에 입력 버퍼 상태 비트(비트 1)가 1이면 아직 키보드가 데이터를 가져가지 않음
	if(inByte(0x64) & 0x02) return TRUE;
	return FALSE;
}

// ACK 기다리며 ACK가 아닌 다른 스캔코드는 변환시켜 큐에 삽입
BOOL ackForQueue(void) {
	int i, j;
	BYTE data;
	BOOL res = FALSE;

	// ACK가 오기 전 키보드 출력 버퍼(포트 0x60)에 키 데이터가 저장될 수 있어 키보드에서 전달된 데이터를 최대 100개 수신해 확인
	for(j = 0; j < 100; j++) {
		// 0xFFFF만큼 루프 수행. 루프 수행 후 출력 버퍼(포트 0x60)가 차 있지 않으면 무시
		for(i = 0; i < 0xFFFF; i++) if(outputBufCheck() == TRUE) break;
		data = inByte(0x60);
		if(data == 0xFA) {
			res = TRUE;
			break;
		} else convertNPutCode(data);
	}
	return res;
}

// 키보드 활성화
BOOL activeKeyboard(void) {
	int i, j;
	BOOL preInterrupt;
	BOOL res;

	// 인터럽트 불가
	preInterrupt = setInterruptFlag(FALSE);
	
	// 컨트롤 레지스터(포트 0x64)에 키보드 활성화 커맨드(0xAE)를 전달하여 키보드 디바이스 활성화
	outByte(0x64, 0xAE);

	// 입력 버퍼(포트 0x60)가 비어있을 때까지 기다렸다가 키보드에 활성화 커맨드 전송
	// 0xFFFF만큼 루프를 수행할 시간이면 충분히 커맨드 전송
	// 0xFFFF 루프를 수행한 후 입력 버퍼(포트 0x60)가 비어있지 않다면 무시하고 전송
	for(i = 0; i < 0xFFFF; i++) if(inputBufCheck() == FALSE) break;

	// 입력 버퍼(포트 0x60)로 키보드 활성화(0xF4) 커맨드 전달해 키보드로 전송
	outByte(0x60, 0xF4);

	// ACK가 올 때까지 대기
	res = ackForQueue();

	// 이전 인터럽트 상태 복원
	setInterruptFlag(preInterrupt);
	return res;
}

// 출력 버퍼(포트 0x60)에서 키를 읽음
BYTE getCode(void) {
	// 출력 버퍼(포트 0x60)에 데이터가 있을 때까지 대기
	while(outputBufCheck() == FALSE);
	return inByte(0x60);
}

// 키보드 LED의 ON/OFF 변경
BOOL changeLED(BOOL caps, BOOL num, BOOL scroll) {
	int i, j;
	BOOL preInterrupt;
	BOOL res;
	BYTE data;

	// 인터럽트 불가
	preInterrupt = setInterruptFlag(FALSE);

	// 키보드에 LED 변경 커맨드 전송하고 커맨드가 처리될 때까지 대기
	for(i = 0; i < 0xFFFF; i++) if(inputBufCheck() == FALSE) break;

	// 출력 버퍼(포트 0x60)로 LED 상태 변경 커맨드(0xED) 전송
	outByte(0x60, 0xED);
	for(i = 0; i < 0xFFFF; i++) if(inputBufCheck() == FALSE) break;

	// ACK 올 때까지 대기
	res = ackForQueue();

	if(res == FALSE) {
		// 이전 인터럽트 상태 복원
		setInterruptFlag(preInterrupt);
		return FALSE;
	}

	// LED 변경 값을 키보드로 전송하고 데이터가 처리 완료될 때까지 대기
	outByte(0x60, (caps << 2) | (num << 1) | scroll);

	// 입력 버퍼(포트 0x60)가 비어있으면 키보드가 LED데이터 가져간 것
	for(i = 0; i < 0xFFFF; i++) if(inputBufCheck() == FALSE) break;

	// ACK 올 때까지 대기
	res = ackForQueue();

	// 이전 인터럽트 상태 복원
	setInterruptFlag(preInterrupt);
	return res;
}

// A20 게이트를 활성화
void onA20Gate(void) {
	BYTE outputPortData;
	int i;

	// 컨트롤 레지스터(포트 0x64)에 키보드 컨트롤러의 출력 포트 값을 읽는 커맨드(0xD0) 전송
	outByte(0x64, 0xD0);

	// 출력 포트 데이터 기다렸다가 읽음
	for(i = 0; i < 0xFFFF; i++) if(outputBufCheck() == TRUE) break;

	// 출력 포트(포트 0x60)에 수신된 키보드 컨트롤러 출력 포트 값 읽음
	outputPortData = inByte(0x60);

	// A20 게이트 비트 설정
	outputPortData |= 0x01;

	// 입력 버퍼(포트 0x60)에 데이터가 비어 있으면 출력 포트에 값을 쓰는 커맨드와 출력 포트 데이터 전송
	for(i = 0; i < 0xFFFF; i++) if(inputBufCheck() == FALSE) break;

	// 커맨드 레지스터(0x64)에 출력 포트 설정 커맨드(0xD1)를 전달
	outByte(0x64, 0xD1);

	// 입력 버퍼(0x60)에 A20 게이트 비트가 1로 설정된 값 전달
	outByte(0x60, outputPortData);
}

// 프로세서 리셋
void reBoot(void) {
	int i;

	// 입력 버퍼(포트 0x60)에 데이터가 비어있으면 출력 포트에 값을 쓰는 커맨드와 출력 포트 데이터 전송
	for(i = 0; i < 0xFFFF; i++) if(inputBufCheck() == FALSE) break;

	// 커맨드 레지스터(0x64)에 출력 포트 설정 커맨드(0xD1)를 전달
	outByte(0x64, 0xD1);

	// 입력 버퍼(0x60)에 0을 전달하여 프로세서 리셋
	outByte(0x60, 0x00);

	while(1);
}


// 스캔 코드가 알파벳 범위인지 여부 반환
BOOL isEngScanCode(BYTE scanCode) {
	// 변환 테이블 값을 직접 읽어 알파벳 범위 확인
	if(('a' <= gs_keyMapTable[scanCode].normCode) && (gs_keyMapTable[scanCode].normCode <= 'z')) return TRUE;
	return FALSE;
}

// 숫자 또는 기호 범위인지 여부 반환
BOOL isNumScanCode(BYTE scanCode) {
	// 숫자 패드나 확장 키 범위를 제외한 범위(스캔 코드 2~53)에서 영문자 아니면 숫자, 기호
	if((2 <= scanCode) && (scanCode <= 53) && (isEngScanCode(scanCode) == FALSE)) return TRUE;
	return FALSE;
}

// 숫자 패드 범위인지 여부 반환
BOOL isPadScanCode(BYTE scanCode) {
	// 숫자 패드는 스캔 코드의 71~83 위치
	if((71 <= scanCode) && (scanCode <= 83)) return TRUE;
	return FALSE;
}

// 조합된 키 값을 사용해야 하는지 여부 반환
BOOL isCombineCode(BOOL scanCode) {
	BYTE downScanCode;
	BOOL combineKey;

	downScanCode = scanCode & 0x7F;

	// 알파벳 키라면 Shift키와 Caps Lock영향 받음
	if(isEngScanCode(downScanCode) == TRUE) {
		// 만약 Shift키와 Caps Lock 키 중 하나만 눌러져 있으면 반환
		if(gs_keyManager.shiftDown ^ gs_keyManager.capsOn) combineKey = TRUE;
		else combineKey = FALSE;
	} else if(isNumScanCode(downScanCode) == TRUE) {	// 숫자와 기호 키라면 Shift키 영향 받음
		// Shift 키가 눌러져 있으면 반환
		if(gs_keyManager.shiftDown == TRUE) combineKey = TRUE;
		else combineKey = FALSE;
	} else if((isPadScanCode(downScanCode) == TRUE) && (gs_keyManager.exCodeIn == FALSE)) {
		// 숫자 패드 키라면 Num Lock 키 영향 받음. 0xE0만 제외하면 확장 키 코드와 숫자 패드 코드가
		// 겹치므로 확장 키 코드가 수신되지 않았을 때만 처리 조합된 코드 사용
		// Num Lock 키 눌러져 있으면 반환
		if(gs_keyManager.numOn == TRUE) combineKey = TRUE;
		else combineKey = FALSE;
	}
	return combineKey;
}

// 조합 키의 상태를 갱신하고 LED 상태도 동기화 함
void updateKeyNLED(BYTE scanCode) {
	BOOL down, ledStat = FALSE;
	BYTE downScanCode;

	// 눌림 또는 떨어짐 상태처리, 최상위 비트(비트 7)가 1이면 키가 떨어졌음을 의미
	if(scanCode & 0x80) {
		down = FALSE;
		downScanCode = scanCode & 0x7F;
	} else {
		down = TRUE;
		downScanCode = scanCode;
	}

	// 조합 키 검색. Shift 키의 스캔 코드(42 or 54)면 Shift 키 상태 갱신
	if((downScanCode == 42) || (downScanCode == 54)) gs_keyManager.shiftDown = down;
	else if((downScanCode == 58) && (down == TRUE)) {	// Caps Lock 키 스캔 코드(58)면 Caps Lock 상태 갱신 및 LED 변경
		gs_keyManager.capsOn ^= TRUE;
		ledStat = TRUE;
	} else if((downScanCode == 69) && (down == TRUE)) {	// Num Lock 키 스캔 코드(69)면 Num Lock 상태 갱신 및 LED 변경
		gs_keyManager.numOn ^= TRUE;
		ledStat = TRUE;
	} else if((downScanCode == 70) && (down == TRUE)) {
		gs_keyManager.scrollOn ^= TRUE;
		ledStat = TRUE;
	}

	// LED 상태가 변했으면 키보드로 커맨드 전송해 LED 변경
	if(ledStat == TRUE) changeLED(gs_keyManager.capsOn, gs_keyManager.numOn, gs_keyManager.scrollOn);
}

// 스캔 코드를 ASCII 코드로 변환
BOOL convertCode(BYTE scanCode, BYTE *ascii, BOOL *flag) {
	BOOL combineKey;

	// 이전에 Pause 키가 수신되었으면 남은 스캔 코드 무시
	if(gs_keyManager.skipForPause > 0) {
		gs_keyManager.skipForPause--;
		return FALSE;
	}

	// Pause 키 특별히 처리
	if(scanCode == 0xE1) {
		*ascii = KEY_PAUSE;
		*flag = KEY_FLAGS_DOWN;
		gs_keyManager.skipForPause = KEY_SKIPCOUNTFORPAUSE;
		return TRUE;
	} else if(scanCode == 0xE0) {	// 확장 키 코드가 들어오면 실제 키 값은 다음에 들어오니 플래그만 설정
		gs_keyManager.exCodeIn = TRUE;
		return FALSE;
	}

	combineKey = isCombineCode(scanCode);

	// 키 값 설정
	if(combineKey == TRUE) *ascii = gs_keyMapTable[scanCode & 0x7F].combCode;
	else *ascii = gs_keyMapTable[scanCode & 0x7F].normCode;

	// 확장 키 유무 설정
	if(gs_keyManager.exCodeIn == TRUE) {
		*flag = KEY_FLAGS_EXTENDEDKEY;
		gs_keyManager.exCodeIn = FALSE;
	} else *flag = 0;

	// 눌러짐이나 떨어짐 유무 설정
	if((scanCode & 0x80) == 0) *flag |= KEY_FLAGS_DOWN;

	// 조합 키 눌림이나 떨어짐 상태 갱신
	updateKeyNLED(scanCode);
	return TRUE;
}

// 키보드 초기화
BOOL initKeyboard(void) {
	// 큐 초기화
	initQueue(&gs_keyQ, gs_keyQBuf, KEY_MAXQUEUECOUNT, sizeof(KEYDATA));

	// 키보드 활성화
	return activeKeyboard();
}

// 스캔 코드를 내부적으로 사용하는 키 데이터로 바꾼 후 키 큐에 삽입
BOOL convertNPutCode(BYTE scanCode) {
	KEYDATA data;
	BOOL res = FALSE;
	BOOL preInterrupt;

	// 스캔 코드를 키 데이터에 삽입
	data.scanCode = scanCode;

	// 스캔 코드를 ASCII 코드와 키 상태로 변환해 키 데이터에 삽입
	if(convertCode(scanCode, &data.ascii, &data.flag) == TRUE) {
		// 인터럽트 불가
		preInterrupt = setInterruptFlag(FALSE);

		// 키 큐에 삽입
		res = addData(&gs_keyQ, &data);

		// 이전 인터럽트 플래그 복원
		setInterruptFlag(preInterrupt);
	}
	return res;
}

// 키 큐에서 키 데이터 제거
BOOL rmKeyData(KEYDATA *data) {
	BOOL res;
	BOOL preInterrupt;

	// 큐가 비었으면 키 데이터 못꺼냄
	if(isQEmpty(&gs_keyQ) == TRUE) return FALSE;

	// 인터럽트 불가
	preInterrupt = setInterruptFlag(FALSE);

	// 키 큐에서 키 데이터 제거
	res = rmData(&gs_keyQ, data);

	// 이전 인터럽트 플래그 복원
	setInterruptFlag(preInterrupt);
	return res;
}
