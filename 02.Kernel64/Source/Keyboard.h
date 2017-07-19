/*
 * Keyboard.h
 *
 *  Created on: 2017. 7. 20.
 *      Author: Yummy
 */

#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include "Types.h"

// 매크로. Pause 키를 처리하기 위해 무시해야 하는 나머지 스캔 코드의 수
#define KEY_SKIPCOUNTFORPAUSE	2

// 키 상태에 대한 플래그
#define KEY_FLAGS_UP		0x00
#define KEY_FLAGS_DOWN		0x01
#define KEY_FLAGS_EXTENDEDKEY	0x02

// 스캔 코드 매핑 테이블에 대한 매크로
#define KEY_MAPPINGTABLEMAXCOUNT 89

#define KEY_NONE	0x00
#define KEY_ENTER	'\n'
#define KEY_TAB		'\t'
#define KEY_ESC		0x1B
#define KEY_BACKSPACE	0x08
#define KEY_CTRL	0x81
#define KEY_LSHIFT	0x82
#define KEY_RSHIFT	0x83
#define KEY_PRINTSCREEN	0x84
#define KEY_LALT	0x85
#define KEY_CAPSLOCK	0x86
#define KEY_F1		0x87
#define KEY_F2		0x88
#define KEY_F3		0x89
#define KEY_F4		0x8A
#define KEY_F5		0x8B
#define KEY_F6		0x8C
#define KEY_F7		0x8D
#define KEY_F8		0x8E
#define KEY_F9		0x8F
#define KEY_F10		0x90
#define KEY_NUMLOCK	0x91
#define KEY_SCROLLLOCK	0x92
#define KEY_HOME	0x93
#define KEY_UP		0x94
#define KEY_PAGEUP	0x95
#define KEY_LEFT	0x96
#define KEY_CENTER	0x97
#define KEY_RIGHT	0x98
#define KEY_END		0x99
#define KEY_DOWN	0x9A
#define KEY_PAGEDOWN	0x9B
#define KEY_INS		0x9C
#define KEY_DEL		0x9D
#define KEY_F11		0x9E
#define KEY_F12		0x9F
#define KEY_PAUSE	0xA0

// 구조체
#pragma pack(push, 1)

// 스캔 코드 테이블을 구성하는 항목
typedef struct keyMapEntry {
	// Shift 키나 Caps Lock 키와 조합되지 않는 ASCII
	BYTE normCode;
	// Shift 키나 Caps Lock 키와 조합된 ASCII
	BYTE combCode;
} KEYMAPPINGENTRY;

#pragma pack(pop)

// 키보드 상태 관리 자료구조
typedef struct keyboardManager {
	// 조합 키 정보
	BOOL shiftDown;
	BOOL capsOn;
	BOOL numOn;
	BOOL scrollOn;

	// 확장 키를 처리하기 위한 정보
	BOOL exCodeIn;
	int skipForPause;
} KEYBOARDMANAGER;

// 함수
BOOL outputBufCheck(void);
BOOL inputBufCheck(void);
BOOL activeKeyboard(void);
BOOL getScanCode(void);
BOOL changeLED(BOOL caps, BOOL num, BOOL scroll);
void onA20Gate(void);
void reboot(void);
BOOL isEngScanCode(BYTE scanCode);
BOOL isNumScanCode(BYTE scanCode);
BOOL isPadScanCode(BYTE scanCode);
BOOL isCombineCode(BOOL scanCode);
void updateKeyNLED(BYTE scanCode);
BOOL convertCode(BYTE scanCode, BYTE *ascii, BOOL *flag);

#endif /*__KEYBOARD_H__*/
