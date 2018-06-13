/*
 * GUITask.h
 *
 *  Created on: 2017. 9. 5.
 *      Author: Yummy
 */

#ifndef __GUITASK_H__
#define __GUITASK_H__

#include <Types.h>
#include <Win.h>

#pragma once

// 태스크가 보내는 유저 이벤트 타입 정의
#define EVENT_USER_TESTMSG		0x80000001

// 시스템 모니터 태스크 매크로
// 프로세서 정보 표시 막대 너비
#define SYSMON_PROCESSOR_WIDTH	150
// 프로세서 정보 표시 영역과 영역 사이 여백
#define SYSMON_PROCESSOR_MARGIN	20
// 프로세서 정보 표시 영역 높이
#define SYSMON_PROCESSOR_HEIGHT	150
// 시스템 모니터 윈도우 높이
#define SYSMON_WINDOW_HEIGHT	310
// 메모리 정보 표시 영역 높이
#define SYSMON_MEMORY_HEIGHT	100
// 막대 색
#define SYSMON_BAR_COLOR		RGB(102, 0, 255)

void kBaseGUITask(void);
void kFirstGUITask(void);
void kExitGUITask(void);

// 시스템 모니터 태스크 함수
void kSysMonTask(void); 
static void kDrawProcInfo(QWORD winID, int x, int y, BYTE id);
static void kDrawMemInfo(QWORD winID, int y, int winWidth);

// GUI 콘솔 셸 태스크 함수
void kGUIShell(void);
static void kProcConsoleBuf(QWORD winID);

// 이미지 뷰어 태스크 함수
void kImgViewTask(void);
static void kDrawFileName(QWORD winID, RECT *area, char *fileName, int len);
static BOOL kCreateImgOnWinExe(QWORD mainWinID, const char *fileName);

#endif /*__GUITASK_H__*/
