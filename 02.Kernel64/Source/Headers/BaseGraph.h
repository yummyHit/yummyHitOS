/*
 * BaseGraph.h
 *
 *  Created on: 2017. 8. 31.
 *      Author: Yummy
 */

#ifndef __BASEGRAPH_H__
#define __BASEGRAPH_H__

#include <Types.h>

#pragma once

// 매크로. 색 저장시 사용할 자료구조, 16비트 색을 사용
typedef WORD		COLOR;

// 0 ~ 255 범위의 R, G, B를 16비트 색 형식으로 변환. 0 ~ 31, 0 ~ 63으로 낮추어 사용하니 각 8과 4로 나눠야 함. Shift 연산으로 사용 가능
#define RGB(r, g, b)	(((BYTE)(r) >> 3) << 11 | ((BYTE)(g) >> 2) << 5 | ((BYTE)(b) >> 3))

// 구조체. 사각형의 정보를 담는 자료구조
typedef struct rect {
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

inline void kInDrawPixel(const RECT *area, COLOR *addr, int x, int y, COLOR color);
void kInDrawLine(const RECT *area, COLOR *addr, int x1, int y1, int x2, int y2, COLOR color);
void kInDrawRect(const RECT *area, COLOR *addr, int x1, int y1, int x2, int y2, COLOR color, BOOL fill);
void kInDrawCircle(const RECT *area, COLOR *addr, int _x, int _y, int rad, COLOR color, BOOL fill);
void kInDrawText(const RECT *area, COLOR *addr, int x, int y, COLOR text, COLOR background, const char *buf, int len);

inline BOOL kIsInRect(const RECT *area, int x, int y);
inline int kGetRectWidth(const RECT *area);
inline int kGetRectHeight(const RECT *area);
inline void kSetRectData(int x1, int y1, int x2, int y2, RECT *rect);
inline BOOL kGetRectCross(const RECT *area1, const RECT *area2, RECT *inter);
inline BOOL kIsRectCross(const RECT *area1, const RECT *area2);

#endif
