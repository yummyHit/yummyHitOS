/*
 * writer.c
 *
 *  Created on: 2019. 2. 10.
 *      Author: Yummy
 */

#include "yummyHitLib.h"
#include "kor.h"
#include "writer.h"

// 응용프로그램 C언어 엔트리 포인트
int main(char *argv) {
	QWORD winID;
	int x, y, width, height;
	EVENT recvEvent;
	KEYEVENT *keyEvent;
	WINDOWEVENT *winEvent;
	RECT monArea;
	BUFINFO bufManager;
	BOOL mode;

	// 그래픽 모드 판단. 그래픽 모드로 시작했는지 여부 확인
	if(isGUIMode() == FALSE) {
		printf("It is GUI Task. You must execute GUI Mode.\n");
		return -1;
	}

	// 윈도우와 라인 인덱스 생성 후 첫 번째 라인부터 출력
	getMonArea(&monArea);
	width = (MAX_LENGTH * FONT_ENG_WIDTH) + 5;
	height = 40;
	x = (getRectWidth(&monArea) - width) / 2;
	y = (getRectHeight(&monArea) - height) / 2;
	winID = makeWin(x, y, width, height, WINDOW_FLAGS_DEFAULT | WINDOW_FLAGS_RESIZABLE, "Writer");

	if(winID == WINDOW_INVALID_ID) {
		printf("Window create failed..\n");
		return -1;
	}

	// 버퍼 정보 초기화 후 영문 입력모드 설정
	memset(&bufManager, 0, sizeof(bufManager));
	mode = FALSE;

	// 커서를 메모 입력 영역 앞쪽에 세로로 길게 출력 후 윈도우 재표시
	drawRect(winID, 3, 4 + WINDOW_TITLE_HEIGHT, 5, 3 + WINDOW_TITLE_HEIGHT + FONT_ENG_HEIGHT, RGB(159, 48, 215), TRUE);
	showWin(winID, TRUE);

	// GUI 태스크 이벤트 처리 루프
	while(1) {
		// 이벤트 큐에서 이벤트 수신
		if(winToEvent(winID, &recvEvent) == FALSE) {
			sleep(10);
			continue;
		}

		// 수신된 이벤트 타입에 따라 나눠 처리
		switch(recvEvent.type) {
			// 키 이벤트 처리
			case EVENT_KEY_DOWN:
				keyEvent = &(recvEvent.keyEvent);
				if(keyEvent->flag & KEY_FLAGS_DOWN) {
					// 키 값에 따른 현재 라인 변경 값 설정
					switch(keyEvent->ascii) {
						case KEY_ALT:
							bufManager.inputLen = 0;
							// 한영전환은 alt키, 한글 입력 모드 중 alt 키 눌리면 한글 조합 종료
							if(mode == TRUE) {
								// 키 입력 버퍼 초기화
								if((bufManager.processing[0] != '\0') && (bufManager.outputLen + 2 < MAX_LENGTH)) {
									// 조합 중인 한글을 윈도우 화면에 출력하는 버퍼로 복사
									memcpy(bufManager.output + bufManager.outputLen, bufManager.processing, 2);
									bufManager.outputLen += 2;

									// 조합 중인 한글 저장하는 버퍼 초기화
									bufManager.processing[0] = '\0';
								}
							}
							// 영문 입력 모드 중 alt 키 눌러지면 한글 조합용 버퍼 초기화
							else {
								bufManager.complete[0] = '\0';
								bufManager.processing[0] = '\0';
							}
							mode = TRUE - mode;
							break;
						case KEY_BACKSPACE:
							// 한글 조합중이면 입력 버퍼 내용 삭제 후 남은 키 입력 버퍼 내용으로 한글 조합
							if((mode == TRUE) && (bufManager.inputLen > 0)) {
								// 키 입력 버퍼 내용을 하나 제거, 한글 재조합
								bufManager.inputLen--;
								makeKorean(bufManager.input, &bufManager.inputLen, bufManager.processing, bufManager.complete);
							}
							// 한글 조합 중이 아니면 윈도우 화면에 출력하는 버퍼 내용 삭제
							else {
								if(bufManager.outputLen > 0) {
									// 화면 출력 버퍼에 들어 있는 내용이 2바이트 이상이고 버퍼에 저장된 값에 최상위 비트가 켜져있으면 한글로 간주
									if((bufManager.outputLen >= 2) && (bufManager.output[bufManager.outputLen - 1] & 0x80)) {
										bufManager.outputLen -= 2;
										memset(bufManager.output + bufManager.outputLen, 0, 2);
									}
									// 한글 아니면 1바이트만 삭제
									else {
										bufManager.outputLen--;
										bufManager.output[bufManager.outputLen] = '\0';
									}
								}
							}
							break;
						default:
							// 나머지 키들은 현재 입력 모드에 따라 한글 조합 또는 화면에 출력하는 버퍼로 삽입
							if(keyEvent->ascii & 0x80) break;	// 특수 키 무시

							// 한글 조합 중이면 한글 조합 처리(한글을 출력할 공간 확보도 확인)
							if((mode == TRUE) && (bufManager.outputLen + 2 <= MAX_LENGTH)) {
								// 자모음이 시프트와 조합되는 경우를 대비해 쌍자음이나 쌍모음 제외한 나머지는 소문자로 변환
								tolowerKorean(&keyEvent->ascii);

								// 입력 버퍼에 키 입력 값 채우고 데이터 길이 증가
								bufManager.input[bufManager.inputLen] = keyEvent->ascii;
								bufManager.inputLen++;

								// 한글 조합에 필요한 버퍼 넘겨 한글 조합
								if(makeKorean(bufManager.input, &bufManager.inputLen, bufManager.processing, bufManager.complete) == TRUE) {
									// 조합이 완료된 버퍼에 값이 있는가 확인해 화면 출력 버퍼로 복사
									if(bufManager.complete[0] != '\0') {
										memcpy(bufManager.output + bufManager.outputLen, bufManager.complete, 2);
										bufManager.outputLen += 2;

										// 조합된 한글을 복사한 후 현재 조합 중인 한글 출력할 공간이 없다면 버퍼 모두 초기화
										if(bufManager.outputLen + 2 > MAX_LENGTH) {
											bufManager.inputLen = 0;
											bufManager.processing[0] = '\0';
										}
									}
								}
								// 조합에 실패하면 입력 버퍼에 마지막으로 입력된 값이 한글 자모음이 아니므로 모든 버퍼의 값 모두 출력 버퍼로 복사
								else {
									// 조합이 완료된 버퍼에 값이 있는가 확인해 화면 출력 버퍼로 복사
									if(bufManager.complete[0] != '\0') {
										// 완성된 한글을 출력 버퍼로 복사
										memcpy(bufManager.output + bufManager.outputLen, bufManager.complete, 2);
										bufManager.outputLen += 2;
									}

									// 화면 출력 버퍼 공간이 충분하면 키 입력 버퍼에 마지막으로 입력된 한글 자모가 아닌 값 복사
									if(bufManager.outputLen < MAX_LENGTH) {
										bufManager.output[bufManager.outputLen] = bufManager.input[0];
										bufManager.outputLen++;
									}

									// 키 입력 버퍼 비움
									bufManager.inputLen = 0;
								}
							}
							// 한글 입력 모드가 아닌 경우
							else if((mode == FALSE) && (bufManager.outputLen + 1 <= MAX_LENGTH)) {
								// 키 입력을 그대로 화면 출력 버퍼로 저장
								bufManager.output[bufManager.outputLen] = keyEvent->ascii;
								bufManager.outputLen++;
							}
							break;
					}

					// 화면 출력 버퍼에 있는 문자열 모두 출력
					drawText(winID, 2, WINDOW_TITLE_HEIGHT + 4, RGB(0, 0, 0), RGB(255, 255, 255), bufManager.output, MAX_LENGTH);

					// 현재 조합 중인 한글이 있다면 화면 출력 버퍼 내용이 출력된 다음 위치에 조합 중인 한글 출력
					if(bufManager.processing[0] != '\0')
						drawText(winID, 2 + (bufManager.outputLen * FONT_ENG_WIDTH), WINDOW_TITLE_HEIGHT + 4, RGB(0, 0, 0), RGB(255, 255, 255), bufManager.processing, 2);

					// 커서를 세로로 길게 출력
					drawRect(winID, 3 + (bufManager.outputLen * FONT_ENG_WIDTH), 4 + WINDOW_TITLE_HEIGHT, 5 + (bufManager.outputLen * FONT_ENG_WIDTH), 3 + WINDOW_TITLE_HEIGHT + FONT_ENG_HEIGHT, RGB(159, 48, 215), TRUE);

					showWin(winID, TRUE);
				}
				break;

				// 윈도우 크기 변경 처리
				// 오프셋 처리하여 출력하는 함수 만들어야 함.
			case EVENT_WINDOW_RESIZE:
				winEvent = &(recvEvent.winEvent);
				width = getRectWidth(&(winEvent->area));
				height = getRectHeight(&(winEvent->area));

				showWin(winID, TRUE);
				break;

				// 윈도우 닫기 버튼 처리
			case EVENT_WINDOW_CLOSE:
				// 윈도우 삭제 후 메모리 해제
				delWin(winID);
				return 0;
				break;

				// 그 외
			deafult:
				break;
		}
	}

	return 0;
}

// 한글 자음과 모음 범위에서 쌍자음과 쌍모음을 제외한 나머지 모두 소문자로 변환
void tolowerKorean(BYTE *input) {
	if((*input < 'A') || (*input > 'Z')) return;

	// 쌍자음 또는 쌍모음 여부 판별
	switch(*input) {
		case 'Q':	// 'ㅃ'
		case 'W':   // 'ㅉ'
		case 'E':   // 'ㄸ'
		case 'R':   // 'ㄲ'
		case 'T':   // 'ㅆ'
		case 'O':   // 'ㅒ'
		case 'P':   // 'ㅖ'
			return;
			break;
	}

	// 소문자로 변환
	*input = tolower(*input);
}
