/*
 * Keyboard.c
 *
 *  Created on: 2017. 7. 20.
 *      Author: Yummy
 */

#include <Types.h>
#include <AsmUtil.h>
#include <Keyboard.h>
#include <Queue.h>
#include <Synchronize.h>

/* 키보드 컨트롤러와 키보드 제어에 관련된 함수 */

// 출력 버퍼(포트 0x60)에 수신된 데이터가 있는지 여부 반환
BOOL kOutputBufChk(void) {
	// 상태 레지스터(포트 0x64)에서 읽은 값에 출력 버퍼 상태 비트(비트 0)가 1이면 출력 버퍼에 키보드가 전송한 데이터 존재
	if(kInByte(0x64) & 0x01) return TRUE;
	return FALSE;
}

// 입력 버퍼(포트 0x60)에 프로세서가 쓴 데이터가 남아있는지 여부 반환
BOOL kInputBufChk(void) {
	// 상태 레지스터(포트 0x64)에서 읽은 값에 입력 버퍼 상태 비트(비트 1)가 1이면 아직 키보드가 데이터를 가져가지 않음
	if(kInByte(0x64) & 0x02) return TRUE;
	return FALSE;
}

// ACK 기다리며 ACK가 아닌 다른 스캔코드는 변환시켜 큐에 삽입
BOOL kAckForQ(void) {
	int i, j;
	BYTE data;
	BOOL res = FALSE, mouseData;

	// ACK가 오기 전 키보드 출력 버퍼(포트 0x60)에 키 데이터가 저장될 수 있어 키보드에서 전달된 데이터를 최대 100개 수신해 확인
	for(j = 0; j < 100; j++) {
		// 0xFFFF만큼 루프 수행. 루프 수행 후 출력 버퍼(포트 0x60)가 차 있지 않으면 무시
		for(i = 0; i < 0xFFFF; i++) if(kOutputBufChk() == TRUE) break;

		// 출력 버퍼(포트 0x60)를 읽기 전 먼저 상태 레지스터(포트 0x64)를 읽어 마우스 데이터인지 확인
		if(kIsMouseData() == TRUE) mouseData = TRUE;
		else mouseData = FALSE;

		data = kInByte(0x60);
		if(data == 0xFA) {
			res = TRUE;
			break;
		} else {	// ACK(0xFA)가 아니면 데이터가 수신된 디바이스에 따라 키보드 큐나 마우스 큐에 삽입
			if(mouseData == FALSE) kConvertNPutCode(data);
			else kGatherMouseData(data);
		}
	}
	return res;
}

// 키보드 활성화
BOOL kActiveKeyboard(void) {
	int i, j;
	BOOL preInterrupt, res;

	// 인터럽트 불가
	preInterrupt = kSetInterruptFlag(FALSE);
	
	// 컨트롤 레지스터(포트 0x64)에 키보드 활성화 커맨드(0xAE)를 전달하여 키보드 디바이스 활성화
	kOutByte(0x64, 0xAE);

	// 입력 버퍼(포트 0x60)가 비어있을 때까지 기다렸다가 키보드에 활성화 커맨드 전송
	// 0xFFFF만큼 루프를 수행할 시간이면 충분히 커맨드 전송
	// 0xFFFF 루프를 수행한 후 입력 버퍼(포트 0x60)가 비어있지 않다면 무시하고 전송
	for(i = 0; i < 0xFFFF; i++) if(kInputBufChk() == FALSE) break;

	// 입력 버퍼(포트 0x60)로 키보드 활성화(0xF4) 커맨드 전달해 키보드로 전송
	kOutByte(0x60, 0xF4);

	// ACK가 올 때까지 대기
	res = kAckForQ();

	// 이전 인터럽트 상태 복원
	kSetInterruptFlag(preInterrupt);
	return res;
}

// 출력 버퍼(포트 0x60)에서 키를 읽음
BYTE kGetCode(void) {
	// 출력 버퍼(포트 0x60)에 데이터가 있을 때까지 대기
	while(kOutputBufChk() == FALSE);
	return kInByte(0x60);
}

// 키보드 LED의 ON/OFF 변경
BOOL kAlterLED(BOOL caps, BOOL num, BOOL scroll) {
	int i, j;
	BOOL preInterrupt, res;
	BYTE data;

	// 인터럽트 불가
	preInterrupt = kSetInterruptFlag(FALSE);

	// 키보드에 LED 변경 커맨드 전송하고 커맨드가 처리될 때까지 대기
	for(i = 0; i < 0xFFFF; i++) if(kInputBufChk() == FALSE) break;

	// 출력 버퍼(포트 0x60)로 LED 상태 변경 커맨드(0xED) 전송
	kOutByte(0x60, 0xED);
	for(i = 0; i < 0xFFFF; i++) if(kInputBufChk() == FALSE) break;

	// ACK 올 때까지 대기
	res = kAckForQ();

	if(res == FALSE) {
		// 이전 인터럽트 상태 복원
		kSetInterruptFlag(preInterrupt);
		return FALSE;
	}

	// LED 변경 값을 키보드로 전송하고 데이터가 처리 완료될 때까지 대기
	kOutByte(0x60, (caps << 2) | (num << 1) | scroll);

	// 입력 버퍼(포트 0x60)가 비어있으면 키보드가 LED데이터 가져간 것
	for(i = 0; i < 0xFFFF; i++) if(kInputBufChk() == FALSE) break;

	// ACK 올 때까지 대기
	res = kAckForQ();

	// 이전 인터럽트 상태 복원
	kSetInterruptFlag(preInterrupt);
	return res;
}

// A20 게이트를 활성화
void kOnA20Gate(void) {
	BYTE portData;
	int i;

	// 컨트롤 레지스터(포트 0x64)에 키보드 컨트롤러의 출력 포트 값을 읽는 커맨드(0xD0) 전송
	kOutByte(0x64, 0xD0);

	// 출력 포트 데이터 기다렸다가 읽음
	for(i = 0; i < 0xFFFF; i++) if(kOutputBufChk() == TRUE) break;

	// 출력 포트(포트 0x60)에 수신된 키보드 컨트롤러 출력 포트 값 읽음
	portData = kInByte(0x60);

	// A20 게이트 비트 설정
	portData |= 0x01;

	// 입력 버퍼(포트 0x60)에 데이터가 비어 있으면 출력 포트에 값을 쓰는 커맨드와 출력 포트 데이터 전송
	for(i = 0; i < 0xFFFF; i++) if(kInputBufChk() == FALSE) break;

	// 커맨드 레지스터(0x64)에 출력 포트 설정 커맨드(0xD1)를 전달
	kOutByte(0x64, 0xD1);

	// 입력 버퍼(0x60)에 A20 게이트 비트가 1로 설정된 값 전달
	kOutByte(0x60, portData);
}

// 프로세서 리셋
void kReBoot(void) {
	int i;

	// 입력 버퍼(포트 0x60)에 데이터가 비어있으면 출력 포트에 값을 쓰는 커맨드와 출력 포트 데이터 전송
	for(i = 0; i < 0xFFFF; i++) if(kInputBufChk() == FALSE) break;

	// 커맨드 레지스터(0x64)에 출력 포트 설정 커맨드(0xD1)를 전달
	kOutByte(0x64, 0xD1);	// kOutByte(0x64, 0xFE);	// 0xFE가 기본 reboot

	// 입력 버퍼(0x60)에 0을 전달하여 프로세서 리셋
	kOutByte(0x60, 0x00);

	while(1);
}

void kShutDown(void) {
	int i;
	
	for(i = 0; i < 0xFFFF; i++) if(kInputBufChk() == FALSE) break;

	kOutByte(0xF4, 0x00);
	while(1);
}

// 키를 저장하는 큐
static QUEUE gs_keyQ;

// 키보드 상태 관리하는 키보드 매니저
static KEYBOARDMANAGER gs_keyManager = {0,};

// 키 저장하는 버퍼
static KEYDATA gs_keyQBuf[KEY_MAXQUEUECNT];

// 스캔 코드를 ASCII 코드로 변환하는 테이블
static KEYMAPPINGENTRY gs_keyMapTable[KEY_MAPPINGTABLEMAXCNT] =
{
 	{ KEY_NONE		,	KEY_NONE	}, //    0
	{ KEY_ESC		,	KEY_ESC		}, //    1
	{ '1'			,	'!'		}, //    2
	{ '2'			,	'@'		}, //    3
	{ '3'			,	'#'		}, //    4
	{ '4'			,	'$'		}, //    5
	{ '5'			,	'%'		}, //    6
	{ '6'			,	'^'		}, //    7
	{ '7'			,	'&'		}, //    8
	{ '8'			,	'*'		}, //    9
	{ '9'			,	'('		}, //    10
	{ '0'			,	')'		}, //    11
	{ '-'			,	'_'		}, //    12
	{ '='			,	'+'		}, //    13
	{ KEY_BACKSPACE		,	KEY_BACKSPACE	}, //    14
	{ KEY_TAB		,	KEY_TAB		}, //    15
	{ 'q'			,	'Q'		}, //    16
	{ 'w'			,	'W'		}, //    17
	{ 'e'			,	'E'		}, //    18
	{ 'r'			,	'R'		}, //    19
	{ 't'			,	'T'		}, //    20
	{ 'y'			,	'Y'		}, //    21
	{ 'u'			,	'U'		}, //    22
	{ 'i'			,	'I'		}, //    23
	{ 'o'			,	'O'		}, //    24
	{ 'p'			,	'P'		}, //    25
	{ '['			,	'{'		}, //    26
	{ ']'			,	'}'		}, //    27
	{ '\n'			,	'\n'		}, //    28
	{ KEY_CTRL		,	KEY_CTRL	}, //    29
	{ 'a'			,	'A'		}, //    30
	{ 's'			,	'S'		}, //    31
	{ 'd'			,	'D'		}, //    32
	{ 'f'			,	'F'		}, //    33
	{ 'g'			,	'G'		}, //    34
	{ 'h'			,	'H'		}, //    35
	{ 'j'			,	'J'		}, //    36
	{ 'k'			,	'K'		}, //    37
	{ 'l'			,	'L'		}, //    38
	{ ';'			,	':'		}, //    39
	{ '\''			,	'"'		}, //    40
	{ '`'			,	'~'		}, //    41
	{ KEY_LSHIFT		,	KEY_LSHIFT	}, //    42
	{ '\\'			,	'|'		}, //    43
	{ 'z'			,	'Z'		}, //    44
	{ 'x'			,	'X'		}, //    45
	{ 'c'			,	'C'		}, //    46
	{ 'v'			,	'V'		}, //    47
	{ 'b'			,	'B'		}, //    48
	{ 'n'			,	'N'		}, //    49
	{ 'm'			,	'M'		}, //    50
	{ ','			,	'<'		}, //    51
	{ '.'			,	'>'		}, //    52
	{ '/'			,	'?'		}, //    53
	{ KEY_RSHIFT		,	KEY_RSHIFT	}, //    54
	{ '*'			,	'*'		}, //    55
	{ KEY_LART		,	KEY_LART	}, //    56
	{ ' '			,	' '		}, //    57
	{ KEY_CAPSLOCK		,	KEY_CAPSLOCK	}, //    58
	{ KEY_F1		,	KEY_F1		}, //    59
	{ KEY_F2		,	KEY_F2		}, //    60
	{ KEY_F3		,	KEY_F3		}, //    61
	{ KEY_F4		,	KEY_F4		}, //    62
	{ KEY_F5		,	KEY_F5		}, //    63
	{ KEY_F6		,	KEY_F6		}, //    64
	{ KEY_F7		,	KEY_F7		}, //    65
	{ KEY_F8		,	KEY_F8 		}, //    66
	{ KEY_F9		,	KEY_F9		}, //    67
	{ KEY_F10		,	KEY_F10		}, //    68
	{ KEY_NUMLOCK		,	KEY_NUMLOCK	}, //    69
	{ KEY_SCROLLLOCK	,	KEY_SCROLLLOCK	}, //    70
	{ KEY_HOME		,	'7'		}, //    71
	{ KEY_ARROW_UP		,	'8'		}, //    72
	{ KEY_PAGE_UP		,	'9'		}, //    73
	{ '-'			,	'-'		}, //    74
	{ KEY_ARROW_LEFT	,	'4'		}, //    75
	{ KEY_CENTER		,	'5'		}, //    76
	{ KEY_ARROW_RIGHT	,	'6'		}, //    77
	{ '+'			,	'+'		}, //    78
	{ KEY_END		,	'1'		}, //    79
	{ KEY_ARROW_DOWN	,	'2'		}, //    80
	{ KEY_PAGE_DOWN		,	'3'		}, //    81
	{ KEY_INS		,	'0'		}, //    82
	{ KEY_DEL		,	'.'		}, //    83
	{ KEY_NONE		,	KEY_NONE	}, //    84
	{ KEY_NONE		,	KEY_NONE	}, //    85
	{ KEY_NONE		,	KEY_NONE	}, //    86
	{ KEY_F11		,	KEY_F11		}, //    87
	{ KEY_F12		,	KEY_F12		}  //    88
};

// 스캔 코드가 알파벳 범위인지 여부 반환
BOOL kIsEngScanCode(BYTE scanCode) {
	// 변환 테이블 값을 직접 읽어 알파벳 범위 확인
	if(('a' <= gs_keyMapTable[scanCode].normCode) && (gs_keyMapTable[scanCode].normCode <= 'z')) return TRUE;
	return FALSE;
}

// 숫자 또는 기호 범위인지 여부 반환
BOOL kIsNumScanCode(BYTE scanCode) {
	// 숫자 패드나 확장 키 범위를 제외한 범위(스캔 코드 2~53)에서 영문자 아니면 숫자, 기호
	if((2 <= scanCode) && (scanCode <= 53) && (kIsEngScanCode(scanCode) == FALSE)) return TRUE;
	return FALSE;
}

// 숫자 패드 범위인지 여부 반환
BOOL kIsPadScanCode(BYTE scanCode) {
	// 숫자 패드는 스캔 코드의 71~83 위치
	if((71 <= scanCode) && (scanCode <= 83)) return TRUE;
	return FALSE;
}

// 조합된 키 값을 사용해야 하는지 여부 반환
BOOL kIsCombineCode(BYTE scanCode) {
	BYTE downScanCode;
	BOOL combineKey = FALSE;

	downScanCode = scanCode & 0x7F;

	// 알파벳 키라면 Shift키와 Caps Lock영향 받음
	if(kIsEngScanCode(downScanCode) == TRUE) {
		// 만약 Shift키와 Caps Lock 키 중 하나만 눌러져 있으면 반환
		if(gs_keyManager.shiftDown ^ gs_keyManager.capsOn) combineKey = TRUE;
		else combineKey = FALSE;
	} else if(kIsNumScanCode(downScanCode) == TRUE) {	// 숫자와 기호 키라면 Shift키 영향 받음
		// Shift 키가 눌러져 있으면 반환
		if(gs_keyManager.shiftDown == TRUE) combineKey = TRUE;
		else combineKey = FALSE;
	} else if((kIsPadScanCode(downScanCode) == TRUE) && (gs_keyManager.exCodeIn == FALSE)) {
		// 숫자 패드 키라면 Num Lock 키 영향 받음. 0xE0만 제외하면 확장 키 코드와 숫자 패드 코드가
		// 겹치므로 확장 키 코드가 수신되지 않았을 때만 처리 조합된 코드 사용
		// Num Lock 키 눌러져 있으면 반환
		if(gs_keyManager.numOn == TRUE) combineKey = TRUE;
		else combineKey = FALSE;
	}
	return combineKey;
}

// 조합 키의 상태를 갱신하고 LED 상태도 동기화 함
void kUpdateKeyNLED(BYTE scanCode) {
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
	if(ledStat == TRUE) kAlterLED(gs_keyManager.capsOn, gs_keyManager.numOn, gs_keyManager.scrollOn);
}

// 스캔 코드를 ASCII 코드로 변환
BOOL kConvertCode(BYTE scanCode, BYTE *ascii, BOOL *flag) {
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
		gs_keyManager.skipForPause = KEY_SKIPCNTFORPAUSE;
		return TRUE;
	} else if(scanCode == 0xE0) {	// 확장 키 코드가 들어오면 실제 키 값은 다음에 들어오니 플래그만 설정
		gs_keyManager.exCodeIn = TRUE;
		return FALSE;
	}

	combineKey = kIsCombineCode(scanCode);

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
	kUpdateKeyNLED(scanCode);
	return TRUE;
}

// 키보드 초기화
BOOL kInitKeyboard(void) {
	// 큐 초기화
	kInitQueue(&gs_keyQ, gs_keyQBuf, KEY_MAXQUEUECNT, sizeof(KEYDATA));

	// 스핀락 초기화
	kInitSpinLock(&(gs_keyManager.spinLock));

	// 키보드 활성화
	return kActiveKeyboard();
}

// 스캔 코드를 내부적으로 사용하는 키 데이터로 바꾼 후 키 큐에 삽입
BOOL kConvertNPutCode(BYTE scanCode) {
	KEYDATA data;
	BOOL res = FALSE, preInterrupt;

	// 스캔 코드를 키 데이터에 삽입
	data.scanCode = scanCode;

	// 스캔 코드를 ASCII 코드와 키 상태로 변환해 키 데이터에 삽입
	if(kConvertCode(scanCode, &data.ascii, &data.flag) == TRUE) {
		// 임계 영역 시작
		kLock_spinLock(&(gs_keyManager.spinLock));

		// 키 큐에 삽입
		res = kAddQData(&gs_keyQ, &data);

		// 임계 영역 끝
		kUnlock_spinLock(&(gs_keyManager.spinLock));
	}
	return res;
}

// 키 큐에서 키 데이터 제거, 수신 대기
BOOL kGetKeyData(KEYDATA *data) {
	BOOL res, preInterrupt;

	// 임계 영역 시작
	kLock_spinLock(&(gs_keyManager.spinLock));

	// 키 큐에서 키 데이터 제거
	res = kRmQData(&gs_keyQ, data);

	// 임계 영역 끝
	kUnlock_spinLock(&(gs_keyManager.spinLock));
	return res;
}
