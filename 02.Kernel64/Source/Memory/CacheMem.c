/*
 * CacheMem.c
 *
 *  Created on: 2017. 8. 15.
 *      Author: Yummy
 */

#include <CacheMem.h>
#include <FileSystem.h>
#include <DynMem.h>

// 파일 시스템 캐시 자료구조
static CACHEMEM gs_cacheMem;

// 파일 시스템 캐시 초기화
BOOL initCacheMem(void) {
	int i;

	// 자료구조 초기화
	memSet(&gs_cacheMem, 0, sizeof(gs_cacheMem));

	// 접근 시간 초기화
	gs_cacheMem.accessTime[CACHE_CLUSTER_AREA] = 0;
	gs_cacheMem.accessTime[CACHE_DATA_AREA] = 0;

	// 캐시 버퍼의 최댓값 설정
	gs_cacheMem.maxCnt[CACHE_CLUSTER_AREA] = CACHE_MAXCLUSTER_AREACNT;
	gs_cacheMem.maxCnt[CACHE_DATA_AREA] = CACHE_MAXDATA_AREACNT;

	// 클러스터 링크 테이블 영역용 메모리 할당, 클러스터 링크 테이블은 512바이트로 관리
	gs_cacheMem.buf[CACHE_CLUSTER_AREA] = (BYTE*)allocMem(CACHE_MAXCLUSTER_AREACNT * 512);
	if(gs_cacheMem.buf[CACHE_CLUSTER_AREA] == NULL) return FALSE;

	// 할당받은 메모리 영역을 나누어 캐시 버퍼에 등록
	for(i = 0; i < CACHE_MAXCLUSTER_AREACNT; i++) {
		// 캐시 버퍼에 메모리 공간 할당
		gs_cacheMem.cacheBuf[CACHE_CLUSTER_AREA][i].buf = gs_cacheMem.buf[CACHE_CLUSTER_AREA] + (i * 512);

		// 태그를 유효하지 않은 것으로 설정해 빈 것으로 만듬
		gs_cacheMem.cacheBuf[CACHE_CLUSTER_AREA][i].tag = CACHE_INVALID_TAG;
	}

	// 데이터 영역용 메모리 할당, 데이터 영역은 클러스터 단위(4KB)로 관리
	gs_cacheMem.buf[CACHE_DATA_AREA] = (BYTE*)allocMem(CACHE_MAXDATA_AREACNT * FILESYSTEM_CLUSTER_SIZE);
	if(gs_cacheMem.buf[CACHE_DATA_AREA] == NULL) {
		// 실패하면 이전 할당 메모리 해제
		freeMem(gs_cacheMem.buf[CACHE_CLUSTER_AREA]);
		return FALSE;
	}

	// 할당받은 메모리 영역 나눠 캐시 버퍼에 등록
	for(i = 0; i < CACHE_MAXDATA_AREACNT; i++) {
		// 캐시 버퍼에 메모리 공간 할당
		gs_cacheMem.cacheBuf[CACHE_DATA_AREA][i].buf = gs_cacheMem.buf[CACHE_DATA_AREA] + (i * FILESYSTEM_CLUSTER_SIZE);

		// 태그를 유효하지 않은 것으로 설정해 빈 것으로 만듬
		gs_cacheMem.cacheBuf[CACHE_DATA_AREA][i].tag = CACHE_INVALID_TAG;
	}

	return TRUE;
}

// 캐시 버퍼에서 빈 것 찾아 반환
CACHEBUF *allocCacheBuf(int idx) {
	CACHEBUF *buf;
	int i;

	// 캐시 테이블 최대 개수 넘으면 실패
	if(idx > CACHE_MAXCACHE_IDX) return FALSE;

	// 접근 시간 필드가 최댓값까지 가면 전체 접근 시간 낮춤
	downAccessTime(idx);

	// 최대 개수만큼 검색해 빈 것 반환
	buf = gs_cacheMem.cacheBuf[idx];
	for(i = 0; i < gs_cacheMem.maxCnt[idx]; i++) if(buf[i].tag == CACHE_INVALID_TAG) {
		// 임시로 캐시 태그 설정해 할당된 것으로 만듬
		buf[i].tag = CACHE_INVALID_TAG - 1;

		// 접근 시간 갱신
		buf[i].accessTime = gs_cacheMem.accessTime[idx]++;
		return &(buf[i]);
	}

	return NULL;
}

// 캐시 버퍼에서 태그가 일치하는 것 찾아 반환
CACHEBUF *findCacheBuf(int idx, DWORD tag) {
	CACHEBUF *buf;
	int i;

	// 캐시 테이블 최대 개수 넘으면 실패
	if(idx > CACHE_MAXCACHE_IDX) return FALSE;

	// 접근 시간 필드가 최댓값까지 증가하면 전체 접근 시간 낮춤
	downAccessTime(idx);

	// 최대 개수만큼 검색해 빈 것 반환
	buf = gs_cacheMem.cacheBuf[idx];
	for(i = 0; i < gs_cacheMem.maxCnt[idx]; i++) if(buf[i].tag == tag) {
		// 접근 시간 갱신
		buf[i].accessTime = gs_cacheMem.accessTime[idx]++;
		return &(buf[i]);
	}

	return NULL;
}

// 접근한 시간 전체적으로 낮춤
static void downAccessTime(int idx) {
	CACHEBUF tmp, *buf;
	BOOL sorted;
	int i, j;

	// 캐시 테이블 최대 개수 넘으면 실패
	if(idx > CACHE_MAXCACHE_IDX) return;

	// 접근 시간이 아직 최대치를 넘지 않았으면 접근 시간 그대로
	if(gs_cacheMem.accessTime[idx] < 0xfffffffe) return;

	// 캐시 버퍼를 오름차순으로 정렬, 버블 정렬(Bubble Sort) 사용
	buf = gs_cacheMem.cacheBuf[idx];
	for(j = 0; j < gs_cacheMem.maxCnt[idx] - 1; j++) {
		sorted = TRUE;
		for(i = 0; i < gs_cacheMem.maxCnt[idx] - 1 - j; i++) if(buf[i].accessTime > buf[i + 1].accessTime) {
			// 인접한 두 데이터 비교해 접근 시간이 큰 것을 오른쪽(i+1)에 위치, 두 데이터를 교환하므로 정렬되지 않은 것으로 표시
			sorted = FALSE;

			// i번째 캐시와 i+1번째 캐시 교환
			memCpy(&tmp, &(buf[i]), sizeof(CACHEBUF));
			memCpy(&(buf[i]), &(buf[i + 1]), sizeof(CACHEBUF));
			memCpy(&(buf[i + 1]), &tmp, sizeof(CACHEBUF));
		}

		// 다 정렬되었으면 루프 종료
		if(sorted == TRUE) break;
	}

	// 오름차순으로 정렬했으니 인덱스 증가시 접근 시간 최신인 캐시 버퍼. 접근 시간을 0부터 순차적으로 설정해 데이터 갱신
	for(i = 0; i < gs_cacheMem.maxCnt[idx]; i++) buf[i].accessTime = i;

	// 접근 시간을 파일 시스템 캐시 자료구조에 저장해 다음부터 변경된 값으로 접근 시간 설정
	gs_cacheMem.accessTime[idx] = i;
}

// 캐시 버퍼에서 내보낼 것 찾음
CACHEBUF *getTargetCacheBuf(int idx) {
	DWORD oldTime;
	CACHEBUF *buf;
	int oldIdx, i;

	// 캐시 테이블 최대 개수 넘으면 실패
	if(idx > CACHE_MAXCACHE_IDX) return FALSE;

	// 접근 시간을 최대로 해 접근 시간이 가장 오래된 캐시 버퍼 검색
	oldIdx = -1;
	oldTime = 0xFFFFFFFF;

	// 캐시 버퍼에서 사용 중이지 않거나 접근한지 가장 오래된 것 찾아 반환
	buf = gs_cacheMem.cacheBuf[idx];
	for(i = 0; i < gs_cacheMem.maxCnt[idx]; i++) {
		// 빈 캐시 버퍼가 있으면 반환
		if(buf[i].tag == CACHE_INVALID_TAG) {
			oldIdx = i;
			break;
		}

		// 접근 시간이 현재 값보다 오래되었으면 저장
		if(buf[i].accessTime < oldTime) {
			oldTime = buf[i].accessTime;
			oldIdx = i;
		}
	}

	// 캐시 버퍼를 찾지 못하면 문제 발생
	if(oldIdx == -1) {
		printF("Find Cache Buffer Error...\n");
		return NULL;
	}

	// 선택된 캐시 버퍼 접근 시간 갱신
	buf[oldIdx].accessTime = gs_cacheMem.accessTime[idx]++;
	return &(buf[oldIdx]);
}

// 캐시 버퍼 내용 모두 비움
void clearCacheBuf(int idx) {
	CACHEBUF *buf;
	int i;

	// 캐시 버퍼 초기화 설정
	buf = gs_cacheMem.cacheBuf[idx];
	for(i = 0; i < gs_cacheMem.maxCnt[idx]; i++) buf[i].tag = CACHE_INVALID_TAG;

	// 접근 시간 초기화
	gs_cacheMem.accessTime[idx] = 0;
}

// 캐시 버퍼 포인터와 최대 개수 반환
BOOL getCacheBufCnt(int idx, CACHEBUF **buf, int *maxCnt) {
	// 캐시 테이블 최대 개수 넘으면 실패
	if(idx > CACHE_MAXCACHE_IDX) return FALSE;

	// 캐시 버퍼 포인터와 최댓값 반환
	*buf = gs_cacheMem.cacheBuf[idx];
	*maxCnt = gs_cacheMem.maxCnt[idx];
	return TRUE;
}
