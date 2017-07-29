/*
 * Shell.h
 *
 *  Created on: 2017. 7. 23.
 *      Author: Yummy
 */

#ifndef __SHELL_H__
#define __SHELL_H__

#include <Types.h>

// 매크로
#define SHELL_MAXCOMMANDBUFFERCOUNT	300
#define SHELL_PROMPTMESSAGE		"sh-yummyHit \$ "

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
void startShell(void);
void execCMD(const char *buf);
void initParam(PARAMLIST *list, const char *param);
int getNextParam(PARAMLIST *list, char *param);

// 커맨드 처리 함수
void csHelp(const char *buf);
void csClear(const char *buf);
void csFree(const char *buf);
void csStrConvert(const char *buf);
void csHalt(const char *buf);
void csSetTime(const char *buf);
void csWait(const char *buf);
void csRTSC(const char *buf);
void csCPUSpeed(const char *buf);
void csDate(const char *buf);

#endif /*__SHELL_H__*/
