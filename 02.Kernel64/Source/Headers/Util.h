/*
 * Util.h
 *
 *  Created on: 2017. 7. 22.
 *      Author: Yummy
 */

#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdarg.h>
#include <Types.h>

#pragma once

#define _MIN(x, y)	(((x) < (y)) ? (x) : (y))
#define _MAX(x, y)	(((x) > (y)) ? (x) : (y))

void memSet(void *dest, BYTE data, int size);
void memSetWord(void *dest, WORD data, int size);
int memCpy(void *dest, const void *src, int size);
int memCmp(const void *dest, const void *src, int size);
BOOL setInterruptFlag(BOOL _onInterrupt);
void chkTotalMemSize(void);
QWORD getTotalMemSize(void);
void revStr(char *buf);
long aToi(const char *buf, int radix);
QWORD hexToQW(const char *buf);
long decimalToL(const char *buf);
int iToa(long v, char *buf, int radix);
int hexToStr(QWORD v, char *buf);
int decimalToStr(long v, char *buf);
int sprintF(char *buf, const char *format, ...);
int vsprintF(char *buf, const char *format, va_list v);
QWORD getTickCnt(void);
void _sleep(QWORD ms);

extern volatile QWORD g_tickCnt;

#endif /*__UTIL_H__*/
