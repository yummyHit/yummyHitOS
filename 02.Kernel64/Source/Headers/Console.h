/*
 * Console.h
 *
 *  Created on: 2017. 7. 23.
 *      Author: Yummy
 */

#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include <Types.h>
#include <Synchronize.h>
#include <Queue.h>
#include <Keyboard.h>

#pragma once
// 매크로. 비디오 메모리 속성 값 설정
#define CONSOLE_BACKGROUND_BLACK		0x00
#define CONSOLE_BACKGROUND_BLUE			0x10
#define CONSOLE_BACKGROUND_GREEN		0x20
#define CONSOLE_BACKGROUND_CYAN			0x30
#define CONSOLE_BACKGROUND_RED			0x40
#define CONSOLE_BACKGROUND_MAGENTA		0x50
#define CONSOLE_BACKGROUND_BROWN		0x60
#define CONSOLE_BACKGROUND_WHITE		0x70
#define CONSOLE_BACKGROUND_BLINK		0x80
#define CONSOLE_FOREGROUND_DARKBLACK		0x00
#define CONSOLE_FOREGROUND_DARKBLUE		0x01
#define CONSOLE_FOREGROUND_DARKGREEN		0x02
#define CONSOLE_FOREGROUND_DARKCYAN		0x03
#define CONSOLE_FOREGROUND_DARKRED		0x04
#define CONSOLE_FOREGROUND_DARKMAGENTA		0x05
#define CONSOLE_FOREGROUND_DARKBROWN		0x06
#define CONSOLE_FOREGROUND_DARKWHITE		0x07
#define CONSOLE_FOREGROUND_BRIGHTBLACK		0x08
#define CONSOLE_FOREGROUND_BRIGHTBLUE		0x09
#define CONSOLE_FOREGROUND_BRIGHTGREEN		0x0A
#define CONSOLE_FOREGROUND_BRIGHTCYAN		0x0B
#define CONSOLE_FOREGROUND_BRIGHTRED		0x0C
#define CONSOLE_FOREGROUND_BRIGHTMAGENTA	0x0D
#define CONSOLE_FOREGROUND_BRIGHTYELLOW		0x0E
#define CONSOLE_FOREGROUND_BRIGHTWHITE		0x0F
// 기본 문자 색상
#define CONSOLE_DEFAULTTEXTCOLOR		(CONSOLE_BACKGROUND_BLUE | CONSOLE_FOREGROUND_BRIGHTYELLOW)
#define MATRIX_COLOR				(CONSOLE_BACKGROUND_BLACK | CONSOLE_FOREGROUND_BRIGHTGREEN)

// 콘솔 너비, 높이, 비디오 메모리 시작 어드레스 설정
#define CONSOLE_WIDTH		80
#define CONSOLE_HEIGHT		25
#define CONSOLE_VIDEOMEMADDR	0xB8000

// 비디오 컨트롤러 IO포트 어드레스 및 레지스터
#define VGA_PORT_INDEX		0x3D4
#define VGA_PORT_DATA		0x3D5
#define VGA_INDEX_HIGHCURSOR	0x0E
#define VGA_INDEX_LOWCURSOR	0x0F

// 그래픽 모드에서 사용하는 키 큐에 저장할 수 있는 최대 수
#define CONSOLE_GUIKEYQ_MAXCNT	100

// 구조체, 1바이트로 정렬
#pragma pack(push, 1)

// 콘솔에 대한 정보 저장 자료구조
typedef struct csManager {
	// 문자, 커서 출력 위치
	int nowPrintOffset;

	// 출력할 화면 버퍼 어드레스
	CHARACTER *monBuf;

	// 그래픽 모드에서 사용할 키 큐와 뮤텍스
	QUEUE keyQForGUI;
	MUTEX lock;

	// 셸 태스크 종료할지 여부
	volatile BOOL exit;
} CONSOLEMANAGER;

#pragma pack(pop)

void initConsole(int x, int y);
void setCursor(int x, int y);
void getCursor(int *x, int *y);
int printF(const char *format, ...);
int csPrint(const char *buf);
void clearMonitor(void);
void clearMatrix(void);
BYTE getCh(void);
void printXY(int x, int y, BYTE color, const char *str);
CONSOLEMANAGER *getConsoleManager(void);
BOOL rmGUIKeyQ(KEYDATA *data);
BOOL addGUIKeyQ(KEYDATA *data);
void setShellExitFlag(BOOL flag);

void yummy_ascii_art(const char *buf);

#endif /*__CONSOLE_H__*/
