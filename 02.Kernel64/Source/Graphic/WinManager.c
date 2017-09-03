/*
 * WinManager.c
 *
 *  Created on: 2017. 9. 3.
 *      Author: Yummy
 */

#include <Types.h>
#include <Win.h>
#include <WinManager.h>
#include <VBE.h>
#include <Mouse.h>
#include <Task.h>

// 윈도우 매니저 태스크
void startWinManager(void) {
	int xMouse, yMouse;
	BOOL mouseRes, keyRes, eventRes;

	// GUI 시스템 초기화
	initGUISystem();

	// 현재 마우스 위치에 커서 출력
	getWinCursor(&xMouse, &yMouse);
	moveCursor(xMouse, yMouse);

	// 윈도우 매니저 태스크 루프
	while(1) {
		// 마우스 데이터 처리
		mouseRes = procMouseData();

		// 키 데이터 처리
		keyRes = procKeyData();

		// 윈도우 매니저의 이벤트 큐에 수신된 데이터 처리. 수신된 모든 이벤트 처리함
		eventRes = FALSE;
		while(procEventData() == TRUE) eventRes = TRUE;

		// 처리한 데이터가 하나도 없다면 _sleep() 수행
		if((mouseRes == FALSE) && (keyRes == FALSE) && (eventRes == FALSE)) _sleep(0);
	}
}

// 수신된 마우스 데이터 처리
BOOL procMouseData(void) {
	QWORD mouseID, winID;
	BYTE btnStat, btnUpdate;
	int x, y, xMouse, yMouse, preX, preY;
	RECT area;
	EVENT event;
	WINDOWMANAGER *win;
	static int winCnt = 0;
	char title[WINDOW_TITLE_MAXLEN];
	char *str1 = "### YummyHitOS is reached Window Version ###", *str2 = "###     YummyHitOs'll be back soon !!    ###";

	// 마우스 데이터 수신되기 기다림
	if(rmMouseData(&btnStat, &x, &y) == FALSE) return FALSE;

	// 윈도우 매니저 반환
	win = getWinManager();

	// 현재 마우스 커서 위치 반환
	getWinCursor(&xMouse, &yMouse);

	// 움직이기 이전 좌표 저장
	preX = xMouse;
	preY = yMouse;

	// 마우스가 움직인 거리를 이전 커서 위치에 더해 현재 좌표 계산
	xMouse += x;
	yMouse += y;

	// 새로운 위치로 마우스 커서 이동, 다시 현재 커서 위치 반환. 마우스 커서가 화면을 벗어나지 않도록 처리된 커서 좌표 사용.
	moveCursor(xMouse, yMouse);
	getWinCursor(&xMouse, &yMouse);

	// 현재 마우스 커서 아래 윈도우 검색
	mouseID = findWinPoint(xMouse, yMouse);

	// 버튼 상태가 변했는지 확인 후 상태에 따라 마우스 및 윈도우 데이터 처리. 이전 버튼 상태와 현재 버튼 상태를 XOR해 설정 확인
	btnUpdate = win->preBtnStat ^ btnStat;

	// 마우스 왼쪽 버튼에 변화가 생긴 경우 처리
	if(btnUpdate & MOUSE_LCLICK_ON) {
		// 왼쪽 버튼이 눌린 경우 처리
		if(btnStat & MOUSE_LCLICK_ON) {
			// 마우스로 윈도우를 선택했으니 마우스 아래 윈도우가 최상위로 올라옴. 동시에 윈도우 선택과 선택해제 이벤트 전송
			if(mouseID != win->backgroundID) moveWinTop(mouseID);

			// 왼쪽 버튼이 눌린 위치가 제목 표시줄 위치면 윈도우 이동 혹은 닫기 버튼인지 확인 후 처리
			if(isInTitle(mouseID, xMouse, yMouse) == TRUE) {
				// 닫기 버튼이 눌렸으면 닫기 처리. 닫기 버튼이 아니면 이동 모드
				if(isCloseBtn(mouseID, xMouse, yMouse) == TRUE) {
					// 윈도우 닫기 이벤트 전송
					setWinEvent(mouseID, EVENT_WINDOW_CLOSE, &event);
					eventToWin(mouseID, &event);

					// 테스트를 위한 부분
					delWin(mouseID);
				} else {
					// 윈도우 이동 모드 설정
					win->moveMode = TRUE;

					// 현재 윈도우를 이동하는 윈도우로 설정
					win->moveID = mouseID;
				}
			} else {
				// 왼쪽 버튼 눌림 이벤트 전송
				setMouseEvent(mouseID, EVENT_MOUSE_LCLICK_ON, xMouse, yMouse, btnStat, &event);
				eventToWin(mouseID, &event);
			}
		} else {
			// 왼쪽 버튼이 떨어진 경우 처리. 윈도우가 이동 중이면 모드만 해제
			if(win->moveMode == TRUE) {
				// 이동 중이라는 플래그 해제
				win->moveMode = FALSE;
				win->moveID = WINDOW_INVALID_ID;
			} else {
				// 왼쪽 버튼 떨어짐 이벤트 전송
				setMouseEvent(mouseID, EVENT_MOUSE_LCLICK_OFF, xMouse, yMouse, btnStat, &event);
				eventToWin(mouseID, &event);
			}
		}
	} else if(btnUpdate & MOUSE_RCLICK_ON) {
		// 마우스 오른쪽 버튼에 변화가 생긴 경우 처리
		if(btnStat & MOUSE_RCLICK_ON) {
			// 오른쪽 버튼 눌림 이벤트 전송
			setMouseEvent(mouseID, EVENT_MOUSE_RCLICK_ON, xMouse, yMouse, btnStat, &event);
			eventToWin(mouseID, &event);

			// 테스트를 위한 오른쪽 버튼시 윈도우 생성
			sprintF(title, "YummyHitOS by %d", winCnt++);
			winID = createWin(xMouse - 10, yMouse - WINDOW_TITLE_HEIGHT / 2, 400, 200, WINDOW_FLAGS_DRAWFRAME | WINDOW_FLAGS_DRAWTITLE, title);

			// 윈도우 내부에 텍스트 출력 후 윈도우를 화면에 나타냄
			drawText(winID, 10, WINDOW_TITLE_HEIGHT + 10, RGB(102, 0, 204), WINDOW_COLOR_BACKGROUND, str1, strLen(str1));
			drawText(winID, 10, WINDOW_TITLE_HEIGHT + 30, RGB(119, 68, 255), WINDOW_COLOR_BACKGROUND, str2, strLen(str2));
			showWin(winID, TRUE);
		} else {
			// 오른쪽 버튼 떨어짐 이벤트 전송
			setMouseEvent(mouseID, EVENT_MOUSE_RCLICK_OFF, xMouse, yMouse, btnStat, &event);
			eventToWin(mouseID, &event);
		}
	} else if(btnUpdate & MOUSE_WHEEL_ON) {
		// 마우스 가운데 휠 변화 처리
		if(btnStat & MOUSE_WHEEL_ON) {
			// 휠 눌림 이벤트 전송
			setMouseEvent(mouseID, EVENT_MOUSE_WHEEL_ON, xMouse, yMouse, btnStat, &event);
			eventToWin(mouseID, &event);
		} else {
			// 휠 떨어짐 이벤트 전송
			setMouseEvent(mouseID, EVENT_MOUSE_WHEEL_OFF, xMouse, yMouse, btnStat, &event);
			eventToWin(mouseID, &event);
		}
	} else {
		// 마우스 버튼이 그대로면 마우스 이동 처리만 수행
		setMouseEvent(mouseID, EVENT_MOUSE_MOVE, xMouse, yMouse, btnStat, &event);
		eventToWin(mouseID, &event);
	}

	// 윈도우가 이동 중이면 윈도우 이동 처리
	if(win->moveMode == TRUE) {
		// 윈도우 위치 얻음. 윈도우 현재 위치를 통해 마우스가 이동한 만큼 옮겨줌.
		if(getWinArea(win->moveID, &area) == TRUE) moveWin(win->moveID, area.x1 + xMouse - preX, area.y1 + yMouse - preY);
		else {
			// 윈도우 위치를 못얻으면 윈도우가 없는 것이니 이동 모드 해제
			win->moveMode = FALSE;
			win->moveID = WINDOW_INVALID_ID;
		}
	}

	// 다음 처리에 사용할 현재 버튼 상태 저장
	win->preBtnStat = btnStat;
	return TRUE;
}

// 수신된 키 데이터 처리
BOOL procKeyData(void) {
	KEYDATA key;
	EVENT event;
	QWORD winID;

	// 키보드 데이터가 수신되기 기다림
	if(getKeyData(&key) == FALSE) return FALSE;

	// 최상위 윈도우로 메시지 전송
	winID = getTopWin();
	setKeyEvent(winID, &key, &event);
	return eventToWin(winID, &event);
}

// 이벤트 큐에 수신된 이벤트 처리
BOOL procEventData(void) {
	EVENT event;
	WINDOWEVENT *winEvent;
	QWORD winID;
	RECT area;

	// 윈도우 매니저의 이벤트 큐에 이벤트 수신 대기
	if(winManagerToEvent(&event) == FALSE) return FALSE;

	winEvent = &(event.winEvent);

	// 타입별 처리
	switch(event.type) {
	// 현재 윈도우가 있는 영역 업데이트
	case EVENT_WINDOWMANAGER_UPDATEBYID:
		if(getWinArea(winEvent->id, &area) == TRUE) updateWinArea(&area);
		break;
	// 윈도우 내부 영역 업데이트
	case EVENT_WINDOWMANAGER_UPDATEBYWINAREA:
		// 윈도우 기준 한 좌표를 화면 좌표로 반환해 업데이트
		if(rectToMonitor(winEvent->id, &(winEvent->area), &area) == TRUE) updateWinArea(&area);
		break;
	// 화면 좌표로 전달된 영역 업데이트
	case EVENT_WINDOWMANAGER_UPDATEBYMONAREA:
		updateWinArea(&(winEvent->area));
		break;
	default:
		break;
	}

	return TRUE;
}
