/*
 * custom_lib.h
 *
 *  Created on: 2018. 6. 17.
 *      Author: Yummy
 */

#ifndef __CUSTOM_LIB_H__
#define __CUSTOM_LIB_H__

#include "types.h"
#include <stdarg.h>

#pragma once

void memset(void *dest, BYTE data, int size);
int memcpy(void *dest, const void *src, int size);
int memcmp(const void *dest, const void *src, int size);
int strcpy(char *dest, const char *src);
int strcmp(char *dest, const char *src);
int strlen(const char *buf);
long atoi(const char *buf, int radix);
QWORD hex2qword(const char *buf);
long int2long(const char *buf);
int itoa(long v, char *buf, int radix);
int hex2str(QWORD v, char *buf);
int int2str(long v, char *buf);
void strrev(char *buf);
int sprintf(char *buf, const char *format, ...);
int vsprintf(char *buf, const char *format, va_list v);
int printf(const char *format, ...);
void srand(QWORD seed);
QWORD rand(void);

BOOL isInRect(const RECT *area, int x, int y);
int getRectWidth(const RECT *area);
int getRectHeight(const RECT *area);
BOOL isRectCross(const RECT *area1, const RECT *area2);
BOOL getRectCross(const RECT *area1, const RECT *area2, RECT *inter);
BOOL monitorToPoint(QWORD id, const POINT *xy, POINT *xyWin);
BOOL pointToMon(QWORD id, const POINT *xy, POINT *xyMon);
BOOL monitorToRect(QWORD id, const RECT *area, RECT *areaWin);
BOOL rectToMon(QWORD id, const RECT *area, RECT *areaMon);
void setRectData(int x1, int y1, int x2, int y2, RECT *rect);
BOOL setMouseEvent(QWORD id, QWORD type, int x, int y, BYTE btnStat, EVENT *event);
BOOL setWinEvent(QWORD id, QWORD type, EVENT *event);
void setKeyEvent(QWORD win, const KEYDATA *key, EVENT *event);

#endif /*__CUSTOM_LIB_H__*/
