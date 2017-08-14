/*
 * DynMem.h
 *
 *  Created on: 2017. 8. 6.
 *      Author: Yummy
 */

#ifndef __DYNMEM_H__
#define __DYNMEM_H__

#include <Types.h>

#pragma once

// 매크로. 동적 메모리 영역의 시작 어드레스, 1MB 단위로 정렬
#define DYNMEM_START_ADDR	((TASK_STACKPOOLADDR + 0xfffff) & 0xffffffffff00000)
// 버디 블록 최소 크기, 1KB
#define DYNMEM_MIN_SIZE		(1 * 1024)

// 비트맵 플래그
#define DYNMEM_EXIST		0x01
#define DYNMEM_EMPTY		0x00

// 구조체. 비트맵 관리 자료구조
typedef struct bitmap {
	BYTE *bitmap;
	QWORD bitCnt;
} BITMAP;

// 버디 블록 관리 자료구조
typedef struct dynmemManager {
	// 블록 리스트의 총 개수와 가장 크기가 가장 작은 블록 개수, 할당된 메모리 크기
	int maxLvCnt;
	int smallBlockCnt;
	QWORD usedSize;

	// 블록 풀 시작 주소와 끝 주소
	QWORD startAddr;
	QWORD endAddr;

	// 할당된 메모리가 속한 블록 리스트의 인덱스 저장 영역과 비트맵 자료구조 주소
	BYTE *allocBlockIdx;
	BITMAP *bitmapLv;
} DYNMEM;

void initDynMem(void);
void *allocMem(QWORD size);
BOOL freeMem(void *addr);
void getDynMemInfo(QWORD *dynmemStartAddr, QWORD *dynmemTotalSize, QWORD *metaDataSize, QWORD *usedMemSize);
DYNMEM *getDynMemManager(void);

static QWORD calcDynMemSize(void);
static int calcMetaBlockCnt(QWORD memSize);
static int allocBuddyBlock(QWORD alignSize);
static QWORD getBuddyBlockSize(QWORD size);
static int getMatchSizeIdx(QWORD alignSize);
static int findFreeBlock(int blockIdx);
static void setFlagBitmap(int blockIdx, int offset, BYTE flag);
static BOOL freeBuddyBlock(int blockIdx, int offset);
static BYTE getFlagBitmap(int blockIdx, int offset);

#endif /*__DYNMEM_H__*/
