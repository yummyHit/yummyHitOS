/*
 * cache.h
 *
 *  Created on: 2017. 8. 15.
 *      Author: Yummy
 */

#ifndef __CACHE_H__
#define __CACHE_H__

#include <types.h>

#pragma once

// 매크로, 클러스터 링크 테이블 영역 최대 캐시 버퍼 개수
#define CACHE_MAXCLUSTER_AREACNT	16
// 데이터 영역 최대 캐시 버퍼 수
#define CACHE_MAXDATA_AREACNT		32
// 유효하지 않은 태그, 비어있는 캐시 버퍼
#define CACHE_INVALID_TAG		0xFFFFFFFF

// 캐시 테이블 최대 개수, 클러스터 링크 테이블과 데이터 영역만 있으니 2로 설정
#define CACHE_MAXCACHE_IDX		2
// 클러스터 링크 테이블 영역 인덱스
#define CACHE_CLUSTER_AREA		0
// 데이터 영역 인덱스
#define CACHE_DATA_AREA			1

#pragma pack(push, 1)

// 구조체, 파일 시스템 캐시를 구성하는 캐시 버퍼 구조체
typedef struct kCacheBuf {
	// 캐시와 대응하는 클러스터 링크 테이블 영역이나 데이터 영역 인덱스
	DWORD tag;

	// 캐시 버퍼 접근 시간
	DWORD accessTime;

	// 데이터 내용 변경 여부
	BOOL modified;

	// 데이터 버퍼
	BYTE *buf;
} CACHEBUF;

// 파일 시스템 캐시 관리하는 캐시 매니저 구조체
typedef struct kCacheManager {
	// 클러스터 링크 테이블 영역과 데이터 영역 접근 시간 필드
	DWORD accessTime[CACHE_MAXCACHE_IDX];

	// 클러스터 링크 테이블 영역과 데이터 영역 데이터 버퍼
	BYTE *buf[CACHE_MAXCACHE_IDX];

	// 클러스터 링크 테이블 영역과 데이터 영역 캐시 버퍼. 두 값 중 큰 값만큼 생성
	CACHEBUF cacheBuf[CACHE_MAXCACHE_IDX][CACHE_MAXDATA_AREACNT];

	// 캐시 버퍼 최대값 저장
	DWORD maxCnt[CACHE_MAXCACHE_IDX];
} CACHEMEM;

#pragma pack(pop)

BOOL kInitCacheMem(void);
CACHEBUF *kAllocCacheBuf(int idx);
CACHEBUF *kFindCacheBuf(int idx, DWORD tag);
CACHEBUF *kGetTargetCacheBuf(int idx);
void kClearCacheBuf(int idx);
BOOL kGetCacheBufCnt(int idx, CACHEBUF **buf, int *maxCnt);
static void kDownAccessTime(int idx);

#endif /*__CACHE_H__*/
