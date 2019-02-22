/*
 * kor.h
 *
 *  Created on: 2019. 2. 10.
 *      Author: Yummy
 */

#ifndef __KOR_H__
#define __KOR_H__

#include "yummyHitLib.h"

#pragma once
// 영어 대문자를 소문자로 바꿔주는 매크로
#define tolower(x) ((('A' <= (x)) && ((x) <= 'Z')) ? ((x) - 'A' + 'a') : (x))

#pragma pack(push, 1)

// 한글 입력 테이블 구성 단위
typedef struct koreanTableItem {
	// 한글
	char *kor;

	// 한글에 대응하는 키 입력
	char *input;
} KOREANITEMS;

// 한글 입력 테이블 인덱스 구성 단위
typedef struct koreanIndexTable {
	// 한글 낱자 입력할 때 사용하는 첫 번째 키
	char firstChar;

	// 한글 입력 테이블 시작 인덱스
	DWORD idx;
} KOREANINDEX;

#pragma pack(pop)

BOOL isConsonant(char input);
BOOL isVowel(char input);
BOOL findKorean(const char *buf, int bufCnt, int *idx, int *len);
BOOL makeKorean(char *buf, int *bufLen, char *processing, char *complete);

#endif /*__KOR_H__*/
