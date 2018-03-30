/*
 * AppPanelTask.h
 *
 *  Created on: 2017. 9. 10.
 *      Author: Yummy
 */

#ifndef __APPPANELTASK_H__
#define __APPPANELTASK_H__

#include <Types.h>
#include <Win.h>
#include <Font.h>

#pragma once

// 애플리케이션 패널 윈도우 높이
#define APP_PANEL_HEIGHT		31
// 애플리케이션 패널 윈도우 제목
#define APP_PANEL_TITLE			"SYS_APP_PANEL"
// 애플리케이션 패널 윈도우에 표시할 시계 너비. "09:00 AM" 형태로 표시하므로 8 * 폰트 너비로 계산
#define APP_PANEL_CLOCKWIDTH		(8 * FONT_ENG_WIDTH)

// 애플리케이션 리스트 윈도우에 나타낼 아이템 높이
#define APP_PANEL_LISTITEM_HEIGHT	(FONT_ENG_HEIGHT + 4)
// 애플리케이션 리스트 윈도우 제목
#define APP_PANEL_LISTTITLE		"SYS_APP_LIST"

// 애플리케이션 패널에서 사용하는 색
#define APP_PANEL_COLOR_OUTLINE		RGB(102, 102, 255)
#define APP_PANEL_COLOR_MIDLINE		RGB(102, 102, 204)
#define APP_PANEL_COLOR_INLINE		RGB(102, 0, 255)
#define APP_PANEL_COLOR_BACKGROUND	RGB(255, 255, 135)
#define APP_PANEL_COLOR_ACTIVE		RGB(229, 229, 229)

// 애플리케이션 패널이 사용하는 정보 저장 자료구조
typedef struct appPanelData {
	// 애플리케이션 패널 윈도우 ID
	QWORD panelID;

	// 애플리케이션 리스트 윈도우 ID
	QWORD listID;

	// 애플리케이션 패널 버튼 위치
	RECT btnArea;

	// 애플리케이션 리스트 윈도우 너비
	int listWidth;

	// 애플리케이션 리스트 윈도우에서 이전에 마우스가 위치한 아이템 인덱스
	int preMouseIdx;

	// 애플리케이션 리스트 윈도우가 화면에 표시되었는지 여부
	BOOL winShow;
} APPPANELDATA;

// GUI 태스크 정보 저장 자료구조
typedef struct appEntry {
	// GUI 태스크 이름
	char *name;

	// GUI 태스크 엔트리 포인트
	void *entryPoint;
} APPENTRY;

void appPanelGUITask(void);

static void drawClockInAppPanel(QWORD panelID);
static BOOL createAppPanelWin(void);
static void drawDigitClock(QWORD id);
static BOOL createAppListWin(void);
static void drawAppListItem(int idx, BOOL select);
static BOOL procAppPanelWinEvent(void);
static BOOL procAppListWinEvent(void);
static int getMouseItemIdx(int y);

#endif /*__APPPANELTASK_H__*/
