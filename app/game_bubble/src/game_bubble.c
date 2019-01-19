/*
 * game_bubble.c
 *
 *  Created on: 2019. 1. 17.
 *      Author: Yummy
 */

#include "yummyHitLib.h"
#include "game_bubble.h"

BUBBLEINFO g_bubbleInfo = {0,};

int main(char *argv) {
	QWORD winID, lastTickCnt;
	EVENT recvEvent;
	char *startMsg = "Click to Start!";
	POINT mouse;
	RECT monArea;
	int x, y;

	// 윈도우 화면 가운데 가로x세로 크기 생성
	getMonArea(&monArea);
	x = (getRectWidth(&monArea) - BUBBLE_WIDTH) / 2;
	y = (getRectHeight(&monArea) - BUBBLE_HEIGHT) / 2;
	winID = makeWin(x, y, BUBBLE_WIDTH, BUBBLE_HEIGHT, WINDOW_FLAGS_DEFAULT, "Bubble Pang");

	if(winID == WINDOW_INVALID_ID) {
		printf("Window create failed..\n");
		return -1;
	}

	// 게임 관련 정보 초기화 후 사용할 버퍼 할당
	mouse.x = BUBBLE_WIDTH / 2;
	mouse.y = BUBBLE_HEIGHT / 2;

	if(bubbleInit() == FALSE) {
		// 초기화 실패 시 윈도우 삭제
		delWin(winID);
		return -1;
	}

	// 난수 초기값 설정
	srand(getTickCnt());

	// 게임 정보와 게임 영역 출력 후 게임 시작 대기 메시지 표시
	drawInfo(winID);
	drawGameArea(winID, &mouse);
	drawText(winID, 72, 150, RGB(255, 255, 255), RGB(0, 0, 0), startMsg, strlen(startMsg));

	// 출력된 메시지 화면에 표시
	showWin(winID, TRUE);

	// GUI 태스크 이벤트 및 게임 루프 처리하는 부분
	lastTickCnt = getTickCnt();
	while(1) {
		// 이벤트 큐에서 이벤트 수신
		if(winToEvent(winID, &recvEvent) == TRUE) {
			// 수신된 이벤트 타입에 따라 나눠 처리
			switch(recvEvent.type) {
				case EVENT_MOUSE_LCLICK_ON:
					// 게임 시작 클릭이면 수행
					if(g_bubbleInfo.start == FALSE) {
						bubbleInit();
						g_bubbleInfo.start = TRUE;
						break;
					}

					// 마우스 클릭된 곳의 물방울 삭제
					delBubble(&(recvEvent.mouseEvent.point));

					// 마우스 위치 저장
					memcpy(&mouse, &(recvEvent.mouseEvent.point), sizeof(mouse));
					break;

				// 마우스 이동 정보
				case EVENT_MOUSE_MOVE:
					// 마우스 위치 저장
					memcpy(&mouse, &(recvEvent.mouseEvent.point), sizeof(mouse));
					break;

				// 윈도우 닫기 버튼 처리
				case EVENT_WINDOW_CLOSE:
					// 윈도우 삭제 후 메모리 해제
					delWin(winID);
					free(g_bubbleInfo.buf);
					return 0;
					break;
			}
		}

		// 게임 루프 처리 부분
		if((g_bubbleInfo.start == TRUE) && ((getTickCnt() - lastTickCnt) > DEFAULT_TIME)) {
			lastTickCnt = getTickCnt();

			// 물방울 생성
			if((rand() % 7) == 1) makeBubble();

			// 물방울 이동
			moveBubble();

			// 게임 영역 표시
			drawGameArea(winID, &mouse);

			// 게임 정보 표시
			drawInfo(winID);

			// 플레이어 생명이 0이면 종료
			if(g_bubbleInfo.life <= 0) {
				g_bubbleInfo.start = FALSE;

				// 게임 종료 메시지 출력
				drawText(winID, 80, 130, RGB(255, 255, 255), RGB(0, 0, 0), "Game Over...", 12);
				drawText(winID, 72, 150, RGB(255, 255, 255), RGB(0, 0, 0), startMsg, strlen(startMsg));
			}

			// 변경된 윈도우 내부를 화면에 출력
			showWin(winID, TRUE);
		} else sleep(0);
	}

	return 0;
}

// 게임 관련 정보 초기화
BOOL bubbleInit(void) {
	// 물방울 최대 개수만큼 메모리 할당
	if(g_bubbleInfo.buf == NULL) {
		g_bubbleInfo.buf = malloc(sizeof(BUBBLE) * MAX_BUBBLE_CNT);
		if(g_bubbleInfo.buf == NULL) {
			printf("malloc() failed..\n");
			return FALSE;
		}
	}

	// 물방울 정보 초기화
	memset(g_bubbleInfo.buf, 0, sizeof(BUBBLE) * MAX_BUBBLE_CNT);
	g_bubbleInfo.cnt = 0;

	// 게임 시작되었다는 정보, 점수, 생명 설정
	g_bubbleInfo.start = FALSE;
	g_bubbleInfo.score = 0;
	g_bubbleInfo.life = MAX_LIFE;

	return TRUE;
}

// 물방울 생성
BOOL makeBubble(void) {
	BUBBLE *target;
	int i;

	// 물방울 최대 개수와 살아있는 물방울 개수 비교해 생성할지 여부 결정
	if(g_bubbleInfo.cnt >= MAX_BUBBLE_CNT) return FALSE;

	// 빈 물방울 자료구조 검색
	for(i = 0; i < MAX_BUBBLE_CNT; i++) {
		// 물방울이 살아있지 않으면 재할당
		if(g_bubbleInfo.buf[i].alive == FALSE) {
			// 선택된 물방울 자료구조
			target = &(g_bubbleInfo.buf[i]);

			// 물방울이 살아있다고 설정한 후 물방울 이동 속도 초기화
			target->alive = TRUE;
			target->speed = (rand() % 8) + DEFAULT_SPEED;

			// X좌표와 Y좌표는 물방울이 게임 영역 내부에 위치하도록 설정
			target->x = rand() % (BUBBLE_WIDTH - (2 * RADIUS)) + RADIUS;
			target->y = INFO_HEIGHT + WINDOW_TITLE_HEIGHT + RADIUS + 1;

			// 물방울 색 설정
			target->color = RGB(rand() % 256, rand() % 256, rand() % 256);

			// 살아있는 물방울 수 증가
			g_bubbleInfo.cnt++;
			return TRUE;
		}
	}

	return FALSE;
}

// 물방울 이동
void moveBubble(void) {
	BUBBLE *target;
	int i;

	// 살아있는 모든 물방울 이동
	for(i = 0; i < MAX_BUBBLE_CNT; i++) {
		// 물방울이 살아있으면 이동
		if(g_bubbleInfo.buf[i].alive == TRUE) {
			// 현재 물방울 자료구조
			target = &(g_bubbleInfo.buf[i]);

			// 물방울 Y좌표에 이동속도 증가
			target->y += target->speed;

			// 게임 영역 끝에 닿으면 물방울 제거 후 생명 깎음
			if((target->y + RADIUS) >= BUBBLE_HEIGHT) {
				target->alive = FALSE;
				g_bubbleInfo.cnt--;
				if(g_bubbleInfo.life > 0) g_bubbleInfo.life--;
			}
		}
	}
}

// 마우스 아래 물방울 삭제 후 점수 증가
void delBubble(POINT *mouse) {
	BUBBLE *target;
	int i;
	QWORD distance;

	// 살아있는 모든 물방울 검색하여 마우스 아래 물방울 제거
	for(i = MAX_BUBBLE_CNT - 1; i >= 0; i--) {
		// 물방울이 살아있으면 거리 계산 후 삭제 여부 결정
		if(g_bubbleInfo.buf[i].alive == TRUE) {
			target = &(g_bubbleInfo.buf[i]);

			// 마우스 클릭된 위치와 원 중심이 반지름 거리 안쪽이면 삭제
			distance = ((mouse->x - target->x) * (mouse->x - target->x)) + ((mouse->y - target->y) * (mouse->y - target->y));
			// 물방울 중심과 마우스 클릭 위치 사이 거리를 물방울 반지름과 비교해 작다면 물방울 내부 클릭이므로 삭제
			if(distance < (RADIUS * RADIUS)) {
				target->alive = FALSE;

				// 살아있는 물방울 수 줄이고 점수 증가
				g_bubbleInfo.cnt--;
				g_bubbleInfo.score++;
				break;
			}
		}
	}
}

// 게임 정보 출력
void drawInfo(QWORD winID) {
	char buf[200];
	int len;

	// 게임 정보 영역 표시
	drawRect(winID, 1, WINDOW_TITLE_HEIGHT - 1, BUBBLE_WIDTH - 2, WINDOW_TITLE_HEIGHT + INFO_HEIGHT, RGB(159, 48, 215), TRUE);

	// 임시 버퍼에 출력할 정보 저장
	sprintf(buf, "Life: %d, Score: %d\n", g_bubbleInfo.life, g_bubbleInfo.score);
	len = strlen(buf);

	// 저장된 정보를 게임 정보 표시 영역 가운데 출력
	drawText(winID, (BUBBLE_WIDTH - (len * FONT_ENG_WIDTH)) / 2, WINDOW_TITLE_HEIGHT + 2, RGB(255, 255, 255), RGB(159, 48, 215), buf, len);
}

// 게임 영역 출력
void drawGameArea(QWORD winID, POINT *mouse) {
	BUBBLE *target;
	int i;

	// 게임 영역 배경 출력
	drawRect(winID, 0, WINDOW_TITLE_HEIGHT + INFO_HEIGHT, BUBBLE_WIDTH - 1, BUBBLE_HEIGHT - 1, RGB(0, 0, 0), TRUE);

	// 살아있는 모든 물방울 표시
	for(i = 0; i < MAX_BUBBLE_CNT; i++) {
		// 물방울이 살아있으면 화면에 표시
		if(g_bubbleInfo.buf[i].alive == TRUE) {
			target = &(g_bubbleInfo.buf[i]);

			// 물방울 내부와 외부 그림
			drawCircle(winID, target->x, target->y, RADIUS, target->color, TRUE);
			drawCircle(winID, target->x, target->y, RADIUS, ~target->color, FALSE);
		}
	}

	// 마우스가 있는 위치를 검사해 조준선 표시
	if(mouse->y < (WINDOW_TITLE_HEIGHT + RADIUS))
		mouse->y = WINDOW_TITLE_HEIGHT + RADIUS;

	// 조준선을 +로 표시
	drawLine(winID, mouse->x, mouse->y - RADIUS, mouse->x, mouse->y + RADIUS, RGB(255, 0, 0));
	drawLine(winID, mouse->x - RADIUS, mouse->y, mouse->x + RADIUS, mouse->y, RGB(255, 0, 0));

	// 게임 영역 테두리 표시
	drawRect(winID, 0, WINDOW_TITLE_HEIGHT + INFO_HEIGHT, BUBBLE_WIDTH - 1, BUBBLE_HEIGHT - 1, RGB(0, 255, 0), FALSE);
}
