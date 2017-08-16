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
#include <CacheMem.h>

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

// 핸들의 최대 개수, 최대 태스크 수 3배로 생성
#define FILESYSTEM_HANDLE_MAXCNT	(TASK_MAXCNT * 3)

// 파일 이름의 최대 길이
#define FILESYSTEM_MAXFILENAMELEN	24

// 핸들 타입 정의
#define FILESYSTEM_TYPE_FREE		0
#define FILESYSTEM_TYPE_FILE		1
#define FILESYSTEM_TYPE_DIR		2

// SEEK 옵션 정의
#define FILESYSTEM_SEEK_SET		0
#define FILESYSTEM_SEEK_CUR		1
#define FILESYSTEM_SEEK_END		2

// 하드 디스크 제어에 관련된 함수 포인터 타입 정의
typedef BOOL (*_readHDDInfo)(BOOL pri, BOOL master, HDDINFO *hddInfo);
typedef int (*_readHDDSector)(BOOL pri, BOOL master, DWORD lba, int sectorCnt, char *buf);
typedef int (*_writeHDDSector)(BOOL pri, BOOL master, DWORD lba, int sectorCnt, char *buf);

// 파일 시스템 함수를 표준 입출력 함수 이름으로 재정의
#define fopen		fileOpen
#define fread		fileRead
#define fwrite		fileWrite
#define fseek		fileSeek
#define fclose		fileClose
#define fremove		removeFile
#define dopen		dirOpen
#define dread		dirRead
#define drewind		dirRewind
#define dclose		dirClose

// 파일 시스템 매크로를 표준 입출력 매크로로 재정의
#define SEEK_SET	FILESYSTEM_SEEK_SET
#define SEEK_CUR	FILESYSTEM_SEEK_CUR
#define SEEK_END	FILESYSTEM_SEEK_END

// 파일 시스템 타입과 필드를 표준 입출력 타입으로 재정의
#define size_t		DWORD
#define dirent		directoryEntry
#define d_name		fileName

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
	char fileName[FILESYSTEM_MAXFILENAMELEN];
	// 파일 실제 크기
	DWORD size;
	// 파일이 시작하는 클러스터 인덱스
	DWORD startClusterIdx;
} DIRENTRY;

// 파일 관리하는 파일 핸들 자료구조
typedef struct fileHandle {
	// 파일이 존재하는 디렉터리 엔트리 오프셋
	int dirEntryOffset;
	// 파일 크기
	DWORD size;
	// 파일의 시작 클러스터 인덱스
	DWORD startClusterIdx;
	// 현재 I/O가 수행중인 클러스터의 인덱스
	DWORD nowClusterIdx;
	// 현재 클러스터의 바로 이전 클러스터의 인덱스
	DWORD preClusterIdx;
	// 파일 포인터 현재 위치
	DWORD nowOffset;
} FILEHANDLE;

// 디렉터리 관리하는 디렉터리 핸들 자료구조
typedef struct directoryHandle {
	// 루트 디렉터리 저장해둔 버퍼
	DIRENTRY *dirBuf;
	// 디렉터리 포인터의 현재 위치
	int nowOffset;
} DIRHANDLE;

// 파일과 디렉터리에 대한 정보가 들어있는 자료구조
typedef struct fileDirHandle {
	// 자료구조 타입 설정. 파일 핸들이나 디렉터리 핸들 또는 빈 핸들 타입 지정 가능
	BYTE type;
	// type 값에 따라 파일이나 디렉터리로 사용
	union {
		// 파일 핸들
		FILEHANDLE fileHandle;
		// 디렉터리 핸들
		DIRHANDLE dirHandle;
	};
} FILE, DIR;

// 파일 시스템 관리
typedef struct fileSystemManager {
	// 파일 시스템이 정상적으로 인식되었는지 여부
	BOOL mnt;

	// 각 영역 섹터 수 및 시작 LBA 주소
	DWORD reserved_sectorCnt;
	DWORD linkStartAddr;
	DWORD linkAreaSize;
	DWORD dataStartAddr;
	// 데이터 영역 클러스터 총 개수
	DWORD totalClusterCnt;

	// 마지막으로 클러스터를 할당한 클러스터 링크 테이블의 섹터 오프셋 저장
	DWORD lastAllocLinkOffset;

	// 파일 시스템 동기화 객체
	MUTEX mut;

	// 핸들 풀 어드레스
	FILE *sysHandle;

	// 캐시 사용 여부
	BOOL onCache;
} FILESYSTEMMANAGER;

#pragma pack(pop)

BOOL initFileSystem(void);
BOOL _mount(void);
BOOL _format(void);
BOOL getHDDInfo(HDDINFO *info);
// Low Level Function
static BOOL readClusterLink(DWORD offset, BYTE *buf);
static BOOL writeClusterLink(DWORD offset, BYTE *buf);
static BOOL readDataArea(DWORD offset, BYTE *buf);
static BOOL writeDataArea(DWORD offset, BYTE *buf);
static DWORD findFreeCluster(void);
static BOOL setClusterLink(DWORD idx, DWORD data);
static BOOL getClusterLink(DWORD idx, DWORD *data);
static int findFreeDirEntry(void);
static BOOL setDirEntry(int idx, DIRENTRY *entry);
static BOOL getDirEntry(int idx, DIRENTRY *entry);
static int findDirEntry(const char *name, DIRENTRY *entry);
void getFileSystemInfo(FILESYSTEMMANAGER *manager);

// High Level Function
FILE *fileOpen(const char *name, const char *mode);
DWORD fileRead(void *buf, DWORD size, DWORD cnt, FILE *file);
DWORD fileWrite(const void *buf, DWORD size, DWORD cnt, FILE *file);
int fileSeek(FILE *file, int offset, int point);
int fileClose(FILE *file);
int removeFile(const char *name);
DIR *dirOpen(const char *name);
struct directoryEntry *dirRead(DIR *dir);
void dirRewind(DIR *dir);
int dirClose(DIR *dir);
BOOL filePadding(FILE *file, DWORD cnt);
BOOL isFileOpen(const DIRENTRY *entry);

static void *allocFileDirHandle(void);
static void freeFileDirHandle(FILE *file);
static BOOL makeFile(const char *name, DIRENTRY *entry, int *dirEntryIdx);
static BOOL freeClusterAll(DWORD idx);
static BOOL updateDirEntry(FILEHANDLE *handle);

static BOOL inReadClusterNonCache(DWORD offset, BYTE *buf);
static BOOL inReadClusterOnCache(DWORD offset, BYTE *buf);
static BOOL inWriteClusterNonCache(DWORD offset, BYTE *buf);
static BOOL inWriteClusterOnCache(DWORD offset, BYTE *buf);
static BOOL inReadDataNonCache(DWORD offset, BYTE *buf);
static BOOL inReadDataOnCache(DWORD offset, BYTE *buf);
static BOOL inWriteDataNonCache(DWORD offset, BYTE *buf);
static BOOL inWriteDataOnCache(DWORD offset, BYTE *buf);
static CACHEBUF *allocCacheBufOnFlush(int idx);
BOOL flushFileSystemCache(void);

#endif /*__FILESYSTEM_H__*/
