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

void drawPixel(int x, int y, COLOR color);
void drawLine(int x1, int y1, int x2, int y2, COLOR color);
void drawRect(int x1, int y1, int x2, int y2, COLOR color, BOOL fill);
void drawCircle(int _x, int _y, int rad, COLOR color, BOOL fill);
void drawText(int x, int y, COLOR text, COLOR background, const char *buf, int len);

#endif
