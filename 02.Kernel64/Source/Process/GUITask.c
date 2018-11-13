/*
 * GUITask.c
 *
 *  Created on: 2017. 9. 5.
 *      Author: Yummy
 */

#include <GUITask.h>
#include <Win.h>
#include <Font.h>
#include <MP.h>
#include <Console.h>
#include <CLITask.h>
#include <Shell.h>
#include <FileSystem.h>
#include <JPEG.h>
#include <DynMem.h>

// 기본 GUI 태스크
void kBaseGUITask(void) {
	QWORD id;
	int xMouse, yMouse, width, height;
	EVENT recvEvent;
	MOUSEEVENT *mouseEvent;
	KEYEVENT *keyEvent;
	WINDOWEVENT *winEvent;

	// 그래픽 모드 판단. 그래픽 모드로 시작했는지 여부 확인
	if(kIsGUIMode() == FALSE) {
		kPrintf("It is GUI Task. You must execute GUI Mode.\n");
		return;
	}

	// 윈도우 생성. 마우스 현재 위치 반환
	kGetWinCursor(&xMouse, &yMouse);

	// 윈도우 크기와 제목 설정
	width = 500;
	height = 200;

	// 윈도우 생성 함수 호출, 마우스가 있던 위치를 기준으로 생성
	id = kCreateWin(xMouse - 10, yMouse - WINDOW_TITLE_HEIGHT / 2, width, height, WINDOW_FLAGS_DEFAULT | WINDOW_FLAGS_RESIZABLE, "YummyHitOS Window");
	// 윈도우 생성 못하면 실패
	if(id == WINDOW_INVALID_ID) return;

	// GUI 태스크 이벤트 처리 루프
	while(1) {
		// 이벤트 큐에서 이벤트 수신
		if(kWinToEvent(id, &recvEvent) == FALSE) {
			kSleep(0);
			continue;
		}

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
				break;
				// 키 이벤트 처리
			case EVENT_KEY_DOWN:
			case EVENT_KEY_UP:
				// 여기에 키보드 이벤트 처리 코드 구현
				keyEvent = &(recvEvent.keyEvent);
				break;
				// 윈도우 이벤트 처리
			case EVENT_WINDOW_SELECT:
			case EVENT_WINDOW_DESELECT:
			case EVENT_WINDOW_MOVE:
			case EVENT_WINDOW_RESIZE:
			case EVENT_WINDOW_CLOSE:
				// 여기에 윈도우 이벤트 처리 코드 구현
				winEvent = &(recvEvent.winEvent);

				// 윈도우 닫기 이벤트면 윈도우 삭제 후 태스크 종료
				if(recvEvent.type == EVENT_WINDOW_CLOSE) {
					kDelWin(id);
					return;
				}
				break;
			default:
				break;
		}
	}
}

// YummyHitOS GUI 태스크
void kFirstGUITask(void) {
	QWORD id, winID;
	int xMouse, yMouse, width, height, y, i;
	EVENT recvEvent, sendEvent;
	MOUSEEVENT *mouseEvent;
	KEYEVENT *keyEvent;
	WINDOWEVENT *winEvent;
	char buf[50];
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
	if(kIsGUIMode() == FALSE) {
		kPrintf("It is GUI Task. You must execute GUI Mode.\n");
		return;
	}

	// 윈도우 생성. 마우스 현재 위치 반환
	kGetWinCursor(&xMouse, &yMouse);

	// 윈도우 크기와 제목 설정
	width = 500;
	height = 200;

	// 윈도우 생성 함수 호출. 마우스가 있던 위치를 기준으로 생성하고 번호를 추가해 윈도우마다 개별 이름 할당
	kSprintf(buf, "YummyHitOS Window %d", winCnt++);
	id = kCreateWin(xMouse - 10, yMouse - WINDOW_TITLE_HEIGHT / 2, width, height, WINDOW_FLAGS_DEFAULT | WINDOW_FLAGS_RESIZABLE, buf);
	// 윈도우 생성 못하면 실패
	if(id == WINDOW_INVALID_ID) return;

	// 윈도우 매니저가 윈도우로 전송하는 이벤트 표시 영역 그림. 이벤트 정보 출력할 위치 저장
	y = WINDOW_TITLE_HEIGHT + 10;

	// 이벤트 정보 표시 영역 테두리와 윈도우 ID 표시
	kDrawRect(id, 10, y + 8, width - 10, y + 70, RGB(0, 0, 0), FALSE);
	kSprintf(buf, "GUI Event Information[Window ID: 0x%Q]", id);
	kDrawText(id, 20, y, RGB(186, 140, 255), RGB(255, 255, 255), buf, kStrLen(buf));

	// 화면 아래 이벤트 전송 버튼 그림. 버튼 영역 설정
	kSetRectData(10, y + 80, width - 10, height - 10, &area);
	// 배경은 윈도우 배경색으로 설정, 문자는 보라색으로 설정해 버튼 그림
	kDrawBtn(id, &area, WINDOW_COLOR_BACKGROUND, "User Message Send Button(Unused)", RGB(102, 102, 255));
	// 윈도우를 화면에 표시
	kShowWin(id, TRUE);

	// GUI 태스크 이벤트 처리 루프
	while(1) {
		// 이벤트 큐에서 이벤트 수신
		if(kWinToEvent(id, &recvEvent) == FALSE) {
			kSleep(0);
			continue;
		}

		// 윈도우 이벤트 정보가 표시될 영역을 배경색으로 칠해 이전에 표시한 데이터 모두 지움
		kDrawRect(id, 11, y + 20, width - 11, y + 69, WINDOW_COLOR_BACKGROUND, TRUE);

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
				kSprintf(buf, "Mouse Event: %s", str[recvEvent.type]);
				kDrawText(id, 20, y + 20, RGB(119, 68, 255), WINDOW_COLOR_BACKGROUND, buf, kStrLen(buf));

				// 마우스 데이터 출력
				kSprintf(buf, "Data: X = %d, Y = %d, Button = %X", mouseEvent->point.x, mouseEvent->point.y, mouseEvent->btnStat);
				kDrawText(id, 20, y + 40, RGB(119, 68, 255), WINDOW_COLOR_BACKGROUND, buf, kStrLen(buf));

				// 마우스 눌림 또는 떨어짐 이벤트면 버튼 색깔 다시 그림. 마우스 왼쪽 버튼이 눌렸을 때 처리
				if(recvEvent.type == EVENT_MOUSE_LCLICK_ON) {
					// 버튼 영역에 마우스 왼쪽 버튼이 눌렸는지 판단
					if(kIsInRect(&area, mouseEvent->point.x, mouseEvent->point.y) == TRUE) {
						// 버튼 배경을 노란색으로 변경해 표시
						kDrawBtn(id, &area, RGB(255, 255, 135), "User Message Send Button(Using)", RGB(102, 102, 255));
						kUpdateMonID(id);

						// 다른 윈도우로 유저 이벤트 전송. 생성된 모든 윈도우 찾아 이벤트 전송
						sendEvent.type = EVENT_USER_TESTMSG;
						sendEvent.data[0] = id;
						sendEvent.data[1] = 0x4444;
						sendEvent.data[2] = 0x7777;

						// 생성된 윈도우 수만큼 루프 수행하며 이벤트 전송
						for(i = 0; i < winCnt; i++) {
							// 윈도우 제목으로 윈도우 검색
							kSprintf(buf, "YummyHitOS Window %d", i);
							winID = kFindWinTitle(buf);
							// 윈도우가 존재하며 윈도우 자신이 아닌 경우 이벤트 전송
							if((winID != WINDOW_INVALID_ID) && (winID != id)) kEventToWin(winID, &sendEvent);
						}
					}
				} else kDrawBtn(id, &area, WINDOW_COLOR_BACKGROUND, "User Message Send Button(Unused)", RGB(102, 102, 255));
				break;
				// 키 이벤트 처리
			case EVENT_KEY_DOWN:
			case EVENT_KEY_UP:
				// 여기에 키보드 이벤트 처리 코드 구현
				keyEvent = &(recvEvent.keyEvent);

				// 키 이벤트 타입 출력
				kSprintf(buf, "Key Event: %s", str[recvEvent.type]);
				kDrawText(id, 20, y + 20, RGB(119, 68, 255), WINDOW_COLOR_BACKGROUND, buf, kStrLen(buf));

				// 키 데이터 출력
				kSprintf(buf, "Data: Key = %c, Flag = %X", keyEvent->ascii, keyEvent->flag);
				kDrawText(id, 20, y + 40, RGB(119, 68, 255), WINDOW_COLOR_BACKGROUND, buf, kStrLen(buf));
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
				kSprintf(buf, "Window Event: %s", str[recvEvent.type]);
				kDrawText(id, 20, y + 20, RGB(119, 68, 255), WINDOW_COLOR_BACKGROUND, buf, kStrLen(buf));

				// 윈도우 데이터 출력
				kSprintf(buf, "Data: X1 = %d, Y1 = %d, X2 = %d, Y2 = %d", winEvent->area.x1, winEvent->area.y1, winEvent->area.x2, winEvent->area.y2);
				kDrawText(id, 20, y + 40, RGB(119, 68, 255), WINDOW_COLOR_BACKGROUND, buf, kStrLen(buf));

				// 윈도우 닫기 이벤트면 윈도우 삭제 후 태스크 종료
				if(recvEvent.type == EVENT_WINDOW_CLOSE) {
					kDelWin(id);
					return;
				}
				break;
			default:
				// 여기에 알 수 없는 이벤트 처리 코드 넣기
				kSprintf(buf, "Unknown Event: 0x%X", recvEvent.type);
				kDrawText(id, 20, y + 20, RGB(119, 68, 255), WINDOW_COLOR_BACKGROUND, buf, kStrLen(buf));

				// 윈도우 데이터 출력
				kSprintf(buf, "Data0 = 0x%Q, Data1 = 0x%Q, Data2 = 0x%Q", recvEvent.data[0], recvEvent.data[1], recvEvent.data[2]);
				kDrawText(id, 20, y + 40, RGB(119, 68, 255), WINDOW_COLOR_BACKGROUND, buf, kStrLen(buf));
				break;
		}

		// 윈도우 화면 업데이트
		kShowWin(id, TRUE);
	}
}

void kExitGUITask(void) {
	// when you use older qemu version, your program can work this loop
//	for (const char *s = "Shutdown"; *s; ++s) outByte(0x8900, *s);

	// Source/Driver/Keyboard.c to shutDown(void) -> outByte(0xF4, 0x00);
	kShutDown();

	// Source/Handler/AsmUtil.asm to _shutdown: -> same to reboot. reBoot() func is consist of c lang, _shutdown callee is consist of asm.
	//_shutdown();
}

// 시스템 모니터 태스크
// 시스템 상태 감시하여 화면에 표시
void kSysMonTask(void) {
	QWORD id, lastTickCnt, lastDynMemSize, dynMemSize, tmp;
	int i, winWidth, procCnt;
	DWORD lastCPULoad[MAXPROCESSORCNT];
	int lastTaskCnt[MAXPROCESSORCNT];
	EVENT recvEvent;
	WINDOWEVENT *winEvent;
	BOOL changed;
	RECT monArea;

	// 그래픽 모드 판단
	if(kIsGUIMode() == FALSE) {
		kPrintf("It is GUI Task. You must execute GUI Mode.\n");
		return;
	}

	// 윈도우 생성
	// 화면 영역 크기 반환
	kGetMonArea(&monArea);

	// 현재 프로세서 개수로 윈도우 너비 계산
	procCnt = kGetProcCnt();
	winWidth = procCnt * (SYSMON_PROCESSOR_WIDTH + SYSMON_PROCESSOR_MARGIN) + SYSMON_PROCESSOR_MARGIN;

	// 윈도우를 화면 가운데 생성 후 화면에 표시 안함. 프로세서 정보와 메모리 정보 표시 영역 그린 후 화면에 표시
	id = kCreateWin((monArea.x2 - winWidth) / 2, (monArea.y2 - SYSMON_WINDOW_HEIGHT) / 2, winWidth, SYSMON_WINDOW_HEIGHT, WINDOW_FLAGS_DEFAULT & ~WINDOW_FLAGS_SHOW | WINDOW_FLAGS_RESIZABLE, "System Monitoring.. +_____+");

	// 윈도우 생성 못하면 실패
	if(id == WINDOW_INVALID_ID) return;
	
	// 프로세서 정보 표시하는 영역 3픽셀 두께로 표시하고 문자열 출력
	kDrawLine(id, 5, WINDOW_TITLE_HEIGHT + 15, winWidth - 5, WINDOW_TITLE_HEIGHT + 15, RGB(0, 0, 0));
	kDrawLine(id, 5, WINDOW_TITLE_HEIGHT + 16, winWidth - 5, WINDOW_TITLE_HEIGHT + 16, RGB(0, 0, 0));
	kDrawLine(id, 5, WINDOW_TITLE_HEIGHT + 17, winWidth - 5, WINDOW_TITLE_HEIGHT + 17, RGB(0, 0, 0));
	kDrawText(id, 9, WINDOW_TITLE_HEIGHT + 8, RGB(119, 68, 255), WINDOW_COLOR_BACKGROUND, "Processor Info", 21);

	// 메모리 정보 표시하는 영역 3픽셀 두께로 표시하고 문자열 출력
	kDrawLine(id, 5, WINDOW_TITLE_HEIGHT + SYSMON_PROCESSOR_HEIGHT + 50, winWidth - 5, WINDOW_TITLE_HEIGHT + SYSMON_PROCESSOR_HEIGHT + 50, RGB(0, 0, 0));
	kDrawLine(id, 5, WINDOW_TITLE_HEIGHT + SYSMON_PROCESSOR_HEIGHT + 51, winWidth - 5, WINDOW_TITLE_HEIGHT + SYSMON_PROCESSOR_HEIGHT + 51, RGB(0, 0, 0));
	kDrawLine(id, 5, WINDOW_TITLE_HEIGHT + SYSMON_PROCESSOR_HEIGHT + 52, winWidth - 5, WINDOW_TITLE_HEIGHT + SYSMON_PROCESSOR_HEIGHT + 52, RGB(0, 0, 0));
	kDrawText(id, 9, WINDOW_TITLE_HEIGHT + SYSMON_PROCESSOR_HEIGHT + 43, RGB(119, 68, 255), WINDOW_COLOR_BACKGROUND, "Memory Info", 18);

	// 윈도우 화면에 표시
	kShowWin(id, TRUE);

	// 루프 돌며 시스템 정보 감시 결과 화면에 표시
	lastTickCnt = 0;

	// 마지막으로 측정한 프로세서 부하와 태스크 수와 메모리 사용량 모두 0으로 설정
	kMemSet(lastCPULoad, 0, sizeof(lastCPULoad));
	kMemSet(lastTaskCnt, 0, sizeof(lastTaskCnt));
	lastDynMemSize = 0;

	// GUI 태스크 이벤트 처리 루프
	while(1) {
		// 이벤트 큐에서 이벤트 수신
		if(kWinToEvent(id, &recvEvent) == TRUE) {
			// 수신된 이벤트 타입에 따라 처리
			switch(recvEvent.type) {
				case EVENT_WINDOW_RESIZE:
				case EVENT_WINDOW_CLOSE:
					// 여기에 윈도우 이벤트 처리 코드 구현
					winEvent = &(recvEvent.winEvent);

					// 윈도우 닫기 이벤트면 윈도우 삭제 후 태스크 종료
					if(recvEvent.type == EVENT_WINDOW_CLOSE) {
						kDelWin(id);
						return;
					}

					break;

					// 그 외..
				default:
					break;
			}
		}

		// 0.5초마다 한 번씩 시스템 상태 확인
		if((kGetTickCnt() - lastTickCnt) < 500) {
			kSleep(1);
			continue;
		}

		// 마지막 측정 시간을 최신 업데이트
		lastTickCnt = kGetTickCnt();

		// 프로세서 정보 출력
		// 프로세서 수 만큼 부하와 태스크 수 확인 후 달라진 점 화면에 업데이트
		for(i = 0; i < procCnt; i++) {
			changed = FALSE;

			// 프로세서 부하 검사
			if(lastCPULoad[i] != kGetProcLoad(i)) {
				lastCPULoad[i] = kGetProcLoad(i);
				changed = TRUE;
			}

			// 태스크 수 검사
			if(lastTaskCnt[i] != kGetTaskCnt(i)) {
				lastTaskCnt[i] = kGetTaskCnt(i);
				changed = TRUE;
			}

			// 이전과 비교해 변경 사항 화면에 업데이트
			if(changed == TRUE) {
				// 화면에 현재 프로세서 부하 표시
				kDrawProcInfo(id, i * SYSMON_PROCESSOR_WIDTH + (i + 1) * SYSMON_PROCESSOR_MARGIN, WINDOW_TITLE_HEIGHT + 28, i);
			}
		}

		// 동적 메모리 정보 출력
		kGetDynMemInfo(&tmp, &tmp, &tmp, &dynMemSize);

		// 현재 동적 할당 메모리 사용량이 이전과 다르면 메모리 정보 출력
		if(dynMemSize != lastDynMemSize) {
			lastDynMemSize = dynMemSize;

			// 메모리 정보 출력
			kDrawMemInfo(id, WINDOW_TITLE_HEIGHT + SYSMON_PROCESSOR_HEIGHT + 60, winWidth);
		}
	}
}

// 개별 프로세서 정보 화면에 표시
static void kDrawProcInfo(QWORD winID, int x, int y, BYTE id) {
	char buf[100];
	RECT area;
	QWORD procLoad, usageBar;
	int midX;

	// 프로세서 ID 표시
	kSprintf(buf, "Processor ID: %d", id);
	kDrawText(winID, x + 10, y, RGB(119, 68, 255), WINDOW_COLOR_BACKGROUND, buf, kStrLen(buf));

	// 프로세서 태스크 개수 표시
	kSprintf(buf, "Task Count: %d", kGetTaskCnt(id));
	kDrawText(winID, x + 10, y + 18, RGB(119, 68, 255), WINDOW_COLOR_BACKGROUND, buf, kStrLen(buf));

	// 프로세서 부하 나타내는 막대 표시
	// 프로세서 부하 표시
	procLoad = kGetProcLoad(id);
	if(procLoad > 100) procLoad = 100;

	// 부하 표시하는 막대 전체 테두리 표시
	kDrawRect(winID, x, y + 36, x + SYSMON_PROCESSOR_WIDTH, y + SYSMON_PROCESSOR_HEIGHT, RGB(0, 0, 0), FALSE);

	// 프로세서 사용량 나타내는 막대 길이(막대 전체 길이 * 프로세서 부하 / 100)
	usageBar = (SYSMON_PROCESSOR_HEIGHT - 40) * procLoad / 100;

	// 부하 표시하는 영역 막대 내부 표시
	// 채워진 막대 표시, 테두리 2픽셀 정도 여유 공간 남김
	kDrawRect(winID, x + 2, y + (SYSMON_PROCESSOR_HEIGHT - usageBar) - 2, x + SYSMON_PROCESSOR_WIDTH - 2, y + SYSMON_PROCESSOR_HEIGHT - 2, SYSMON_BAR_COLOR, TRUE);
	// 빈 막대 표시. 테두리 2픽셀 정도 여유 공간 남김
	kDrawRect(winID, x + 2, y + 38, x + SYSMON_PROCESSOR_WIDTH - 2, y + (SYSMON_PROCESSOR_HEIGHT - usageBar) - 1, WINDOW_COLOR_BACKGROUND, TRUE);

	// 프로세서 부하 표시. 막대 가운데 부하 표시
	kSprintf(buf, "Usage: %d%%", procLoad);
	midX = (SYSMON_PROCESSOR_WIDTH - (kStrLen(buf) * FONT_ENG_WIDTH)) / 2;
	kDrawText(winID, x + midX, y + 80, RGB(119, 68, 255), WINDOW_COLOR_BACKGROUND, buf, kStrLen(buf));

	// 프로세서 정보가 표시된 영역만 화면 업데이트
	kSetRectData(x, y, x + SYSMON_PROCESSOR_WIDTH, y + SYSMON_PROCESSOR_HEIGHT, &area);
	kUpdateMonWinArea(winID, &area);
}

// 메모리 정보 출력
static void kDrawMemInfo(QWORD winID, int y, int winWidth) {
	char buf[100];
	QWORD totalMemKBSize, dynMemStartAddr, dynMemSize, nowPercent, tmp;
	int usageBar, midX;
	RECT area;

	// Mbyte -> KByte 단위 변환
	totalMemKBSize = kGetTotalMemSize() * 1024;

	// 메모리 정보 표시
	kSprintf(buf, "Total Size: %d KB		", totalMemKBSize);
	kDrawText(winID, SYSMON_PROCESSOR_MARGIN + 10, y + 3, RGB(119, 68, 255), WINDOW_COLOR_BACKGROUND, buf, kStrLen(buf));

	// 동적 메모리 정보 반환
	kGetDynMemInfo(&dynMemStartAddr, &tmp, &tmp, &dynMemSize);
	kSprintf(buf, "Used Size: %d KB		", (dynMemSize + dynMemStartAddr) / 1024);
	kDrawText(winID, SYSMON_PROCESSOR_MARGIN + 10, y + 21, RGB(119, 68, 255), WINDOW_COLOR_BACKGROUND, buf, kStrLen(buf));

	// 메모리 사용량 나타내는 막대 표시
	// 메모리 사용량 표시 막대 전체에 테두리 표시
	kDrawRect(winID, SYSMON_PROCESSOR_MARGIN, y + 40, winWidth - SYSMON_PROCESSOR_MARGIN, y + SYSMON_MEMORY_HEIGHT - 32, RGB(0, 0, 0), FALSE);
	// 메모리 사용량 계산
	nowPercent = (dynMemStartAddr + dynMemSize) * 100 / 1024 / totalMemKBSize;
	if(nowPercent > 100) nowPercent = 100;

	// 메모리 사용량 나타내는 막대 길이(막대 전체 길이 * 메모리 사용량 / 100)
	usageBar = (winWidth - 2 * SYSMON_PROCESSOR_MARGIN) * nowPercent / 100;

	// 메모리 사용량 표시하는 영역 막대 내부 표시
	// 색칠된 막대 표시. 테두리 2픽셀 정도 여유 공간 남김
	kDrawRect(winID, SYSMON_PROCESSOR_MARGIN + 2, y + 42, SYSMON_PROCESSOR_MARGIN + 2 + usageBar, y + SYSMON_MEMORY_HEIGHT - 34, SYSMON_BAR_COLOR, TRUE);
	// 빈 막대 표시. 테두리 2픽셀 정도 여유 공간 남김
	kDrawRect(winID, SYSMON_PROCESSOR_MARGIN + 2 + usageBar, y + 42, winWidth - SYSMON_PROCESSOR_MARGIN - 2, y + SYSMON_MEMORY_HEIGHT - 34, WINDOW_COLOR_BACKGROUND, TRUE);

	// 사용량 나타내는 텍스트 표시. 막대 가운데 사용량 표시
	kSprintf(buf, "Usage: %d%%", nowPercent);
	midX = (winWidth - (kStrLen(buf) * FONT_ENG_WIDTH)) / 2;
	kDrawText(winID, midX, y + 45, RGB(119, 68, 255), WINDOW_COLOR_BACKGROUND, buf, kStrLen(buf));

	// 메모리 정보가 표시된 영역만 화면 업데이트
	kSetRectData(0, y, winWidth, y + SYSMON_MEMORY_HEIGHT, &area);
	kUpdateMonWinArea(winID, &area);
}

// 이전 화면 버퍼 값 보관 영역
static CHARACTER gs_preMonBuf[CONSOLE_WIDTH * CONSOLE_HEIGHT];

// GUI 버전 콘솔 셸 태스크
void kGUIShell(void) {
	static QWORD winID = WINDOW_INVALID_ID;
	int winWidth, winHeight;
	EVENT recvEvent;
	KEYEVENT *keyEvent;
	RECT monArea;
	KEYDATA keyData;
	TCB *shellTask;
	QWORD taskID;

	// 그래픽 모드 판단
	if(kIsGUIMode() == FALSE) {
		kPrintf("It is GUI Task. You must execute GUI Mode.\n");
		return;
	}

	// GUI 콘솔 셸 윈도우가 존재하면 생성된 윈도우를 최상위로 만들고 태스크 종료
	if(winID != WINDOW_INVALID_ID) {
		kMoveWinTop(winID);
		return;
	}

	// 윈도우 생성
	// 전체 화면 영역 크기 반환
	kGetMonArea(&monArea);

	// 윈도우 크기 설정, 화면 버퍼에 들어가는 문자 최대 너비와 높이를 고려해 계산
	winWidth = CONSOLE_WIDTH * FONT_ENG_WIDTH + 4;
	winHeight = CONSOLE_HEIGHT * FONT_ENG_HEIGHT + WINDOW_TITLE_HEIGHT + 2;

	// 윈도우 생성 함수 호출, 화면 가운데 생성
	winID = kCreateWin((monArea.x2 - winWidth) / 2, (monArea.y2 - winHeight) / 2, winWidth, winHeight, WINDOW_FLAGS_DEFAULT, "YummyHit Terminal");
	// 생성 못하면 실패
	if(winID == WINDOW_INVALID_ID) return;

	// 셸 커맨드 처리하는 콘솔 셸 태스크 생성
	kSetShellExitFlag(FALSE);
	shellTask = kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kStartShell, TASK_LOADBALANCING_ID);
	if(shellTask == NULL) {
		// 콘솔 셸 태스크 생성에 실패하면 윈도우 삭제 후 종료
		kDelWin(winID);
		return;
	}
	// 콘솔 셸 태스크 ID 저장
	taskID = shellTask->link.id;

	// 이전 화면 버퍼 초기화
	kMemSet(gs_preMonBuf, 0xFF, sizeof(gs_preMonBuf));

	// GUI 태스크 이벤트 처리 루프
	while(1) {
		// 화면 버퍼의 변경된 내용을 윈도우에 업데이트
		kProcConsoleBuf(winID);

		// 이벤트 큐에서 이벤트 수신
		if(kWinToEvent(winID, &recvEvent) == FALSE) {
			kSleep(0);
			continue;
		}

		// 수신된 이벤트를 타입에 따라 처리
		switch(recvEvent.type) {
			// 키 이벤트 처리
			case EVENT_KEY_DOWN:
			case EVENT_KEY_UP:
				// 키보드 이벤트 처리 코드
				keyEvent = &(recvEvent.keyEvent);
				keyData.ascii = keyEvent->ascii;
				keyData.flag = keyEvent->flag;
				keyData.scanCode = keyEvent->scanCode;

				// 키를 그래픽 모드용 키 큐로 삽입해 셸 태스크 입력으로 전달
				kAddGUIKeyQ(&keyData);
				break;
			// 윈도우 이벤트 처리
			case EVENT_WINDOW_CLOSE:
				// 생성한 셸 태스크가 종료되도록 종료 플래그 설정 후 종료될 때까지 대기
				kSetShellExitFlag(TRUE);
				while(kIsTaskExist(taskID) == TRUE) kSleep(1);

				// 윈도우 삭제 후 윈도우 ID 초기화
				kDelWin(winID);
				winID = WINDOW_INVALID_ID;
				return;

				break;
				// 그 외 정보
			default:
				// 알 수 없는 이벤트 처리 코드
				break;
		}
	}
}

// 화면 버퍼 변경된 내용 GUI 콘솔 셸 윈도우 화면 업데이트
static void kProcConsoleBuf(QWORD winID) {
	int i, j;
	CONSOLEMANAGER *manager;
	CHARACTER *monBuf, *ls_preMonBuf;
	RECT lineArea;
	BOOL changed, fullUpdate;
	static QWORD lastTickCnt = 0;

	// 콘솔 관리하는 자료구조 반환 받아 화면 버퍼 어드레스 저장 후 이전 화면 버퍼 어드레스 저장
	manager = kGetConsoleManager();
	monBuf = manager->monBuf;
	ls_preMonBuf = gs_preMonBuf;

	// 화면 전체 업데이트한 지 5초 지나면 무조건 화면 전체 다시 그림
	if(kGetTickCnt() - lastTickCnt > 5000) {
		lastTickCnt = kGetTickCnt();
		fullUpdate = TRUE;
	} else fullUpdate = FALSE;

	// 화면 버퍼 높이만큼 반복
	for(j = 0; j < CONSOLE_HEIGHT; j++) {
		// 처음엔 변경되지 않은 것으로 플래그 설정
		changed = FALSE;

		// 현재 라인에 변화가 있는지 비교 후 변경 여부 플래그 처리
		for(i = 0; i < CONSOLE_WIDTH; i++) {
			// 문자 비교 후 다르거나 전체를 새로 그려야 하면 이전 화면 버퍼에 업데이트 후 변경 여부 플래그 설정
			if((monBuf->character != ls_preMonBuf->character) || (fullUpdate == TRUE)) {
				// 문자 화면에 출력
				kDrawText(winID, i * FONT_ENG_WIDTH + 2, j * FONT_ENG_HEIGHT + WINDOW_TITLE_HEIGHT, RGB(0, 255, 0), RGB(0, 0, 0), &(monBuf->character), 1);

				// 이전 화면 버퍼로 값을 옮겨 놓고 현재 라인에 이전과 다른 데이터 표시 
				kMemCpy(ls_preMonBuf, monBuf, sizeof(CHARACTER));
				changed = TRUE;
			}

			monBuf++;
			ls_preMonBuf++;
		}

		// 현재 라인에서 변경된 데이터가 있다면 현재 라인만 화면 업데이트
		if(changed == TRUE) {
			// 현재 라인 영역 계산
			kSetRectData(2, j * FONT_ENG_HEIGHT + WINDOW_TITLE_HEIGHT, 5 + FONT_ENG_WIDTH * CONSOLE_WIDTH, (j + 1) * FONT_ENG_HEIGHT + WINDOW_TITLE_HEIGHT - 1, &lineArea);
			// 윈도우 일부만 화면 업데이트
			kUpdateMonWinArea(winID, &lineArea);
		}
	}
}

// 이미지 뷰어 태스크
void kImgViewTask(void) {
	QWORD winID;
	int xMouse, yMouse, winWidth, winHeight, editBoxWidth, fileNameLen;
	RECT editBoxArea, btnArea, monArea;
	EVENT recvEvent, sendEvent;
	char fileName[FILESYSTEM_FILENAME_MAXLEN + 1];
	MOUSEEVENT *mouseEvent;
	KEYEVENT *keyEvent;
	POINT monXY, xy;

	// 그래픽 모드로 시작했는지 확인
	if(kIsGUIMode() == FALSE) {
		kPrintf("It is GUI Task. You must execute GUI Mode.\n");
		return;
	}

	// 윈도우 생성. 전체 화면 영역 크기 반환
	kGetMonArea(&monArea);

	// 윈도우 크기 설정, 화면 버퍼에 들어가는 문자 최대 너비와 높이 고려
	winWidth = FONT_ENG_WIDTH * FILESYSTEM_FILENAME_MAXLEN + 165;
	winHeight = 35 + WINDOW_TITLE_HEIGHT + 5;

	// 윈도우 생성 함수 호출, 화면 가운데 생성
	winID = kCreateWin((monArea.x2 - winWidth) / 2, (monArea.y2 - winHeight) / 2, winWidth, winHeight, WINDOW_FLAGS_DEFAULT & ~WINDOW_FLAGS_SHOW, "JPEG Viewer");
	if(winID == WINDOW_INVALID_ID) return;		// 윈도우 생성 못하면 실패

	// 파일 이름 입력 에디트 박스 표시
	kDrawText(winID, 5, WINDOW_TITLE_HEIGHT + 6, RGB(119, 68, 255), WINDOW_COLOR_BACKGROUND, "File Name", 9);
	editBoxWidth = FONT_ENG_WIDTH * FILESYSTEM_FILENAME_MAXLEN + 4;
	kSetRectData(85, WINDOW_TITLE_HEIGHT + 5, 85 + editBoxWidth, WINDOW_TITLE_HEIGHT + 25, &editBoxArea);
	kDrawRect(winID, editBoxArea.x1, editBoxArea.y1, editBoxArea.x2, editBoxArea.y2, RGB(0, 0, 0), FALSE);

	// 파일 이름 버퍼 비운 후 에디트 박스에 빈 파일 이름 표시
	fileNameLen = 0;
	kMemSet(fileName, 0, sizeof(fileName));
	kDrawFileName(winID, &editBoxArea, fileName, fileNameLen);

	// 이미지 출력 버튼 영역 지정
	kSetRectData(editBoxArea.x2 + 10, editBoxArea.y1, editBoxArea.x2 + 70, editBoxArea.y2, &btnArea);
	kDrawBtn(winID, &btnArea, WINDOW_COLOR_BACKGROUND, "Show", RGB(102, 102, 255));

	// 윈도우 표시
	kShowWin(winID, TRUE);

	// GUI 태스크의 이벤트 처리 루프
	while(1) {
		// 이벤트 큐에서 이벤트 수신
		if(kWinToEvent(winID, &recvEvent) == FALSE) {
			kSleep(0);
			continue;
		}

		// 수신된 이벤트를 타입에 따라 나누어 처리
		switch(recvEvent.type) {
			// 마우스 이벤트 처리
			case EVENT_MOUSE_LCLICK_ON:
				mouseEvent = &(recvEvent.mouseEvent);

				// 마우스 왼쪽 버튼이 이미지 출력 버튼 위에서 눌러지면 저장된 파일 이름을 이용해 이미지 화면에 표시
				if(kIsInRect(&btnArea, mouseEvent->point.x, mouseEvent->point.y) == TRUE) {
					// 버튼을 눌린 것으로 표시
					kDrawBtn(winID, &btnArea, RGB(255, 255, 135), "Show", RGB(102, 102, 255));
					// 버튼이 있는 영역만 화면 업데이트
					kUpdateMonWinArea(winID, &btnArea);

					// 이미지 출력 윈도우 생성 후 이벤트 처리
					if(kCreateImgOnWinExe(winID, fileName) == FALSE) kSleep(200);	// 윈도우 생성 실패시 버튼이 눌러졌다가 떨어지는 효과를 위해 200ms 대기 

					// 버튼 떨어진 것으로 표시
					kDrawBtn(winID, &btnArea, WINDOW_COLOR_BACKGROUND, "Show", RGB(102, 102, 255));
					// 버튼이 있는 영역만 화면 업데이트
					kUpdateMonWinArea(winID, &btnArea);
				}
				break;

			// 키 이벤트 처리
			case EVENT_KEY_DOWN:
				keyEvent = &(recvEvent.keyEvent);

				// 백스페이스 키는 삽입된 문자 삭제
				if((keyEvent->ascii == KEY_BACKSPACE) && (fileNameLen > 0)) {
					// 버퍼에 삽입된 마지막 문자 삭제
					fileName[fileNameLen] = '\0';
					fileNameLen--;

					// 입력된 내용 에디트 박스에 표시
					kDrawFileName(winID, &editBoxArea, fileName, fileNameLen);
				} else if((keyEvent->ascii == KEY_ENTER) && (fileNameLen > 0)) {	// 엔터 키는 이미지 출력 버튼 눌린 것으로 처리
					// 버튼 XY 좌표를 화면 좌표로 변환해 마우스 이벤트 좌표로 사용
					xy.x = btnArea.x1 + 1;
					xy.y = btnArea.y1 + 1;
					kPointToMon(winID, &xy, &monXY);

					// 이미지 출력 버튼에 마우스 왼쪽 버튼이 눌린 것처럼 마우스 이벤트 전송
					kSetMouseEvent(winID, EVENT_MOUSE_LCLICK_ON, monXY.x + 1, monXY.y + 1, 0, &sendEvent);
					kEventToWin(winID, &sendEvent);
				} else if(keyEvent->ascii == KEY_ESC) {		// ESC 키는 윈도우 닫힘 버튼이 눌린 것으로 처리
					// 윈도우 닫기 이벤트 윈도우로 전송
					kSetWinEvent(winID, EVENT_WINDOW_CLOSE, &sendEvent);
					kEventToWin(winID, &sendEvent);
				} else if((keyEvent->ascii <= 128) && (keyEvent->ascii != KEY_BACKSPACE) && (fileNameLen < FILESYSTEM_FILENAME_MAXLEN)) {
					// 그 외 키는 파일 이름 버퍼에 공간 있을 때 버퍼에 삽입. 입력된 키 파일 이름 버퍼 마지막에 삽입
					fileName[fileNameLen] = keyEvent->ascii;
					fileNameLen++;

					// 입력된 내용 에디트 박스에 표시
					kDrawFileName(winID, &editBoxArea, fileName, fileNameLen);
				}
				break;

			// 윈도우 이벤트 처리
			case EVENT_WINDOW_CLOSE:
				if(recvEvent.type == EVENT_WINDOW_CLOSE) {
					// 윈도우 삭제
					kDelWin(winID);
					return;
				}
				break;

			// 그 외 정보
			default:
				// 알 수 없는 이벤트 처리 코드 삽입
				break;
		}
	}
}

// 에디트 박스 영역에 문자 출력
static void kDrawFileName(QWORD winID, RECT *area, char *fileName, int len) {
	// 에디트 박스 배경 모두 흰색
	kDrawRect(winID, area->x1 + 1, area->y1 + 1, area->x2 - 1, area->y2 - 1, WINDOW_COLOR_BACKGROUND, TRUE);

	// 파일 이름 출력
	kDrawText(winID, area->x1 + 2, area->y1 + 2, RGB(102, 102, 255), WINDOW_COLOR_BACKGROUND, fileName, len);

	// 파일 이름 길이가 파일 시스템 최대 길이가 아니면 커서 출력
	if(len < FILESYSTEM_FILENAME_MAXLEN) kDrawText(winID, area->x1 + 2 + FONT_ENG_WIDTH * len, area->y1 + 2, RGB(102, 102, 255), WINDOW_COLOR_BACKGROUND, "_", 1);

	// 에디트 박스 영역만 화면 업데이트
	kUpdateMonWinArea(winID, area);
}

// JPEG 파일 읽어 새로 생성한 윈도우에 표시 후 이벤트 처리
static BOOL kCreateImgOnWinExe(QWORD mainWinID, const char *fileName) {
	DIR *dir;
	struct dirent *entry;
	DWORD fileSize;
	RECT monArea;
	QWORD winID;
	WINDOW *win;
	BYTE *fileBuf;
	COLOR *outBuf;
	int winWidth;
	FILE *fp;
	JPEG *jpg;
	EVENT recvEvent;
	KEYEVENT *keyEvent;
	BOOL exit;

	// initialize
	fp = NULL;
	fileBuf = NULL;
	outBuf = NULL;
	winID = WINDOW_INVALID_ID;

	// 루트 디렉터리를 열어 파일 검색
	dir = dopen("/");
	fileSize = 0;

	// 디렉터리에서 파일 검색
	while(1) {
		// 디렉터리에서 엔트리 읽음
		entry = dread(dir);
		// 더 이상 파일이 없으면 break
		if(entry == NULL) break;

		// 파일 이름 길이와 내용이 같은 것 검색
		if((kStrLen(entry->d_name) == kStrLen(fileName)) && (kMemCmp(entry->d_name, fileName, kStrLen(fileName)) == 0)) {
			fileSize = entry->size;
			break;
		}
	}
	// 디렉터리 핸들 반환. 핸들이 반환되지 않으면 메모리가 해제되지 않고 남으므로 꼭 해제해야 함.
	dclose(dir);

	if(fileSize == 0) {
		kPrintf("[JPEG Viewer] %s file doesn't exist or size is zero\n", fileName);
		return FALSE;
	}

	// 파일 읽은 후 이미지 디코딩
	fp = fopen(fileName, "rb");
	if(fp == NULL) {
		kPrintf("[JPEG Viewer] %s file open fail\n", fileName);
		return FALSE;
	}

	// 메모리를 파일 크기만큼 할당 후 JPEG 자료구조 할당
	fileBuf = (BYTE*)kAllocMem(fileSize);
	jpg = (JPEG*)kAllocMem(sizeof(JPEG));
	if((fileBuf == NULL) || (jpg == NULL)) {
		kPrintf("[JPEG Viewer] Buffer allocation fail\n");
		kFreeMem(fileBuf);
		kFreeMem(jpg);
		fclose(fp);
		return FALSE;
	}

	// 파일 읽은 후JPEG 파일 포맷 확인
	if((fread(fileBuf, 1, fileSize, fp) != fileSize) || (kJpgInit(jpg, fileBuf, fileSize) == FALSE)) {
		kPrintf("[JPEG Viewer] Read fail or file is not JPEG format\n");
		kFreeMem(fileBuf);
		kFreeMem(jpg);
		fclose(fp);
		return FALSE;
	}

	// 디코드 결과 출력용 버퍼 생성
	outBuf = kAllocMem(jpg->width * jpg->height * sizeof(COLOR));

	// 디코드를 수행한 후 정상 처리시 윈도우 생성
	if((outBuf != NULL) && (kJpgDecode(jpg, outBuf) == TRUE)) {
		// 전체 화면 영역 크기 반환
		kGetMonArea(&monArea);
		// 윈도우 생성, 이미지 크기와 제목 표시줄 크기 고려
		winID = kCreateWin((monArea.x2 - jpg->width) / 2, (monArea.y2 - jpg->height) / 2, jpg->width, jpg->height + WINDOW_TITLE_HEIGHT, WINDOW_FLAGS_DEFAULT & ~WINDOW_FLAGS_SHOW | WINDOW_FLAGS_RESIZABLE, fileName);
	}

	// 윈도우 생성 실패 또는 출력 버퍼 할당 또는 디코딩 실패시 종료
	if((winID == WINDOW_INVALID_ID) || (outBuf == NULL)) {
		kPrintf("[JPEG Viewer] Window create fail or output buffer allocation fail\n");
		kFreeMem(fileBuf);
		kFreeMem(jpg);
		kFreeMem(outBuf);
		kDelWin(winID);
		return FALSE;
	}

	// 윈도우 너비로 제목 표시줄 영역 제외 나머지 화면 버퍼 영역 디코딩된 이미지 복사
	win = kFindWinLock(winID);
	if(win != NULL) {
		winWidth = kGetRectWidth(&(win->area));
		kMemCpy(win->winBuf + (WINDOW_TITLE_HEIGHT * winWidth), outBuf, jpg->width * jpg->height * sizeof(COLOR));

		kUnlock(&(win->lock));
	}

	// 파일 버퍼 해제 후 윈도우 화면에 표시
	kFreeMem(fileBuf);
	kShowWin(winID, TRUE);

	// ESC 키와 윈도우 닫기 버튼 처리 이벤트 루프. 정상적으로 윈도우 생성하여 표시했으면 파일 이름 입력 윈도우 숨김
	kShowWin(mainWinID, FALSE);

	exit = FALSE;
	while(exit == FALSE) {
		// 이벤트 큐에서 이벤트 수신
		if(kWinToEvent(winID, &recvEvent) == FALSE) {
			kSleep(0);
			continue;
		}

		// 수신된 이벤트 타입에 따라 처리
		switch(recvEvent.type) {
			// 키 이벤트 처리
			case EVENT_KEY_DOWN:
				keyEvent = &(recvEvent.keyEvent);
				// ESC 키가 눌리면 그림을 표시하는 윈도우 삭제 후 파일 이름 입력 윈도우 표시 후 종료
				if(keyEvent->ascii == KEY_ESC) {
					kDelWin(winID);
					kShowWin(mainWinID, TRUE);
					exit = TRUE;
				}
				break;

			// 윈도우 크기 변경 이벤트 처리
			case EVENT_WINDOW_RESIZE:
				// 변경된 윈도우에 디코딩 된 이미지 전송
				kBitBlt(winID, 0, WINDOW_TITLE_HEIGHT, outBuf, jpg->width, jpg->height);
				// 윈도우를 한 번 더 표시해 윈도우 내부에 전송된 이미지 업데이트
				kShowWin(winID, TRUE);
				break;

			// 윈도우 닫기 이벤트 처리
			case EVENT_WINDOW_CLOSE:
				// 닫기 버튼 눌리면 이미지 출력 윈도우 삭제 후 파일 이름 입력 윈도우 표시 후 종료
				if(recvEvent.type == EVENT_WINDOW_CLOSE) {
					kDelWin(winID);
					kShowWin(mainWinID, TRUE);
					exit = TRUE;
				}
				break;

			default:
				// 그 외 알 수 없는 이벤트 처리 코드 구현
				break;
		}
	}

	kFreeMem(jpg);
	kFreeMem(outBuf);

	return TRUE;
}
