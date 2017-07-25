/*
 * Keyboard.h
 *
 *  Created on: 2017. 7. 20.
 *      Author: Yummy
 */

#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include "Types.h"
#include "Queue.h"

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
// 키 큐 최대 크기
#define KEY_MAXQUEUECOUNT 100

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
static KEYDATA gs_keyQBuf[KEY_MAXQUEUECOUNT];

// 스캔 코드를 ASCII 코드로 변환하는 테이블
static KEYMAPPINGENTRY gs_keyMapTable[KEY_MAPPINGTABLEMAXCOUNT] =
{
    /*  0   */  {   KEY_NONE        ,   KEY_NONE        },
    /*  1   */  {   KEY_ESC         ,   KEY_ESC         },
    /*  2   */  {   '1'             ,   '!'             },
    /*  3   */  {   '2'             ,   '@'             },
    /*  4   */  {   '3'             ,   '#'             },
    /*  5   */  {   '4'             ,   '$'             },
    /*  6   */  {   '5'             ,   '%'             },
    /*  7   */  {   '6'             ,   '^'             },
    /*  8   */  {   '7'             ,   '&'             },
    /*  9   */  {   '8'             ,   '*'             },
    /*  10  */  {   '9'             ,   '('             },
    /*  11  */  {   '0'             ,   ')'             },
    /*  12  */  {   '-'             ,   '_'             },
    /*  13  */  {   '='             ,   '+'             },
    /*  14  */  {   KEY_BACKSPACE   ,   KEY_BACKSPACE   },
    /*  15  */  {   KEY_TAB         ,   KEY_TAB         },
    /*  16  */  {   'q'             ,   'Q'             },
    /*  17  */  {   'w'             ,   'W'             },
    /*  18  */  {   'e'             ,   'E'             },
    /*  19  */  {   'r'             ,   'R'             },
    /*  20  */  {   't'             ,   'T'             },
    /*  21  */  {   'y'             ,   'Y'             },
    /*  22  */  {   'u'             ,   'U'             },
    /*  23  */  {   'i'             ,   'I'             },
    /*  24  */  {   'o'             ,   'O'             },
    /*  25  */  {   'p'             ,   'P'             },
    /*  26  */  {   '['             ,   '{'             },
    /*  27  */  {   ']'             ,   '}'             },
    /*  28  */  {   '\n'            ,   '\n'            },
    /*  29  */  {   KEY_CTRL        ,   KEY_CTRL        },
    /*  30  */  {   'a'             ,   'A'             },
    /*  31  */  {   's'             ,   'S'             },
    /*  32  */  {   'd'             ,   'D'             },
    /*  33  */  {   'f'             ,   'F'             },
    /*  34  */  {   'g'             ,   'G'             },
    /*  35  */  {   'h'             ,   'H'             },
    /*  36  */  {   'j'             ,   'J'             },
    /*  37  */  {   'k'             ,   'K'             },
    /*  38  */  {   'l'             ,   'L'             },
    /*  39  */  {   ';'             ,   ':'             },
    /*  40  */  {   '\''            ,   '\"'            },
    /*  41  */  {   '`'             ,   '~'             },
    /*  42  */  {   KEY_LSHIFT      ,   KEY_LSHIFT      },
    /*  43  */  {   '\\'            ,   '|'             },
    /*  44  */  {   'z'             ,   'Z'             },
    /*  45  */  {   'x'             ,   'X'             },
    /*  46  */  {   'c'             ,   'C'             },
    /*  47  */  {   'v'             ,   'V'             },
    /*  48  */  {   'b'             ,   'B'             },
    /*  49  */  {   'n'             ,   'N'             },
    /*  50  */  {   'm'             ,   'M'             },
    /*  51  */  {   ','             ,   '<'             },
    /*  52  */  {   '.'             ,   '>'             },
    /*  53  */  {   '/'             ,   '?'             },
    /*  54  */  {   KEY_RSHIFT      ,   KEY_RSHIFT      },
    /*  55  */  {   '*'             ,   '*'             },
    /*  56  */  {   KEY_LALT        ,   KEY_LALT        },
    /*  57  */  {   ' '             ,   ' '             },
    /*  58  */  {   KEY_CAPSLOCK    ,   KEY_CAPSLOCK    },
    /*  59  */  {   KEY_F1          ,   KEY_F1          },
    /*  60  */  {   KEY_F2          ,   KEY_F2          },
    /*  61  */  {   KEY_F3          ,   KEY_F3          },
    /*  62  */  {   KEY_F4          ,   KEY_F4          },
    /*  63  */  {   KEY_F5          ,   KEY_F5          },
    /*  64  */  {   KEY_F6          ,   KEY_F6          },
    /*  65  */  {   KEY_F7          ,   KEY_F7          },
    /*  66  */  {   KEY_F8          ,   KEY_F8          },
    /*  67  */  {   KEY_F9          ,   KEY_F9          },
    /*  68  */  {   KEY_F10         ,   KEY_F10         },
    /*  69  */  {   KEY_NUMLOCK     ,   KEY_NUMLOCK     },

    /*  70  */  {   KEY_SCROLLLOCK  ,   KEY_SCROLLLOCK  },
    /*  71  */  {   KEY_HOME        ,   '7'             },
    /*  72  */  {   KEY_UP          ,   '8'             },
    /*  73  */  {   KEY_PAGEUP      ,   '9'             },
    /*  74  */  {   '-'             ,   '-'             },
    /*  75  */  {   KEY_LEFT        ,   '4'             },
    /*  76  */  {   KEY_CENTER      ,   '5'             },
    /*  77  */  {   KEY_RIGHT       ,   '6'             },
    /*  78  */  {   '+'             ,   '+'             },
    /*  79  */  {   KEY_END         ,   '1'             },
    /*  80  */  {   KEY_DOWN        ,   '2'             },
    /*  81  */  {   KEY_PAGEDOWN    ,   '3'             },
    /*  82  */  {   KEY_INS         ,   '0'             },
    /*  83  */  {   KEY_DEL         ,   '.'             },
    /*  84  */  {   KEY_NONE        ,   KEY_NONE        },
    /*  85  */  {   KEY_NONE        ,   KEY_NONE        },
    /*  86  */  {   KEY_NONE        ,   KEY_NONE        },
    /*  87  */  {   KEY_F11         ,   KEY_F11         },
    /*  88  */  {   KEY_F12         ,   KEY_F12         }   
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
BOOL isCombineCode(BOOL scanCode);
void updateKeyNLED(BYTE scanCode);
BOOL convertCode(BYTE scanCode, BYTE *ascii, BOOL *flag);
BOOL initKeyboard(void);
BOOL convertNPutCode(BYTE scanCode);
BOOL rmKeyData(KEYDATA *data);

#endif /*__KEYBOARD_H__*/
