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
#define FILESYSTEM_PER_SECTOR		8
// 파일 클러스터 마지막 표시
#define FILESYSTEM_LAST_CLUSTER		0xFFFFFFFF
// 빈 클러스터 표시
#define FILESYSTEM_FREE_CLUSTER		0x00
// 루트 디렉터리에 있는 최대 디렉터리 엔트리 수
#define FILESYSTEM_MAXDIR_ENTRYCNT	((FILESYSTEM_PER_SECTOR * 512) / sizeof(DIRENTRY))
// 클러스터 크기(바이트 수)
#define FILESYSTEM_CLUSTER_SIZE		(FILESYSTEM_PER_SECTOR * 512)

// 핸들의 최대 개수, 최대 태스크 수 3배로 생성
#define FILESYSTEM_HANDLE_MAXCNT	(TASK_MAXCNT * 3)

// 파일 이름의 최대 길이
#define FILESYSTEM_FILENAME_MAXLEN	24

// 핸들 타입 정의
#define FILESYSTEM_TYPE_FREE		0
#define FILESYSTEM_TYPE_FILE		1
#define FILESYSTEM_TYPE_DIR		2

// SEEK 옵션 정의
#define FILESYSTEM_SEEK_SET		0
#define FILESYSTEM_SEEK_CUR		1
#define FILESYSTEM_SEEK_END		2

// 하드 디스크 제어에 관련된 함수 포인터 타입 정의
typedef BOOL (*_kReadHDDInfo)(BOOL pri, BOOL master, HDDINFO *hddInfo);
typedef int (*_kReadHDDSector)(BOOL pri, BOOL master, DWORD lba, int sectorCnt, char *buf);
typedef int (*_kWriteHDDSector)(BOOL pri, BOOL master, DWORD lba, int sectorCnt, char *buf);

// 파일 시스템 함수를 표준 입출력 함수 이름으로 재정의
#define fopen		kFileOpen
#define fread		kFileRead
#define fwrite		kFileWrite
#define fseek		kFileSeek
#define fclose		kFileClose
#define fremove		kRemoveFile
#define dopen		kDirOpen
#define dread		kDirRead
#define drewind		kDirRewind
#define dclose		kDirClose

// 파일 시스템 매크로를 표준 입출력 매크로로 재정의
#define SEEK_SET	FILESYSTEM_SEEK_SET
#define SEEK_CUR	FILESYSTEM_SEEK_CUR
#define SEEK_END	FILESYSTEM_SEEK_END

// 파일 시스템 타입과 필드를 표준 입출력 타입으로 재정의
#define size_t		DWORD
#define dirent		kDirectoryEntry
#define d_name		fileName

// 구조체. 1바이트로 정렬
#pragma pack(push, 1)

// 파티션 자료구조
typedef struct kPartition {
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
typedef struct kMBR {
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
typedef struct kDirectoryEntry {
	// 파일 이름
	char fileName[FILESYSTEM_FILENAME_MAXLEN];
	// 파일 실제 크기
	DWORD size;
	// 파일이 시작하는 클러스터 인덱스
	DWORD startClusterIdx;
} DIRENTRY;

// 파일 관리하는 파일 핸들 자료구조
typedef struct kFileHandle {
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
typedef struct kDirectoryHandle {
	// 루트 디렉터리 저장해둔 버퍼
	DIRENTRY *dirBuf;
	// 디렉터리 포인터의 현재 위치
	int nowOffset;
} DIRHANDLE;

// 파일과 디렉터리에 대한 정보가 들어있는 자료구조
typedef struct kFileDirHandle {
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
typedef struct kFileSystemManager {
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

BOOL kInitFileSystem(void);
BOOL kMount(void);
BOOL kFormat(void);
BOOL kGetHDDInfo(HDDINFO *info);
// Low Level Function
static BOOL kReadClusterLink(DWORD offset, BYTE *buf);
static BOOL kWriteClusterLink(DWORD offset, BYTE *buf);
static BOOL kReadDataArea(DWORD offset, BYTE *buf);
static BOOL kWriteDataArea(DWORD offset, BYTE *buf);
static DWORD kFindFreeCluster(void);
static BOOL kSetClusterLink(DWORD idx, DWORD data);
static BOOL kGetClusterLink(DWORD idx, DWORD *data);
static int kFindFreeDirEntry(void);
static BOOL kSetDirEntry(int idx, DIRENTRY *entry);
static BOOL kGetDirEntry(int idx, DIRENTRY *entry);
static int kFindDirEntry(const char *name, DIRENTRY *entry);
void kGetFileSystemInfo(FILESYSTEMMANAGER *manager);

// High Level Function
FILE *kFileOpen(const char *name, const char *mode);
DWORD kFileRead(void *buf, DWORD size, DWORD cnt, FILE *file);
DWORD kFileWrite(const void *buf, DWORD size, DWORD cnt, FILE *file);
int kFileSeek(FILE *file, int offset, int point);
int kFileClose(FILE *file);
int kRemoveFile(const char *name);
DIR *kDirOpen(const char *name);
struct kDirectoryEntry *dirRead(DIR *dir);
void kDirRewind(DIR *dir);
int kDirClose(DIR *dir);
BOOL kFilePadding(FILE *file, DWORD cnt);
BOOL kIsFileOpen(const DIRENTRY *entry);

static void *kAllocFileDirHandle(void);
static void kFreeFileDirHandle(FILE *file);
static BOOL kMakeFile(const char *name, DIRENTRY *entry, int *dirEntryIdx);
static BOOL kFreeClusterAll(DWORD idx);
static BOOL kUpdateDirEntry(FILEHANDLE *handle);

static BOOL kInReadClusterNonCache(DWORD offset, BYTE *buf);
static BOOL kInReadClusterOnCache(DWORD offset, BYTE *buf);
static BOOL kInWriteClusterNonCache(DWORD offset, BYTE *buf);
static BOOL kInWriteClusterOnCache(DWORD offset, BYTE *buf);
static BOOL kInReadDataNonCache(DWORD offset, BYTE *buf);
static BOOL kInReadDataOnCache(DWORD offset, BYTE *buf);
static BOOL kInWriteDataNonCache(DWORD offset, BYTE *buf);
static BOOL kInWriteDataOnCache(DWORD offset, BYTE *buf);
static CACHEBUF *kAllocCacheBufOnFlush(int idx);
BOOL kFlushFileSystemCache(void);

#endif /*__FILESYSTEM_H__*/
