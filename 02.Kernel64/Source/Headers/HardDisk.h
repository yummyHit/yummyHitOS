/*
 * HardDisk.h
 *
 *  Created on: 2017. 8. 8.
 *      Author: Yummy
 */

#ifndef __HARDDISK_H__
#define __HARDDISK_H__

#include <CLITask.h>
#include <Synchronize.h>

#pragma once

// 매크로. 첫 번째 PATA 포트(Primary PATA Port)와 두 번째 PATA 포트(Secondary PATA Port)의 정보
#define HDD_PORT_PRIBASE		0x1F0
#define HDD_PORT_SECBASE		0x170

// 포트 인덱스 관련
#define HDD_PORT_IDX_DATA		0x00
#define HDD_PORT_IDX_SECTORCNT		0x02
#define HDD_PORT_IDX_SECTORNUM		0x03
#define HDD_PORT_IDX_CYLINDERLSB	0x04
#define HDD_PORT_IDX_CYLINDERMSB	0x05
#define HDD_PORT_IDX_DRIVENHEAD		0x06
#define HDD_PORT_IDX_STAT		0x07
#define HDD_PORT_IDX_CMD		0x07
#define HDD_PORT_IDX_DIGITOUT		0x206

// 커맨드 레지스터 관련
#define HDD_CMD_READ			0x20
#define HDD_CMD_WRITE			0x30
#define HDD_CMD_ID			0xEC

// 상태 레지스터 관련
#define HDD_STAT_ERR			0x01
#define HDD_STAT_IDX			0x02
#define HDD_STAT_OKDATA			0x04
#define HDD_STAT_DATAREQ		0x08
#define HDD_STAT_SEEKFIN		0x10
#define HDD_STAT_WRITEERR		0x20
#define HDD_STAT_READY			0x40
#define HDD_STAT_BUSY			0x80

// 드라이브와 헤드 레지스터 관련
#define HDD_DRIVENHEAD_LBA		0xE0
#define HDD_DRIVENHEAD_SLAVE		0x10

// 디지털 출력 레지스터 관련
#define HDD_DIGITOUT_RESET		0x04
#define HDD_DIGITOUT_OFFINTERRUPT	0x01

// 하드 디스크 응답 대기 시간(ms)
#define HDD_WAIT_TIME			500
// 한 번에 HDD 읽거나 쓸 수 있는 섹터 수
#define HDD_MAX_RW_SECTORCNT		256

// 구조체. 1바이트로 정렬
#pragma pack(push, 1)

typedef struct HDDInformation {
	// 설정 값
	WORD config;

	// 실린더 수
	WORD cylinderNum;
	WORD reserved_A;

	// 헤드 수
	WORD headNum;
	WORD trackUnformat;
	WORD sectorUnformat;

	// 실린더당 섹터 수
	WORD perSectorNum;
	WORD sectorGap;
	WORD lockByte;
	WORD statWordNum;

	// 하드 디스크 시리얼 번호
	WORD serialNum[10];
	WORD type;
	WORD bufSize;
	WORD eccNum;
	WORD firmwareRevision[4];

	// 하드 디스크 모델 번호
	WORD modelNum[20];
	WORD reserved_B[13];

	// 디스크 총 섹터 수
	DWORD totalSector;
	WORD reserved_C[196];
} HDDINFO;

#pragma pack(pop)

// 하드 디스크 관리 구조체
typedef struct HDDManager {
	// HDD 존재 여부와 쓰기 수행 여부
	BOOL hddDetect;
	BOOL isWrite;

	// 인터럽트 발생 여부와 동기화 객체
	volatile BOOL priInterruptOccur;
	volatile BOOL secInterruptOccur;
	MUTEX mut;

	// HDD 정보
	HDDINFO hddInfo;
} HDDMANAGER;

BOOL kInitHDD(void);
BOOL kReadHDDInfo(BOOL pri, BOOL master, HDDINFO *hddInfo);
int kReadHDDSector(BOOL pri, BOOL master, DWORD lba, int sectorCnt, char *buf);
int kWriteHDDSector(BOOL pri, BOOL master, DWORD lba, int sectorCnt, char *buf);
void kSetHDDInterruptFlag(BOOL pri, BOOL flag);

static void kSwapByte(WORD *data, int cnt);
static BYTE kReadHDDStat(BOOL pri);
static BOOL kIsHDDBusy(BOOL pri);
static BOOL kIsHDDReady(BOOL pri);
static BOOL kWaitHDDBusy(BOOL pri);
static BOOL kWaitHDDReady(BOOL pri);
static BOOL kWaitHDDInterrupt(BOOL pri);

#endif /*__HARDDISK_H__*/
