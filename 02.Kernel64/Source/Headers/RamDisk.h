/*
 * RamDisk.h
 *
 *  Created on: 2017. 8. 15.
 *      Author: Yummy
 */

#ifndef __RAMDISK_H__
#define __RAMDISK_H__

#include <Types.h>
#include <Synchronize.h>
#include <HardDisk.h>

#pragma once

// 매크로, 램 디스크 총 섹터 수 8MB 크기로 생성
#define RDD_TOTALSECTORCNT	(8 * 1024 * 1024 / 512)

// 구조체, 1바이트로 정렬
#pragma pack(push, 1)

// 램 디스크 자료구조 구조체
typedef struct kRddManager {
	// 램 디스크용으로 할당받은 메모리 영역
	BYTE *buf;

	// 총 섹터 수
	DWORD totalSectorCnt;

	// 동기화 객체
	MUTEX mut;
} RDDMANAGER;

#pragma pack(pop)

BOOL kInitRDD(DWORD sectorCnt);
BOOL kReadRDDInfo(BOOL pri, BOOL master, HDDINFO *hddInfo);
int kReadRDDSector(BOOL pri, BOOL master, DWORD lba, int sectorCnt, char *buf);
int kWriteRDDSector(BOOL pri, BOOL master, DWORD lba, int sectorCnt, char *buf);

#endif /*__RAMDISK_H__*/
