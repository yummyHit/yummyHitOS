/*
 * FileSystem.c
 *
 *  Created on: 2017. 8. 9.
 *      Author: Yummy
 */

#include <FileSystem.h>
#include <HardDisk.h>
#include <DynMem.h>
#include <CLITask.h>
#include <Util.h>
#include <CacheMem.h>
#include <RamDisk.h>

// 파일 시스템 자료구조
static FILESYSTEMMANAGER gs_fileSystemManager;
// 파일 시스템 임시 버퍼
static BYTE gs_tmpBuf[FILESYSTEM_PER_SECTOR * 512];

// 하드 디스크 제어 관련 함수 포인터 선언
_kReadHDDInfo gs_readHDDInfo = NULL;
_kReadHDDSector gs_readHDDSector = NULL;
_kWriteHDDSector gs_writeHDDSector = NULL;

// 파일 시스템 초기화
BOOL kInitFileSystem(void) {
	BOOL onCache = FALSE;

	// 자료구조 초기화 및 동기화 객체 초기화
	kMemSet(&gs_fileSystemManager, 0, sizeof(gs_fileSystemManager));
	kInitMutex(&(gs_fileSystemManager.mut));

	// 하드 디스크 초기화
	if(kInitHDD() == TRUE) {
		// 초기화가 성공하면 함수 포인터를 하드 디스크용 함수로 설정
		gs_readHDDInfo = kReadHDDInfo;
		gs_readHDDSector = kReadHDDSector;
		gs_writeHDDSector = kWriteHDDSector;

		onCache = TRUE;
	} else if(kInitRDD(RDD_TOTALSECTORCNT) == TRUE) {
		// 초기화가 성공하면 함수 포인터를 램 디스크용 함수로 설정
		gs_readHDDInfo = kReadRDDInfo;
		gs_readHDDSector = kReadRDDSector;
		gs_writeHDDSector = kWriteRDDSector;

		// 램 디스크는 데이터가 남아 있지 않으니 매번 파일 시스템 생성
		if(kFormat() == FALSE) return FALSE;
	} else return FALSE;

	// 파일 시스템 연결
	if(kMount() == FALSE) return FALSE;

	// 핸들을 위한 공간 할당
	gs_fileSystemManager.sysHandle = (FILE*)kAllocMem(FILESYSTEM_HANDLE_MAXCNT * sizeof(FILE));

	// 메모리 할당이 실패하면 하드 디스크가 인식되지 않은 것으로 설정
	if(gs_fileSystemManager.sysHandle == NULL) {
		gs_fileSystemManager.mnt = FALSE;
		return FALSE;
	}

	// 핸들 풀 모두 0으로 설정해 초기화
	kMemSet(gs_fileSystemManager.sysHandle, 0, FILESYSTEM_HANDLE_MAXCNT * sizeof(FILE));

	// 캐시 활성화
	if(onCache == TRUE) gs_fileSystemManager.onCache = kInitCacheMem();

	return TRUE;
}

// 저수준 함수. 하드 디스크의 MBR을 읽어 파일 시스템 확인 후 관련 각종 정보를 자료구조에 삽입
BOOL kMount(void) {
	MBR *mbr;

	// 동기화 처리
	kLock(&(gs_fileSystemManager.mut));

	// MBR 읽음
	if(gs_readHDDSector(TRUE, TRUE, 0, 1, gs_tmpBuf) == FALSE) {
		// 동기화 처리
		kUnlock(&(gs_fileSystemManager.mut));
		return FALSE;
	}

	// 시그너처 확인 후 같으면 자료구조에 각 영역 정보 삽입
	mbr = (MBR*)gs_tmpBuf;
	if(mbr->sign != FILESYSTEM_SIGN) {
		// 동기화 처리
		kUnlock(&(gs_fileSystemManager.mut));
		return FALSE;
	}

	// 파일 시스템 인식 성공
	gs_fileSystemManager.mnt = TRUE;

	// 각 영역 시작 LBA 어드레스와 섹터 수 계산
	gs_fileSystemManager.reserved_sectorCnt = mbr->reserved_sectorCnt;
	gs_fileSystemManager.linkStartAddr = mbr->reserved_sectorCnt + 1;
	gs_fileSystemManager.linkAreaSize = mbr->linkSectorCnt;
	gs_fileSystemManager.dataStartAddr = mbr->reserved_sectorCnt + mbr->linkSectorCnt + 1;
	gs_fileSystemManager.totalClusterCnt = mbr->totalClusterCnt;

	// 동기화 처리
	kUnlock(&(gs_fileSystemManager.mut));
	return TRUE;
}

// 하드 디스크에 파일 시스템 생성
BOOL kFormat(void) {
	HDDINFO *hdd;
	MBR *mbr;
	DWORD totalSectorCnt, remainSectorCnt, maxClusterCnt, clusterCnt, linkSectorCnt, i;

	char buf[16];
	DWORD len, size = 0, tmp;
	FILE *fp;

	// 동기화 처리
	kLock(&(gs_fileSystemManager.mut));

	// 하드 디스크 정보 얻어 하드 디스크 총 섹터 수 구함
	hdd = (HDDINFO*)gs_tmpBuf;
	if(gs_readHDDInfo(TRUE, TRUE, hdd) == FALSE) {
		// 동기화 처리
		kUnlock(&(gs_fileSystemManager.mut));
		return FALSE;
	}
	totalSectorCnt = hdd->totalSector;

	// 전체 섹터 수를 4KB, 클러스터 크기로 나눠 최대 클러스터 수 계산
	maxClusterCnt = totalSectorCnt / FILESYSTEM_PER_SECTOR;

	// 최대 클러스터 수에 맞춰 링크 테이블 섹터 수 계산. 링크 데이터는 4바이터이니 한 섹터에 128개 들어감.
	// 총 개수를 128로 나눈 후 올림하여 클러스터 링크 섹터 수 구함
	linkSectorCnt = (maxClusterCnt + 127) / 128;

	// 예약된 영역은 현재 사용하지 않으니 디스크 전체 영역에서 MBR 영역과 클러스터 링크 테이블 영역 크기를 뺀 나머지가
	// 실제 데이터 영역. 해당 영역을 클러스터 크기로 나눠 실제 클러스터 개수 구함
	remainSectorCnt = totalSectorCnt - linkSectorCnt - 1;
	clusterCnt = remainSectorCnt / FILESYSTEM_PER_SECTOR;

	// 실제 사용 가능한 클러스터 수에 맞춰 다시 한 번 계산
	linkSectorCnt = (clusterCnt + 127) / 128;

	// MBR 영역 읽기
	if(gs_readHDDSector(TRUE, TRUE, 0, 1, gs_tmpBuf) == FALSE) {
		// 동기화 처리
		kUnlock(&(gs_fileSystemManager.mut));
		return FALSE;
	}

	// 파티션 정보와 파일 시스템 정보 설정
	mbr = (MBR*)gs_tmpBuf;
	kMemSet(mbr->part, 0, sizeof(mbr->part));
	mbr->sign = FILESYSTEM_SIGN;
	mbr->reserved_sectorCnt = 0;
	mbr->linkSectorCnt = linkSectorCnt;
	mbr->totalClusterCnt = clusterCnt;

	// MBR 영역에 1섹터 씀
	if(gs_writeHDDSector(TRUE, TRUE, 0, 1, gs_tmpBuf) == FALSE) {
		// 동기화 처리
		kUnlock(&(gs_fileSystemManager.mut));
		return FALSE;
	}

	// MBR 이후부터 루트 디렉터리까지 모두 0으로 초기화
	kMemSet(gs_tmpBuf, 0, 512);
	for(i = 0; i < (linkSectorCnt + FILESYSTEM_PER_SECTOR); i++) {
		// 루트 디렉터리(클러스터 0)는 이미 파일시스템이 사용중이니 할당된 것으로 표시
		if(i == 0) ((DWORD*)(gs_tmpBuf))[0] = FILESYSTEM_LAST_CLUSTER;
		else ((DWORD*)(gs_tmpBuf))[0] = FILESYSTEM_FREE_CLUSTER;

		// 1섹터씩 씀
		if(gs_writeHDDSector(TRUE, TRUE, i + 1, 1, gs_tmpBuf) == FALSE) {
			// 동기화 처리
			kUnlock(&(gs_fileSystemManager.mut));
			return FALSE;
		}
	}

	// 캐시 버퍼를 모두 비움
	if(gs_fileSystemManager.onCache == TRUE) {
		kClearCacheBuf(CACHE_CLUSTER_AREA);
		kClearCacheBuf(CACHE_DATA_AREA);
	}

	// 동기화 처리
	kUnlock(&(gs_fileSystemManager.mut));
	return TRUE;
}

// 파일 시스템에 연결된 하드 디스크 정보 반환
BOOL kGetHDDInfo(HDDINFO *info) {
	BOOL res;

	// 동기화 처리
	kLock(&(gs_fileSystemManager.mut));

	res = gs_readHDDInfo(TRUE, TRUE, info);

	// 동기화 처리
	kUnlock(&(gs_fileSystemManager.mut));

	return res;
}

// 클러스터 링크 테이블 내 오프셋에서 한 섹터 읽음
static BOOL kReadClusterLink(DWORD offset, BYTE *buf) {
	// 캐시 여부에 따라 다른 읽기 함수 호출
	if(gs_fileSystemManager.onCache == FALSE) kInReadClusterNonCache(offset, buf);
	else kInReadClusterOnCache(offset, buf);
}

// 클러스터 링크 테이블 내 오프셋에서 한 섹터 읽음. 내부적으로 사용하며 캐시 미사용
static BOOL kInReadClusterNonCache(DWORD offset, BYTE *buf) {
	// 클러스터 링크 테이블 영역 시작 어드레스를 더함
	return gs_readHDDSector(TRUE, TRUE, offset + gs_fileSystemManager.linkStartAddr, 1, buf);
}

// 클러스터 링크 테이블 내 오프셋에서 한 섹터 읽음. 내부적으로 사용하며 캐시 사용
static BOOL kInReadClusterOnCache(DWORD offset, BYTE *buf) {
	CACHEBUF *cacheBuf;

	// 먼저 캐시에 해당 클러스터 링크 테이블 있는지 확인
	cacheBuf = kFindCacheBuf(CACHE_CLUSTER_AREA, offset);

	// 캐시 버퍼에 있다면 캐시 내용 복사
	if(cacheBuf != NULL) {
		kMemCpy(buf, cacheBuf->buf, 512);
		return TRUE;
	}

	// 캐시 버퍼에 없다면 하드 디스크에서 직접 읽음
	if(kInReadClusterNonCache(offset, buf) == FALSE) return FALSE;

	// 캐시 할당 후 캐시 내용 갱신
	cacheBuf = kAllocCacheBufOnFlush(CACHE_CLUSTER_AREA);
	if(cacheBuf == NULL) return FALSE;

	// 캐시 버퍼에 읽은 내용 복사 후 태그 정보 갱신
	kMemCpy(cacheBuf->buf, buf, 512);
	cacheBuf->tag = offset;

	// 읽기 수행했으니 버퍼 내용을 수정되지 않은 것으로 표시
	cacheBuf->modified = FALSE;
	return TRUE;
}

// 클러스터 링크 테이블 영역의 캐시 버퍼 또는 데이터 영역의 캐시 버퍼에서 할당, 빈 캐시 버퍼가 없으면 오래된 것 중 골라 비운 후 사용
static CACHEBUF *kAllocCacheBufOnFlush(int idx) {
	CACHEBUF *cacheBuf;

	// 캐시 버퍼에 없다면 캐시 할당받아 캐시 내용 갱신
	cacheBuf = kAllocCacheBuf(idx);
	// 캐시를 할당받을 수 없다면 캐시 버퍼에서 오래된 것 찾아 비운 후 사용
	if(cacheBuf == NULL) {
		cacheBuf = kGetTargetCacheBuf(idx);
		// 오래된 캐시 버퍼도 할당 받을 수 없으면 오류
		if(cacheBuf == NULL) {
			kPrintF("Cache Allocation Fail...\n");
			return NULL;
		}

		// 캐시 버퍼 데이터가 수정되면 하드 디스크로 이동
		if(cacheBuf->modified == TRUE) switch(idx) {
			// 클러스터 링크 테이블 영역의 캐시인 경우
			case CACHE_CLUSTER_AREA:
				// 쓰기 실패시 오류
				if(kInWriteClusterNonCache(cacheBuf->tag, cacheBuf->buf) == FALSE) {
					kPrintF("Write to Cache Buffer Fail(Cluster)...\n");
					return NULL;
				}
				break;

			// 데이터 영역의 캐시인 경우
			case CACHE_DATA_AREA:
				// 쓰기 실패시 오류
				if(kInWriteDataNonCache(cacheBuf->tag, cacheBuf->buf) == FALSE) {
					kPrintF("Write to Cache Buffer Fail(Data)...\n");
					return NULL;
				}
				break;
			// 기타 오류
			default:
				kPrintF("kAllocCacheBufOnFlush Function Fail...\n");
				return NULL;
				break;
		}
	}

	return cacheBuf;
}

// 클러스터 링크 테이블 내 오프셋에 한 섹터 씀
static BOOL kWriteClusterLink(DWORD offset, BYTE *buf) {
	// 캐시 여부에 따라 다른 쓰기 함수 호출
	if(gs_fileSystemManager.onCache == FALSE) return kInWriteClusterNonCache(offset, buf);
	else return kInWriteClusterOnCache(offset, buf);
}

// 클러스터 링크 테이블 내 오프셋에 한 섹터 씀, 내부적으로 사용하며 캐시 미사용
static BOOL kInWriteClusterNonCache(DWORD offset, BYTE *buf) {
	// 클러스터 링크 테이블 영역의 시작 어드레스 더함
	return gs_writeHDDSector(TRUE, TRUE, offset + gs_fileSystemManager.linkStartAddr, 1, buf);
}

// 클러스터 링크 테이블 내 오프셋에 한 섹터 씀, 내부적으로 사용하며 캐시 사용
static BOOL kInWriteClusterOnCache(DWORD offset, BYTE *buf) {
	CACHEBUF *cacheBuf;

	// 캐시에 해당 클러스터 링크 테이블 있는지 확인
	cacheBuf = kFindCacheBuf(CACHE_CLUSTER_AREA, offset);

	// 캐시 버퍼에 있다면 캐시에 씀
	if(cacheBuf != NULL) {
		kMemCpy(cacheBuf->buf, buf, 512);

		// 쓰기를 수행했으니 버퍼 내용을 모두 수정으로 표시
		cacheBuf->modified = TRUE;
		return TRUE;
	}

	// 캐시 버퍼에 없다면 캐시 버퍼를 할당받아 캐시 내용 갱신
	cacheBuf = kAllocCacheBufOnFlush(CACHE_CLUSTER_AREA);
	if(cacheBuf == NULL) return FALSE;

	// 캐시 버퍼에 쓰고 태그 정보 갱신
	kMemCpy(cacheBuf->buf, buf, 512);
	cacheBuf->tag = offset;

	// 쓰기를 수행했으니 버퍼 내용 수정으로 변경
	cacheBuf->modified = TRUE;

	return TRUE;
}

// 데이터 영역 오프셋에서 한 클러스터 읽음
static BOOL kReadDataArea(DWORD offset, BYTE *buf) {
	// 캐시 여부에 따라 다른 읽기 함수 호출
	if(gs_fileSystemManager.onCache == FALSE) kInReadDataNonCache(offset, buf);
	else kInReadDataOnCache(offset, buf);
}

// 데이터 영역 오프셋에서 한 클러스터 읽음, 내부적으로 사용하며 캐시 미사용
static BOOL kInReadDataNonCache(DWORD offset, BYTE *buf) {
	// 데이터 영역 시작 어드레스 더함
	return gs_readHDDSector(TRUE, TRUE, (offset * FILESYSTEM_PER_SECTOR) + gs_fileSystemManager.dataStartAddr, FILESYSTEM_PER_SECTOR, buf);
}

// 데이터 영역 오프셋세엇 한 클러스터 읽음, 내부적으로 사용하며 캐시 사용
static BOOL kInReadDataOnCache(DWORD offset, BYTE *buf) {
	CACHEBUF *cacheBuf;

	// 캐시에 해당 데이터 클러스터 있는지 확인
	cacheBuf = kFindCacheBuf(CACHE_DATA_AREA, offset);

	// 캐시 버퍼에 있다면 캐시 내용 복사
	if(cacheBuf != NULL) {
		kMemCpy(buf, cacheBuf->buf, FILESYSTEM_CLUSTER_SIZE);
		return TRUE;
	}

	// 캐시 버퍼에 없다면 하드 디스크에서 직접 읽음
	if(kInReadDataNonCache(offset, buf) == FALSE) return FALSE;

	// 캐시 버퍼를 할당받아 캐시 내용 갱신
	cacheBuf = kAllocCacheBufOnFlush(CACHE_DATA_AREA);
	if(cacheBuf == NULL) return FALSE;

	// 캐시 버퍼에 읽은 내용 복사 후 태그 정보 갱신
	kMemCpy(cacheBuf->buf, buf, FILESYSTEM_CLUSTER_SIZE);
	cacheBuf->tag = offset;

	// 읽기를 수행했으니 버퍼 내용은 그대로
	cacheBuf->modified = FALSE;
	return TRUE;
}

// 데이터 영역 오프셋에 한 클러스터 씀
static BOOL kWriteDataArea(DWORD offset, BYTE *buf) {
	// 캐시 여부에 따라 다른 쓰기 함수 호출
	if(gs_fileSystemManager.onCache == FALSE) kInWriteDataNonCache(offset, buf);
	else kInWriteDataOnCache(offset, buf);
}

// 데이터 영역 오프셋에 한 클러스터 씀, 내부적으로 사용하며 캐시 미사용
static BOOL kInWriteDataNonCache(DWORD offset, BYTE *buf) {
	// 데이터 영역 시작 어드레스 더함
	return gs_writeHDDSector(TRUE, TRUE, (offset * FILESYSTEM_PER_SECTOR) + gs_fileSystemManager.dataStartAddr, FILESYSTEM_PER_SECTOR, buf);
}

// 데이터 영역 오프셋에 한 클러스터 씀, 내부적으로 사용하며 캐시 사용
static BOOL kInWriteDataOnCache(DWORD offset, BYTE *buf) {
	CACHEBUF *cacheBuf;

	// 캐시 버퍼에 해당 데이터 클러스터 있는지 확인
	cacheBuf = kFindCacheBuf(CACHE_DATA_AREA, offset);

	// 캐시 버퍼에 있다면 캐시 씀
	if(cacheBuf != NULL) {
		kMemCpy(cacheBuf->buf, buf, FILESYSTEM_CLUSTER_SIZE);

		// 쓰기를 수행했으니 버퍼 내용 갱신
		cacheBuf->modified = TRUE;

		return TRUE;
	}

	// 캐시 버퍼에 없다면 캐시를 할당 받아 캐시 내용 갱신
	cacheBuf = kAllocCacheBufOnFlush(CACHE_DATA_AREA);
	if(cacheBuf == NULL) return FALSE;

	// 캐시 버퍼에 쓰고 태그 정보 갱신
	kMemCpy(cacheBuf->buf, buf, FILESYSTEM_CLUSTER_SIZE);
	cacheBuf->tag = offset;

	// 쓰기를 수행했으니 버퍼 내용 갱신
	cacheBuf->modified = TRUE;

	return TRUE;
}

// 클러스터 링크 테이블 영역에서 빈 클러스터 검색
static DWORD kFindFreeCluster(void) {
	DWORD linkCnt, lastOffset, nowOffset, i, j;

	// 파일 시스템을 인식 못하면 실패
	if(gs_fileSystemManager.mnt == FALSE) return FILESYSTEM_LAST_CLUSTER;

	// 마지막으로 클러스터 할당한 클러스터 링크 테이블의 섹터 오프셋 가져옴
	lastOffset = gs_fileSystemManager.lastAllocLinkOffset;

	// 마지막으로 할당한 위치부터 루프 돌며 빈 클러스터 검색
	for(i = 0; i < gs_fileSystemManager.linkAreaSize; i++) {
		// 클러스터 링크 테이블의 마지막 섹터면 전체 섹터만큼 도는 것이 아니라 남은 클러스터 수만큼 루프 돌아야 함
		if((lastOffset + i) == (gs_fileSystemManager.linkAreaSize - 1)) linkCnt = gs_fileSystemManager.totalClusterCnt % 128;
		else linkCnt = 128;

		// 이번에 읽어야 할 클러스터 링크 테이블의 섹터 오프셋 구해서 읽음
		nowOffset = (lastOffset + i) % gs_fileSystemManager.linkAreaSize;
		if(kReadClusterLink(nowOffset, gs_tmpBuf) == FALSE) return FILESYSTEM_LAST_CLUSTER;

		// 섹터 내에서 루프 돌며 빈 클러스터 검색
		for(j = 0; j < linkCnt; j++) if(((DWORD*)gs_tmpBuf)[j] == FILESYSTEM_FREE_CLUSTER) break;

		// 찾았다면 클러스터 인덱스 반환
		if(j != linkCnt) {
			// 마지막으로 클러스터를 할당한 클러스터 링크 내 섹터 오프셋 저장
			gs_fileSystemManager.lastAllocLinkOffset = nowOffset;

			// 현재 클러스터 링크 테이블 오프셋을 감안해 클러스터 인덱스 계산
			return (nowOffset * 128) + j;
		}
	}
	return FILESYSTEM_LAST_CLUSTER;
}

// 클러스터 링크 테이블에 값 설정
static BOOL kSetClusterLink(DWORD idx, DWORD data) {
	DWORD offset;

	// 파일 시스템을 인식 못하면 실패
	if(gs_fileSystemManager.mnt == FALSE) return FALSE;

	// 한 섹터에 128개 클러스터 링크가 들어가니 128로 나누면 섹터 오프셋
	offset = idx / 128;

	// 해당 섹터를 읽어 링크 정보를 설정한 후 다시 저장
	if(kReadClusterLink(offset, gs_tmpBuf) == FALSE) return FALSE;

	((DWORD*)gs_tmpBuf)[idx % 128] = data;

	if(kWriteClusterLink(offset, gs_tmpBuf) == FALSE) return FALSE;

	return TRUE;
}

// 클러스터 링크 테이블 값 반환
static BOOL kGetClusterLink(DWORD idx, DWORD *data) {
	DWORD offset;

	// 파일 시스템을 인식 못하면 실패
	if(gs_fileSystemManager.mnt == FALSE) return FALSE;

	// 한 섹터에 128개 클러스터 링크가 들어가니 128로 나누면 섹터 오프셋
	offset = idx / 128;

	if(offset > gs_fileSystemManager.linkAreaSize) return FALSE;

	// 해당 섹터를 읽어 링크 정보 반환
	if(kReadClusterLink(offset, gs_tmpBuf) == FALSE) return FALSE;

	*data = ((DWORD*)gs_tmpBuf)[idx % 128];
	return TRUE;
}

// 루트 디렉터리에서 빈 디렉터리 엔트리 반환
static int kFindFreeDirEntry(void) {
	DIRENTRY *entry;
	int i;

	// 파일 시스템 인식 못하면 실패
	if(gs_fileSystemManager.mnt == FALSE) return -1;

	// 루트 디렉터리 읽음
	if(kReadDataArea(0, gs_tmpBuf) == FALSE) return -1;

	// 루트 디렉터리 내 루프를 돌며 빈 엔트리, 즉 시작 클러스터 번호가 0인 엔트리 검색
	entry = (DIRENTRY*)gs_tmpBuf;
	for(i = 0; i < FILESYSTEM_MAXDIR_ENTRYCNT; i++) if(entry[i].startClusterIdx == 0) return i;

	return -1;
}

// 루트 디렉터리의 해당 인덱스에 디렉터리 엔트리 설정
static BOOL kSetDirEntry(int idx, DIRENTRY *entry) {
	DIRENTRY *root;

	// 파일 시스템을 인식하지 못했거나 인덱스가 올바르지 않으면 실패
	if((gs_fileSystemManager.mnt == FALSE) || (idx < 0) || (idx >= FILESYSTEM_MAXDIR_ENTRYCNT)) return FALSE;

	// 루트 디렉터리 읽음
	if(kReadDataArea(0, gs_tmpBuf) == FALSE) return FALSE;

	// 루트 디렉터리에 있는 해당 데이터 갱신
	root = (DIRENTRY*)gs_tmpBuf;
	kMemCpy(root + idx, entry, sizeof(DIRENTRY));

	// 루트 디렉터리에 씀
	if(kWriteDataArea(0, gs_tmpBuf) == FALSE) return FALSE;
	return TRUE;
}

// 루트 디렉터리의 해당 인덱스에 위치하는 디렉터리 엔트리 반환
static BOOL kGetDirEntry(int idx, DIRENTRY *entry) {
	DIRENTRY *root;

	// 파일 시스템을 인식하지 못했거나 인덱스가 올바르지 않으면 실패
	if((gs_fileSystemManager.mnt == FALSE) || (idx < 0) || (idx >= FILESYSTEM_MAXDIR_ENTRYCNT)) return FALSE;

	// 루트 디렉터리 읽음
	if(kReadDataArea(0, gs_tmpBuf) == FALSE) return FALSE;

	// 루트 디렉터리에 있는 해당 데이터 갱신
	root = (DIRENTRY*)gs_tmpBuf;
	kMemCpy(entry, root + idx, sizeof(DIRENTRY));
	return TRUE;
}

// 루트 디렉터리에서 파일 이름이 일치하는 엔트리 찾아 인덱스 반환
static int kFindDirEntry(const char *name, DIRENTRY *entry) {
	DIRENTRY *root;
	int i, len;

	// 파일 시스템을 인식 못하면 실패
	if(gs_fileSystemManager.mnt == FALSE) return -1;

	// 루트 디렉터리 읽음
	if(kReadDataArea(0, gs_tmpBuf) == FALSE) return -1;

	len = kStrLen(name);
	// 루트 디렉터리 안에서 루프 돌며 파일 이름이 일치하는 엔트리 반환
	root = (DIRENTRY*)gs_tmpBuf;
	for(i = 0; i < FILESYSTEM_MAXDIR_ENTRYCNT; i++) if(kMemCmp(root[i].fileName, name, len) == 0) {
		kMemCpy(entry, root + i, sizeof(DIRENTRY));
		return i;
	}
	return -1;
}

// 파일 시스템 정보 반환
void kGetFileSystemInfo(FILESYSTEMMANAGER *manager) {
	kMemCpy(manager, &gs_fileSystemManager, sizeof(gs_fileSystemManager));
}

// 비어 있는 핸들 할당
static void *kAllocFileDirHandle(void) {
	int i;
	FILE *file;

	// 핸들 풀 모두 검색해 빈 핸들 반환
	file = gs_fileSystemManager.sysHandle;
	for(i = 0; i < FILESYSTEM_HANDLE_MAXCNT; i++) {
		// 비어있으면 반환
		if(file->type == FILESYSTEM_TYPE_FREE) {
			file->type = FILESYSTEM_TYPE_FILE;
			return file;
		}

		// 다음으로 이동
		file++;
	}
	return NULL;
}

// 사용한 핸들 반환
static void kFreeFileDirHandle(FILE *file) {
	// 전체 영역 초기화
	kMemSet(file, 0, sizeof(FILE));

	// 비어 있는 타입으로 설정
	file->type = FILESYSTEM_TYPE_FREE;
}

// 파일 생성
static BOOL kMakeFile(const char *name, DIRENTRY *entry, int *dirEntryIdx) {
	DWORD cluster;

	// 빈 클러스터 찾아 할당된 것으로 설정
	cluster = kFindFreeCluster();
	if((cluster == FILESYSTEM_LAST_CLUSTER) || (kSetClusterLink(cluster, FILESYSTEM_LAST_CLUSTER) == FALSE)) return FALSE;

	// 빈 디렉터리 엔트리 검색
	*dirEntryIdx = kFindFreeDirEntry();
	if(*dirEntryIdx == -1) {
		// 실패하면 할당받은 클러스터 반환
		kSetClusterLink(cluster, FILESYSTEM_FREE_CLUSTER);
		return FALSE;
	}

	// 디렉터리 엔트리 설정
	kMemCpy(entry->fileName, name, kStrLen(name) + 1);
	entry->startClusterIdx = cluster;
	entry->size = 0;

	// 디렉터리 엔트리 등록
	if(kSetDirEntry(*dirEntryIdx, entry) == FALSE) {
		// 실패하면 할당받은 클러스터 반환
		kSetClusterLink(cluster, FILESYSTEM_FREE_CLUSTER);
		return FALSE;
	}
	return TRUE;
}

// 파라미터로 넘어온 클러스터부터 파일의 끝까지 연결된 클러스터 모두 반환
static BOOL kFreeClusterAll(DWORD idx) {
	DWORD nowIdx, nextIdx;

	// 클러스터 인덱스 초기화
	nowIdx = idx;

	while(nowIdx != FILESYSTEM_LAST_CLUSTER) {
		// 다음 클러스터 인덱스 가져옴
		if(kGetClusterLink(nowIdx, &nextIdx) == FALSE) return FALSE;

		// 현재 클러스터를 빈 것으로 설정해 해제
		if(kSetClusterLink(nowIdx, FILESYSTEM_FREE_CLUSTER) == FALSE) return FALSE;

		// 현재 클러스터 인덱스를 다음 클러스터의 인덱스로 바꿈
		nowIdx = nextIdx;
	}
	return TRUE;
}

// 파일을 열거나 생성
FILE *kFileOpen(const char *name, const char *mode) {
	DIRENTRY entry;
	int offset, len;
	DWORD secCluster;
	FILE *file;

	// 파일 이름 검사
	len = kStrLen(name);
	if((len > (sizeof(entry.fileName) - 1)) || (len == 0)) return NULL;

	// 동기화 처리
	kLock(&(gs_fileSystemManager.mut));

	// 파일이 먼저 존재하는지 확인 후 없으면 옵션을 보고 파일 생성
	offset = kFindDirEntry(name, &entry);
	if(offset == -1) {
		// 파일이 없다면 읽기 옵션은 실패
		if(mode[0] == 'r') {
			// 동기화 처리
			kUnlock(&(gs_fileSystemManager.mut));
			return NULL;
		}

		// 나머지 옵션은 파일 생성
		if(kMakeFile(name, &entry, &offset) == FALSE) {
			// 동기화 처리
			kUnlock(&(gs_fileSystemManager.mut));
			return NULL;
		}
	} else if(mode[0] == 'w') { 	// 파일 내용을 비워야 한다면 파일에 연결된 클러스터를 모두 제거 후 파일 크기 0으로 설정
		// 시작 클러스터의 다음 클러스터 찾음
		if(kGetClusterLink(entry.startClusterIdx, &secCluster) == FALSE) {
			// 동기화 처리
			kUnlock(&(gs_fileSystemManager.mut));
			return NULL;
		}

		// 시작 클러스터를 마지막 클러스터로 만듬
		if(kSetClusterLink(entry.startClusterIdx, FILESYSTEM_LAST_CLUSTER) == FALSE) {
			// 동기화 처리
			kUnlock(&(gs_fileSystemManager.mut));
			return NULL;
		}

		// 다음 클러스터부터 마지막 클러스터까지 모두 해제
		if(kFreeClusterAll(secCluster) == FALSE) {
			// 동기화 처리
			kUnlock(&(gs_fileSystemManager.mut));
			return NULL;
		}

		// 파일 내용이 모두 비워졌으니 크기를 0으로 설정
		entry.size = 0;
		if(kSetDirEntry(offset, &entry) == FALSE) {
			// 동기화 처리
			kUnlock(&(gs_fileSystemManager.mut));
			return NULL;
		}
	}

	// 파일 핸들을 할당받아 데이터 설정
	file = kAllocFileDirHandle();
	if(file == NULL) {
		// 동기화 처리
		kUnlock(&(gs_fileSystemManager.mut));
		return NULL;
	}

	// 파일 핸들에 파일 정보 설정
	file->type = FILESYSTEM_TYPE_FILE;
	file->fileHandle.dirEntryOffset = offset;
	file->fileHandle.size = entry.size;
	file->fileHandle.startClusterIdx = entry.startClusterIdx;
	file->fileHandle.nowClusterIdx = entry.startClusterIdx;
	file->fileHandle.preClusterIdx = entry.startClusterIdx;
	file->fileHandle.nowOffset = 0;

	// 만약 Append 옵션이면 파일의 끝으로 이동
	if(mode[0] == 'a') kFileSeek(file, 0, FILESYSTEM_SEEK_END);

	// 동기화 처리
	kUnlock(&(gs_fileSystemManager.mut));
	return file;
}

// 파일을 읽어 버퍼로 복사
DWORD kFileRead(void *buf, DWORD size, DWORD cnt, FILE *file) {
	DWORD totalCnt, readCnt = 0, offset, copySize, nextClusterIdx;
	FILEHANDLE *handle;

	// 핸들이 파일 타입이 아니면 실패
	if((file == NULL) || (file->type != FILESYSTEM_TYPE_FILE)) return 0;
	handle = &(file->fileHandle);

	// 파일의 끝이거나 마지막 클러스터면 종료
	if((handle->nowOffset == handle->size) || (handle->nowClusterIdx == FILESYSTEM_LAST_CLUSTER)) return 0;

	// 파일 끝과 비교해 실제로 읽을 수 있는 값 계산
	totalCnt = _MIN(size * cnt, handle->size - handle->nowOffset);

	// 동기화 처리
	kLock(&(gs_fileSystemManager.mut));

	// 계산된 값만큼 다 읽을때까지 반복
	while(readCnt != totalCnt) {
		// 현재 클러스터를 읽음
		if(kReadDataArea(handle->nowClusterIdx, gs_tmpBuf) == FALSE) break;

		// 클러스터 내 파일 포인터가 존재하는 오프셋 계산
		offset = handle->nowOffset % FILESYSTEM_CLUSTER_SIZE;

		// 여러 클러스터에 걸쳐져 있다면 현재 클러스터에서 남은 만큼 읽고 다음 클러스터로 이동
		copySize = _MIN(FILESYSTEM_CLUSTER_SIZE - offset, totalCnt - readCnt);
		kMemCpy((char*)buf + readCnt, gs_tmpBuf + offset, copySize);

		// 읽은 바이트 수와 파일 포인터 위치 갱신
		readCnt += copySize;
		handle->nowOffset += copySize;

		// 현재 클러스터를 다 읽었으면 다음 클러스터로 이동
		if((handle->nowOffset % FILESYSTEM_CLUSTER_SIZE) == 0) {
			// 현재 클러스터의 링크 데이터를 찾아 다음 클러스터 얻음
			if(kGetClusterLink(handle->nowClusterIdx, &nextClusterIdx) == FALSE) break;

			// 클러스터 정보 갱신
			handle->preClusterIdx = handle->nowClusterIdx;
			handle->nowClusterIdx = nextClusterIdx;
		}
	}

	// 동기화 처리
	kUnlock(&(gs_fileSystemManager.mut));
	return (readCnt / size);
}

// 루트 디렉터리에서 디렉터리 엔트리 값 갱신
static BOOL kUpdateDirEntry(FILEHANDLE *handle) {
	DIRENTRY entry;

	// 디렉터리 엔트리 검색
	if((handle == NULL) || (kGetDirEntry(handle->dirEntryOffset, &entry) == FALSE)) return FALSE;

	// 파일 크기와 시작 클러스터 변경
	entry.size = handle->size;
	entry.startClusterIdx = handle->startClusterIdx;

	// 변경된 디렉터리 엔트리 설정
	if(kSetDirEntry(handle->dirEntryOffset, &entry) == FALSE) return FALSE;
	return TRUE;
}

// 버퍼 데이터를 파일에 씀
DWORD kFileWrite(const void *buf, DWORD size, DWORD cnt, FILE *file) {
	DWORD writeCnt = 0, totalCnt, offset, copySize, allocIdx, nextIdx;
	FILEHANDLE *handle;

	// 핸들이 파일 타입이 아니면 실패
	if((file == NULL) || (file->type != FILESYSTEM_TYPE_FILE)) return 0;
	handle = &(file->fileHandle);

	// 총 바이트 수
	totalCnt = size * cnt;

	// 동기화 처리
	kLock(&(gs_fileSystemManager.mut));

	// 다 쓸 때까지 반복
	while(writeCnt != totalCnt) {
		// 현재 클러스터가 파일의 끝이면 클러스터 할당해 연결
		if(handle->nowClusterIdx == FILESYSTEM_LAST_CLUSTER) {
			// 빈 클러스터 검색
			allocIdx = kFindFreeCluster();
			if(allocIdx == FILESYSTEM_LAST_CLUSTER) break;

			// 검색된 클러스터를 마지막 클러스터로 설정
			if(kSetClusterLink(allocIdx, FILESYSTEM_LAST_CLUSTER) == FALSE) break;

			// 파일의 마지막 클러스터에 할당된 클러스터 연결
			if(kSetClusterLink(handle->preClusterIdx, allocIdx) == FALSE) {
				// 실패하면 할당된 클러스터 해제
				kSetClusterLink(allocIdx, FILESYSTEM_FREE_CLUSTER);
				break;
			}

			// 현재 클러스터를 할당된 것으로 변경
			handle->nowClusterIdx = allocIdx;

			// 새로 할당받았으니 임시 클러스터 버퍼를 0으로 채움
			kMemSet(gs_tmpBuf, 0, FILESYSTEM_LAST_CLUSTER);
		} else if(((handle->nowOffset % FILESYSTEM_CLUSTER_SIZE) != 0) || ((totalCnt - writeCnt) < FILESYSTEM_CLUSTER_SIZE)) {
			// 한 클러스터를 채우지 못하면 클러스터를 읽어 임시 클러스터로 복사
			// 전체 클러스터를 덮어쓰는 경우가 아니면 부분만 덮어써야 하니 현재 클러스터를 읽음
			if(kReadDataArea(handle->nowClusterIdx, gs_tmpBuf) == FALSE) break;
		}

		// 클러스터 내 파일 포인터가 존재하는 오프셋 계산
		offset = handle->nowOffset % FILESYSTEM_CLUSTER_SIZE;

		// 여러 클러스터에 걸쳐져 있다면 현재 클러스터에서 남은 만큼 쓰고 다음 클러스터로 이동
		copySize = _MIN(FILESYSTEM_CLUSTER_SIZE - offset, totalCnt - writeCnt);
		kMemCpy(gs_tmpBuf + offset, (char*)buf + writeCnt, copySize);

		// 임시 버퍼에 삽입된 값을 디스크에 씀
		if(kWriteDataArea(handle->nowClusterIdx, gs_tmpBuf) == FALSE) break;

		// 쓴 바이트 수와 파일 포인터 위치 갱신
		writeCnt += copySize;
		handle->nowOffset += copySize;

		// 현재 클러스터 끝까지 다 썼으면 다음 클러스터로 이동
		if((handle->nowOffset % FILESYSTEM_CLUSTER_SIZE) == 0) {
			// 현재 클러스터의 링크 데이터로 다음 클러스터 얻음
			if(kGetClusterLink(handle->nowClusterIdx, &nextIdx) == FALSE) break;

			// 클러스터 정보 갱신
			handle->preClusterIdx = handle->nowClusterIdx;
			handle->nowClusterIdx = nextIdx;
		}
	}

	// 파일 크기가 변했다면 루트 디렉터리에 있는 디렉터리 엔트리 정보 갱신
	if(handle->size < handle->nowOffset) {
		handle->size = handle->nowOffset;
		kUpdateDirEntry(handle);
	}

	// 동기화 처리
	kUnlock(&(gs_fileSystemManager.mut));
	return (writeCnt / size);
}

// 파일을 count만큼 0으로 채움
BOOL kFilePadding(FILE *file, DWORD cnt) {
	BYTE *buf;
	DWORD remainCnt, writeCnt;

	// 핸들이 NULL이면 실패
	if(file == NULL) return FALSE;

	// 속도 향상을 위해 메모리를 할당받아 클러스터 단위로 쓰기 수행 메모리 할당
	buf = (BYTE*)kAllocMem(FILESYSTEM_CLUSTER_SIZE);
	if(buf == NULL) return FALSE;

	// 0으로 채움
	kMemSet(buf, 0, FILESYSTEM_CLUSTER_SIZE);
	remainCnt = cnt;

	// 클러스터 단위로 반복해 쓰기 수행
	while(remainCnt != 0) {
		writeCnt = _MIN(remainCnt, FILESYSTEM_CLUSTER_SIZE);
		if(kFileWrite(buf, 1, writeCnt, file) != writeCnt) {
			kFreeMem(buf);
			return FALSE;
		}
		remainCnt -= writeCnt;
	}
	kFreeMem(buf);
	return TRUE;
}

// 파일 포인터 위치 이동
int kFileSeek(FILE *file, int offset, int point) {
	DWORD realOffset, moveOffset, nowOffset, lastOffset, moveCnt, i, startClusterIdx, preClusterIdx, nowClusterIdx;
	FILEHANDLE *handle;

	// 핸들이 파일 타입이 아니면 종료
	if((file == NULL) || (file->type != FILESYSTEM_TYPE_FILE)) return 0;
	handle = &(file->fileHandle);

	// 옵션에 따라 실제 위치 계산. 음수면 파일의 시작 방향으로 이동, 양수면 파일의 끝 방향으로 이동
	switch(point) {
	// 파일 시작을 기준으로 이동
	case FILESYSTEM_SEEK_SET:
		// 파일의 처음이므로 이동할 오프셋이 음수면 0으로 설정
		if(offset <= 0) realOffset = 0;
		else realOffset = offset;
		break;
	// 현재 위치를 기준으로 이동
	case FILESYSTEM_SEEK_CUR:
		// 이동할 오프셋이 음수이며 현재 파일 포인터 값보다 크면 더 이상 갈 수 없으니 처음으로 이동
		if((offset < 0) && (handle->nowOffset <= (DWORD)-offset)) realOffset = 0;
		else realOffset = handle->nowOffset + offset;
		break;
	// 파일 끝부분을 기준으로 이동
	case FILESYSTEM_SEEK_END:
		// 이동할 오프셋이 음수이며 현재 파일 포인터 값보다 크면 더 이상 갈 수 없으므로 처음으로 이동
		if((offset < 0) && (handle->size <= (DWORD)-offset)) realOffset = 0;
		else realOffset = handle->size + offset;
		break;
	}

	// 파일의 마지막 클러스터 오프셋
	lastOffset = handle->size / FILESYSTEM_CLUSTER_SIZE;
	// 파일 포인터가 옮겨질 위치의 클러스터 오프셋
	moveOffset = realOffset / FILESYSTEM_CLUSTER_SIZE;
	// 현재 파일 포인터가 있는 클러스터 오프셋
	nowOffset = handle->nowOffset / FILESYSTEM_CLUSTER_SIZE;

	// 이동하는 클러스터 위치가 파일 마지막 클러스터 오프셋을 넘기면 현재 클러스터에서 마지막까지 이동 후 write 함수를 이용해 공백으로 나머지 채움
	if(lastOffset < moveOffset) {
		moveCnt = lastOffset - nowOffset;
		startClusterIdx = handle->nowClusterIdx;
	} else if(nowOffset <= moveOffset) {	// 이동하는 클러스터 위치가 현재 클러스터와 같거나 다음이라면 현재 클러스터 기준 차이나는 만큼 이동
		moveCnt = moveOffset - nowOffset;
		startClusterIdx = handle->nowClusterIdx;
	} else {	// 이동하는 클러스터 위치가 현재 클러스터 이전이라면, 첫 번째 클러스터부터 이동하며 검색
		moveCnt = moveOffset;
		startClusterIdx = handle->startClusterIdx;
	}

	// 동기화 처리
	kLock(&(gs_fileSystemManager.mut));

	// 클러스터 이동
	nowClusterIdx = startClusterIdx;
	for(i = 0; i < moveCnt; i++) {
		// 값 보관
		preClusterIdx = nowClusterIdx;

		// 다음 클러스터 인덱스 읽음
		if(kGetClusterLink(preClusterIdx, &nowClusterIdx) == FALSE) {
			// 동기화 처리
			kUnlock(&(gs_fileSystemManager.mut));
			return -1;
		}
	}

	// 클러스터를 이동했으면 클러스터 정보 갱신
	if(moveCnt > 0) {
		handle->preClusterIdx = preClusterIdx;
		handle->nowClusterIdx = nowClusterIdx;
	} else if(startClusterIdx == handle->startClusterIdx) {	// 첫 번째 클러스터로 이동하는 경우 핸들의 클러스터 값을 시작 클러스터로 설정
		handle->preClusterIdx = handle->startClusterIdx;
		handle->nowClusterIdx = handle->startClusterIdx;
	}

	// 실제 파일 크기를 넘어 파일 포인터가 이동했으면 파일 끝에서부터 남은 크기만큼 0으로 채워줌
	if(lastOffset < moveOffset) {
		handle->nowOffset = handle->size;
		// 동기화 처리
		kUnlock(&(gs_fileSystemManager.mut));

		// 남은 크기만큼 0으로 채움
		if(kFilePadding(file, realOffset - handle->size) == FALSE) return 0;
	}

	handle->nowOffset = realOffset;

	// 동기화 처리
	kUnlock(&(gs_fileSystemManager.mut));
	return 0;
}

// 파일 닫음
int kFileClose(FILE *file) {
	// 핸들 타입이 파일이 아니면 실패
	if((file == NULL) || (file->type != FILESYSTEM_TYPE_FILE)) return -1;

	// 핸들 반환
	kFreeFileDirHandle(file);
	return 0;
}

// 핸들 풀 검사해 파일이 열려있는지 확인
BOOL kIsFileOpen(const DIRENTRY *entry) {
	int i;
	FILE *file;

	// 핸들 풀의 시작 어드레스부터 끝까지 열린 파일만 검색
	file = gs_fileSystemManager.sysHandle;
	// 파일 타입 중 시작 클러스터가 일치하면 반환
	for(i = 0; i < FILESYSTEM_HANDLE_MAXCNT; i++) if((file[i].type == FILESYSTEM_TYPE_FILE) && (file[i].fileHandle.startClusterIdx == entry->startClusterIdx)) return TRUE;
	return FALSE;
}

// 파일 삭제
int kRemoveFile(const char *name) {
	DIRENTRY entry;
	int offset, len;

	// 파일 이름 검사
	len = kStrLen(name);
	if((len > (sizeof(entry.fileName) - 1)) || (len == 0)) return NULL;

	// 동기화 처리
	kLock(&(gs_fileSystemManager.mut));

	// 파일이 존재하는지 확인
	offset = kFindDirEntry(name, &entry);
	if(offset == -1) {
		// 동기화 처리
		kUnlock(&(gs_fileSystemManager.mut));
		return -1;
	}

	// 다른 태스크에서 해당 파일을 열고 있는지 핸들 풀을 검색해 파일이 열려있으면 삭제 불가
	if(kIsFileOpen(&entry) == TRUE) {
		// 동기화 처리
		kUnlock(&(gs_fileSystemManager.mut));
		return -1;
	}

	// 파일 구성 클러스터 모두 해제
	if(kFreeClusterAll(entry.startClusterIdx) == FALSE) {
		// 동기화 처리
		kUnlock(&(gs_fileSystemManager.mut));
		return -1;
	}

	// 디렉터리 엔트리를 빈 것으로 설정
	kMemSet(&entry, 0, sizeof(entry));
	if(kSetDirEntry(offset, &entry) == FALSE) {
		// 동기화 처리
		kUnlock(&(gs_fileSystemManager.mut));
		return -1;
	}

	// 동기화 처리
	kUnlock(&(gs_fileSystemManager.mut));
	return 0;
}

// 디렉터리 오픈
DIR *kDirOpen(const char *name) {
	DIR *dir;
	DIRENTRY *buf;

	// 동기화 처리
	kLock(&(gs_fileSystemManager.mut));

	// 루트 디렉터리밖에 없으니 디렉터리 이름은 무시하고 핸들만 할당받아 반환
	dir = kAllocFileDirHandle();
	if(dir == NULL) {
		// 동기화 처리
		kUnlock(&(gs_fileSystemManager.mut));
		return NULL;
	}

	// 루트 디렉터리를 저장할 버퍼 할당
	buf = (DIRENTRY*)kAllocMem(FILESYSTEM_CLUSTER_SIZE);
	if(dir == NULL) {
		// 실패하면 핸들 반환
		kFreeFileDirHandle(dir);
		// 동기화 처리
		kUnlock(&(gs_fileSystemManager.mut));
		return NULL;
	}

	// 루트 디렉터리 읽음
	if(kReadDataArea(0, (BYTE*)buf) == FALSE) {
		// 실패하면 핸들과 메모리 모두 반환
		kFreeFileDirHandle(dir);
		kFreeMem(buf);
		// 동기화 처리
		kUnlock(&(gs_fileSystemManager.mut));
		return NULL;
	}

	// 디렉터리 타입으로 설정 후 현재 디렉터리 엔트리 오프셋 초기화
	dir->type = FILESYSTEM_TYPE_DIR;
	dir->dirHandle.nowOffset = 0;
	dir->dirHandle.dirBuf = buf;

	// 동기화 처리
	kUnlock(&(gs_fileSystemManager.mut));
	return dir;
}

// 디렉터리 엔트리를 반환하고 다음으로 이동
struct directoryEntry *kDirRead(DIR *dir) {
	DIRHANDLE *handle;
	DIRENTRY *entry;

	// 핸들 타입이 디렉터리가 아니면 실패
	if((dir == NULL) || (dir->type != FILESYSTEM_TYPE_DIR)) return NULL;
	handle = &(dir->dirHandle);

	// 오프셋 범위가 클러스터에 존재하는 최댓값 넘으면 실패
	if((handle->nowOffset < 0) || (handle->nowOffset >= FILESYSTEM_MAXDIR_ENTRYCNT)) return NULL;

	// 동기화 처리
	kLock(&(gs_fileSystemManager.mut));

	// 루트 디렉터리에 있는 최대 디렉터리 엔트리 개수만큼 검색
	entry = handle->dirBuf;
	while(handle->nowOffset < FILESYSTEM_MAXDIR_ENTRYCNT) {
		// 파일이 존재하면 해당 디렉터리 엔트리 반환
		if(entry[handle->nowOffset].startClusterIdx != 0) {
			// 동기화 처리
			kUnlock(&(gs_fileSystemManager.mut));
			return &(entry[handle->nowOffset++]);
		}
		handle->nowOffset++;
	}

	// 동기화 처리
	kUnlock(&(gs_fileSystemManager.mut));
	return NULL;
}

// 디렉터리 포인터를 디렉터리 처음으로 이동
void kDirRewind(DIR *dir) {
	DIRHANDLE *handle;

	// 핸들 타입이 디렉터리가 아니면 실패
	if((dir == NULL) || (dir->type != FILESYSTEM_TYPE_DIR)) return;
	handle  = &(dir->dirHandle);

	// 동기화 처리
	kLock(&(gs_fileSystemManager.mut));

	// 디렉터리 엔트리 포인터만 0으로 바꿔줌
	handle->nowOffset = 0;

	// 동기화 처리
	kUnlock(&(gs_fileSystemManager.mut));
}

// 열린 디렉터리 닫음
int kDirClose(DIR *dir) {
	DIRHANDLE *handle;

	// 핸들 타입이 디렉터리가 아니면 실패
	if((dir == NULL) || (dir->type != FILESYSTEM_TYPE_DIR)) return -1;
	handle = &(dir->dirHandle);

	// 동기화 처리
	kLock(&(gs_fileSystemManager.mut));

	// 루트 디렉터리의 버퍼를 해제하고 핸들을 반환
	kFreeMem(handle->dirBuf);
	kFreeFileDirHandle(dir);

	// 동기화 처리
	kUnlock(&(gs_fileSystemManager.mut));

	return 0;
}

// 파일 시스템 캐시를 모두 하드 디스크에 씀
BOOL kFlushFileSystemCache(void) {
	CACHEBUF *cacheBuf;
	int cnt, i;

	// 캐시가 비활성화 되었다면 함수 수행 중지
	if(gs_fileSystemManager.onCache == FALSE) return TRUE;

	// 동기화 처리
	kLock(&(gs_fileSystemManager.mut));

	// 클러스터 링크 테이블 영역 캐시 정보를 얻어 내용이 변한 캐시 버퍼를 모두 디스크에 씀
	kGetCacheBufCnt(CACHE_CLUSTER_AREA, &cacheBuf, &cnt);
	for(i = 0; i < cnt; i++) if(cacheBuf[i].modified == TRUE) {
		if(kInWriteClusterNonCache(cacheBuf[i].tag, cacheBuf[i].buf) == FALSE) return FALSE;
		// 버퍼 내용을 하드 디스크에 썼으니 변경되지 않은것으로 설정
		cacheBuf[i].modified = FALSE;
	}

	// 데이터 영역 캐시 정보를 얻어 내용이 변한 캐시 버퍼를 모두 디스크에 씀
	kGetCacheBufCnt(CACHE_DATA_AREA, &cacheBuf, cnt);
	for(i = 0; i < cnt; i++) if(cacheBuf[i].modified == TRUE) {
		if(kInWriteDataNonCache(cacheBuf[i].tag, cacheBuf[i].buf) == FALSE) return FALSE;
		// 버퍼 내용을 하드 디스크에 썼으니 변경되지 않은 것
		cacheBuf[i].modified = FALSE;
	}

	// 동기화 처리
	kUnlock(&(gs_fileSystemManager.mut));
	return TRUE;
}
