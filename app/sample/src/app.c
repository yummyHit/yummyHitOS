/*
 * app.c
 *
 *  Created on: 2018. 7. 3.
 *      Author: Yummy
 */

#include "yummyHitLib.h"

// 응용프로그램이 보내는 유저 이벤트 타입 정의
#define EVENT_USER_TESTMSG		0x80000001

// 응용프로그램 C 언어 엔트리 포인트
int main(char *argv) {
	QWORD id, winID;
	int xMouse, yMouse, width, height, y, i;
	EVENT recvEvent, sendEvent;
	MOUSEEVENT *mouseEvent;
	KEYEVENT *keyEvent;
	WINDOWEVENT *winEvent;
	char buf[1024];
	static int winCnt = 0;
	char *str[] = {
		"Unknown",
		"MOUSE_MOVE",
		"MOUSE_LCLICK_ON",
		"MOUSE_LCLICK_OFF",
		"MOUSE_RCLICK_ON",
		"MOUSE_RCLICK_OFF",
		"MOUSE_WHEEL_ON",
		"MOUSE_WHEEL_OFF",
		"WINDOW_SELECT",
		"WINDOW_DESELECT",
		"WINDOW_MOVE",
		"WINDOW_RESIZE",
		"WINDOW_CLOSE",
		"KEY_DOWN",
		"KEY_UP"	};
	RECT area;

	// 그래픽 모드 판단. 그래픽 모드로 시작했는지 여부 확인
	if(isGUIMode() == FALSE) {
		printf("It is GUI Task. You must execute GUI Mode.\n");
		return 0;
	}

	// 윈도우 생성. 마우스 현재 위치 반환
	getWinCursor(&xMouse, &yMouse);

	// 윈도우 크기와 제목 설정
	width = 500;
	height = 200;

	// 윈도우 생성 함수 호출. 마우스가 있던 위치를 기준으로 생성하고 번호를 추가해 윈도우마다 개별 이름 할당
	sprintf(buf, "YummyHitOS Window %d", winCnt++);
	id = makeWin(xMouse - 10, yMouse - WINDOW_TITLE_HEIGHT / 2, width, height, WINDOW_FLAGS_DEFAULT | WINDOW_FLAGS_RESIZABLE, buf);
	// 윈도우 생성 못하면 실패
	if(id == WINDOW_INVALID_ID) return 0;

	// 윈도우 매니저가 윈도우로 전송하는 이벤트 표시 영역 그림. 이벤트 정보 출력할 위치 저장
	y = WINDOW_TITLE_HEIGHT + 10;

	// 이벤트 정보 표시 영역 테두리와 윈도우 ID 표시
	drawRect(id, 10, y + 8, width - 10, y + 70, RGB(0, 0, 0), FALSE);
	sprintf(buf, "GUI Event Information[Window ID: 0x%Q, User Mode:%s]", id, argv);
	drawText(id, 20, y, RGB(186, 140, 255), RGB(255, 255, 255), buf, strlen(buf));

	// 화면 아래 이벤트 전송 버튼 그림. 버튼 영역 설정
	setRectData(10, y + 80, width - 10, height - 10, &area);
	// 배경은 윈도우 배경색으로 설정, 문자는 보라색으로 설정해 버튼 그림
	drawBtn(id, &area, WINDOW_COLOR_BACKGROUND, "User Message Send Button(Unused)", RGB(102, 102, 255));
	// 윈도우를 화면에 표시
	showWin(id, TRUE);

	// GUI 태스크 이벤트 처리 루프
	while(1) {
		// 이벤트 큐에서 이벤트 수신
		if(winToEvent(id, &recvEvent) == FALSE) {
			sleep(0);
			continue;
		}

		// 윈도우 이벤트 정보가 표시될 영역을 배경색으로 칠해 이전에 표시한 데이터 모두 지움
		drawRect(id, 11, y + 20, width - 11, y + 69, WINDOW_COLOR_BACKGROUND, TRUE);

		// 수신된 이벤트를 타입에 따라 나눠 처리
		switch(recvEvent.type) {
			// 마우스 이벤트 처리
			case EVENT_MOUSE_MOVE:
			case EVENT_MOUSE_LCLICK_ON:
			case EVENT_MOUSE_LCLICK_OFF:
			case EVENT_MOUSE_RCLICK_ON:
			case EVENT_MOUSE_RCLICK_OFF:
			case EVENT_MOUSE_WHEEL_ON:
			case EVENT_MOUSE_WHEEL_OFF:
				// 여기에 마우스 이벤트 처리 코드 구현
				mouseEvent = &(recvEvent.mouseEvent);

				// 마우스 이벤트 타입 출력
				sprintf(buf, "Mouse Event: %s", str[recvEvent.type]);
				drawText(id, 20, y + 20, RGB(119, 68, 255), WINDOW_COLOR_BACKGROUND, buf, strlen(buf));

				// 마우스 데이터 출력
				sprintf(buf, "Data: X = %d, Y = %d, Button = %X", mouseEvent->point.x, mouseEvent->point.y, mouseEvent->btnStat);
				drawText(id, 20, y + 40, RGB(119, 68, 255), WINDOW_COLOR_BACKGROUND, buf, strlen(buf));

				// 마우스 눌림 또는 떨어짐 이벤트면 버튼 색깔 다시 그림. 마우스 왼쪽 버튼이 눌렸을 때 처리
				if(recvEvent.type == EVENT_MOUSE_LCLICK_ON) {
					// 버튼 영역에 마우스 왼쪽 버튼이 눌렸는지 판단
					if(isInRect(&area, mouseEvent->point.x, mouseEvent->point.y) == TRUE) {
						// 버튼 배경을 노란색으로 변경해 표시
						drawBtn(id, &area, RGB(255, 255, 135), "User Message Send Button(Using)", RGB(102, 102, 255));
						updateMonID(id);

						// 다른 윈도우로 유저 이벤트 전송. 생성된 모든 윈도우 찾아 이벤트 전송
						sendEvent.type = EVENT_USER_TESTMSG;
						sendEvent.data[0] = id;
						sendEvent.data[1] = 0x4444;
						sendEvent.data[2] = 0x7777;

						// 생성된 윈도우 수만큼 루프 수행하며 이벤트 전송
						for(i = 0; i < winCnt; i++) {
							// 윈도우 제목으로 윈도우 검색
							sprintf(buf, "YummyHitOS Window %d", i);
							winID = findWinTitle(buf);
							// 윈도우가 존재하며 윈도우 자신이 아닌 경우 이벤트 전송
							if((winID != WINDOW_INVALID_ID) && (winID != id)) eventToWin(winID, &sendEvent);
						}
					}
				} else drawBtn(id, &area, WINDOW_COLOR_BACKGROUND, "User Message Send Button(Unused)", RGB(102, 102, 255));
				break;
				// 키 이벤트 처리
			case EVENT_KEY_DOWN:
			case EVENT_KEY_UP:
				// 여기에 키보드 이벤트 처리 코드 구현
				keyEvent = &(recvEvent.keyEvent);

				// 키 이벤트 타입 출력
				sprintf(buf, "Key Event: %s", str[recvEvent.type]);
				drawText(id, 20, y + 20, RGB(119, 68, 255), WINDOW_COLOR_BACKGROUND, buf, strlen(buf));

				// 키 데이터 출력
				sprintf(buf, "Data: Key = %c, Flag = %X", keyEvent->ascii, keyEvent->flag);
				drawText(id, 20, y + 40, RGB(119, 68, 255), WINDOW_COLOR_BACKGROUND, buf, strlen(buf));
				break;
				// 윈도우 이벤트 처리
			case EVENT_WINDOW_SELECT:
			case EVENT_WINDOW_DESELECT:
			case EVENT_WINDOW_MOVE:
			case EVENT_WINDOW_RESIZE:
			case EVENT_WINDOW_CLOSE:
				// 여기에 윈도우 이벤트 처리 코드 구현
				winEvent = &(recvEvent.winEvent);

				// 윈도우 이벤트 타입 출력
				sprintf(buf, "Window Event: %s", str[recvEvent.type]);
				drawText(id, 20, y + 20, RGB(119, 68, 255), WINDOW_COLOR_BACKGROUND, buf, strlen(buf));

				// 윈도우 데이터 출력
				sprintf(buf, "Data: X1 = %d, Y1 = %d, X2 = %d, Y2 = %d", winEvent->area.x1, winEvent->area.y1, winEvent->area.x2, winEvent->area.y2);
				drawText(id, 20, y + 40, RGB(119, 68, 255), WINDOW_COLOR_BACKGROUND, buf, strlen(buf));

				// 윈도우 닫기 이벤트면 윈도우 삭제 후 태스크 종료
				if(recvEvent.type == EVENT_WINDOW_CLOSE) {
					delWin(id);
					return 0;
				}
				break;
			default:
				// 여기에 알 수 없는 이벤트 처리 코드 넣기
				sprintf(buf, "Unknown Event: 0x%X", recvEvent.type);
				drawText(id, 20, y + 20, RGB(119, 68, 255), WINDOW_COLOR_BACKGROUND, buf, strlen(buf));

				// 윈도우 데이터 출력
				sprintf(buf, "Data0 = 0x%Q, Data1 = 0x%Q, Data2 = 0x%Q", recvEvent.data[0], recvEvent.data[1], recvEvent.data[2]);
				drawText(id, 20, y + 40, RGB(119, 68, 255), WINDOW_COLOR_BACKGROUND, buf, strlen(buf));
				break;
		}

		// 윈도우 화면 업데이트
		showWin(id, TRUE);
	}

	return 0;
}
