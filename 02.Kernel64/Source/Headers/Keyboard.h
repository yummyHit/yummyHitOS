/*
 * Keyboard.h
 *
 *  Created on: 2017. 7. 20.
 *      Author: Yummy
 */

#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include <Types.h>
#include <Queue.h>
#include <Synchronize.h>

#pragma once
// 매크로. Pause 키를 처리하기 위해 무시해야 하는 나머지 스캔 코드의 수
#define KEY_SKIPCNTFORPAUSE	2

// 키 상태에 대한 플래그
#define KEY_FLAGS_UP		0x00
#define KEY_FLAGS_DOWN		0x01
#define KEY_FLAGS_EXTENDEDKEY	0x02

// 스캔 코드 매핑 테이블에 대한 매크로
#define KEY_MAPPINGTABLEMAXCNT	89

#define KEY_NONE		0x00
#define KEY_ENTER		'\n'
#define KEY_TAB			'\t'
#define KEY_ESC			0x1B
#define KEY_BACKSPACE		0x08
#define KEY_CTRL		0x81
#define KEY_LSHIFT		0x82
#define KEY_RSHIFT		0x83
#define KEY_PRINTSCREEN 	0x84
#define KEY_LART		0x85
#define KEY_CAPSLOCK		0x86
#define KEY_F1			0x87
#define KEY_F2			0x88
#define KEY_F3			0x89
#define KEY_F4			0x8A
#define KEY_F5			0x8B
#define KEY_F6			0x8C
#define KEY_F7			0x8D
#define KEY_F8			0x8E
#define KEY_F9			0x8F
#define KEY_F10			0x90
#define KEY_NUMLOCK		0x91
#define KEY_SCROLLLOCK		0x92
#define KEY_HOME		0x93
#define KEY_ARROW_UP		0x94
#define KEY_PAGE_UP		0x95
#define KEY_ARROW_LEFT		0x96
#define KEY_CENTER		0x97
#define KEY_ARROW_RIGHT		0x98
#define KEY_END			0x99
#define KEY_ARROW_DOWN		0x9A
#define KEY_PAGE_DOWN		0x9B
#define KEY_INS			0x9C
#define KEY_DEL			0x9D
#define KEY_F11			0x9E
#define KEY_F12			0x9F
#define KEY_PAUSE		0xA0
// 키 큐 최대 크기
#define KEY_MAXQUEUECNT		100

// 구조체
#pragma pack(push, 1)

// 스캔 코드 테이블을 구성하는 항목
typedef struct keyMapEntry {
	// Shift 키나 Caps Lock 키와 조합되지 않는 ASCII
	BYTE normCode;
	// Shift 키나 Caps Lock 키와 조합된 ASCII
	BYTE combCode;
} KEYMAPPINGENTRY;

// 키보드 상태 관리 자료구조
typedef struct keyboardManager {
	// 자료구조 동기화를 위한 스핀락
	SPINLOCK spinLock;

	// 조합 키 정보
	BOOL shiftDown;
	BOOL capsOn;
	BOOL numOn;
	BOOL scrollOn;

	// 확장 키를 처리하기 위한 정보
	BOOL exCodeIn;
	int skipForPause;
} KEYBOARDMANAGER;

// 키 큐에 삽입할 데이터 구조체
typedef struct keyData {
	// 키보드에서 전달된 스캔 코드
	BYTE scanCode;
	// 스캔 코드를 변환한 ASCII 코드
	BYTE ascii;
	// 키 상태 저장하는 플래그(눌림, 떨어짐, 확장 키 여부)
	BYTE flag;
} KEYDATA;

#pragma pack(pop)

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

// 함수
BOOL outputBufCheck(void);
BOOL inputBufCheck(void);
BOOL ackForQueue(void);
BOOL activeKeyboard(void);
BOOL getCode(void);
BOOL changeLED(BOOL caps, BOOL num, BOOL scroll);
void onA20Gate(void);
void reBoot(void);
BOOL isEngScanCode(BYTE scanCode);
BOOL isNumScanCode(BYTE scanCode);
BOOL isPadScanCode(BYTE scanCode);
BOOL isCombineCode(BYTE scanCode);
void updateKeyNLED(BYTE scanCode);
BOOL convertCode(BYTE scanCode, BYTE *ascii, BOOL *flag);
BOOL initKeyboard(void);
BOOL convertNPutCode(BYTE scanCode);
BOOL rmKeyData(KEYDATA *data);

#endif /*__KEYBOARD_H__*/
