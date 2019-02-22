/*
 * syslib.h
 *
 *  Created on: 2018. 6. 17.
 *      Author: Yummy
 */

#ifndef __SYSLIB_H__
#define __SYSLIB_H__

#include "types.h"
#include "syslist.h"

#pragma once
// 파라미터로 전달할 수 있는 최대 수
#define SYSCALL_MAXPARAMCNT	20

// 파라미터 자료구조에서 N번째 가리키는 매크로
#define PARAM(x)	(param.val[(x)])

#pragma pack(push, 1)

// 시스템 콜 호출시 전달하는 파라미터 자료구조
typedef struct kSystemCallParameterTable {
	QWORD val[SYSCALL_MAXPARAMCNT];
} PARAMTBL;

#pragma pack(pop)

// 시스템 콜 함수
QWORD syscall(QWORD num, PARAMTBL *param);

// 콘솔 함수
int printCS(const char *buf);
BOOL setCursor(int x, int y);
BOOL getCursor(int *x, int *y);
BOOL clearMonitor(void);
BYTE getch(void);

// 메모리 관련 함수
void *malloc(QWORD size);
BOOL free(void *addr);

// 파일 및 디렉터리 관련 함수
FILE *fopen(const char *file, const char *mode);
DWORD fread(void *buf, DWORD size, DWORD cnt, FILE *file);
DWORD fwrite(const void *buf, DWORD size, DWORD cnt, FILE *file);
int fseek(FILE *file, int offset, int origin);
int fclose(FILE *file);
int remove(const char *file);
DIR *opendir(const char *dir);
DIRENTRY *readdir(DIR *dir);
BOOL rewinddir(DIR *dir);
int closedir(DIR *dir);
BOOL isfopen(const DIRENTRY *entry);

// 하드 디스크 관련 함수
int readHDDSector(BOOL pri, BOOL master, DWORD lba, int sectorCnt, char *buf);
int writeHDDSector(BOOL pri, BOOL master, DWORD lba, int sectorCnt, char *buf);

// 태스크 및 스케줄러 관련 함수
QWORD makeTask(QWORD flag, void *memAddr, QWORD memSize, QWORD epAddr, BYTE affinity);
BOOL schedule(void);
BOOL alterPriority(QWORD id, BYTE priority, BOOL intterupt);
BOOL taskFin(QWORD id);
void exit(int val);
int getTaskCnt(BYTE id);
BOOL istask(QWORD id);
QWORD getProcLoad(BYTE id);
BOOL alterAffinity(QWORD id, BYTE affinity);
QWORD exec(const char *fileName, const char *argv, BYTE affinity);
QWORD createThread(QWORD ep, QWORD arg, BYTE affinity);

// GUI 시스템 관련 함수
QWORD getBackgroundID(void);
void getMonArea(RECT *area);
QWORD makeWin(int x, int y, int width, int height, DWORD flag, const char *title);
BOOL delWin(QWORD id);
BOOL showWin(QWORD id, BOOL show);
QWORD findWinPoint(int x, int y);
QWORD findWinTitle(const char *title);
BOOL isWin(QWORD id);
QWORD getTopWin(void);
BOOL moveWinTop(QWORD id);
BOOL isInTitle(QWORD id, int x, int y);
BOOL isCloseBtn(QWORD id, int x, int y);
BOOL moveWin(QWORD id, int x, int y);
BOOL resizeWin(QWORD id, int x, int y, int width, int height);
BOOL getWinArea(QWORD id, RECT *area);
BOOL updateMonID(QWORD id);
BOOL updateMonWinArea(QWORD id, const RECT *area);
BOOL updateMonArea(const RECT *area);
BOOL eventToWin(QWORD id, const EVENT *event);
BOOL winToEvent(QWORD id, EVENT *event);
BOOL drawWinFrame(QWORD id);
BOOL drawWinBackground(QWORD id);
BOOL drawWinTitle(QWORD id, const char *title, BOOL select);
BOOL drawBtn(QWORD id, RECT *btnArea, COLOR background, const char *text, COLOR textColor);
BOOL drawPixel(QWORD id, int x, int y, COLOR color);
BOOL drawLine(QWORD id, int x1, int y1, int x2, int y2, COLOR color);
BOOL drawRect(QWORD id, int x1, int y1, int x2, int y2, COLOR color, BOOL fill);
BOOL drawCircle(QWORD id, int x, int y, int rad, COLOR color, BOOL fill);
BOOL drawText(QWORD id, int x, int y, COLOR text, COLOR background, const char *str, int len);
void moveCursor(int x, int y);
void getWinCursor(int *x, int *y);
BOOL bitblt(QWORD id, int x, int y, COLOR *buf, int width, int height);

// JPEG 관련 함수
BOOL jpgInit(JPEG *jpg, BYTE *buf, DWORD size);
BOOL jpgDecode(JPEG *jpg, COLOR *rgb);

// RTC 관련 함수
BOOL readTime(BYTE *hour, BYTE *min, BYTE *sec);
BOOL readDate(WORD *year, BYTE *month, BYTE *day, BYTE *week);

// Serial 통신 관련 함수
void sendSerialData(BYTE *buf, int size);
int recvSerialData(BYTE *buf, int size);
void clearSerialFIFO(void);

// 기타 유틸 함수
QWORD getTotalMemSize(void);
QWORD getTickCnt(void);
void sleep(QWORD millisec);
BOOL isGUIMode(void);

#endif /*__SYSLIB_H__*/
