/*
 * FileSystem.h
 *
 *  Created on: 2017. 8. 9.
 *      Author: Yummy
 */

#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include <Types.h>
#include <Synchronize.h>
#include <HardDisk.h>

#pragma once

// 매크로와 함수 포인터
#define FILESYSTEM_SIGN			0x7E38CF10
// 클러스터 크기(섹터 수), 4KB
#define FILESYSTEM_PERSECTOR		8
// 파일 클러스터 마지막 표시
#define FILESYSTEM_LAST_CLUSTER		0xFFFFFFFF
// 빈 클러스터 표시
#define FILESYSTEM_FREE_CLUSTER		0x00
// 루트 디렉터리에 있는 최대 디렉터리 엔트리 수
#define FILESYSTEM_MAXDIRENTRYCNT	((FILESYSTEM_PERSECTOR * 512) / sizeof(DIRENTRY))
// 클러스터 크기(바이트 수)
#define FILESYSTEM_CLUSTER_SIZE		(FILESYSTEM_PERSECTOR * 512)

// 파일 이름의 최대 길이
#define FILESYSTEM_MAXFILENAMELEN	24
// 하드 디스크 제어에 관련된 함수 포인터 타입 정의
typedef BOOL (*_readHDDInfo)(BOOL pri, BOOL master, HDDINFO *hddInfo);
typedef int (*_readHDDSector)(BOOL pri, BOOL master, DWORD lba, int sectorCnt, char *buf);
typedef int (*_writeHDDSector)(BOOL pri, BOOL master, DWORD lba, int sectorCnt, char *buf);

// 구조체. 1바이트로 정렬
#pragma pack(push, 1)

// 파티션 자료구조
typedef struct partition {
	// 부팅 가능 플래그. 0x80이면 부팅 가능 / 0x00이면 부팅 불가
	BYTE bootFlag;
	// 파티션 시작 주소. 현재 거의 사용 안함
	BYTE startCHSAddr[3];
	// 파티션 타입
	BYTE type;
	// 파티션 마지막 주소. 역시 거의 사용 안함
	BYTE endCHSAddr[3];
	// LBA 주소로 나타낸 파티션 시작 주소
	DWORD startLBAAddr;
	// 파티션에 포함된 섹터 수
	DWORD size;
} PARTITION;

// MBR 자료구조
typedef struct mbr {
	// 부트 로더 코드 위치
	BYTE bootCode[430];

	// 파일 시스템 시그너처, 0x7E38CF10
	DWORD sign;
	// 예약된 영역 섹터 수
	DWORD reserved_sectorCnt;
	// 클러스터 링크 테이블 섹터 수
	DWORD linkSectorCnt;
	// 클러스터 전체 개수
	DWORD totalClusterCnt;

	// 파티션 테이블
	PARTITION part[4];

	// 부트 로더 시그너처, 0xAA55
	BYTE bootSign[2];
} MBR;

// 디렉터리 엔트리 자료구조
typedef struct directoryEntry {
	// 파일 이름
	char name[FILESYSTEM_MAXFILENAMELEN];
	// 파일 실제 크기
	DWORD size;
	// 파일이 시작하는 클러스터 인덱스
	DWORD startClusterIdx;
} DIRENTRY;

#pragma pack(pop)

// 파일 시스템 관리
typedef struct fileSystemManager {
	// 파일 시스템이 정상적으로 인식되었는지 여부
	BOOL mnt;

	// 각 영역 섹터 수 및 시작 LBA 주소
	DWORD reserved_sectorCnt;
	DWORD linkStartAddr;
	DWORD linkSectorCnt;
	DWORD dataStartAddr;
	// 데이터 영역 클러스터 총 개수
	DWORD totalClusterCnt;

	// 마지막으로 클러스터를 할당한 클러스터 링크 테이블의 섹터 오프셋 저장
	DWORD lastAllocLinkOffset;

	// 파일 시스템 동기화 객체
	MUTEX mut;
} FILESYSTEMMANAGER;

BOOL initFileSystem(void);
BOOL _mount(void);
BOOL _format(void);
BOOL getHDDInfo(HDDINFO *info);

BOOL readClusterLink(DWORD offset, BYTE *buf);
BOOL writeClusterLink(DWORD offset, BYTE *buf);
BOOL readCluster(DWORD offset, BYTE *buf);
BOOL writeCluster(DWORD offset, BYTE *buf);
DWORD findFreeCluster(void);
BOOL setClusterLink(DWORD idx, DWORD data);
BOOL getClusterLink(DWORD idx, DWORD *data);
int findFreeDirEntry(void);
BOOL setDirEntry(int idx, DIRENTRY *entry);
BOOL getDirEntry(int idx, DIRENTRY *entry);
int findDirEntry(const char *name, DIRENTRY *entry);
void getFileSystemInfo(FILESYSTEMMANAGER *manager);

#endif /*__FILESYSTEM_H__*/
