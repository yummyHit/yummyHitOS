/*
 * notepad.c
 *
 *  Created on: 2019. 1. 11.
 *      Author: Yummy
 */

#include "yummyHitLib.h"
#include "notepad.h"

// 응용프로그램 C언어 엔트리 포인트
int main(char *argv) {
	QWORD winID;
	int x, y, width, height, move;
	TXTINFO info;
	EVENT recvEvent;
	KEYEVENT *keyEvent;
	WINDOWEVENT *winEvent;
	DWORD offset;
	RECT monArea;

	// 그래픽 모드 판단. 그래픽 모드로 시작했는지 여부 확인
	if(isGUIMode() == FALSE) {
		printf("It is GUI Task. You must execute GUI Mode.\n");
		return -1;
	}

	// 파일 내용을 파일 버퍼에 저장, 라인별 파일 오프셋 저장용 버퍼 생성
	if(strlen(argv) == 0) {
		printf("ex)exec notepad.elf text.txt\n");
		return -1;
	}

	// 파일을 찾은 후 파일의 크기만큼 메모리 할당하여 파일 저장 및 버퍼 생성
	if(readfile(argv, &info) == FALSE) {
		printf("%s file not found\n", argv);
		return -1;
	}

	// 윈도우와 라인 인덱스 생성 후 첫 번째 라인부터 출력
	getMonArea(&monArea);
	width = 644;
	height = 500;
	x = (getRectWidth(&monArea) - width) / 2;
	y = (getRectHeight(&monArea) - height) / 2;
	winID = makeWin(x, y, width, height, WINDOW_FLAGS_DEFAULT | WINDOW_FLAGS_RESIZABLE, "Notepad");

	if(winID == WINDOW_INVALID_ID) {
		printf("Window create failed..\n");
		return -1;
	}

	// 라인별 파일 오프셋 계싼 후 현재 화면에 출력하는 라인 인덱스 초기화
	calcLineOffset(width, height, &info);
	info.nowLine = 0;

	// 현재 라인부터 화면 전체 크기 표시
	drawTextBuf(winID, &info);

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
						// Page UP, Page Down 키는 화면 라인 단위
						case KEY_PAGE_UP:
							move = -info.rowCnt;
							break;
						case KEY_PAGE_DOWN:
							move = info.rowCnt;
							break;
							// UP, Down 키는 한 줄 단위
						case KEY_ARROW_UP:
							move = -1;
							break;
						case KEY_ARROW_DOWN:
							move = 1;
							break;
							// 그 외 움직일 필요 없으면 종료
						default:
							move = 0;
							break;
					}

					// 최대 최소 라인 범위를 벗어나면 인덱스 조정
					if(info.nowLine + move < 0)
						move = -info.nowLine;
					else if(info.nowLine + move >= info.maxLine)
						move = info.maxLine - info.nowLine - 1;

					if(move == 0) break;

					// 현재 라인 인덱스 변경 후 출력
					info.nowLine += move;
					drawTextBuf(winID, &info);
				}
				break;

				// 윈도우 크기 변경 처리
			case EVENT_WINDOW_RESIZE:
				winEvent = &(recvEvent.winEvent);
				width = getRectWidth(&(winEvent->area));
				height = getRectHeight(&(winEvent->area));

				// 현재 라인의 오프셋 저장
				offset = info.lineOffset[info.nowLine];

				// 변경된 화면 크기로 라인 수, 라인당 문자 수, 오프셋 계산 후 현재 라인 재계산
				calcLineOffset(width, height, &info);
				info.nowLine = offset / info.colCnt;

				// 현재 라인부터 출력
				drawTextBuf(winID, &info);
				break;

				// 윈도우 닫기 버튼 처리
			case EVENT_WINDOW_CLOSE:
				// 윈도우 삭제 후 메모리 해제
				delWin(winID);
				free(info.buf);
				free(info.lineOffset);
				return 0;
				break;

				// 그 외
			deafult:
				break;
		}
	}
}

// 파일을 찾아 파일 크기만큼 버퍼 할당 후 라인별 파일 오프셋 버퍼 할당, 파일 내용을 읽어 메모리에 저장
BOOL readfile(const char *fileName, TXTINFO *info) {
	DIR *dir;
	DIRENTRY *entry;
	DWORD fileSize, readSize;
	FILE *fp;

	// 루트 디렉터리를 열어 파일 검색
	dir = opendir("/");
	fileSize = 0;

	// 디렉터리에서 파일 검색
	while(1) {
		// 디렉터리에서 엔트리 하나 읽음
		entry = readdir(dir);
		// 더 이상 파일 없으면 끝
		if(entry == NULL) break;

		// 파일 이름 길이와 내용 같은 것 검색
		if((strlen(entry->d_name) == strlen(fileName)) && (memcmp(entry->d_name, fileName, strlen(fileName)) == 0)) {
			fileSize = entry->size;
			break;
		}
	}

	// 디렉터리 핸들 반환.
	closedir(dir);

	if(fileSize == 0) {
		printf("%s file not found or size is zero!\n");
		return FALSE;
	}

	// 파일 이름 저장
	memcpy(&(info->d_name), fileName, sizeof(info->d_name));
	info->d_name[sizeof(info->d_name) - 1] = '\0';

	// 파일 전체 읽는 임시 버퍼 및 라인별 오프셋 저장할 버퍼를 할당, 파일 내용 모두 저장
	// 라인별 오프셋 저장할 버퍼 할당
	info->lineOffset = malloc(sizeof(DWORD) * MAX_LINE_CNT);
	if(info->lineOffset == NULL) {
		printf("malloc() failed..\n");
		return FALSE;
	}

	// 파일 내용 저장할 버퍼 할당
	info->buf = (BYTE*)malloc(fileSize);
	if(info->buf == NULL) {
		printf("%dbytes malloc() failed..\n", fileSize);
		free(info->lineOffset);
		return FALSE;
	}

	// 파일 열어서 모두 메모리에 저장
	fp = fopen(fileName, "r");
	if((fp != NULL) && (fread(info->buf, 1, fileSize, fp) == fileSize)) {
		fclose(fp);
		printf("%s file read success!\n", fileName);
	} else {
		printf("%s file read failed..\n", fileName);
		free(info->lineOffset);
		free(info->buf);
		fclose(fp);
		return FALSE;
	}

	// 파일 크기 저장
	info->size = fileSize;
	return TRUE;
}

// 파일 버퍼 내용 분석하여 라인별 오프셋 계산
void calcLineOffset(int width, int height, TXTINFO *info) {
	DWORD i;
	int lineIdx = 0, colIdx = 0;

	// 여유 공간과 제목 표시줄 높이를 고려해 라인별 문자 수와 출력가능 라인 수 계산
	info->colCnt = (width - MARGIN * 2) / FONT_ENG_WIDTH;
	info->rowCnt = (height - (WINDOW_TITLE_HEIGHT * 2) - (MARGIN * 2)) / FONT_ENG_HEIGHT;

	// 파일 처음부터 끝까지 라인 번호 계산해 오프셋 저장
	info->lineOffset[0] = 0;
	for(i = 0; i < info->size; i++) {
		// 라인 피드 문자 무시
		if(info->buf[i] == '\r') continue;
		else if(info->buf[i] == '\t') {
			colIdx = colIdx + TAB_SPACE;
			colIdx -= colIdx % TAB_SPACE;
		}
		else colIdx++;

		// 출력할 위치가 라인별 문자 수를 넘거나 탭 문자를 출력할 공간이 없는 경우, 또는 줄바꿈 문자가 검출되면 줄 바꿈
		if((colIdx >= info->colCnt) || (info->buf[i] == '\n')) {
			lineIdx++;
			colIdx = 0;

			// 라인 인덱스 버퍼에 오프셋 삽입
			if(i + 1 < info->size) info->lineOffset[lineIdx] = i + 1;

			// 노트패드가 지원하는 최대 라인 수 넘기면 종료
			if(lineIdx >= MAX_LINE_CNT) break;
		}
	}

	// 가장 마지막 라인 번호 저장
	info->maxLine = lineIdx;
}

// 윈도우 화면 버퍼에 현재 라인부터 화면에 출력
BOOL drawTextBuf(QWORD winID, TXTINFO *info) {
	DWORD i, j, baseOffset;
	BYTE tmp;
	int xOffset, yOffset, lineCnt, colCnt, len, width, colIdx = 0;
	char buf[100];
	RECT winArea;

	// 좌표 기준값
	xOffset = MARGIN;
	yOffset = WINDOW_TITLE_HEIGHT;
	getWinArea(winID, &winArea);

	// 파일 정보 표시 영역에 정보 출력
	// 파일 이름과 현재 라인, 전체 라인 수 출력
	width = getRectWidth(&winArea);
	drawRect(winID, 2, yOffset, width - 3, WINDOW_TITLE_HEIGHT * 2, RGB(159, 48, 215), TRUE);
	// 임시 버퍼에 정보 저장
	sprintf(buf, "File: %s, Line: %d/%d\n", info->d_name, info->nowLine + 1, info->maxLine);
	len = strlen(buf);
	// 저장된 정보를 파일 정보 표시 영역 가운데에 출력
	drawText(winID, (width - (len * FONT_ENG_WIDTH)) / 2, WINDOW_TITLE_HEIGHT + 2, RGB(255, 255, 255), RGB(159, 48, 215), buf, strlen(buf));

	// 파일 내용 표시 영역에 파일 내용 출력
	// 데이터 출력할 부분 모두 흰색으로 덮어쓴 후 라인 출력
	yOffset = (WINDOW_TITLE_HEIGHT * 2) + MARGIN;
	drawRect(winID, xOffset, yOffset, xOffset + (FONT_ENG_WIDTH * info->colCnt), yOffset + (FONT_ENG_HEIGHT * info->rowCnt), RGB(255,255,255), TRUE);

	// 루프 수행하며 라인 단위로 출력
	// 현재 라인에서 남은 라인 수와 한 화면에 출력가능 라인 수 비교해 작은 것 선택
	lineCnt = _MIN(info->rowCnt, (info->maxLine - info->nowLine));
	for(j = 0; j < lineCnt; j++) {
		// 출력할 라인 파일 오프셋
		baseOffset = info->lineOffset[info->nowLine + j];

		// 루프를 수행하며 현재 라인에 문자 출력
		colCnt = _MIN(info->colCnt, (info->size - baseOffset));
		for(i = 0, colIdx = 0; (i < colCnt) && (colIdx < info->colCnt); i++) {
			tmp = info->buf[i + baseOffset];
			// 줄바꿈 문자가 보이면 종료
			if(tmp == '\n') break;
			// 탭 문자이면 탭 문자 크기 단위로 출력할 오프셋 변경
			else if(tmp == '\t') {
				colIdx = colIdx + TAB_SPACE;
				colIdx -= colIdx % TAB_SPACE;
			}
			// 라인 피드 문자면 무시
			else if(tmp == '\r');
			// 기타 문자 출력
			else {
				// 출력할 위치에 문자 출력 후 다음 위치로 이동
				drawText(winID, xOffset + (colIdx * FONT_ENG_WIDTH), yOffset + (j * FONT_ENG_HEIGHT), RGB(0, 0, 0), RGB(255, 255, 255), &tmp, 1);
				colIdx++;
			}
		}
	}

	// 윈도우 전체 갱신하여 변경된 화면 업데이트
	showWin(winID, TRUE);

	return TRUE;
}
