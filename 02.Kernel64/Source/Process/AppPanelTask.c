/*
 * AppPanelTask.c
 *
 *  Created on: 2017. 9. 9.
 *      Author: Yummy
 */

#include <AppPanelTask.h>
#include <RTC.h>
#include <CLITask.h>
#include <GUITask.h>

// 애플리케이션 테이블
APPENTRY gs_appTbl[] = {
	{"Base GUI Task", baseGUITask},
	{"First GUI Task", firstGUITask},
	{"Monitoring Task", sysMonTask},
	{"JPEG Viewer Task", imgViewTask},
	{"YummyHit Terminal", GUIShell},
	{"Exit", exitGUITask},
};

// 애플리케이션 패널에서 사용하는 자료구조
APPPANELDATA gs_appPanelData;

// 애플리케이션 패널 태스크
void appPanelGUITask(void) {
	EVENT recvEvent;
	BOOL appPanelRes, appListRes;

	// 그래픽 모드 판단
	if(isGUIMode() == FALSE) {
		printF("It is GUI Task. You must execute GUI Mode.\n");
		return;
	}

	// 애플리케이션 패널 윈도우와 응용프로그램 리스트 윈도우 생성
	if((createAppPanelWin() == FALSE) || (createAppListWin() == FALSE)) return;

	// GUI 태스크 이벤트 처리 루프
	while(1) {
		// 윈도우 이벤트 처리
		appPanelRes = procAppPanelWinEvent();
		appListRes = procAppListWinEvent();

		// 처리한 이벤트가 없으면 프로세서 반환
		if((appPanelRes == FALSE) && (appListRes == FALSE)) _sleep(0);
	}
}

// 애플리케이션 패널 윈도우 생성
static BOOL createAppPanelWin(void) {
	WINDOWMANAGER *win;
	QWORD id;

	// 윈도우 매니저 반환
	win = getWinManager();

	// 화면 위쪽에 애플리케이션 패널 윈도우 생성. 가로로 가득 차도록
	id = createWin(0, 0, win->area.x2 + 1, APP_PANEL_HEIGHT, NULL, APP_PANEL_TITLE);
	// 윈도우 생성 못하면 실패
	if(id == WINDOW_INVALID_ID) return FALSE;

	// 애플리케이션 패널 윈도우 테두리와 내부 표시
	drawRect(id, 0, 0, win->area.x2, APP_PANEL_HEIGHT - 1, APP_PANEL_COLOR_OUTLINE, FALSE);
	drawRect(id, 1, 1, win->area.x2 - 1, APP_PANEL_HEIGHT - 2, APP_PANEL_COLOR_MIDLINE, FALSE);
	drawRect(id, 2, 2, win->area.x2 - 2, APP_PANEL_HEIGHT - 3, APP_PANEL_COLOR_INLINE, FALSE);
	drawRect(id, 3, 3, win->area.x2 - 3, APP_PANEL_HEIGHT - 4, APP_PANEL_COLOR_BACKGROUND, TRUE);

	// 애플리케이션 패널 왼쪽에 GUI 태스크 리스트 보여주는 버튼 표시
	setRectData(5, 5, 80, 25, &(gs_appPanelData.btnArea));
	drawBtn(id, &(gs_appPanelData.btnArea), APP_PANEL_COLOR_ACTIVE, "Menu", RGB(102, 0, 255));

	// 애플리케이션 패널 윈도우 오른쪽에 시계 표시
	drawDigitClock(id);

	// 애플리케이션 패널을 화면에 표시
	showWin(id, TRUE);

	// 애플리케이션 패널 자료구조에 윈도우 ID 저장
	gs_appPanelData.panelID = id;

	return TRUE;
}

// 애플리케이션 패널에 시계 표시
static void drawDigitClock(QWORD id) {
	RECT winArea, updateArea;
	static BYTE ls_preHour, ls_preMinute, ls_preSecond;
	BYTE hour, minute, second;
	char buf[10] = "00:00 AM";

	// 현재 시각을 RTC에서 반환
	readTime(&hour, &minute, &second);

	// 이전 시간과 변화가 없으면 시계를 표시할 필요 없음
	if((ls_preHour == hour) && (ls_preMinute == minute) && (ls_preSecond == second)) return;

	// 다음 비교를 위해 시, 분, 초 업데이트
	ls_preHour = hour;
	ls_preMinute = minute;
	ls_preSecond = second;

	// 시간이 12시간 넘으면 PM으로 변경
	if(hour >= 12) {
		if(hour > 12) hour -= 12;
		buf[6] = 'P';
	}

	// 시간 설정
	buf[0] = '0' + hour / 10;
	buf[1] = '0' + hour % 10;
	// 분 설정
	buf[3] = '0' + minute / 10;
	buf[4] = '0' + minute % 10;

	// 초에 따라 가운데 : 깜빡임
	if((second % 2) == 1) buf[2] = ' ';
	else buf[2] = ':';

	// 애플리케이션 패널 윈도우 위치 반환
	getWinArea(id, &winArea);

	// 시계 영역 테두리 표시
	setRectData(winArea.x2 - APP_PANEL_CLOCKWIDTH - 13, 5, winArea.x2 - 5, 25, &updateArea);
	drawRect(id, updateArea.x1, updateArea.y1, updateArea.x2, updateArea.y2, APP_PANEL_COLOR_INLINE, FALSE);

	// 시계 표시
	drawText(id, updateArea.x1 + 4, updateArea.y1 + 3, RGB(102, 0, 255), APP_PANEL_COLOR_BACKGROUND, buf, strLen(buf));

	// 시계 그려진 영역만 화면 업데이트
	updateMonWinArea(id, &updateArea);
}

// 애플리케이션 패널에 수신된 이벤트 처리
static BOOL procAppPanelWinEvent(void) {
	EVENT recvEvent;
	MOUSEEVENT *mouseEvent;
	BOOL procRes;
	QWORD panelID, listID;

	// 윈도우 ID 저장
	panelID = gs_appPanelData.panelID;
	listID = gs_appPanelData.listID;
	procRes = FALSE;

	// 이벤트 처리 루프
	while(1) {
		// 애플리케이션 패널 윈도우의 오른쪽에 시계 표시
		drawDigitClock(gs_appPanelData.panelID);

		// 이벤트 큐에서 이벤트 수신
		if(winToEvent(panelID, &recvEvent) == FALSE) break;
		procRes = TRUE;

		// 수신된 이벤트를 타입에 따라 나눠 처리
		switch(recvEvent.type) {
		// 마우스 왼쪽 클릭 처리
		case EVENT_MOUSE_LCLICK_ON:
			mouseEvent = &(recvEvent.mouseEvent);
			// 마우스 왼쪽 클릭이 눌리면 애플리케이션 리스트 윈도우 표시
			if(isInRect(&(gs_appPanelData.btnArea), mouseEvent->point.x, mouseEvent->point.y) == FALSE) break;
			// 버튼이 떨어진 상태
			if(gs_appPanelData.winShow == FALSE) {
				drawBtn(panelID, &(gs_appPanelData.btnArea), APP_PANEL_COLOR_BACKGROUND, "Menu", RGB(102, 0, 255));
				updateMonWinArea(panelID, &(gs_appPanelData.btnArea));

				// 애플리케이션 리스트 윈도우에 아무것도 선택되지 않은 것으로 초기화하고 윈도우를 화면에 최상위로 표시
				if(gs_appPanelData.preMouseIdx != -1) {
					drawAppListItem(gs_appPanelData.preMouseIdx, FALSE);
					gs_appPanelData.preMouseIdx = -1;
				}
				moveWinTop(gs_appPanelData.listID);
				showWin(gs_appPanelData.listID, TRUE);
				// 플래그는 화면에 표시된 것으로 설정
				gs_appPanelData.winShow = TRUE;
			} else {
				// 버튼이 눌린 상태
				drawBtn(panelID, &(gs_appPanelData.btnArea), APP_PANEL_COLOR_ACTIVE, "Menu", RGB(102, 0, 255));
				updateMonWinArea(panelID, &(gs_appPanelData.btnArea));

				// 애플리케이션 리스트 윈도우 숨김
				showWin(listID, FALSE);
				// 플래그는 화면에 표시 되지 않은 것으로 설정
				gs_appPanelData.winShow = FALSE;
			}
			break;
		// 그 외 이벤트 처리
		default:
			break;
		}
	}

	return procRes;
}

// 애플리케이션 리스트 윈도우 생성
static BOOL createAppListWin(void) {
	int i, cnt, maxNameLen, nameLen, x, y, width;
	QWORD id;

	// 애플리케이션 테이블에 정의된 이름 중 가장 긴 것 검색
	maxNameLen = 0;
	cnt = sizeof(gs_appTbl) / sizeof(APPENTRY);
	for(i = 0; i < cnt; i++) {
		nameLen = strLen(gs_appTbl[i].name);
		if(maxNameLen < nameLen) maxNameLen = nameLen;
	}

	// 윈도우 너비 계산, 20은 좌우 10픽셀 여유공간
	width = maxNameLen * FONT_ENG_WIDTH + 20;

	// 윈도우 위치는 애플리케이션 패널 버튼 아래로 설정
	x = gs_appPanelData.btnArea.x1;
	y = gs_appPanelData.btnArea.y2 + 6;

	// 아이템 개수와 최대 길이로 애플리케이션 리스트 윈도우 생성. 애플리케이션 윈도우는 윈도우 제목 표시줄이 필요 없으니 속성은 NULL
	id = createWin(x, y, width, cnt * APP_PANEL_LISTITEM_HEIGHT + 1, NULL, APP_PANEL_LISTTITLE);
	// 윈도우 생성 못하면 실패
	if(id == WINDOW_INVALID_ID) return FALSE;

	// 애플리케이션 패널 자료구조에 윈도우 너비 저장
	gs_appPanelData.listWidth = width;

	// 시작할 때 애플리케이션 리스트 숨김
	gs_appPanelData.winShow = FALSE;

	// 애플리케이션 패널 자료구조에 윈도우 ID 저장하고 이전에 마우스가 위치한 아이템은 없는 것으로 설정
	gs_appPanelData.listID = id;
	gs_appPanelData.preMouseIdx = -1;

	// 윈도우 내부에 응용프로그램 이름과 영역 표시
	for(i = 0; i < cnt; i++) drawAppListItem(i, FALSE);

	moveWin(id, gs_appPanelData.btnArea.x1, gs_appPanelData.btnArea.y2 + 6);
	return TRUE;
}

// 애플리케이션 리스트 윈도우에 GUI 태스크 아이템 표시
static void drawAppListItem(int idx, BOOL mouseOver) {
	QWORD id;
	int width;
	COLOR color;
	RECT itemArea;

	// 애플리케이션 리스트 윈도우 ID와 너비
	id = gs_appPanelData.listID;
	width = gs_appPanelData.listWidth;

	// 마우스가 위에 있는지 여부에 따라 내부 색 다르게 표시
	if(mouseOver == TRUE) color = APP_PANEL_COLOR_ACTIVE;
	else color = APP_PANEL_COLOR_BACKGROUND;

	// 리스트 아이템에 테두리 표시
	setRectData(0, idx * APP_PANEL_LISTITEM_HEIGHT, width - 1, (idx + 1) * APP_PANEL_LISTITEM_HEIGHT, &itemArea);
	drawRect(id, itemArea.x1, itemArea.y1, itemArea.x2, itemArea.y2, APP_PANEL_COLOR_INLINE, FALSE);

	// 리스트 아이템 내부 채움
	drawRect(id, itemArea.x1 + 1, itemArea.y1 + 1, itemArea.x2 - 1, itemArea.y2 - 1, color, TRUE);

	// GUI 태스크 이름 표시
	drawText(id, itemArea.x1 + 10, itemArea.y1 + 2, RGB(102, 0, 255), color, gs_appTbl[idx].name, strLen(gs_appTbl[idx].name));

	// 업데이트된 아이템을 화면에 갱신
	updateMonWinArea(id, &itemArea);
}

// 애플리케이션 리스트에 수신된 이벤트 처리
static BOOL procAppListWinEvent(void) {
	EVENT recvEvent, event;
	MOUSEEVENT *mouseEvent;
	BOOL procRes;
	QWORD panelID, listID;
	int mouseIdx;

	// 윈도우 ID 저장
	panelID = gs_appPanelData.panelID;
	listID = gs_appPanelData.listID;
	procRes = FALSE;

	// 이벤트 처리 루프
	while(1) {
		// 이벤트 큐에서 이벤트 수신
		if(winToEvent(listID, &recvEvent) == FALSE) break;
		procRes = TRUE;

		// 수신된 이벤트를 타입에 따라 나눠 처리
		switch(recvEvent.type) {
		// 마우스 이동 처리
		case EVENT_MOUSE_MOVE:
			mouseEvent = &(recvEvent.mouseEvent);

			// 마우스가 위치한 아이템 계산
			mouseIdx = getMouseItemIdx(mouseEvent->point.y);

			// 현재 마우스가 위치한 아이템과 이전에 위치한 아이템이 다를 때 수행
			if((mouseIdx == gs_appPanelData.preMouseIdx) || (mouseIdx == -1)) break;

			// 이전에 마우스가 위치한 아이템은 기본 상태로 표시
			if(gs_appPanelData.preMouseIdx != -1) drawAppListItem(gs_appPanelData.preMouseIdx, FALSE);

			// 지금 마우스 커서가 있는 위치는 마우스가 위치한 상태로 표시
			drawAppListItem(mouseIdx, TRUE);

			// 마우스가 위치한 아이템 저장
			gs_appPanelData.preMouseIdx = mouseIdx;
			break;
		// 마우스 왼쪽 클릭 처리
		case EVENT_MOUSE_LCLICK_ON:
			mouseEvent = &(recvEvent.mouseEvent);

			// 지금 마우스 커서가 있는 위치는 선택된 것으로 표시
			mouseIdx = getMouseItemIdx(mouseEvent->point.y);
			if(mouseIdx == -1) break;

			// 선택된 아이템 실행
			createTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)gs_appTbl[mouseIdx].entryPoint, TASK_LOADBALANCING_ID);

			// 애플리케이션 패널에 마우스 왼쪽 클릭이 눌린 메시지 전송해 처리
			setMouseEvent(panelID, EVENT_MOUSE_LCLICK_ON, gs_appPanelData.btnArea.x1 + 1, gs_appPanelData.btnArea.y1 + 1, NULL, &event);
			eventToWin(panelID, &event);
			break;
		// 그 외 이벤트 처리
		default:
			break;
		}
	}

	return procRes;
}

// 마우스 커서가 위치한 애플리케이션 리스트 윈도우 아이템 인덱스 반환
static int getMouseItemIdx(int y) {
	int cnt, idx;

	// 애플리케이션 테이블 총 아이템 수
	cnt = sizeof(gs_appTbl) / sizeof(APPENTRY);

	// 마우스 좌표로 아이템 인덱스 계산
	idx = y / APP_PANEL_LISTITEM_HEIGHT;
	// 범위를 벗어나면 -1
	if((idx < 0) || (idx >= cnt)) return -1;

	return idx;
}
