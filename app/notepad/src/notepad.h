/*
 * notepad.h
 *
 *  Created on: 2019. 1. 11.
 *      Author: Yummy
 */

#ifndef __NOTEPAD_H__
#define __NOTEPAD_H__

#include "yummyHitLib.h"

#pragma once

// 최대로 표시할 수 있는 라인 수
#define MAX_LINE_CNT	(256 * 1024)
// 윈도우 영역과 파일 내용 표시 영역 사이 여유 공간
#define MARGIN			5
// 탭이 차지하는 크기
#define TAB_SPACE		4

#pragma pack(push, 1)

// 텍스트 정보를 저장하는 구조체
typedef struct TextInfo {
	// 파일 버퍼와 파일 크기
	BYTE *buf;
	DWORD size;

	// 파일 내용 표시 영역에 출력할 수 있는 라인 수와 라인별 문자 수
	int colCnt;
	int rowCnt;

	// 라인 번호에 따른 오프셋 저장하는 버퍼
	DWORD *lineOffset;

	// 파일 최대 라인 수
	int maxLine;
	// 현재 라인 인덱스
	int nowLine;

	// 파일 이름
	char fileName[100];
} TXTINFO;

#pragma pack(pop)

BOOL readfile(const char *fileName, TXTINFO *info);
void calcLineOffset(int width, int height, TXTINFO *info);
BOOL drawTextBuf(QWORD winID, TXTINFO *info);

#endif /*__NOTEPAD_H__*/
