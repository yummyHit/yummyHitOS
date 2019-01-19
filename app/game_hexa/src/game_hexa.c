/*
 * game_hexa.c
 *
 *  Created on: 2019. 1. 18.
 *      Author: Yummy
 */

#include "yummyHitLib.h"
#include "game_hexa.h"

HEXAINFO g_hexaInfo = {0,};

int main(char *argv) {
	QWORD winID, lastTickCnt;
	EVENT recvEvent;
	KEYEVENT *keyEvent;
	char *startMsg = "Click to Start!";
	RECT monArea;
	int x, y;
	BYTE block;

	// 윈도우를 화면 가운데 생성
	getMonArea(&monArea);
	x = (getRectWidth(&monArea) - HEXA_WIDTH) / 2;
	y = (getRectHeight(&monArea) - HEXA_HEIGHT) / 2;
	winID = makeWin(x, y, HEXA_WIDTH, HEXA_HEIGHT, WINDOW_FLAGS_DEFAULT, "Hexa Bomb");

	if(winID == WINDOW_INVALID_ID) {
		printf("Window create failed..\n");
		return -1;
	}

	// 게임 관련 정보 초기화 후 사용할 버퍼 할당
	hexaInit();

	// 난수 초기값 설정
	srand(getTickCnt());

	// 게임 정보와 게임 영역 출력 후 게임 시작 대기 메시지 표시
	drawInfo(winID);
	drawGameArea(winID);
	drawText(winID, 80, 200, RGB(255, 255, 255), RGB(0, 0, 0), startMsg, strlen(startMsg));

	// 출력된 메시지 화면에 표시
	showWin(winID, TRUE);

	// GUI 태스크 이벤트 및 게임 루프 처리 부분
	lastTickCnt = getTickCnt();
	while(1) {
		// 이벤트 큐에서 이벤트 수신
		if(winToEvent(winID, &recvEvent) == TRUE) {
			// 수신된 이벤트 타입에 따라 나눠 처리
			switch(recvEvent.type) {
				case EVENT_MOUSE_LCLICK_ON:
					// 게임 시작 클릭이면 수행
					if(g_hexaInfo.start == FALSE) {
						hexaInit();
						g_hexaInfo.start = TRUE;
						break;
					}
					break;

				// 키보드 눌림 처리
				case EVENT_KEY_DOWN:
					keyEvent = &(recvEvent.keyEvent);
					if(g_hexaInfo.start == FALSE) break;

					switch(keyEvent->ascii) {
						// 왼쪽 이동
						case KEY_ARROW_LEFT:
							if(ismovable(g_hexaInfo.x - 1, g_hexaInfo.y) == TRUE) {
								g_hexaInfo.x -= 1;
								drawGameArea(winID);
							}
							break;

						// 오른쪽 이동
						case KEY_ARROW_RIGHT:
							if(ismovable(g_hexaInfo.x + 1, g_hexaInfo.y) == TRUE) {
								g_hexaInfo.x += 1;
								drawGameArea(winID);
							}
							break;

						// 움직이는 블록 구성하는 작은 블록 순서 변경
						case KEY_ARROW_UP:
							block = g_hexaInfo.block[0];
							memcpy(&(g_hexaInfo.block), &(g_hexaInfo.block[1]), BLOCK_CNT - 1);
							g_hexaInfo.block[BLOCK_CNT - 1] = block;
							drawGameArea(winID);
							break;

						// 블록을 아래로 이동
						case KEY_ARROW_DOWN:
							if(ismovable(g_hexaInfo.x, g_hexaInfo.y + 1) == TRUE) g_hexaInfo.y += 1;
							drawGameArea(winID);
							break;

						// 블록을 아래 끝까지 이동
						case ' ':
							while(ismovable(g_hexaInfo.x, g_hexaInfo.y + 1) == TRUE) g_hexaInfo.y += 1;
							drawGameArea(winID);
							break;
					}

					// 변경된 내용 화면에 출력
					showWin(winID, TRUE);
					break;

				// 윈도우 닫기 버튼 처리
				case EVENT_WINDOW_CLOSE:
					// 윈도우 삭제
					delWin(winID);
					return 0;
					break;
			}
		}

		// 게임 루프 처리 부분
		if((g_hexaInfo.start == TRUE) && ((getTickCnt() - lastTickCnt) > (300 - (g_hexaInfo.level * 10)))) {
			lastTickCnt = getTickCnt();

			// 블록을 한 칸 아래 내리고 더 이상 내릴 수 없으면 고정
			if(ismovable(g_hexaInfo.x, g_hexaInfo.y + 1) == FALSE) {
				// 블록 고정 불가능시 게임 종료
				if(islockable(g_hexaInfo.x, g_hexaInfo.y) == FALSE) {
					g_hexaInfo.start = FALSE;

					// 게임 종료 메시지 출력
					drawText(winID, 90, 230, RGB(255, 255, 255), RGB(0, 0, 0), "Game Over...", 12);
					drawText(winID, 80, 200, RGB(255, 255, 255), RGB(0, 0, 0), startMsg, strlen(startMsg));
				}

				// 보드에 블록 검사 후 세 개 이상 연속된 블록 삭제, 화면에 출력
				delBlock(winID);

				// 새로운 블록 생성
				makeBlock();
			} else {
				g_hexaInfo.y++;
				drawGameArea(winID);
			}

			// 변경된 내용 화면에 출력
			showWin(winID, TRUE);
		} else sleep(0);
	}

	return 0;
}

// 게임 관련 정보 초기화
void hexaInit(void) {
	// 게임 정보 초기화
	memset(&g_hexaInfo, 0, sizeof(g_hexaInfo));

	// 블록 색 설정
	g_hexaInfo.color[1] = RGB(255, 0, 0);
	g_hexaInfo.color[2] = RGB(0, 0, 255);
	g_hexaInfo.color[3] = RGB(255, 0, 255);
	g_hexaInfo.color[4] = RGB(255, 255, 0);
	g_hexaInfo.color[5] = RGB(0, 255, 255);
	g_hexaInfo.edge[1] = RGB(225, 0, 0);
	g_hexaInfo.edge[2] = RGB(0, 0, 225);
	g_hexaInfo.edge[3] = RGB(225, 0, 225);
	g_hexaInfo.edge[4] = RGB(225, 225, 0);
	g_hexaInfo.edge[5] = RGB(0, 225, 225);

	// 움직이는 블록 위치 설정
	g_hexaInfo.x = -1;
	g_hexaInfo.y = 0;
}

// 움직이는 블록 생성
void makeBlock(void) {
	int i;

	// 블록의 시작 좌표는 위쪽 가운데에서 가장 아래 블록부터 표시
	g_hexaInfo.x = BOARD_WIDTH / 2;
	g_hexaInfo.y = -BLOCK_CNT;

	// 움직이는 블록 구성하는 작은 블록 종류 결정
	for(i = 0; i < BLOCK_CNT; i++)
		g_hexaInfo.block[i] = (rand() % BLOCK_ITEMS) + 1;
}

// 움직이는 블록을 특정 위치로 옮길 수 있는지 확인
BOOL ismovable(int x, int y) {
	// 블록 좌표가 게임 영역 안에 있는지 확인
	if((x < 0) || (x >= BOARD_WIDTH) || ((y + BLOCK_CNT) > BOARD_HEIGHT))
		return FALSE;

	// 마지막 블록이 위치할 곳을 확인해 게임판에 움직일 영역이 차있으면 실패
	if(g_hexaInfo.board[y + BLOCK_CNT - 1][x] != EMPTY_BLOCK)
		return FALSE;

	return TRUE;
}

// 블록 고정
BOOL islockable(int x, int y) {
	int i;

	// 블록 고정하는 위치가 0보다 작으면 현재 위치에 블록이 가득 찬 것이므로 실패
	if(y < 0) return FALSE;

	// 움직이는 블록을 현재 좌표 그대로 고정
	for(i = 0; i < BLOCK_CNT; i++)
		g_hexaInfo.board[y + i][x] = g_hexaInfo.block[i];

	// 블록이 고정되었으므로 블록 X축 좌표 -1로 설정
	g_hexaInfo.x = -1;
	return TRUE;
}

// 가로 방향으로 일치하는 블록 찾아서 표시
BOOL horizonBlock(void) {
	int cnt, i, j, k;
	BYTE block;
	BOOL flag = FALSE;

	// 게임판 전체를 검색해 가로 방향으로 3개 이상인 것 찾아서 표시
	for(j = 0; j < BOARD_HEIGHT; j++) {
		cnt = 0;
		block = 0xFF;

		for(i = 0; i < BOARD_WIDTH; i++) {
			// 첫 번째면 블록 종류 저장
			if((cnt == 0) && (g_hexaInfo.board[j][i] != EMPTY_BLOCK)) {
				block = g_hexaInfo.board[j][i];
				cnt++;
			} else {
				// 종류가 일치하면 일치한 블록 수 증가
				if(g_hexaInfo.board[j][i] == block) {
					cnt++;
					// 연속된 블록이 3개면 모아둔 이전 블록 3개를 지울 블록으로 표시
					if(cnt == BLOCK_CNT) {
						for(k = 0; k < cnt; k++)
							g_hexaInfo.erase[j][i - k] = ERASE_BLOCK;
						flag = TRUE;
					} else if(cnt > BLOCK_CNT) {
						// 연속된 블록이 4개 이상이면 즉시 지울 블록으로 표시
						g_hexaInfo.erase[j][i] = ERASE_BLOCK;
					}
				} else {
					// 일치하지 않으면 새로운 블록으로 설정
					if(g_hexaInfo.board[j][i] != EMPTY_BLOCK) {
						cnt = 1;
						block = g_hexaInfo.board[j][i];
					} else {
						cnt = 0;
						block = 0xFF;
					}
				}
			}
		}
	}

	return flag;
}

// 세로 방향으로 일치하는 블록 찾아서 표시
BOOL verticalBlock(void) {
	int cnt, i, j, k;
	BYTE block;
	BOOL flag = FALSE;

	// 게임판 전체를 검색해 세로 방향으로 3개 이상인 것 찾아서 표시
	for(i = 0; i < BOARD_WIDTH; i++) {
		cnt = 0;
		block = 0xFF;

		for(j = 0; j < BOARD_HEIGHT; j++) {
			// 첫 번째면 블록 종류 저장
			if((cnt == 0) && (g_hexaInfo.board[j][i] != EMPTY_BLOCK)) {
				block = g_hexaInfo.board[j][i];
				cnt++;
			} else {
				// 종류가 일치하면 일치한 블록 수 증가
				if(g_hexaInfo.board[j][i] == block) {
					cnt++;
					// 연속된 블록이 3개면 모아둔 이전 블록 3개를 지울 블록으로 표시
					if(cnt == BLOCK_CNT) {
						for(k = 0; k < cnt; k++)
							g_hexaInfo.erase[j - k][i] = ERASE_BLOCK;
						flag = TRUE;
					} else if(cnt > BLOCK_CNT) {
						// 연속된 블록이 4개 이상이면 즉시 지울 블록으로 표시
						g_hexaInfo.erase[j][i] = ERASE_BLOCK;
					}
				} else {
					// 일치하지 않으면 새로운 블록으로 설정
					if(g_hexaInfo.board[j][i] != EMPTY_BLOCK) {
						cnt = 1;
						block = g_hexaInfo.board[j][i];
					} else {
						cnt = 0;
						block = 0xFF;
					}
				}
			}
		}
	}

	return flag;
}

// 대각선 방향으로 일치하는 블록 찾아서 표시
BOOL diagonalBlock(void) {
	int cnt, i, j, k, l;
	BYTE block;
	BOOL flag = FALSE;

	// 게임판 전체를 검색해 세로 방향으로 3개 이상인 것 찾아서 표시
	for(i = 0; i < BOARD_WIDTH; i++)
		for(j = 0; j < BOARD_HEIGHT; j++) {
			cnt = 0;
			block = 0xFF;

			for(k = 0; ((i + k) < BOARD_WIDTH) && ((j + k) < BOARD_HEIGHT); k++) {
				// 첫 번째면 블록 종류 저장
				if((cnt == 0) && (g_hexaInfo.board[j + k][i + k] != EMPTY_BLOCK)) {
					block = g_hexaInfo.board[j + k][i + k];
					cnt++;
				} else {
					// 종류가 일치하면 일치한 블록 수 증가
					if(g_hexaInfo.board[j + k][i + k] == block) {
						cnt++;
						// 연속된 블록이 3개면 모아둔 이전 블록 3개를 지울 블록으로 표시
						if(cnt == BLOCK_CNT) {
							for(l = 0; l < cnt; l++)
								g_hexaInfo.erase[j + k - l][i + k - l] = ERASE_BLOCK;
							flag = TRUE;
						} else if(cnt > BLOCK_CNT) {
							// 연속된 블록이 4개 이상이면 즉시 지울 블록으로 표시
							g_hexaInfo.erase[j + k][i + k] = ERASE_BLOCK;
						}
					} else {
						// 일치하지 않으면 새로운 블록으로 설정
						if(g_hexaInfo.board[j + k][i + k] != EMPTY_BLOCK) {
							cnt = 1;
							block = g_hexaInfo.board[j + k][i + k];
						} else {
							cnt = 0;
							block = 0xFF;
						}
					}
				}
			}
		}

	// 게임판 전체를 검색해 아래에서 위 대각선 방향으로 3개 이상인 것 표시
	for(i = 0; i < BOARD_WIDTH; i++)
		for(j = 0; j < BOARD_HEIGHT; j++) {
			cnt = 0;
			block = 0xFF;

			for(k = 0; ((i + k) < BOARD_WIDTH) && ((j - k) >= 0); k++) {
				// 첫 번째면 블록 종류 지정
				if((cnt == 0) && (g_hexaInfo.board[j - k][i + k] != EMPTY_BLOCK)) {
					block = g_hexaInfo.board[j - k][i + k];
					cnt++;
				} else {
					// 종류가 일치하면 일치한 블록 수 증가
					if(g_hexaInfo.board[j - k][i + k] == block) {
						cnt++;
						// 연속된 블록이 3개면 모아둔 이전 블록 3개를 지울 블록으로 표시
						if(cnt == BLOCK_CNT) {
							for(l = 0; l < cnt; l++)
								g_hexaInfo.erase[j - k + l][i + k - l] = ERASE_BLOCK;
							flag = TRUE;
						} else if(cnt > BLOCK_CNT) {
							// 연속된 블록이 4개 이상이면 즉시 지울 블록으로 표시
							g_hexaInfo.erase[j - k][i + k] = ERASE_BLOCK;
						}
					} else {
						// 일치하지 않으면 새로운 블록으로 설정
						if(g_hexaInfo.board[j - k][i + k] != EMPTY_BLOCK) {
							cnt = 1;
							block = g_hexaInfo.board[j - k][i + k];
						} else {
							cnt = 0;
							block = 0xFF;
						}
					}
				}
			}
		}

	return flag;
}

// 제거할 블록으로 표시된 블록 게임판에서 제거 후 점수 증가
void eraseBlock(void) {
	int i, j;
	
	for(j = 0; j < BOARD_HEIGHT; j++)
		for(i = 0; i < BOARD_WIDTH; i++)
			if(g_hexaInfo.erase[j][i] == ERASE_BLOCK) {
				// 게임판에 블록을 빈 것으로 설정
				g_hexaInfo.board[j][i] = EMPTY_BLOCK;
				// 점수 증가
				g_hexaInfo.score++;
			}
}

// 빈 영역의 블록을 아래 채움
void fillBlock(void) {
	int i, j, empty;

	// 게임판 모든 영역을 돌아 빈 영역에 블록 채움
	for(i = 0; i < BOARD_WIDTH; i++) {
		empty = -1;

		// 아래에서 위로 올라가며 빈 영역 찾아 블록 채움
		for(j = BOARD_HEIGHT - 1; j >= 0; j--) {
			// 빈 블록이면 현재 위치 저장 후 블록 옮길 시 사용
			if((empty == -1) && (g_hexaInfo.board[j][i] == EMPTY_BLOCK)) empty = j;
			else if((empty != -1) && (g_hexaInfo.board[j][i] != EMPTY_BLOCK)) {
				// 중간에 빈 블록이 검출되고 현재 위치에 블록이 있으면 아래 채움
				g_hexaInfo.board[empty][i] = g_hexaInfo.board[j][i];

				// 빈 블록의 Y좌표를 한 칸 올려서 쌓아 올리도록 함
				empty--;

				// 현재 위치 블록은 옮겨졌으니 빈 블록으로 변경
				g_hexaInfo.board[j][i] = EMPTY_BLOCK;
			}
		}
	}
}

// 더 이상 제거할 블록이 없을 때까지 블록 제거 반복
void delBlock(QWORD winID) {
	BOOL flag;

	// 삭제할 블록 필드 초기화
	memset(g_hexaInfo.erase, 0, sizeof(g_hexaInfo.erase));

	while(1) {
		// 블록 제거하기 전에 잠시 대기해 게임판 상태와 블록 상태 유지
		sleep(300);

		flag = FALSE;

		// 가로 방향 삭제 블록 표시
		flag |= horizonBlock();
		// 세로 방향 삭제 블록 표시
		flag |= verticalBlock();
		// 대각선 방향 삭제 블록 표시
		flag |= diagonalBlock();

		// 제거할 블록이 없으면 수행 종료
		if(flag == FALSE) break;

		// 표시된 블록 제거
		eraseBlock();

		// 빈 영역 블록 채움
		fillBlock();

		// 게임 레벨을 30점 단위로 증가
		g_hexaInfo.level = (g_hexaInfo.score / 30) + 1;

		// 지운 블록이 있으면 게임 정보 영역 및 게임 영역 재구성
		drawGameArea(winID);
		drawInfo(winID);

		// 변경된 내용 화면에 출력
		showWin(winID, TRUE);
	}
}

// 게임 정보 출력
void drawInfo(QWORD winID) {
	char buf[200];
	int len;

	// 게임 정보 영역 표시
	drawRect(winID, 1, WINDOW_TITLE_HEIGHT - 1, HEXA_WIDTH - 2, WINDOW_TITLE_HEIGHT + INFO_HEIGHT, RGB(159, 48, 215), TRUE);

	// 임시 버퍼에 출력할 정보 저장
	sprintf(buf, "Level: %d, Score: %d\n", g_hexaInfo.level, g_hexaInfo.score);
	len = strlen(buf);

	// 저장된 정보를 게임 정보 표시 영역 가운데 출력
	drawText(winID, (HEXA_WIDTH - (len * FONT_ENG_WIDTH)) / 2, WINDOW_TITLE_HEIGHT + 2, RGB(255, 255, 255), RGB(159, 48, 215), buf, len);
}

// 게임 영역 출력
void drawGameArea(QWORD winID) {
	COLOR color;
	int i, j, y;

	// 게임 영역이 시작되는 위치
	y = WINDOW_TITLE_HEIGHT + INFO_HEIGHT;

	// 게임 영역 배경 출력
	drawRect(winID, 0, y, HEXA_WIDTH, y + (BLOCK_SIZE * BOARD_HEIGHT), RGB(0, 0, 0), TRUE);

	// 게임판 내용 표시
	for(j = 0; j < BOARD_HEIGHT; j++)
		for(i = 0; i < BOARD_WIDTH; i++) {
			// 빈 블록 아니면 블록 표시
			if(g_hexaInfo.board[j][i] != EMPTY_BLOCK) {
				// 블록 내부 그림
				color = g_hexaInfo.color[g_hexaInfo.board[j][i]];
				drawRect(winID, i * BLOCK_SIZE, y + (j * BLOCK_SIZE), (i + 1) * BLOCK_SIZE, y + ((j + 1) * BLOCK_SIZE), color, TRUE);

				// 블록 외부 테두리 그림
				color = g_hexaInfo.edge[g_hexaInfo.board[j][i]];
				drawRect(winID, i * BLOCK_SIZE, y + (j * BLOCK_SIZE), (i + 1) * BLOCK_SIZE, y + ((j + 1) * BLOCK_SIZE), color, FALSE);
				drawRect(winID, (i * BLOCK_SIZE) + 1, y + (j * BLOCK_SIZE) + 1, ((i + 1) * BLOCK_SIZE) - 1, y + ((j + 1) * BLOCK_SIZE) - 1, color, FALSE);
			}
		}

	// 현재 움직이는 블록 화면에 표시
	if(g_hexaInfo.x != -1)
		for(i = 0; i < BLOCK_CNT; i++) {
			// 제목 표시줄 아래에 블록 표시될 때 표시
			if(WINDOW_TITLE_HEIGHT < (y + ((g_hexaInfo.y + i) * BLOCK_SIZE))) {
				// 블록 내부 그림
				color = g_hexaInfo.color[g_hexaInfo.block[i]];
				drawRect(winID, g_hexaInfo.x * BLOCK_SIZE, y + ((g_hexaInfo.y + i) * BLOCK_SIZE), (g_hexaInfo.x + 1) * BLOCK_SIZE, y + ((g_hexaInfo.y + i + 1) * BLOCK_SIZE), color, TRUE);

				// 블록 외부 테두리 그림
				color = g_hexaInfo.edge[g_hexaInfo.block[i]];
				drawRect(winID, g_hexaInfo.x * BLOCK_SIZE, y + ((g_hexaInfo.y + i) * BLOCK_SIZE), (g_hexaInfo.x + 1) * BLOCK_SIZE, y + ((g_hexaInfo.y + i + 1) * BLOCK_SIZE), color, FALSE);
				drawRect(winID, (g_hexaInfo.x * BLOCK_SIZE) + 1, y + ((g_hexaInfo.y + i) * BLOCK_SIZE) + 1, ((g_hexaInfo.x + 1) * BLOCK_SIZE) - 1, y + ((g_hexaInfo.y + i + 1) * BLOCK_SIZE) - 1, color, FALSE);
			}
		}

	// 게임 영역 테두리 그림
	drawRect(winID, 0, y, HEXA_WIDTH - 1, y + (BLOCK_SIZE * BOARD_HEIGHT) - 1, RGB(112, 1, 223), FALSE);
}
