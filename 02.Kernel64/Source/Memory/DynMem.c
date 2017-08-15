/*
 * DynMem.c
 *
 *  Created on: 2017. 8. 6.
 *      Author: Yummy
 */

#include <DynMem.h>
#include <Util.h>
#include <Task.h>
#include <Synchronize.h>

static DYNMEM gs_dynmem;

// 동적 메모리 영역 초기화
void initDynMem(void) {
	QWORD memSize;
	int i, j, cntLv, metaCnt;
	BYTE *nowBitmap;

	// 동적 메모리 영역으로 사용할 메모리 크기를 이용해 블록을 관리. 필요한 메모리 크기를 최소 블록 단위로 계산
	memSize = calcDynMemSize();
	metaCnt = calcMetaBlockCnt(memSize);

	// 전체 블록 개수에서 관리에 필요한 메타블록 개수를 제외한 나머지 영역에 대해 메타 정보 구성
	gs_dynmem.smallBlockCnt = (memSize / DYNMEM_MIN_SIZE) - metaCnt;

	// 최대 몇 개의 블록 리스트로 구성되는지 계산
	for(i = 0; (gs_dynmem.smallBlockCnt >> i) > 0; i++);	// Do Nothing

	gs_dynmem.maxLvCnt = i;

	// 할당된 메모리가 속한 블록 리스트 인덱스를 저장하는 영역 초기화
	gs_dynmem.allocBlockIdx = (BYTE*)DYNMEM_START_ADDR;
	for(i = 0; i < gs_dynmem.smallBlockCnt; i++) gs_dynmem.allocBlockIdx[i] = 0xFF;

	// 비트맵 자료구조 시작 어드레스 지정
	gs_dynmem.bitmapLv = (BITMAP*)(DYNMEM_START_ADDR + (sizeof(BYTE) * gs_dynmem.smallBlockCnt));
	// 실제 비트맵 어드레스 지정
	nowBitmap = ((BYTE*)gs_dynmem.bitmapLv) + (sizeof(BITMAP) * gs_dynmem.maxLvCnt);
	// 블록 리스트 별 루프를 돌며 비트맵 생성. 초기 상태는 가장 큰 블록과 짜투리 블록만 있으니 나머지는 비어 있게 설정
	for(j = 0; j < gs_dynmem.maxLvCnt; j++) {
		gs_dynmem.bitmapLv[j].bitmap = nowBitmap;
		gs_dynmem.bitmapLv[j].bitCnt = 0;
		metaCnt = gs_dynmem.smallBlockCnt >> j;

		// 8개 이상 남았으면 상위 블록으로 모두 결합 가능하므로 모두 비어 있게 설정
		for(i = 0; i < metaCnt / 8; i++) {
			*nowBitmap = 0x00;
			nowBitmap++;
		}

		// 8로 나누어 떨어지지 않는 나머지 블록 처리
		if((metaCnt % 8) != 0) {
			*nowBitmap = 0x00;
			// 남은 블록이 홀수라면 마지막 한 블록은 결합되어 상위 블록으로 이동 못하므로 마지막 블록을 현재 리스트의 짜투리 블록으로 설정
			i = metaCnt % 8;
			if((i % 2) == 1) {
				*nowBitmap |= (DYNMEM_EXIST << (i - 1));
				gs_dynmem.bitmapLv[j].bitCnt = 1;
			}
			nowBitmap++;
		}
	}

	// 블록 풀의 어드레스와 사용된 메모리 크기 설정
	gs_dynmem.startAddr = DYNMEM_START_ADDR + metaCnt * DYNMEM_MIN_SIZE;
	gs_dynmem.endAddr = calcDynMemSize() + DYNMEM_START_ADDR;
	gs_dynmem.usedSize = 0;
}

// 동적 메모리 영역 크기 계산
static QWORD calcDynMemSize(void) {
	QWORD memSize;

	// 3GB 이상 메모리에는 비디오 메모리와 프로세서가 사용하는 레지스터가 존재하니 3GB 까지만 사용
	memSize = (getTotalMemSize() * 1024 * 1024);
	if(memSize > (QWORD)(3 * 1024 * 1024 * 1024)) memSize = (QWORD)(3 * 1024 * 1024 * 1024);
	return memSize - DYNMEM_START_ADDR;
}

// 동적 메모리 영역을 관리하는데 필요한 정보가 차지하는 공간 계산. 최소 블록 단위로 정렬 후 반환
static int calcMetaBlockCnt(QWORD memSize) {
	long smallBlockCnt, i;
	DWORD allocBlockIdxSize, bitmapSize = 0;

	// 가장 크기가 작은 블록 개수를 계산해 이를 기준으로 비트맵 영역과 할당된 크기 저장하는 영역 계산
	smallBlockCnt = memSize / DYNMEM_MIN_SIZE;
	// 할당된 블록이 속한 블록 리스트의 인덱스를 저장하는데 필요한 영역 계산
	allocBlockIdxSize = smallBlockCnt * sizeof(BYTE);

	// 비트맵 저장하는데 필요한 공간 계산
	for(i = 0; (smallBlockCnt >> i) > 0; i++) {
		// 블록 리스트의 비트맵 포인터를 위한 공간
		bitmapSize += sizeof(BITMAP);
		// 블록 리스트의 비트맵 크기, 바이트 단위로 올림처리
		bitmapSize += ((smallBlockCnt >> i) + 7) / 8;
	}

	// 사용한 메모리 영역 크기를 최소 블록 크기로 올림해서 반환
	return (allocBlockIdxSize + bitmapSize + DYNMEM_MIN_SIZE - 1) / DYNMEM_MIN_SIZE;
}

// 메모리 할당
void *allocMem(QWORD size) {
	QWORD alignSize, relativeAddr;
	long offset;
	int arraySize, blockIdx;

	// 메모리 크기를 버디 블록의 크기로 맞춤
	alignSize = getBuddyBlockSize(size);
	if(alignSize == 0) return NULL;

	// 만약 여유 공간이 충분하지 않으면 실패
	if(gs_dynmem.startAddr + gs_dynmem.usedSize + alignSize > gs_dynmem.endAddr) return NULL;

	// 버디 블록 할당 후 할당된 블록이 속한 블록 리스트 인덱스 반환
	offset = allocBuddyBlock(alignSize);
	if(offset == -1) return NULL;

	blockIdx = getMatchSizeIdx(alignSize);

	// 블록 크기 저장하는 영역에 실제로 할당된 버디 블록이 속한 블록 리스트의 인덱스 저장. 메모리 해제시 블록 리스트 인덱스로 사용
	relativeAddr = alignSize * offset;
	arraySize = relativeAddr / DYNMEM_MIN_SIZE;
	gs_dynmem.allocBlockIdx[arraySize] = (BYTE)blockIdx;
	gs_dynmem.usedSize += alignSize;

	return (void*)(relativeAddr + gs_dynmem.startAddr);
}

// 가장 가까운 버디 블록 크기로 정렬된 크기 반환
static QWORD getBuddyBlockSize(QWORD size) {
	long i;

	for(i = 0; i < gs_dynmem.maxLvCnt; i++) if(size <= (DYNMEM_MIN_SIZE << i)) return (DYNMEM_MIN_SIZE << i);
	return 0;
}

// 버디 블록 알고리즘으로 메모리 블록 할당. 메모리 크기는 버디 블록 크기로 요청해야 함
static int allocBuddyBlock(QWORD alignSize) {
	int blockIdx, freeOffset, i;
	BOOL preInterruptFlag;

	// 블록 크기를 만족하는 블록 리스트 인덱스 검색
	blockIdx = getMatchSizeIdx(alignSize);
	if(blockIdx == -1) return -1;

	// 동기화 처리(임계 영역 시작)
	preInterruptFlag = lockData();

	// 만족하는 블록 리스트부터 최상위 블록 리스트까지 검색해 블록 선택
	for(i = blockIdx; i < gs_dynmem.maxLvCnt; i++) {
		// 블록 리스트의 비트맵 확인해 블록 존재여부 확인
		freeOffset = findFreeBlock(i);
		if(freeOffset != -1) break;
	}

	// 마지막 블록 리스트까지 검색해도 없으면 실패
	if(freeOffset == -1) {
		unlockData(preInterruptFlag);	// 임계 영역 끝
		return -1;
	}

	// 블록을 찾았으니 빈 것으로 표시
	setFlagBitmap(i, freeOffset, DYNMEM_EMPTY);

	// 상위 블록에서 블록 찾았으면 상위 블록 분할
	if(i > blockIdx) {
		// 검색된 블록 리스트에서 검색 시작한 블록 리스트까지 내려가며 왼쪽 블록은 빈 것, 오른쪽 블록은 존재하는 것으로 표시
		for(i = i - 1; i >= blockIdx; i--) {
			// 왼쪽 블록
			setFlagBitmap(i, freeOffset * 2, DYNMEM_EMPTY);
			// 오른쪽 블록
			setFlagBitmap(i, freeOffset * 2 + 1, DYNMEM_EXIST);
			// 왼쪽 블록 분할
			freeOffset = freeOffset * 2;
		}
	}
	unlockData(preInterruptFlag);	// 임계 영역 끝

	return freeOffset;
}

// 전달된 크기와 가장 근접한 블록 리스트 인덱스 반환
static int getMatchSizeIdx(QWORD alignSize) {
	int i;

	for(i = 0; i < gs_dynmem.maxLvCnt; i++) if(alignSize <= (DYNMEM_MIN_SIZE << i)) return i;
	return -1;
}

// 블록 리스트 비트맵 검색 후 블록이 존재하면 블록 오프셋 반환
static int findFreeBlock(int blockIdx) {
	int i, maxCnt;
	BYTE *bitmap;
	QWORD *_bitmap;

	// 비트맵에 데이터가 존재하지 않는다면 실패
	if(gs_dynmem.bitmapLv[blockIdx].bitCnt == 0 ) return -1;

	// 블록 리스트에 존재하는 총 블록 수를 구한 후 그 개수만큼 비트맵 검색
	maxCnt = gs_dynmem.smallBlockCnt >> blockIdx;
	bitmap = gs_dynmem.bitmapLv[blockIdx].bitmap;
	for(i = 0; i < maxCnt;) {
		// QWORD는 8 * 8비트 => 64비트이므로, 64비트를 한꺼번에 검사해서 1인 비트가 있는지 확인
		if(((maxCnt - i) / 64) > 0) {
			_bitmap = (QWORD*)&(bitmap[i / 8]);
			// 만약 8바이트가 모두 0이면 모두 제외
			if(*_bitmap == 0) {
				i += 64;
				continue;
			}
		}

		if((bitmap[i / 8] & (DYNMEM_EXIST << (i % 8))) != 0) return i;
		i++;
	}
	return -1;
}

// 비트맵에 플래그 설정
static void setFlagBitmap(int blockIdx, int offset, BYTE flag) {
	BYTE *bitmap;

	bitmap = gs_dynmem.bitmapLv[blockIdx].bitmap;
	if(flag == DYNMEM_EXIST) {
		// 해당 위치에 데이터가 비어있다면 개수 증가
		if((bitmap[offset / 8] & (0x01 << (offset % 8))) == 0) gs_dynmem.bitmapLv[blockIdx].bitCnt++;
		bitmap[offset / 8] |= (0x01 << (offset % 8));
	} else {	// 해당 위치에 데이터가 존재하면 개수 감소
		if((bitmap[offset / 8] & (0x01 << (offset % 8))) != 0) gs_dynmem.bitmapLv[blockIdx].bitCnt--;
		bitmap[offset / 8] &= ~(0x01 << (offset % 8));
	}
}

// 할당받은 메모리 해제
BOOL freeMem(void *addr) {
	QWORD relativeAddr, blockSize;
	int arraySize, blockIdx, bitmapOffset;

	if(addr == NULL) return FALSE;

	// 넘겨 받은 어드레스를 블록 풀 기준의 어드레스로 변환해 할당했던 블록의 크기 검색
	relativeAddr = ((QWORD)addr) - gs_dynmem.startAddr;
	arraySize = relativeAddr / DYNMEM_MIN_SIZE;

	// 할당되어 있지 않으면 반환 안 함
	if(gs_dynmem.allocBlockIdx[arraySize] == 0xFF) return FALSE;

	// 할당된 블록이 속한 블록 리스트의 인덱스가 저장된 곳 초기화 후 블록이 포함된 블록 리스트 검색
	blockIdx = (int)gs_dynmem.allocBlockIdx[arraySize];
	gs_dynmem.allocBlockIdx[arraySize] = 0xFF;
	// 버디 블록의 최소 크기를 블록 리스트 인덱스로 시프트해 할당된 블록 크기 계산
	blockSize = DYNMEM_MIN_SIZE << blockIdx;

	// 블록 리스트 내 블록 오프셋을 구해 블록 해제
	bitmapOffset = relativeAddr / blockSize;
	if(freeBuddyBlock(blockIdx, bitmapOffset) == TRUE) {
		gs_dynmem.usedSize -= blockSize;
		return TRUE;
	}
	return FALSE;
}

// 블록 리스트의 버디 블록 해제
static BOOL freeBuddyBlock(int blockIdx, int offset) {
	int buddyOffset, i;
	BOOL flag, preInterruptFlag;

	// 동기화 처리(임계 영역 시작)
	preInterruptFlag = lockData();

	// 블록 리스트 끝까지 인접 블록을 검사해 결합할 수 없을 때까지 반복
	for(i = blockIdx; i < gs_dynmem.maxLvCnt; i++) {
		// 현재 블록은 존재하는 상태로 설정
		setFlagBitmap(i, offset, DYNMEM_EXIST);

		// 블록의 오프셋이 왼쪽이면 오른쪽검사, 오른쪽이면 왼쪽검사해 인접 블록이 존재하면 결합
		if((offset % 2) == 0) buddyOffset = offset + 1;
		else buddyOffset = offset -1;
		flag = getFlagBitmap(i, buddyOffset);

		// 블록이 비어있으면 종료
		if(flag == DYNMEM_EMPTY) break;

		// 여기까지 왔으면 인접 블록이 있으니 블록 결합. 블록을 모두 비우고 상위 블록으로 이동
		setFlagBitmap(i, buddyOffset, DYNMEM_EMPTY);
		setFlagBitmap(i, offset, DYNMEM_EMPTY);

		// 상위 블록 리스트의 블록 오프셋으로 변경하고 위 과정을 상위 블록에서 다시 반복
		offset = offset / 2;
	}

	unlockData(preInterruptFlag);
	return TRUE;
}

// 블록 리스트의 해당 위치에 비트맵 반환
static BYTE getFlagBitmap(int blockIdx, int offset) {
	BYTE *bitmap;

	bitmap = gs_dynmem.bitmapLv[blockIdx].bitmap;
	if((bitmap[offset / 8] & (0x01 << (offset % 8))) != 0x00) return DYNMEM_EXIST;
	return DYNMEM_EMPTY;
}

// 동적 메모리 영역 정보 반환
void getDynMemInfo(QWORD *dynmemStartAddr, QWORD *dynmemTotalSize, QWORD *metaDataSize, QWORD *usedMemSize) {
	*dynmemStartAddr = DYNMEM_START_ADDR;
	*dynmemTotalSize = calcDynMemSize();
	*metaDataSize = calcMetaBlockCnt(*dynmemTotalSize) * DYNMEM_MIN_SIZE;
	*usedMemSize = gs_dynmem.usedSize;
}

// 동적 메모리 영역 관리 자료구조 반환
DYNMEM *getDynMemManager(void) {
	return &gs_dynmem;
}
