/*
 * types.h
 *
 *  Created on: 2018. 6. 17.
 *      Author: Yummy
 */

#ifndef __TYPES_H__
#define __TYPES_H__

#pragma once

#define BYTE	unsigned char
#define WORD	unsigned short
#define DWORD	unsigned int
#define QWORD	unsigned long
#define BOOL	unsigned char

#define TRUE	1
#define FALSE	0
#define NULL	0

// 콘솔 너비, 높이, 비디오 메모리 시작 어드레스 설정
#define CONSOLE_WIDTH		80
#define CONSOLE_HEIGHT		25

// 키 상태에 대한 플래그
#define KEY_FLAGS_UP		0x00
#define KEY_FLAGS_DOWN		0x01
#define KEY_FLAGS_EXTENDEDKEY	0x02

#define KEY_NONE		0x00
#define KEY_ENTER		'\n'
#define KEY_TAB			'\t'
#define KEY_ESC			0x1B
#define KEY_BACKSPACE		0x08
#define KEY_CTRL		0x81
#define KEY_LSHIFT		0x82
#define KEY_RSHIFT		0x83
#define KEY_PRINTSCREEN 	0x84
#define KEY_ALT			0x85
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

// 태스크의 우선순위
#define TASK_FLAGS_HIGHEST	0
#define TASK_FLAGS_HIGH		1
#define TASK_FLAGS_MEDIUM	2
#define TASK_FLAGS_LOW		3
#define TASK_FLAGS_LOWEST	4
#define TASK_FLAGS_WAIT		0xFF

// 태스크의 플래그
#define TASK_FLAGS_FIN		0x8000000000000000
#define TASK_FLAGS_SYSTEM	0x4000000000000000
#define TASK_FLAGS_PROC		0x2000000000000000
#define TASK_FLAGS_THREAD	0x1000000000000000
#define TASK_FLAGS_IDLE		0x0800000000000000
#define TASK_FLAGS_USERLV	0x0400000000000000

// 함수 매크로
#define GETPRIORITY(x)		((x) & 0xFF)
#define SETPRIORITY(x, priority)	((x) = ((x) & 0xFFFFFFFFFFFFFF00) | (priority))
#define GETTCBOFFSET(x)		((x) & 0xFFFFFFFF)

// 유효하지 않은 태스크 ID
#define TASK_INVALID_ID		0xFFFFFFFFFFFFFFFF

// 프로세서 친화도 필드에 라애 값이 설정되면 해당 태스크는 특별 요구사항이 없는 것으로 판단하여 태스크 부하 분산 사용
#define TASK_LOADBALANCING_ID	0xFF

// 파일 이름의 최대 길이
#define FILESYSTEM_FILENAME_MAXLEN	24

// SEEK 옵션 정의
#define SEEK_SET		0
#define SEEK_CUR		1
#define SEEK_END		2

// 파일 시스템 타입과 필드를 표준 입출력 타입으로 재정의
#define size_t		DWORD
#define dirent		directoryEntry
#define d_name		fileName

// 매크로. 색 저장시 사용할 자료구조, 16비트 색을 사용
typedef WORD		COLOR;

// 0 ~ 255 범위의 R, G, B를 16비트 색 형식으로 변환. 0 ~ 31, 0 ~ 63으로 낮추어 사용하니 각 8과 4로 나눠야 함. Shift 연산으로 사용 가능
#define RGB(r, g, b)	(((BYTE)(r) >> 3) << 11 | ((BYTE)(g) >> 2) << 5 | ((BYTE)(b) >> 3))

// 윈도우 제목 최대 길이
#define WINDOW_TITLE_MAXLEN		40
// 유효하지 않은 윈도우 ID
#define WINDOW_INVALID_ID		0xFFFFFFFFFFFFFFFF

// 윈도우 속성. 윈도우를 화면에 표시
#define WINDOW_FLAGS_SHOW		0x00000001
// 윈도우 테두리 그림
#define WINDOW_FLAGS_DRAWFRAME		0x00000002
// 윈도우 제목 표시줄 그림
#define WINDOW_FLAGS_DRAWTITLE		0x00000004
// 윈도우 크기 변경 버튼을 그림
#define WINDOW_FLAGS_RESIZABLE		0x00000008
// 윈도우 기본 속성, 제목 표시줄과 프레임을 모두 그리고 화면에 윈도우 표시
#define WINDOW_FLAGS_DEFAULT		(WINDOW_FLAGS_SHOW | WINDOW_FLAGS_DRAWFRAME | WINDOW_FLAGS_DRAWTITLE)

// 제목 표시줄 높이
#define WINDOW_TITLE_HEIGHT		21
// 윈도우 닫기 버튼 크기
#define WINDOW_XBTN_SIZE		19
// 윈도우 최소 너비, 버튼 2개 너비에 30픽셀 여유 공간 확보
#define WINDOW_WIDTH_MIN		(WINDOW_XBTN_SIZE * 2 + 30)
// 윈도우 최소 높이, 제목 표시줄 높이에 30픽셀 여유 공간 확보
#define WINDOW_HEIGHT_MIN		(WINDOW_TITLE_HEIGHT + 30)

// 윈도우 색깔
#define WINDOW_COLOR_FRAME		RGB(102, 102, 255)
#define WINDOW_COLOR_BACKGROUND		RGB(255, 255, 255)
#define WINDOW_COLOR_TITLE_TEXT		RGB(255, 255, 135)
#define WINDOW_COLOR_TITLE_ONBACKGROUND	RGB(102, 0, 255)
#define WINDOW_COLOR_TITLE_OFFBACKGROUND	RGB(156, 96, 246)
#define WINDOW_COLOR_TITLE_FIRSTOUT	RGB(102, 102, 255)
#define WINDOW_COLOR_TITLE_SECONDOUT	RGB(102, 102, 204)
#define WINDOW_COLOR_TITLE_UNDERLINE	RGB(87, 16, 149)
#define WINDOW_COLOR_BTN_OUT		RGB(229, 229, 229)
#define WINDOW_COLOR_BTN_DARK		RGB(86, 86, 86)
#define WINDOW_COLOR_SYSTEM_BACKGROUND	RGB(186, 140, 255)
#define WINDOW_COLOR_XLINE		RGB(102, 0, 255)

// 배경 윈도우 제목
#define WINDOW_BACKGROUND_TITLE		"SYS_BACKGROUND"

// 마우스 이벤트
#define EVENT_UNKNOWN			0
#define EVENT_MOUSE_MOVE		1
#define EVENT_MOUSE_LCLICK_ON		2
#define EVENT_MOUSE_LCLICK_OFF		3
#define EVENT_MOUSE_RCLICK_ON		4
#define EVENT_MOUSE_RCLICK_OFF		5
#define EVENT_MOUSE_WHEEL_ON		6
#define EVENT_MOUSE_WHEEL_OFF		7

// 윈도우 이벤트
#define EVENT_WINDOW_SELECT		8
#define EVENT_WINDOW_DESELECT		9
#define EVENT_WINDOW_MOVE		10
#define EVENT_WINDOW_RESIZE		11
#define EVENT_WINDOW_CLOSE		12

// 키 이벤트
#define EVENT_KEY_DOWN			13
#define EVENT_KEY_UP			14

// 영문 폰트 너비 및 길이
#define FONT_ENG_WIDTH	8
#define FONT_ENG_HEIGHT	16

// 한글 폰트 너비 및 길이
#define FONT_KOR_WIDTH	16
#define FONT_KOR_HEIGHT	16

#define _MIN(x, y)	(((x) < (y)) ? (x) : (y))
#define _MAX(x, y)	(((x) > (y)) ? (x) : (y))

#pragma pack(push, 1)

// 키 큐에 삽입할 데이터 구조체
typedef struct keyData {
	// 키보드에서 전달된 스캔 코드
	BYTE scanCode;
	// 스캔 코드를 변환한 ASCII 코드
	BYTE ascii;
	// 키 상태 저장하는 플래그(눌림, 떨어짐, 확장 키 여부)
	BYTE flag;
} KEYDATA;

// 디렉터리 엔트리 자료구조
typedef struct directoryEntry {
	// 파일 이름
	char fileName[FILESYSTEM_FILENAME_MAXLEN];
	// 파일 실제 크기
	DWORD size;
	// 파일이 시작하는 클러스터 인덱스
	DWORD startClusterIdx;
} DIRENTRY;

// 파일 관리하는 파일 핸들 자료구조
typedef struct fileHandle {
	// 파일이 존재하는 디렉터리 엔트리 오프셋
	int dirEntryOffset;
	// 파일 크기
	DWORD size;
	// 파일의 시작 클러스터 인덱스
	DWORD startClusterIdx;
	// 현재 I/O가 수행중인 클러스터의 인덱스
	DWORD nowClusterIdx;
	// 현재 클러스터의 바로 이전 클러스터의 인덱스
	DWORD preClusterIdx;
	// 파일 포인터 현재 위치
	DWORD nowOffset;
} FILEHANDLE;

// 디렉터리 관리하는 디렉터리 핸들 자료구조
typedef struct directoryHandle {
	// 루트 디렉터리 저장해둔 버퍼
	DIRENTRY *dirBuf;
	// 디렉터리 포인터의 현재 위치
	int nowOffset;
} DIRHANDLE;

// 파일과 디렉터리에 대한 정보가 들어있는 자료구조
typedef struct fileDirHandle {
	// 자료구조 타입 설정. 파일 핸들이나 디렉터리 핸들 또는 빈 핸들 타입 지정 가능
	BYTE type;
	// type 값에 따라 파일이나 디렉터리로 사용
	union {
		// 파일 핸들
		FILEHANDLE fileHandle;
		// 디렉터리 핸들
		DIRHANDLE dirHandle;
	};
} FILE, DIR;

// 구조체. 사각형의 정보를 담는 자료구조
typedef struct rectangular {
	// 왼쪽 위(시작점) X좌표
	int x1;
	// 왼쪽 위(시작점) Y좌표
	int y1;

	// 오른쪽 아래(끝점) X좌표
	int x2;
	// 오른쪽 아래(끝점) Y좌표
	int y2;
} RECT;

// 점의 정보 담는 자료구조
typedef struct point {
	// X와 Y 좌표
	int x;
	int y;
} POINT;

// 구조체. 마우스 이벤트 자료구조
typedef struct mouseEvnet {
	// 윈도우 ID
	QWORD id;

	// 마우스 X, Y좌표와 버튼 상태
	POINT point;
	BYTE btnStat;
} MOUSEEVENT;

// 키 이벤트 자료구조
typedef struct keyEvent {
	// 윈도우 ID
	QWORD id;

	// 키의 ASCII 코드와 스캔 코드
	BYTE ascii;
	BYTE scanCode;

	// 키 플래그
	BYTE flag;
} KEYEVENT;

// 윈도우 이벤트 자료구조
typedef struct windowEvent {
	// 윈도우 ID
	QWORD id;

	// 영역 정보
	RECT area;
} WINDOWEVENT;

// 이벤트 자료구조
typedef struct event {
	// 이벤트 타입
	QWORD type;

	// 이벤트 데이터 영역 공용체
	union {
		// 마우스 이벤트
		MOUSEEVENT mouseEvent;

		// 키 이벤트
		KEYEVENT keyEvent;

		// 윈도우 이벤트
		WINDOWEVENT winEvent;

		// 유저 이벤트
		QWORD data[3];
	};
} EVENT;

// 허프만 테이블
typedef struct {
 	int elem;	// 요소 갯수
	unsigned short code[256];
	unsigned char size[256];
	unsigned char value[256];
} HUFF;

// JPEG decoding 자료구조
typedef struct {
	// SOF
	int width, height;

	// MCU
	int mcu_width, mcu_height;
	int max_h, max_v;
	int compo_cnt;
	int compo_id[3], compo_sample[3];
	int compo_h[3], compo_v[3], compo_qt[3];

	// SOS
	int scan_cnt;
	int scan_id[3], scan_ac[3], scan_dc[3];
	int scan_h[3], scan_v[3], scan_qt[3];	// h, v : sampling component count / qt : quantization table index

	// DRI
	int interval;
	int mcu_buf[32 * 32 * 4];	// buffer
	int *mcu_yuv[4];
	int mcu_preDC[3];

	// DQT
	int dqt[3][64];
	int n_dqt;

	// DHT
	HUFF huff[2][3];

	// i/o
	unsigned char *data;
	int data_idx, data_size;
	unsigned long bit_buf;
	int bit_remain;
} JPEG;

#pragma pack(pop)

#endif /*__TYPES_H__*/
