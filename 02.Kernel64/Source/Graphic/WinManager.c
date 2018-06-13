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
#include <CLITask.h>
#include <GUITask.h>
#include <Font.h>
#include <AppPanelTask.h>

// 윈도우 매니저 태스크
void kStartWinManager(void) {
	int xMouse, yMouse;
	BOOL mouseRes, keyRes, eventRes;
	WINDOWMANAGER *win;

	QWORD lastTickCnt, preLoopCnt, loopCnt, minLoopCnt, backgroundID;
	char tmp[40];
	RECT loopCntArea;

	// GUI 시스템 초기화
	kInitGUISystem();

	// 현재 마우스 위치에 커서 출력
	kGetWinCursor(&xMouse, &yMouse);
	kMoveCursor(xMouse, yMouse);

	// 애플리케이션 패널 태스크 실행
	kCreateTask(TASK_FLAGS_SYSTEM | TASK_FLAGS_THREAD | TASK_FLAGS_LOW, 0, 0, (QWORD)kAppPanelGUITask, TASK_LOADBALANCING_ID);

	// 윈도우 매니저 반환
	win = kGetWinManager();

	// 루프 수행 횟수 측정용 변수 초기화
	lastTickCnt = kGetTickCnt();
	preLoopCnt = 0;
	loopCnt = 0;
	minLoopCnt = 0xFFFFFFFFFFFFFFFF;
	backgroundID = kGetBackgroundID();

	// 윈도우 매니저 태스크 루프
	while(1) {
#if 0
		// 1초마다 윈도우 매니저 태스크 루프를 수행한 횟수를 측정해 최솟값 기록
		if(kGetTickCnt() - lastTickCnt > 1000) {
			lastTickCnt = kGetTickCnt();
			// 1초 전 수행한 태스크 루프의 수와 현재 태스크 루프의 수를 빼 최소 루프 수행 횟수와 비교해 업데이트
			if((loopCnt - preLoopCnt) < minLoopCnt) minLoopCnt = loopCnt - preLoopCnt;
			preLoopCnt = loopCnt;

			// 루프의 최소 수행 횟수를 1초마다 업데이트
			kSprintF(tmp, "MIN Loop Execution Count:%d   ", minLoopCnt);
			kDrawText(backgroundID, 0, 0, RGB(255, 255, 135), RGB(186, 140, 255), tmp, strLen(tmp));

			// 배경 윈도우 전체를 업데이트하면 시간이 오래 걸려 배경 윈도우에 루프 수행 횟수가 출력된 부분만 업데이트
			kSetRectData(0, 0, strLen(tmp) * FONT_ENG_WIDTH, FONT_ENG_HEIGHT, &loopCntArea);
			kUpdateWinArea(&loopCntArea, backgroundID);
		}
		loopCnt++;
#endif

		// 마우스 데이터 처리
		mouseRes = kProcMouseData();

		// 키 데이터 처리
		keyRes = kProcKeyData();

		// 윈도우 매니저의 이벤트 큐에 수신된 데이터 처리. 수신된 모든 이벤트 처리함
		eventRes = FALSE;
		while(kProcEventData() == TRUE) eventRes = TRUE;

		// 이벤트 큐에 이벤트를 처리했다면 윈도우 크기 변경 표식이 지워질 수 있으니 다시 출력
		if((eventRes == TRUE) && (win->resizeMode == TRUE)) kDrawResizeMark(&(win->resizeArea), TRUE);

		// 처리한 데이터가 하나도 없다면 kSleep() 수행
		if((mouseRes == FALSE) && (keyRes == FALSE) && (eventRes == FALSE)) kSleep(0);
	}
}

// 수신된 마우스 데이터 처리
BOOL kProcMouseData(void) {
	QWORD mouseID, winID;
	BYTE btnStat, btnUpdate;
	int x, y, xMouse, yMouse, preX, preY, i, width, height;
	RECT area;
	EVENT event;
	WINDOWMANAGER *win;
	static int winCnt = 0;
	char title[WINDOW_TITLE_MAXLEN];
	char *str1 = "### YummyHitOS is reached Window Version ###", *str2 = "###     YummyHitOs'll be back soon !!    ###";

	// 윈도우 매니저 반환
	win = kGetWinManager();

	// 마우스 이벤트 통합
	for(i = 0; i < WINDOWMANAGER_DATACNT; i++) {
		// 마우스 데이터 수신 대기
		if(kRmMouseData(&btnStat, &x, &y) == FALSE) {
			// 처음으로 확인했는데 데이터 없으면 종료
			if(i == 0) return FALSE;
			else break;
		}

		// 현재 마우스 커서 위치 반환
		kGetWinCursor(&xMouse, &yMouse);

		// 처음 마우스 이벤트가 수신된 것이면 현재 좌표를 이전 마우스 위치로 저장
		if(i == 0) {
			// 움직이기 이전 좌표 저장
			preX = xMouse;
			preY = yMouse;
		}

		// 마우스가 움직인 거리를 이전 커서 위치에 더해 현재 좌표 계산
		xMouse += x;
		yMouse += y;

		// 새로운 위치로 마우스 커서 이동, 다시 현재 커서 위치 반환. 마우스 커서가 화면을 벗어나지 않도록 처리된 커서 좌표 사용.
		kMoveCursor(xMouse, yMouse);
		kGetWinCursor(&xMouse, &yMouse);

		// 버튼 상태는 이전 버튼 상태와 현재 버튼 상태를 XOR하여 1로 설정됐는지 확인
		btnUpdate = win->preBtnStat ^ btnStat;

		// 마우스가 움직였으나 버튼 변화가 있으면 이벤트 처리
		if(btnUpdate != 0) break;
	}

	// 현재 마우스 커서 아래 윈도우 검색
	mouseID = kFindWinPoint(xMouse, yMouse);

	// 마우스 왼쪽 버튼에 변화가 생긴 경우 처리
	if(btnUpdate & MOUSE_LCLICK_ON) {
		// 왼쪽 버튼이 눌린 경우 처리
		if(btnStat & MOUSE_LCLICK_ON) {
			// 마우스로 윈도우를 선택했으니 마우스 아래 윈도우가 최상위로 올라옴. 동시에 윈도우 선택과 선택해제 이벤트 전송
			if(mouseID != win->backgroundID) kMoveWinTop(mouseID);

			// 왼쪽 버튼이 눌린 위치가 제목 표시줄 위치면 윈도우 이동 혹은 닫기 버튼인지 확인 후 처리
			if(kIsInTitle(mouseID, xMouse, yMouse) == TRUE) {
				// 닫기 버튼이 눌렸으면 닫기 처리. 닫기 버튼이 아니면 이동 모드
				if(kIsCloseBtn(mouseID, xMouse, yMouse) == TRUE) {
					// 윈도우 닫기 이벤트 전송
					kSetWinEvent(mouseID, EVENT_WINDOW_CLOSE, &event);
					kEventToWin(mouseID, &event);

					// 테스트를 위한 부분
					kDelWin(mouseID);
				} else if(kIsResizeBtn(mouseID, xMouse, yMouse) == TRUE) {
					// 윈도우 크기 변경 모드 설정
					win->resizeMode = TRUE;

					// 현재 윈도우 크기 변경 윈도우로 설정
					win->resizeID = mouseID;

					// 현재 윈도우 크기 저장
					kGetWinArea(mouseID, &(win->resizeArea));

					// 윈도우 크기 변경 아이콘 표시
					kDrawResizeMark(&(win->resizeArea), TRUE);
				} else {
					// 윈도우 이동 모드 설정
					win->moveMode = TRUE;

					// 현재 윈도우를 이동하는 윈도우로 설정
					win->moveID = mouseID;
				}
			} else {
				// 왼쪽 버튼 눌림 이벤트 전송
				kSetMouseEvent(mouseID, EVENT_MOUSE_LCLICK_ON, xMouse, yMouse, btnStat, &event);
				kEventToWin(mouseID, &event);
			}
		} else {
			// 왼쪽 버튼이 떨어진 경우 처리. 윈도우가 이동 중이면 모드만 해제
			if(win->moveMode == TRUE) {
				// 이동 중이라는 플래그 해제
				win->moveMode = FALSE;
				win->moveID = WINDOW_INVALID_ID;
			} else if(win->resizeMode == TRUE) {
				// 윈도우 매니저 자료구조에 저장된 영역을 이용해 윈도우 크기 변경
				width = kGetRectWidth(&(win->resizeArea));
				height = kGetRectHeight(&(win->resizeArea));
				kResizeWin(win->resizeID, win->resizeArea.x1, win->resizeArea.y1, width, height);

				// 윈도우 크기 변경 아이콘 삭제
				kDrawResizeMark(&(win->resizeArea), FALSE);

				// 윈도우 크기 변경 이벤트 전송
				kSetWinEvent(win->resizeID, EVENT_WINDOW_RESIZE, &event);
				kEventToWin(win->resizeID, &event);

				// 크기 변경 중 플래그 해제
				win->resizeMode = FALSE;
				win->resizeID = WINDOW_INVALID_ID;
			} else {
				// 왼쪽 버튼 떨어짐 이벤트 전송
				kSetMouseEvent(mouseID, EVENT_MOUSE_LCLICK_OFF, xMouse, yMouse, btnStat, &event);
				kEventToWin(mouseID, &event);
			}
		}
	} else if(btnUpdate & MOUSE_RCLICK_ON) {
		// 마우스 오른쪽 버튼에 변화가 생긴 경우 처리
		if(btnStat & MOUSE_RCLICK_ON) {
			// 오른쪽 버튼 눌림 이벤트 전송
			kSetMouseEvent(mouseID, EVENT_MOUSE_RCLICK_ON, xMouse, yMouse, btnStat, &event);
			kEventToWin(mouseID, &event);

			// 테스트를 위해 오른쪽 버튼이 눌리면 GUI 태스크 생성
//			kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, NULL, NULL, (QWORD)firstGUITask, TASK_LOADBALANCING_ID);
		} else  {
			kSetMouseEvent(mouseID, EVENT_MOUSE_RCLICK_OFF, xMouse, yMouse ,btnStat, &event);
			kEventToWin(mouseID, &event);
		}
	} else if(btnUpdate & MOUSE_WHEEL_ON) {
		// 마우스 가운데 휠 변화 처리
		if(btnStat & MOUSE_WHEEL_ON) {
			// 휠 눌림 이벤트 전송
			kSetMouseEvent(mouseID, EVENT_MOUSE_WHEEL_ON, xMouse, yMouse, btnStat, &event);
			kEventToWin(mouseID, &event);
		} else {
			// 휠 떨어짐 이벤트 전송
			kSetMouseEvent(mouseID, EVENT_MOUSE_WHEEL_OFF, xMouse, yMouse, btnStat, &event);
			kEventToWin(mouseID, &event);
		}
	} else {
		// 마우스 버튼이 그대로면 마우스 이동 처리만 수행
		kSetMouseEvent(mouseID, EVENT_MOUSE_MOVE, xMouse, yMouse, btnStat, &event);
		kEventToWin(mouseID, &event);
	}

	// 윈도우가 이동 중이면 윈도우 이동 처리
	if(win->moveMode == TRUE) {
		// 윈도우 위치 얻음. 윈도우 현재 위치를 통해 마우스가 이동한 만큼 옮겨줌.
		if(kGetWinArea(win->moveID, &area) == TRUE) kMoveWin(win->moveID, area.x1 + xMouse - preX, area.y1 + yMouse - preY);
		else {
			// 윈도우 위치를 못얻으면 윈도우가 없는 것이니 이동 모드 해제
			win->moveMode = FALSE;
			win->moveID = WINDOW_INVALID_ID;
		}
	} else if(win->resizeMode == TRUE) {
		// 윈도우 크기 변경 중이면 크기 변경 처리. 이전 위치 크기 변경 아이콘 삭제 
		kDrawResizeMark(&(win->resizeArea), FALSE);

		// 마우스 이동 거리를 이용해 새로운 윈도우 크기 결정
		win->resizeArea.x2 += xMouse - preX;
		win->resizeArea.y1 += yMouse - preY;

		// 윈도우 크기가 최솟값 이하면 최솟값으로 다시 설정
		if((win->resizeArea.x2 < win->resizeArea.x1) || (kGetRectWidth(&(win->resizeArea)) < WINDOW_WIDTH_MIN)) win->resizeArea.x2 = win->resizeArea.x1 + WINDOW_WIDTH_MIN - 1;
		if((win->resizeArea.y2 < win->resizeArea.y1) || (kGetRectWidth(&(win->resizeArea)) < WINDOW_HEIGHT_MIN)) win->resizeArea.y2 = win->resizeArea.y1 + WINDOW_HEIGHT_MIN - 1;

		// 새로운 위치에 윈도우 크기 변경 아이콘 출력
		kDrawResizeMark(&(win->resizeArea), TRUE);
	}

	// 다음 처리에 사용할 현재 버튼 상태 저장
	win->preBtnStat = btnStat;
	return TRUE;
}

// 수신된 키 데이터 처리
BOOL kProcKeyData(void) {
	KEYDATA key;
	EVENT event;
	QWORD winID;

	// 키보드 데이터가 수신되기 기다림
	if(kGetKeyData(&key) == FALSE) return FALSE;

	// 최상위 윈도우로 메시지 전송
	winID = kGetTopWin();
	kSetKeyEvent(winID, &key, &event);
	return kEventToWin(winID, &event);
}

// 이벤트 큐에 수신된 이벤트 처리
BOOL kProcEventData(void) {
	EVENT event[WINDOWMANAGER_DATACNT];
	int cnt, i, j;
	WINDOWEVENT *winEvent, *nextWinEvent;
	QWORD id;
	RECT area, crossArea;

	// 윈도우 매니저 태스크의 이벤트 큐에 수신된 이벤트 통합
	for(i = 0; i < WINDOWMANAGER_DATACNT; i++) {
		// 윈도우 매니저의 이벤트 큐에 이벤트 수신 대기
		if(kWinManagerToEvent(&(event[i])) == FALSE) {
			// 처음부터 이벤트가 수신되지 않았으면 종료
			if(i == 0) return FALSE;
			else break;
		}

		winEvent = &(event[i].winEvent);

		// 윈도우 ID로 업데이트하는 이벤트가 수신되면 윈도우 영역을 이벤트 데이터에 삽입
		if(event[i].type == EVENT_WINDOWMANAGER_UPDATEBYID) {
			// 윈도우 크기를 이벤트 자료구조에 삽입
			if(kGetWinArea(winEvent->id, &area) == FALSE) kSetRectData(0, 0, 0, 0, &(winEvent->area));
			else kSetRectData(0, 0, kGetRectWidth(&area) - 1, kGetRectHeight(&area) - 1, &(winEvent->area));
		}
	}

	// 저장된 이벤트를 검사하며 합칠 수 있는 이벤트는 하나로 만듦
	cnt = i;

	for(j = 0; j < cnt; j++) {
		// 수신된 이벤트 중 이번에 처리할 것과 같은 윈도우에서 발생하는 이벤트 검색
		winEvent = &(event[j].winEvent);
		if((event[j].type != EVENT_WINDOWMANAGER_UPDATEBYID) && (event[j].type != EVENT_WINDOWMANAGER_UPDATEBYWINAREA) && (event[j].type != EVENT_WINDOWMANAGER_UPDATEBYMONAREA)) continue;

		// 수신한 이벤트 끝까지 루프를 수행하며 수신된 이벤트 검사
		for(i = j + 1; i < cnt; i++) {
			nextWinEvent = &(event[i].winEvent);
			// 화면 업데이트가 아니거나 윈도우 ID가 일치하지 않으면 제외
			if(((event[i].type != EVENT_WINDOWMANAGER_UPDATEBYID) && (event[i].type != EVENT_WINDOWMANAGER_UPDATEBYWINAREA) && (event[i].type != EVENT_WINDOWMANAGER_UPDATEBYMONAREA)) || (winEvent->id != nextWinEvent->id)) continue;

			// 겹치는 영역을 계산해 겹치지 않으면 제외
			if(kGetRectCross(&(winEvent->area), &(nextWinEvent->area), &crossArea) == FALSE) continue;

			// 두 영역이 일치하거나 어느 한쪽이 포함되면 이벤트 통합
			if(kMemCmp(&(winEvent->area), &crossArea, sizeof(RECT)) == 0) {
				// 현재 이벤트 윈도우 영역이 겹치는 영역과 일치하면 다음 이벤트 윈도우 영역이 현재 윈도우 영역과 같거나 포함
				// 현재 이벤트에 다음 이벤트 윈도우 영역을 복사하고 다음 이벤트 삭제
				kMemCpy(&(winEvent->area), &(nextWinEvent->area), sizeof(RECT));
				event[i].type = EVENT_UNKNOWN;
			} else if(kMemCmp(&(nextWinEvent->area), &crossArea, sizeof(RECT)) == 0) {
				// 다음 이벤트 윈도우 영역이 겹치는 영역과 일치하면 현재 이벤트 윈도우 영역이 다음 윈도우 영역과 같거나 포함
				// 윈도우 영역을 복사 안하고 다음 이벤트 삭제
				event[i].type = EVENT_UNKNOWN;
			}
		}
	}

	// 통합된 이벤트 모두 처리
	for(i = 0; i < cnt; i++) {
		winEvent = &(event[i].winEvent);

		// 타입별 처리
		switch(event[i].type) {
		// 현재 윈도우가 있는 영역 업데이트
		case EVENT_WINDOWMANAGER_UPDATEBYID:
		// 윈도우 내부 영역 업데이트
		case EVENT_WINDOWMANAGER_UPDATEBYWINAREA:
			// 윈도우 기준 한 좌표를 화면 좌표로 반환해 업데이트
			if(kRectToMon(winEvent->id, &(winEvent->area), &area) == TRUE) kUpdateWinArea(&area, winEvent->id);
			break;
		// 화면 좌표로 전달된 영역 업데이트
		case EVENT_WINDOWMANAGER_UPDATEBYMONAREA:
			kUpdateWinArea(&(winEvent->area), WINDOW_INVALID_ID);
			break;
		default:
			break;
		}
	}
	return TRUE;
}

// 비디오 메모리에 윈도우 크기 변경 아이콘 출력 혹은 삭제
void kDrawResizeMark(const RECT *area, BOOL showMark) {
	RECT markArea;
	WINDOWMANAGER *win;

	// 윈도우 매니저 반환
	win = kGetWinManager();

	// 윈도우 크기 변경 아이콘 출력 시
	if(showMark == TRUE) {
		// 왼쪽 위
		kSetRectData(area->x1, area->y1, area->x1 + WINDOWMANAGER_RESIZEMARKSIZE, area->y1 + WINDOWMANAGER_RESIZEMARKSIZE, &markArea);
		kInDrawRect(&(win->area), win->memAddr, markArea.x1, markArea.y1, markArea.x2, markArea.y1 + WINDOWMANAGER_THICK_RESIZEMARK, WINDOWMANAGER_COLOR_RESIZEMARK, TRUE);
		kInDrawRect(&(win->area), win->memAddr, markArea.x1, markArea.y1, markArea.x1 + WINDOWMANAGER_THICK_RESIZEMARK, markArea.y2, WINDOWMANAGER_COLOR_RESIZEMARK, TRUE);

		// 오른쪽 위
		kSetRectData(area->x2 - WINDOWMANAGER_RESIZEMARKSIZE, area->y1, area->x2, area->y1 + WINDOWMANAGER_RESIZEMARKSIZE, &markArea);
		kInDrawRect(&(win->area), win->memAddr, markArea.x1, markArea.y1, markArea.x2, markArea.y1 + WINDOWMANAGER_THICK_RESIZEMARK, WINDOWMANAGER_COLOR_RESIZEMARK, TRUE);
		kInDrawRect(&(win->area), win->memAddr, markArea.x2 - WINDOWMANAGER_THICK_RESIZEMARK, markArea.y1, markArea.x2, markArea.y2, WINDOWMANAGER_COLOR_RESIZEMARK, TRUE);

		// 왼쪽 아래
		kSetRectData(area->x1, area->y2 - WINDOWMANAGER_RESIZEMARKSIZE, area->x1 + WINDOWMANAGER_RESIZEMARKSIZE, area->y2, &markArea);
		kInDrawRect(&(win->area), win->memAddr, markArea.x1, markArea.y2 - WINDOWMANAGER_THICK_RESIZEMARK, markArea.x2, markArea.y2, WINDOWMANAGER_COLOR_RESIZEMARK, TRUE);
		kInDrawRect(&(win->area), win->memAddr, markArea.x1, markArea.y1, markArea.x1 + WINDOWMANAGER_THICK_RESIZEMARK, markArea.y2, WINDOWMANAGER_COLOR_RESIZEMARK, TRUE);

		// 오른쪽 아래
		kSetRectData(area->x2 - WINDOWMANAGER_RESIZEMARKSIZE, area->y2 - WINDOWMANAGER_RESIZEMARKSIZE, area->x2, area->y2, &markArea);
		kInDrawRect(&(win->area), win->memAddr, markArea.x1, markArea.y2 - WINDOWMANAGER_THICK_RESIZEMARK, markArea.x2, markArea.y2, WINDOWMANAGER_COLOR_RESIZEMARK, TRUE);
		kInDrawRect(&(win->area), win->memAddr, markArea.x2 - WINDOWMANAGER_THICK_RESIZEMARK, markArea.y1, markArea.x2, markArea.y2, WINDOWMANAGER_COLOR_RESIZEMARK, TRUE);
	} else {
		// 윈도우 크기 변경 아이콘 삭제 시
		// 왼쪽 위
		kSetRectData(area->x1, area->y1, area->x1 + WINDOWMANAGER_RESIZEMARKSIZE, area->y1 + WINDOWMANAGER_RESIZEMARKSIZE, &markArea);
		kUpdateWinArea(&markArea, WINDOW_INVALID_ID);

		// 오른쪽 위
		kSetRectData(area->x2 - WINDOWMANAGER_RESIZEMARKSIZE, area->y1, area->x2, area->y1 + WINDOWMANAGER_RESIZEMARKSIZE, &markArea);
		kUpdateWinArea(&markArea, WINDOW_INVALID_ID);

		// 왼쪽 아래
		kSetRectData(area->x1, area->y2 - WINDOWMANAGER_RESIZEMARKSIZE, area->x1 + WINDOWMANAGER_RESIZEMARKSIZE, area->y2, &markArea);
		kUpdateWinArea(&markArea, WINDOW_INVALID_ID);

		// 오른쪽 아래
		kSetRectData(area->x2 - WINDOWMANAGER_RESIZEMARKSIZE, area->y2 - WINDOWMANAGER_RESIZEMARKSIZE, area->x2, area->y2, &markArea);
		kUpdateWinArea(&markArea, WINDOW_INVALID_ID);
	}
}
