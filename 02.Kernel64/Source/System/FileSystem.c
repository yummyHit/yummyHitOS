/*
 * FileSystem.c
 *
 *  Created on: 2017. 8. 9.
 *      Author: Yummy
 */

#include <FileSystem.h>
#include <HardDisk.h>
#include <DynMem.h>

// 파일 시스템 자료구조
static FILESYSTEMMANAGER gs_fileSystemManager;
// 파일 시스템 임시 버퍼
static BYTE gs_tmpBuf[FILESYSTEM_PERSECTOR * 512];

// 하드 디스크 제어 관련 함수 포인터 선언
_readHDDInfo gs_readHDDInfo = NULL;
_readHDDSector gs_readHDDSector = NULL;
_writeHDDSector gs_writeHDDSector = NULL;

// 파일 시스템 초기화
BOOL initFileSystem(void) {
	// 자료구조 초기화 및 동기화 객체 초기화
	memSet(&gs_fileSystemManager, 0, sizeof(gs_fileSystemManager));
	initMutex(&(gs_fileSystemManager.mut));

	// 하드 디스크 초기화
	if(initHDD() == TRUE) {
		// 초기화가 성공하면 함수 포인터를 하드 디스크용 함수로 설정
		gs_readHDDInfo = readHDDInfo;
		gs_readHDDSector = readHDDSector;
		gs_writeHDDSector = writeHDDSector;
	} else return FALSE;

	// 파일 시스템 연결
	if(_mount() == FALSE) return FALSE;
	return TRUE;
}

// 저수준 함수. 하드 디스크의 MBR을 읽어 파일 시스템 확인 후 관련 각종 정보를 자료구조에 삽입
BOOL _mount(void) {
	MBR *mbr;

	// 동기화 처리
	_lock(&(gs_fileSystemManager.mut));

	// MBR 읽음
	if(gs_readHDDSector(TRUE, TRUE, 0, 1, gs_tmpBuf) == FALSE) {
		// 동기화 처리
		_unlock(&(gs_fileSystemManager.mut));
		return FALSE;
	}

	// 시그너처 확인 후 같으면 자료구조에 각 영역 정보 삽입
	mbr = (MBR*)gs_tmpBuf;
	if(mbr->sign != FILESYSTEM_SIGN) {
		// 동기화 처리
		_unlock(&(gs_fileSystemManager.mut));
		return FALSE;
	}

	// 파일 시스템 인식 성공
	gs_fileSystemManager.mnt = TRUE;

	// 각 영역 시작 LBA 어드레스와 섹터 수 계산
	gs_fileSystemManager.reserved_sectorCnt = mbr->reserved_sectorCnt;
	gs_fileSystemManager.linkStartAddr = mbr->reserved_sectorCnt + 1;
	gs_fileSystemManager.linkSectorCnt = mbr->linkSectorCnt;
	gs_fileSystemManager.dataStartAddr = mbr->reserved_sectorCnt + mbr->linkSectorCnt + 1;
	gs_fileSystemManager.totalClusterCnt = mbr->totalClusterCnt;

	// 동기화 처리
	_unlock(&(gs_fileSystemManager.mut));
	return TRUE;
}

// 하드 디스크에 파일 시스템 생성
BOOL _format(void) {
	HDDINFO *hdd;
	MBR *mbr;
	DWORD totalSectorCnt, remainSectorCnt, maxClusterCnt, clusterCnt, linkSectorCnt, i;

	// 동기화 처리
	_lock(&(gs_fileSystemManager.mut));

	// 하드 디스크 정보 얻어 하드 디스크 총 섹터 수 구함
	hdd = (HDDINFO*)gs_tmpBuf;
	if(gs_readHDDInfo(TRUE, TRUE, hdd) == FALSE) {
		// 동기화 처리
		_unlock(&(gs_fileSystemManager.mut));
		return FALSE;
	}
	totalSectorCnt = hdd->totalSector;

	// 전체 섹터 수를 4KB, 클러스터 크기로 나눠 최대 클러스터 수 계산
	maxClusterCnt = totalSectorCnt / FILESYSTEM_PERSECTOR;

	// 최대 클러스터 수에 맞춰 링크 테이블 섹터 수 계산. 링크 데이터는 4바이터이니 한 섹터에 128개 들어감.
	// 총 개수를 128로 나눈 후 올림하여 클러스터 링크 섹터 수 구함
	linkSectorCnt = (maxClusterCnt + 127) / 128;

	// 예약된 영역은 현재 사용하지 않으니 디스크 전체 영역에서 MBR 영역과 클러스터 링크 테이블 영역 크기를 뺀 나머지가
	// 실제 데이터 영역. 해당 영역을 클러스터 크기로 나눠 실제 클러스터 개수 구함
	remainSectorCnt = totalSectorCnt - linkSectorCnt - 1;
	clusterCnt = remainSectorCnt / FILESYSTEM_PERSECTOR;

	// 실제 사용 가능한 클러스터 수에 맞춰 다시 한 번 계산
	linkSectorCnt = (clusterCnt + 127) / 128;

	// MBR 영역 읽기
	if(gs_readHDDSector(TRUE, TRUE, 0, 1, gs_tmpBuf) == FALSE) {
		// 동기화 처리
		_unlock(&(gs_fileSystemManager.mut));
		return FALSE;
	}

	// 파티션 정보와 파일 시스템 정보 설정
	mbr = (MBR*)gs_tmpBuf;
	memSet(mbr->part, 0, sizeof(mbr->part));
	mbr->sign = FILESYSTEM_SIGN;
	mbr->reserved_sectorCnt = 0;
	mbr->linkSectorCnt = linkSectorCnt;
	mbr->totalClusterCnt = clusterCnt;

	// MBR 영역에 1섹터 씀
	if(gs_writeHDDSector(TRUE, TRUE, 0, 1, gs_tmpBuf) == FALSE) {
		// 동기화 처리
		_unlock(&(gs_fileSystemManager.mut));
		return FALSE;
	}

	// MBR 이후부터 루트 디렉터리까지 모두 0으로 초기화
	memSet(gs_tmpBuf, 0, 512);
	for(i = 0; i < (linkSectorCnt + FILESYSTEM_PERSECTOR); i++) {
		// 루트 디렉터리(클러스터 0)는 이미 파일시스템이 사용중이니 할당된 것으로 표시
		if(i == 0) ((DWORD*)(gs_tmpBuf))[0] = FILESYSTEM_LAST_CLUSTER;
		else ((DWORD*)(gs_tmpBuf))[0] = FILESYSTEM_FREE_CLUSTER;

		// 1섹터씩 씀
		if(gs_writeHDDSector(TRUE, TRUE, i + 1, 1, gs_tmpBuf) == FALSE) {
			// 동기화 처리
			_unlock(&(gs_fileSystemManager.mut));
			return FALSE;
		}
	}

	// 동기화 처리
	_unlock(&(gs_fileSystemManager.mut));
	return TRUE;
}

// 파일 시스템에 연결된 하드 디스크 정보 반환
BOOL getHDDInfo(HDDINFO *info) {
	BOOL res;

	// 동기화 처리
	_lock(&(gs_fileSystemManager.mut));

	res = gs_readHDDInfo(TRUE, TRUE, info);

	// 동기화 처리
	_unlock(&(gs_fileSystemManager.mut));

	return res;
}

// 클러스터 링크 테이블 내 오프셋에서 한 섹터 읽음
BOOL readClusterLink(DWORD offset, BYTE *buf) {
	// 클러스터 링크 테이블 영역 시작 어드레스를 더함
	return gs_readHDDSector(TRUE, TRUE, offset + gs_fileSystemManager.linkStartAddr, 1, buf);
}

// 클러스터 링크 테이블 내 오프셋에 한 섹터 씀
BOOL writeClusterLink(DWORD offset, BYTE *buf) {
	// 클러스터 링크 테이블 영역 시작 어드레스를 더함
	return gs_writeHDDSector(TRUE, TRUE, offset + gs_fileSystemManager.linkStartAddr, 1, buf);
}

// 데이터 영역 오프셋에서 한 클러스터 읽음
BOOL readCluster(DWORD offset, BYTE *buf) {
	// 데이터 영역 시작 주소 더함
	return gs_readHDDSector(TRUE, TRUE, (offset * FILESYSTEM_PERSECTOR) + gs_fileSystemManager.dataStartAddr, FILESYSTEM_PERSECTOR, buf);
}

// 데이터 영역 오프셋에 한 클러스터 씀
BOOL writeCluster(DWORD offset, BYTE *buf) {
	// 데이터 영역 시작 주소 더함
	return gs_writeHDDSector(TRUE, TRUE, (offset * FILESYSTEM_PERSECTOR) + gs_fileSystemManager.dataStartAddr, FILESYSTEM_PERSECTOR, buf);
}

// 클러스터 링크 테이블 영역에서 빈 클러스터 검색
DWORD findFreeCluster(void) {
	DWORD linkCnt, lastOffset, nowOffset, i, j;

	// 파일 시스템을 인식 못하면 실패
	if(gs_fileSystemManager.mnt == FALSE) return FILESYSTEM_LAST_CLUSTER;

	// 마지막으로 클러스터 할당한 클러스터 링크 테이블의 섹터 오프셋 가져옴
	lastOffset = gs_fileSystemManager.lastAllocLinkOffset;

	// 마지막으로 할당한 위치부터 루프 돌며 빈 클러스터 검색
	for(i = 0; i < gs_fileSystemManager.linkSectorCnt; i++) {
		// 클러스터 링크 테이블의 마지막 섹터면 전체 섹터만큼 도는 것이 아니라 남은 클러스터 수만큼 루프 돌아야 함
		if((lastOffset + i) == (gs_fileSystemManager.linkSectorCnt - 1)) linkCnt = gs_fileSystemManager.totalClusterCnt % 128;
		else linkCnt = 128;

		// 이번에 읽어야 할 클러스터 링크 테이블의 섹터 오프셋 구해서 읽음
		nowOffset = (lastOffset + i) % gs_fileSystemManager.linkSectorCnt;
		if(readClusterLink(nowOffset, gs_tmpBuf) == FALSE) return FILESYSTEM_LAST_CLUSTER;

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
BOOL setClusterLink(DWORD idx, DWORD data) {
	DWORD offset;

	// 파일 시스템을 인식 못하면 실패
	if(gs_fileSystemManager.mnt == FALSE) return FALSE;

	// 한 섹터에 128개 클러스터 링크가 들어가니 128로 나누면 섹터 오프셋
	offset = idx / 128;

	// 해당 섹터를 읽어 링크 정보를 설정한 후 다시 저장
	if(readClusterLink(offset, gs_tmpBuf) == FALSE) return FALSE;

	((DWORD*)gs_tmpBuf)[idx % 128] = data;

	if(writeClusterLink(offset, gs_tmpBuf) == FALSE) return FALSE;

	return TRUE;
}

// 클러스터 링크 테이블 값 반환
BOOL getClusterLink(DWORD idx, DWORD *data) {
	DWORD offset;

	// 파일 시스템을 인식 못하면 실패
	if(gs_fileSystemManager.mnt == FALSE) return FALSE;

	// 한 섹터에 128개 클러스터 링크가 들어가니 128로 나누면 섹터 오프셋
	offset = idx / 128;

	if(offset > gs_fileSystemManager.linkSectorCnt) return FALSE;

	// 해당 섹터를 읽어 링크 정보 반환
	if(readClusterLink(offset, gs_tmpBuf) == FALSE) return FALSE;

	*data = ((DWORD*)gs_tmpBuf)[idx % 128];
	return TRUE;
}

// 루트 디렉터리에서 빈 디렉터리 엔트리 반환
int findFreeDirEntry(void) {
	DIRENTRY *entry;
	int i;

	// 파일 시스템 인식 못하면 실패
	if(gs_fileSystemManager.mnt == FALSE) return -1;

	// 루트 디렉터리 읽음
	if(readCluster(0, gs_tmpBuf) == FALSE) return -1;

	// 루트 디렉터리 내 루프를 돌며 빈 엔트리, 즉 시작 클러스터 번호가 0인 엔트리 검색
	entry = (DIRENTRY*)gs_tmpBuf;
	for(i = 0; i < FILESYSTEM_MAXDIRENTRYCNT; i++) if(entry[i].startClusterIdx == 0) return i;

	return -1;
}

// 루트 디렉터리의 해당 인덱스에 디렉터리 엔트리 설정
BOOL setDirEntry(int idx, DIRENTRY *entry) {
	DIRENTRY *root;

	// 파일 시스템을 인식하지 못했거나 인덱스가 올바르지 않으면 실패
	if((gs_fileSystemManager.mnt == FALSE) || (idx < 0) || (idx >= FILESYSTEM_MAXDIRENTRYCNT)) return FALSE;

	// 루트 디렉터리 읽음
	if(readCluster(0, gs_tmpBuf) == FALSE) return FALSE;

	// 루트 디렉터리에 있는 해당 데이터 갱신
	root = (DIRENTRY*)gs_tmpBuf;
	memCpy(root + idx, entry, sizeof(DIRENTRY));

	// 루트 디렉터리에 씀
	if(writeCluster(0, gs_tmpBuf) == FALSE) return FALSE;
	return TRUE;
}

// 루트 디렉터리의 해당 인덱스에 위치하는 디렉터리 엔트리 반환
BOOL getDirEntry(int idx, DIRENTRY *entry) {
	DIRENTRY *root;

	// 파일 시스템을 인식하지 못했거나 인덱스가 올바르지 않으면 실패
	if((gs_fileSystemManager.mnt == FALSE) || (idx < 0) || (idx >= FILESYSTEM_MAXDIRENTRYCNT)) return FALSE;

	// 루트 디렉터리 읽음
	if(readCluster(0, gs_tmpBuf) == FALSE) return FALSE;

	// 루트 디렉터리에 있는 해당 데이터 갱신
	root = (DIRENTRY*)gs_tmpBuf;
	memCpy(entry, root + idx, sizeof(DIRENTRY));
	return TRUE;
}

// 루트 디렉터리에서 파일 이름이 일치하는 엔트리 찾아 인덱스 반환
int findDirEntry(const char *name, DIRENTRY *entry) {
	DIRENTRY *root;
	int i, len;

	// 파일 시스템을 인식 못하면 실패
	if(gs_fileSystemManager.mnt == FALSE) return -1;

	// 루트 디렉터리 읽음
	if(readCluster(0, gs_tmpBuf) == FALSE) return -1;

	len = strLen(name);
	// 루트 디렉터리 안에서 루프 돌며 파일 이름이 일치하는 엔트리 반환
	root = (DIRENTRY*)gs_tmpBuf;
	for(i = 0; i < FILESYSTEM_MAXDIRENTRYCNT; i++) if(memCmp(root[i].name, name, len) == 0) {
		memCpy(entry, root + i, sizeof(DIRENTRY));
		return i;
	}
	return -1;
}

// 파일 시스템 정보 반환
void getFileSystemInfo(FILESYSTEMMANAGER *manager) {
	memCpy(manager, &gs_fileSystemManager, sizeof(gs_fileSystemManager));
}
