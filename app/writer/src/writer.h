/*
 * writer.h
 *
 *  Created on: 2019. 2. 10.
 *      Author: Yummy
 */

#ifndef __WRITER_H__
#define __WRITER_H__

#include "yummyHitLib.h"

#pragma once

// 최대로 표시할 수 있는 문자 바이트 수
#define MAX_LENGTH		60

#pragma pack(push, 1)

// 버퍼 정보 저장하는 구조체
typedef struct bufferManager {
	// 한글 조합을 위해 키 입력 저장하는 버퍼
	char input[20];
	int inputLen;

	// 조합 중인 한글과 조합이 완료된 한글을 저장하는 버퍼
	char processing[3];
	char complete[3];

	// 실제로 화면에 출력하는 정보가 저장되어 있는 버퍼
	char output[MAX_LENGTH];
	int outputLen;
} BUFINFO;

#pragma pack(pop)

void tolowerKorean(BYTE *input);

#endif /*__WRITER_H__*/
