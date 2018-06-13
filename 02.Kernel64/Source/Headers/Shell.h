/*
 * Shell.h
 *
 *  Created on: 2017. 7. 23.
 *      Author: Yummy
 */

#ifndef __SHELL_H__
#define __SHELL_H__

#include <Types.h>

#pragma once
// 매크로
#define SHELL_MAXCMDBUFCNT	300
#define SHELL_PROMPTMSG		"sh-yummyHit \$ "

// 문자열 포인터를 파라미터로 받는 함수 포인터 타입 정의
typedef void (*CMDFunc)(const char *param);

// 구조체, 1바이트 정렬
#pragma pack(push, 1)

// 셸 커맨드 저장하는 자료구조
typedef struct shellEntry {
	// 커맨드 문자열
	char *cmd;
	// 커맨드 도움말
	char *help;
	// 커맨드 수행 함수 포인터
	CMDFunc func;
} SHELLENTRY;

// 파라미터 처리를 위해 정보 저장하는 자료구조
typedef struct paramList {
	// 파라미터 버퍼 어드레스
	const char *buf;
	// 파라미터 길이
	int len;
	// 현재 처리할 파라미터가 시작하는 위치
	int nowPosition;
} PARAMLIST;

#pragma pack(pop)

// 실제 셸 코드
void kStartShell(void);
void kExecCMD(const char *buf);
void kInitParam(PARAMLIST *list, const char *param);
int kGetNextParam(PARAMLIST *list, char *param);

// 커맨드 처리 함수
static void kCSHelp(const char *buf);
static void kCSClear(const char *buf);
static void kCSFree(const char *buf);
static void kCSStrConvert(const char *buf);
static void kCSExit(const char *buf);
static void kCSHalt(const char *buf);
static void kCSReboot(const char *buf);
static void kCSSetTime(const char *buf);
static void kCSWait(const char *buf);
static void kCSRTSC(const char *buf);
static void kCSCPUSpeed(const char *buf);
static void kCSDate(const char *buf);
static void kCSCreateTask(const char *buf);
static void kCSChangePriority(const char *buf);
static void kCSTaskList(const char *buf);
static void kCSTaskill(const char *buf);
static void kCSCPULoad(const char *buf);
static void kCSMutexTest(const char *buf);
static void kCSThreadTest(const char *buf);
static void kCSMatrix(const char *buf);
static void kCSGetPIE(const char *buf);
static void kCSDynMemInfo(const char *buf);
static void kCSSeqAllocTest(const char *buf);
static void kCSRandAllocTest(const char *buf);
static void kCSHDDInfo(const char *buf);
static void kCSReadSector(const char *buf);
static void kCSWriteSector(const char *buf);
static void kCSMountHDD(const char *buf);
static void kCSFormatHDD(const char *buf);
static void kCSFileSystemInfo(const char *buf);
static void kCSMakeFile(const char *buf);
static void kCSRemoveFile(const char *buf);
static void kCSRootDir(const char *buf);
static void kCSFileWrite(const char *buf);
static void kCSFileRead(const char *buf);
static void kCSFileIOTest(const char *buf);
static void kCSCacheTest(const char *buf);
static void kCSCacheFlush(const char *buf);
static void kCSDownload(const char *buf);
static void kCSMPConfigInfo(const char *buf);
static void kCSStartAP(const char *buf);
static void kCSSymmetricIOMode(const char *buf);
static void kCSIRQMapTbl(const char *buf);
static void kCSInterruptProcCnt(const char *buf);
static void kCSInterruptLoadBalancing(const char *buf);
static void kCSTaskLoadBalancing(const char *buf);
static void kCSChangeAffinity(const char *buf);
static void kCSVBEModeInfo(const char *buf);

#endif /*__SHELL_H__*/
