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

void kMemSet(void *dest, BYTE data, int size);
inline void kMemSetWord(void *dest, WORD data, int size);
int kMemCpy(void *dest, const void *src, int size);
int kMemCmp(const void *dest, const void *src, int size);
BOOL kSetInterruptFlag(BOOL _onInterrupt);
void kChkTotalMemSize(void);
QWORD kGetTotalMemSize(void);
void kRevStr(char *buf);
long kAtoI(const char *buf, int radix);
QWORD kHexToQW(const char *buf);
long kDecimalToL(const char *buf);
int kItoA(long v, char *buf, int radix);
int kHexToStr(QWORD v, char *buf);
int kDecimalToStr(long v, char *buf);
int kSprintF(char *buf, const char *format, ...);
int kVsprintF(char *buf, const char *format, va_list v);
QWORD kGetTickCnt(void);
void kSleep(QWORD ms);
BOOL kIsGUIMode(void);

extern volatile QWORD g_tickCnt;

#endif /*__UTIL_H__*/
