/*
 * RamDisk.c
 *
 *  Created on: 2017. 8. 15.
 *      Author: Yummy
 */

#include <RamDisk.h>
#include <Util.h>
#include <DynMem.h>

// 램 디스크 관리 자료구조
static RDDMANAGER gs_rddManager;

// 램 디스크 드라이버 초기화 함수
BOOL kInitRDD(DWORD sectorCnt) {
	// 자료구조 초기화
	kMemSet(&gs_rddManager, 0, sizeof(gs_rddManager));

	// 램 디스크로 사용할 메모리 할당
	gs_rddManager.buf = (BYTE*)kAllocMem(sectorCnt * 512);
	if(gs_rddManager.buf == NULL) return FALSE;

	// 총 섹터 수와 동기화 객체 설정
	gs_rddManager.totalSectorCnt = sectorCnt;
	kInitMutex(&(gs_rddManager.mut));

	return TRUE;
}

// 램 디스크 정보 반환
BOOL kReadRDDInfo(BOOL pri, BOOL master, HDDINFO *hddInfo) {
	// 자료구조 초기화
	kMemSet(hddInfo, 0, sizeof(HDDINFO));

	// 총 섹터 수와 시리얼 번호, 모델 번호 설정
	hddInfo->totalSector = gs_rddManager.totalSectorCnt;
	kMemCpy(hddInfo->serialNum, "0000-0000", 9);
	kMemCpy(hddInfo->modelNum, "YummyHitOS RAM Disk v1.0", 24);

	return TRUE;
}

// 램 디스크에서 여러 섹터 읽어 반환
int kReadRDDSector(BOOL pri, BOOL master, DWORD lba, int sectorCnt, char *buf) {
	int realCnt;

	// LBA 어드레스부터 끝까지 읽을 수 있는 섹터 수와 읽어야할 섹터 수 비교해 실제 읽을 수 있는 수 계산
	realCnt = _MIN(gs_rddManager.totalSectorCnt - lba, sectorCnt);

	// 램 디스크 메모리에서 데이터를 실제로 읽을 섹터 수만큼 복사해 반환
	kMemCpy(buf, gs_rddManager.buf + (lba * 512), realCnt * 512);

	return realCnt;
}

// 램 디스크에 여러 섹터 씀
int kWriteRDDSector(BOOL pri, BOOL master, DWORD lba, int sectorCnt, char *buf) {
	int realCnt;

	// LBA 어드레스부터 끝까지 쓸 수 있는 섹터 수와 써야 할 섹터 수 비교해 실제 쓸 수 있는 수 계산
	realCnt = _MIN(gs_rddManager.totalSectorCnt - lba, sectorCnt);

	// 데이터를 실제로 쓸 섹터 수만큼 램 디스크 메모리에 복사
	kMemCpy(gs_rddManager.buf + (lba * 512), buf, realCnt * 512);

	return realCnt;
}
