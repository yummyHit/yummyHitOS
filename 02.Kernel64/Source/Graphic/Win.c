/*
 * Win.c
 *
 *  Created on: 2017. 9. 2.
 *      Author: Yummy
 */

#include <Win.h>
#include <VBE.h>
#include <CLITask.h>
#include <DynMem.h>
#include <Util.h>
#include <Font.h>
#include <JPEG.h>

// GUI 시스템 관련 자료구조
static WINDOWPOOLMANAGER gs_winPoolManager;
// 윈도우 매니저 관련 자료구조
static WINDOWMANAGER gs_winManager;

// 윈도우 풀 관련. 윈도우 풀 초기화
static void kInitWinPool(void) {
	int i;
	void *addr;

	// 자료구조 초기화
	kMemSet(&gs_winPoolManager, 0, sizeof(gs_winPoolManager));

	// 윈도우 풀 메모리 할당
	addr = (void*)kAllocMem(sizeof(WINDOW) * WINDOW_MAXCNT);
	if(addr == NULL) {
		kPrintf("Window Pool Allocate Fail\n");
		while(1);
	}

	// 윈도우 풀 어드레스 지정하고 초기화
	gs_winPoolManager.startAddr = (WINDOW*)addr;
	kMemSet(addr, 0, sizeof(WINDOW) * WINDOW_MAXCNT);

	// 윈도우 풀 ID 할당
	for(i = 0; i < WINDOW_MAXCNT; i++) gs_winPoolManager.startAddr[i].link.id = i;

	// 윈도우 최대 개수와 할당된 횟수 초기화
	gs_winPoolManager.maxCnt = WINDOW_MAXCNT;
	gs_winPoolManager.allocCnt = 1;

	// 뮤텍스 초기화
	kInitMutex(&(gs_winPoolManager.lock));
}

// 윈도우 자료구조 할당
static WINDOW *kAllocWin(void) {
	WINDOW *win;
	int i;

	// 동기화 처리
	kLock(&(gs_winPoolManager.lock));

	// 윈도우가 모두 할당되었으면 실패
	if(gs_winPoolManager.useCnt == gs_winPoolManager.maxCnt) {
		// 동기화 처리
		kUnlock(&(gs_winPoolManager.lock));
		return NULL;
	}

	// 윈도우 풀을 모두 돌며 빈 영역 검색. ID 상위 32비트가 0이면 비어 있는 윈도우 자료구조임
	for(i = 0; i < gs_winPoolManager.maxCnt; i++) if((gs_winPoolManager.startAddr[i].link.id >> 32) == 0) {
		win = &(gs_winPoolManager.startAddr[i]);
		break;
	}

	// 상위 32비트를 0이 아닌 값으로 설정해 할당된 윈도우로 설정
	win->link.id = ((QWORD)gs_winPoolManager.allocCnt << 32) | i;

	// 자료구조가 사용 중인 개수와 할당된 횟수 증가
	gs_winPoolManager.useCnt++;
	gs_winPoolManager.allocCnt++;
	if(gs_winPoolManager.allocCnt == 0) gs_winPoolManager.allocCnt = 1;

	// 동기화 처리
	kUnlock(&(gs_winPoolManager.lock));

	// 윈도우 뮤텍스 초기화
	kInitMutex(&(win->lock));

	return win;
}

// 윈도우 자료구조 해제
static void kFreeWin(QWORD id) {
	int i;

	// 윈도우 ID로 윈도우 풀 오프셋 계산, 윈도우 ID 하위 32비트가 인덱스 역할 함
	i = GETWINDOWOFFSET(id);

	// 동기화 처리
	kLock(&(gs_winPoolManager.lock));

	// 윈도우 자료구조 초기화 및 ID 설정
	kMemSet(&(gs_winPoolManager.startAddr[i]), 0, sizeof(WINDOW));
	gs_winPoolManager.startAddr[i].link.id = i;

	// 사용 중인 자료구조 개수 감소
	gs_winPoolManager.useCnt--;

	// 동기화 처리
	kUnlock(&(gs_winPoolManager.lock));
}

// 윈도우와 윈도우 매니저 관련. GUI 시스템 초기화
void kInitGUISystem(void) {
	VBEMODEINFO *mode;
	QWORD id;
	EVENT *buf;

	// 윈도우 풀 초기화
	kInitWinPool();

	// VBE 모드 정보 블록 반환
	mode = kGetVBEModeInfo();

	// 비디오 메모리 어드레스 설정
	gs_winManager.memAddr = (COLOR*)((QWORD)mode->linearBaseAddr & 0xFFFFFFFF);

	// 마우스 커서 초기 위치 설정
	gs_winManager.xMouse = mode->xPixel / 2;
	gs_winManager.yMouse = mode->yPixel / 2;

	// 화면 영역 범위 설정
	gs_winManager.area.x1 = 0;
	gs_winManager.area.y1 = 0;
	gs_winManager.area.x2 = mode->xPixel - 1;
	gs_winManager.area.y2 = mode->yPixel - 1;

	// 뮤텍스 초기화
	kInitMutex(&(gs_winManager.lock));

	// 윈도우 리스트 초기화
	kInitList(&(gs_winManager.list));

	// 이벤트 큐에서 사용할 이벤트 자료구조 풀 생성
	buf = (EVENT*)kAllocMem(sizeof(EVENT) * WINDOW_MAXCNT);
	if(buf == NULL) {
		kPrintf("Window Manager Event Queue Allocate Fail...\n");
		while(1);
	}
	// 이벤트 큐 초기화
	kInitQueue(&(gs_winManager.queue), buf, WINDOW_MAXCNT, sizeof(EVENT));

	// 화면 업데이트 시 사용할 비트맵 버퍼 생성. 비트맵은 화면 전체 크기로 생성해 공용으로 사용
	gs_winManager.bitmap = kAllocMem((mode->xPixel * mode->yPixel + 7) / 8);
	if(gs_winManager.bitmap == NULL) {
		kPrintf("Draw Bitmap Allocate Fail...\n");
		while(1);
	}

	// 마우스 버튼의 상태와 윈도우 이동 여부 초기화
	gs_winManager.preBtnStat = 0;
	gs_winManager.moveMode = FALSE;
	gs_winManager.moveID = WINDOW_INVALID_ID;

	// 윈도우 크기 변경 정보 초기화
	gs_winManager.resizeMode = FALSE;
	gs_winManager.resizeID = WINDOW_INVALID_ID;
	kMemSet(&(gs_winManager.resizeArea), 0, sizeof(RECT));

	// 배경 윈도우 생성. 플래그에 0을 넘겨 화면에 윈도우를 그리지 않도록 함. 배경 윈도우는 윈도우 내 배경색을 모두 칠한 뒤 나타냄
	id = kCreateWin(0, 0, mode->xPixel, mode->yPixel, 0, WINDOW_BACKGROUND_TITLE);
	gs_winManager.backgroundID = id;
	// 배경 윈도우 내부에 배경색 채움
	kDrawRect(id, 0, 0, mode->xPixel - 1, mode->yPixel - 1, WINDOW_COLOR_SYSTEM_BACKGROUND, TRUE);

	// 배경 화면 이미지 표시
	kDrawBackgroundImg();

	// 배경 윈도우를 화면에 나타냄
	kShowWin(id, TRUE);
}

// 윈도우 매니저 반환
WINDOWMANAGER *kGetWinManager(void) {
	return &gs_winManager;
}

// 배경 윈도우 ID 반환
QWORD kGetBackgroundID(void) {
	return gs_winManager.backgroundID;
}

// 화면 영역 크기 반환
void kGetMonArea(RECT *area) {
	kMemCpy(area, &(gs_winManager.area), sizeof(RECT));
}

// 윈도우 생성
QWORD kCreateWin(int x, int y, int width, int height, DWORD flag, const char *title) {
	WINDOW *win;
	TCB *task;
	QWORD id;
	EVENT event;

	// 크기가 0인 윈도우는 만들 수 없음
	if((width <= 0) || (height <= 0)) return WINDOW_INVALID_ID;

	// 윈도우 자료구조 할당
	win = kAllocWin();
	if(win == NULL) return WINDOW_INVALID_ID;

	// 윈도우에 제목 표시줄이 있으면 버튼 표시 자리를 주어야 함
	if(flag & WINDOW_FLAGS_DRAWTITLE) {
		if(width < WINDOW_WIDTH_MIN) width = WINDOW_WIDTH_MIN;
		if(height < WINDOW_HEIGHT_MIN) height = WINDOW_HEIGHT_MIN;
	}

	// 윈도우 영역 설정
	win->area.x1 = x;
	win->area.y1 = y;
	win->area.x2 = x + width - 1;
	win->area.y2 = y + height - 1;

	// 윈도우 제목 저장
	kMemCpy(win->title, title, WINDOW_TITLE_MAXLEN);
	win->title[WINDOW_TITLE_MAXLEN] = '\0';

	// 윈도우 화면 버퍼와 이벤트 큐에서 사용할 이벤트 자료구조 풀 생성
	win->winBuf = (COLOR*)kAllocMem(width * height * sizeof(COLOR));
	win->qBuf = (EVENT*)kAllocMem(EVENT_WINDOW_MAXCNT * sizeof(EVENT));
	if((win->winBuf == NULL) || (win->qBuf == NULL)) {
		// 윈도우 버퍼, 이벤트 자료구조 풀, 메모리 할당 전부 반환
		kFreeMem(win->winBuf);
		kFreeMem(win->qBuf);
		kFreeWin(win->link.id);
		return WINDOW_INVALID_ID;
	}

	// 이벤트 큐 초기화
	kInitQueue(&(win->queue), win->qBuf, EVENT_WINDOW_MAXCNT, sizeof(EVENT));

	// 윈도우를 생성한 태스크 ID 저장
	task = kGetRunningTask(kGetAPICID());
	win->taskID = task->link.id;

	// 윈도우 속성 설정
	win->flag = flag;

	// 윈도우 배경 그리기
	kDrawWinBackground(win->link.id);

	// 윈도우 테두리 그리기
	if(flag & WINDOW_FLAGS_DRAWFRAME) kDrawWinFrame(win->link.id);

	// 윈도우 제목 표시줄 그리기
	if(flag & WINDOW_FLAGS_DRAWTITLE) kDrawWinTitle(win->link.id, title, TRUE);

	// 동기화 처리
	kLock(&(gs_winManager.lock));

	// 최상위 윈도우 반환
	id = kGetTopWin();

	// 윈도우 리스트의 가장 마지막에 추가해 최상위 윈도우로 설정
	kAddListHead(&gs_winManager.list, win);

	// 동기화 처리
	kUnlock(&(gs_winManager.lock));

	// 윈도우 영역만큼 화면에 업데이트하고 선택되었다는 이벤트 전송
	kUpdateMonID(win->link.id);
	kSetWinEvent(win->link.id, EVENT_WINDOW_SELECT, &event);
	kEventToWin(win->link.id, &event);

	// 최상위 윈도우가 배경 윈도우가 아니면 이전 최상위 윈도우 제목 표시줄 색깔 미선택으로 업데이트 및 선택 해제 전송
	if(id != gs_winManager.backgroundID) {
		kUpdateWinTitle(id, FALSE);
		kSetWinEvent(id, EVENT_WINDOW_DESELECT, &event);
		kEventToWin(id, &event);
	}

	return win->link.id;
}

// 윈도우 삭제. 윈도우 선택 이벤트도 같이 전송
BOOL kDelWin(QWORD id) {
	WINDOW *win;
	RECT area;
	QWORD winID;
	BOOL onWin;
	EVENT event;

	// 동기화 처리
	kLock(&(gs_winManager.lock));

	// 윈도우 검색
	win = kFindWinLock(id);
	if(win == NULL) {
		// 동기화 처리
		kUnlock(&(gs_winManager.lock));
		return FALSE;
	}

	// 윈도우를 삭제하기 전 영역 저장
	kMemCpy(&area, &(win->area), sizeof(RECT));

	// 윈도우 리스트에서 최상위 윈도우 반환
	winID = kGetTopWin();

	// 최상위 윈도우가 지워지는 경우 플래그 설정
	if(winID == id) onWin = TRUE;
	else onWin = FALSE;

	// 윈도우 리스트에서 윈도우 삭제
	if(kDelList(&(gs_winManager.list), id) == NULL) {
		// 동기화 처리
		kUnlock(&(win->lock));
		kUnlock(&(gs_winManager.lock));
		return FALSE;
	}

	// 윈도우 화면 버퍼 반환
	kFreeMem(win->winBuf);
	win->winBuf = NULL;

	// 윈도우 이벤트 큐 버퍼 반환
	kFreeMem(win->qBuf);
	win->qBuf = NULL;

	// 동기화 처리
	kUnlock(&(win->lock));

	// 윈도우 자료구조 반환
	kFreeWin(id);

	// 동기화 처리
	kUnlock(&(gs_winManager.lock));

	// 삭제되기 전에 윈도우가 있던 영역 화면에 업데이트
	kUpdateMonArea(&area);

	// 최상위 윈도우가 지워졌으면 현재 리스트에서 최상위에 있는 윈도우를 활성화하고 선택되었다는 윈도우 이벤트 전송
	if(onWin == TRUE) {
		// 윈도우 리스트에서 최상위 윈도우 반환
		winID = kGetTopWin();

		// 최상위 윈도우 제목 표시줄 활성화된 상태 표시
		if(winID != WINDOW_INVALID_ID) {
			kUpdateWinTitle(winID, TRUE);
			kSetWinEvent(winID, EVENT_WINDOW_SELECT, &event);
			kEventToWin(winID, &event);
		}
	}
	return TRUE;
}

// 태스크 ID가 일치하는 모든 윈도우 삭제
BOOL kDelAllWin(QWORD id) {
	WINDOW *win, *nextWin;

	// 동기화 처리
	kLock(&(gs_winManager.lock));

	// 리스트에서 첫 번째 윈도우 반환
	win = kGetListHead(&(gs_winManager.list));
	while(win != NULL) {
		// 다음 윈도우 미리 구함
		nextWin = kGetNextList(&(gs_winManager.list), win);

		// 배경 윈도우가 아니고 태스크 ID가 일치하면 윈도우 삭제
		if((win->link.id != gs_winManager.backgroundID) && (win->taskID == id)) kDelWin(win->link.id);

		// 미리 구해둔 다음 윈도우 값 설정
		win = nextWin;
	}

	// 동기화 처리
	kUnlock(&(gs_winManager.lock));
}

// 윈도우 ID로 윈도우 포인터 반환
WINDOW *kGetWin(QWORD id) {
	WINDOW *win;

	// 윈도우 ID 유효 범위 검사
	if(GETWINDOWOFFSET(id) >= WINDOW_MAXCNT) return NULL;

	// ID로 윈도우 포인터 찾은 후 ID 일치하면 반환
	win = &gs_winPoolManager.startAddr[GETWINDOWOFFSET(id)];
	if(win->link.id == id) return win;

	return NULL;
}

// 윈도우 ID로 윈도우 포인터 찾아 윈도우 뮤텍스 잠근 후 반환
WINDOW *kFindWinLock(QWORD id) {
	WINDOW *win;
	BOOL res;

	// 윈도우 검색
	win = kGetWin(id);
	if(win == NULL) return NULL;

	// 동기화 처리 후 다시 윈도우 ID로 윈도우 검색
	kLock(&(win->lock));
	// 윈도우를 동기화 한 후 윈도우 ID로 윈도우를 검색할 수 없으면 도중에 윈도우가 바뀐 것이니 NULL 반환
	win = kGetWin(id);
	if((win == NULL) || (win->qBuf == NULL) || (win->winBuf == NULL)) {
		// 동기화 처리
		kUnlock(&(win->lock));
		return NULL;
	}

	return win;
}

// 윈도우를 화면에 나타내거나 숨김
BOOL kShowWin(QWORD id, BOOL show) {
	WINDOW *win;
	RECT area;

	// 윈도우 검색과 동기화 처리
	win = kFindWinLock(id);
	if(win == NULL) return FALSE;

	// 윈도우 속성 설정
	if(show == TRUE) win->flag |= WINDOW_FLAGS_SHOW;
	else win->flag &= ~WINDOW_FLAGS_SHOW;

	// 동기화 처리
	kUnlock(&(win->lock));

	// 윈도우가 있던 영역을 다시 업데이트함으로 윈도우를 나타내거나 숨김
	if(show == TRUE) kUpdateMonID(id);
	else {
		kGetWinArea(id, &area);
		kUpdateMonArea(&area);
	}
	return TRUE;
}

// 특정 영역 포함 윈도우 모두 그림
BOOL kUpdateWinArea(const RECT *area, QWORD id) {
	WINDOW *win;
	RECT crossArea, cursorArea, tmpCrossArea, largeCrossArea[WINDOW_CROSSAREA_LOGMAXCNT];
	DRAWBITMAP bitmap;
	int largeCrossAreaSize[WINDOW_CROSSAREA_LOGMAXCNT], tmpCrossAreaSize, minAreaSize, minAreaIdx, i;

	// 화면 영역과 겹치는 영역이 없으면 그릴 필요 없음
	if(kGetRectCross(&(gs_winManager.area), area, &crossArea) == FALSE) return FALSE;

	// Z 순서 최하위(윈도우 리스트의 첫 번째부터 마지막까지) 루프를 돌며 업데이트 할 영역과 겹치는 윈도우 찾아 비디오 메모리로 전송. 화면에 업데이트할 영역 기록 공간 초기화
	kMemSet(largeCrossAreaSize, 0, sizeof(largeCrossAreaSize));
	kMemSet(largeCrossArea, 0, sizeof(largeCrossArea));

	// 동기화 처리
	kLock(&(gs_winManager.lock));

	// 화면에 업데이트할 영역 저장하는 비트맵 생성
	kCreateBitmap(&crossArea, &bitmap);

	// 현재 윈도우 리스트는 Z 순서의에 따라 정렬되어 있음. 윈도우 리스트를 처음부터 따라가며 그릴 영역 포함하는 윈도우 찾고 그 윈도우부터 최상위 윈도우까지 화면에 전송
	win = kGetListHead(&(gs_winManager.list));
	while(win != NULL) {
		// 윈도우를 화면에 나타내는 옵션이 설정되어 있으며, 업데이트할 부분과 윈도우가 차지하는 영역이 겹치면 겹치는 만큼 화면에 전송
		if((win->flag & WINDOW_FLAGS_SHOW) && (kGetRectCross(&(win->area), &crossArea, &tmpCrossArea) == TRUE)) {
			// 윈도우와 겹치는 영역 넓이 계산
			tmpCrossAreaSize = kGetRectWidth(&tmpCrossArea) * kGetRectHeight(&tmpCrossArea);

			// 이전에 기록한 윈도우 업데이트 영역 검색해 이전 영역에 포함되는지 확인
			for(i = 0; i < WINDOW_CROSSAREA_LOGMAXCNT; i++)
				// 겹치는 영역을 이전에 저장한 영역과 비교해 화면 업데이트 결정. 큰 영역에 포함되면 업데이트 안함
				if((tmpCrossAreaSize <= largeCrossAreaSize[i]) && (kIsInRect(&(largeCrossArea[i]), tmpCrossArea.x1, tmpCrossArea.y1) == TRUE) && (kIsInRect(&(largeCrossArea[i]), tmpCrossArea.x2, tmpCrossArea.y2) == TRUE)) break;

			// 일치하는 업데이트 영역을 찾았으면 이전에 업데이트 되었으니 다음 윈도우로 이동
			if(i < WINDOW_CROSSAREA_LOGMAXCNT) {
				// 다음 윈도우 찾음
				win = kGetNextList(&(gs_winManager.list), win);
				continue;
			}

			// 현재 영역이 이전 업데이트의 가장 큰 영역과 완전히 포함되지 않으면 넓이를 비교해 이전 업데이트의 가장 작은 영역 검색
			minAreaSize = 0xFFFFFF;
			minAreaIdx = 0;
			for(i = 0; i < WINDOW_CROSSAREA_LOGMAXCNT; i++) if(largeCrossAreaSize[i] < minAreaSize) {
				minAreaSize = largeCrossAreaSize[i];
				minAreaIdx = i;
			}

			// 저장된 영역 중 최소 크기보다 현재 겹치는 넓이가 크면 해당 위치를 교체해 영역 크기로 10개 유지
			if(minAreaSize < tmpCrossAreaSize) {
				kMemCpy(&(largeCrossArea[minAreaIdx]), &tmpCrossArea, sizeof(RECT));
				largeCrossAreaSize[minAreaIdx] = tmpCrossAreaSize;
			}

			// 동기화 처리
			kLock(&(win->lock));

			// 윈도우 ID가 유효하면 그 전까지 윈도우를 그리지 않고 비트맵만 업데이트 한 것으로 변경
			if((id != WINDOW_INVALID_ID) && (id != win->link.id)) kFillBitmap(&bitmap, &(win->area), FALSE);
			else kWinBufToFrame(win, &bitmap);

			// 동기화 처리
			kUnlock(&(win->lock));
		}

		// 모든 영역이 업데이트되면 더 이상 그릴 필요 없음
		if(kIsBitmapFin(&bitmap) == TRUE) break;

		// 다음 윈도우 찾음
		win = kGetNextList(&(gs_winManager.list), win);
	}

	// 동기화 처리
	kUnlock(&(gs_winManager.lock));

	// 마우스 커서 영역이 포함되면 마우스 커서도 같이 그림. 마우스 영역을 RECT 자료구조에 설정
	kSetRectData(gs_winManager.xMouse, gs_winManager.yMouse, gs_winManager.xMouse + MOUSE_CURSOR_WIDTH, gs_winManager.yMouse + MOUSE_CURSOR_HEIGHT, &cursorArea);

	// 겹치는지 확인 후 겹치면 마우스 커서도 그림
	if(kIsRectCross(&crossArea, &cursorArea) == TRUE) kDrawCursor(gs_winManager.xMouse, gs_winManager.yMouse);
}

// 윈도우 화면 버퍼의 일부 또는 전체를 프레임 버퍼로 복사
static void kWinBufToFrame(const WINDOW *win, DRAWBITMAP *bitmap) {
	RECT tmpArea, crossArea;
	int crossWidth, crossHeight, monWidth, winWidth, i, byteOffset, bitOffset, xOffset, yOffset, lastOffset, bulkCnt;
	COLOR *memAddr, *bufAddr;
	BYTE tmpBitmap;

	// 전송해야 하는 영역과 화면 영역이 겹치는 부분 임시 계산
	if(kGetRectCross(&(gs_winManager.area), &(bitmap->area), &tmpArea) == FALSE) return;

	// 윈도우 영역과 임시로 계산한 영역이 겹치는 부분 재계산. 두 영역이 겹치지 않으면 비디오 메모리로 전송할 필요 없음
	if(kGetRectCross(&tmpArea, &(win->area), &crossArea) == FALSE) return;

	// 각 영역 너비와 높이 계산
	monWidth = kGetRectWidth(&(gs_winManager.area));
	winWidth = kGetRectWidth(&(win->area));
	crossWidth = kGetRectWidth(&crossArea);
	crossHeight = kGetRectHeight(&crossArea);

	// 겹치는 영역 높이만큼 무한 출력
	for(yOffset = 0; yOffset < crossHeight; yOffset++) {
		// 겹치는 영역 화면 업데이트에서 존재하는 위치 계산
		if(kGetStartBitmap(bitmap, crossArea.x1, crossArea.y1 + yOffset, &byteOffset, &bitOffset) == FALSE) break;

		// 전송 시작할 비디오 메모리 어드레스와 윈도우 화면 버퍼 어드레스 계산
		memAddr = gs_winManager.memAddr + (crossArea.y1 + yOffset) * monWidth + crossArea.x1;

		// 윈도우 화면 버퍼는 화면 전체가 아닌 윈도우 기준 좌표이므로 겹치는 영역을 윈도우 내부 좌표 기준으로 변환
		bufAddr = win->winBuf + (crossArea.y1 - win->area.y1 + yOffset) * winWidth + (crossArea.x1 - win->area.x1);

		// 겹친 영역 너비만큼 출력
		for(xOffset = 0; xOffset < crossWidth;) {
			// 8개 픽셀을 한 번에 업데이트 할 수 있으면 8픽셀 단위로 처리할 수 있는 크기 계산해 한 번에 처리
			if((bitmap->bitmap[byteOffset] == 0xFF) && (bitOffset == 0x00) && ((crossWidth - xOffset) >= 8)) {
				for(bulkCnt = 0; (bulkCnt < ((crossWidth - xOffset) >> 3)); bulkCnt++) if(bitmap->bitmap[byteOffset + bulkCnt] != 0xFF) break;

				// 8픽셀 단위로 한 번에 처리
				kMemCpy(memAddr, bufAddr, (sizeof(COLOR) * bulkCnt) << 3);

				// 메모리 어드레스와 비트맵 정보를 8픽셀 단위로 업데이트
				memAddr += bulkCnt << 3;
				bufAddr += bulkCnt << 3;
				kMemSet(bitmap->bitmap + byteOffset, 0x00, bulkCnt);

				// 전체 개수에서 8픽셀 단위로 전송한 수 만큼 값 더함
				xOffset += bulkCnt << 3;

				// 비트맵 오프셋 변경
				byteOffset += bulkCnt;
				bitOffset = 0;
			} else if((bitmap->bitmap[byteOffset] == 0x00) && (bitOffset == 0x00) && ((crossWidth - xOffset) >= 8 )) {
				// 현재 영역이 이미 업데이트 되어 8개 픽셀을 한 번에 제외할 수 있으면 8픽셀 단위로 처리할 수 있는 크기를 계산해 한 번에 처리.
				for(bulkCnt = 0; (bulkCnt < ((crossWidth - xOffset) >> 3)); bulkCnt++) if(bitmap->bitmap[byteOffset + bulkCnt] != 0x00) break;

				// 메모리 어드레스를 변경된 것으로 업데이트
				memAddr += bulkCnt << 3;
				bufAddr += bulkCnt << 3;

				// 전체 개수에서 8픽셀 단위로 제외한 수만큼 값 더함
				xOffset += bulkCnt << 3;

				// 비트맵 오프셋 변경
				byteOffset += bulkCnt;
				bitOffset = 0;
			} else {
				// 현재 업데이트할 위치 비트맵
				tmpBitmap = bitmap->bitmap[byteOffset];

				// 현재 비트맵에서 출력해야 할 마지막 픽셀 비트 오프셋 구함
				lastOffset = _MIN(8, crossWidth - xOffset + bitOffset);

				// 한 점씩 이동
				for(i = bitOffset; i < lastOffset; i++) {
					// 비트맵이 1로 설정되어 있으면 화면에 출력.
					if(tmpBitmap & (0x01 << i)) {
						*memAddr = *bufAddr;
						// 비트맵 정보를 변경된 것으로 업데이트
						tmpBitmap &= ~(0x01 << i);
					}

					// 메모리 어드레스를 변경된 것으로 업데이트
					memAddr++;
					bufAddr++;
				}

				// 전체 개수에서 1픽셀 단위로 전송한 수만큼 값 더함
				xOffset += (lastOffset - bitOffset);

				// 비트맵 정보를 변경된 것으로 업데이트
				bitmap->bitmap[byteOffset] = tmpBitmap;
				byteOffset++;
				bitOffset = 0;
			}
		}
	}
}

// 특정 위치를 포함하는 윈도우 중 가장 위의 윈도우 반환
QWORD kFindWinPoint(int x, int y) {
	QWORD id;
	WINDOW *win;

	// 마우스는 배경 윈도우를 벗어나지 못하므로 기본 값을 배경 윈도우로 설정
	id = gs_winManager.backgroundID;

	// 동기화 처리
	kLock(&(gs_winManager.lock));

	// 배경 윈도우 다음부터 검색
	win = kGetListHead(&(gs_winManager.list));
	do {
		// 윈도우가 화면에 보이고 윈도우가 X, Y좌표를 포함하면 ID 업데이트
		if((win->flag & WINDOW_FLAGS_SHOW) && (kIsInRect(&(win->area), x, y) == TRUE)) {
			id = win->link.id;
			break;
		}

		// 다음 윈도우 반환
		win = kGetNextList(&(gs_winManager.list), win);
	} while(win != NULL);

	// 동기화 처리
	kUnlock(&(gs_winManager.lock));
	return id;
}

// 윈도우 제목이 일치하는 윈도우 반환
QWORD kFindWinTitle(const char *title) {
	QWORD id;
	WINDOW *win;
	int len;

	id = WINDOW_INVALID_ID;
	len = kStrLen(title);

	// 동기화 처리
	kLock(&(gs_winManager.lock));

	// 배경 윈도우부터 검색
	win = kGetListHead(&(gs_winManager.list));
	while(win != NULL) {
		// 윈도우 제목이 일치하면 루프를 나가 일치하는 윈도우 ID 반환
		if((kStrLen(win->title) == len) && (kMemCmp(win->title, title, len) == 0)) {
			id = win->link.id;
			break;
		}

		// 다음 윈도우 반환
		win = kGetNextList(&(gs_winManager.list), win);
	}

	// 동기화 처리
	kUnlock(&(gs_winManager.lock));
	return id;
}

// 윈도우 존재 여부 반환
BOOL kIsWinExist(QWORD id) {
	// 윈도우 검색 결과가 NULL이면 없음
	if(kGetWin(id) == NULL) return FALSE;
	return TRUE;
}

// 최상위 윈도우 ID 반환
QWORD kGetTopWin(void) {
	WINDOW *win;
	QWORD id;

	// 동기화 처리
	kLock(&(gs_winManager.lock));

	// 윈도우 리스트 최상위 윈도우 반환
	win = (WINDOW*)kGetListHead(&(gs_winManager.list));
	if(win != NULL) id = win->link.id;
	else id = WINDOW_INVALID_ID;

	// 동기화 처리
	kUnlock(&(gs_winManager.lock));
	return id;
}

// 윈도우 Z순서를 최상위로 함. 윈도우를 최상위로 이동시킴과 동시에 윈도우 선택 및 선택 해제 이벤트 전송
BOOL kMoveWinTop(QWORD id) {
	WINDOW *win;
	RECT area;
	DWORD flag;
	QWORD topID;
	EVENT event;

	// 현재 윈도우 리스트에서 선택된 윈도우 ID 반환
	topID = kGetTopWin();
	// 최상위 윈도우가 자신이면 수행할 필요 없음
	if(topID == id) return TRUE;

	kLock(&(gs_winManager.lock));

	// 윈도우 리스트에서 제거해 윈도우 리스트 마지막으로 이동
	win = kDelList(&(gs_winManager.list), id);
	if(win != NULL) {
		kAddListHead(&(gs_winManager.list), win);

		// 윈도우 영역을 윈도우 내부 좌표로 변환해 플래그와 함께 저장. 아래에서 윈도우 화면 업데이트시 사용
		kMonitorToRect(id, &(win->area), &area);
		flag = win->flag;
	}

	// 동기화 처리
	kUnlock(&(gs_winManager.lock));

	// 윈도우가 최상위로 이동했다면 윈도우 제목 표시줄 업데이트 후 선택 및 선택 해제 이벤트 전송
	if(win != NULL) {
		kSetWinEvent(id, EVENT_WINDOW_SELECT, &event);
		kEventToWin(id, &event);

		// 제목 표시줄이 있다면 현재 윈도우의 제목 표시줄은 선택된 것으로 만들고 화면 업데이트
		if(flag & WINDOW_FLAGS_DRAWTITLE) {
			// 윈도우 제목 표시줄 선택 상태로 업데이트
			kUpdateWinTitle(id, TRUE);
			area.y1 += WINDOW_TITLE_HEIGHT;
			kUpdateMonWinArea(id, &area);
		} else {
			// 제목 표시줄 없으면 윈도우 영역 전체 업데이트
			kUpdateMonID(id);
		}

		// 이전에 활성화 된 윈도우는 제목 표시줄을 비활성화로 만들고 선택 해제 이벤트 전송
		kSetWinEvent(topID, EVENT_WINDOW_DESELECT, &event);
		kEventToWin(topID, &event);
		// 제목 표시줄 선택 안된 상태로 업데이트
		kUpdateWinTitle(topID, FALSE);
		return TRUE;
	}
	return FALSE;
}

// X, Y좌표가 윈도우 제목 표시줄 위치에 있는지 반환
BOOL kIsInTitle(QWORD id, int x, int y) {
	WINDOW *win;

	// 윈도우 검색
	win = kGetWin(id);

	// 윈도우가 없거나 제목 표시줄을 안갖으면 미처리
	if((win == NULL) || ((win->flag & WINDOW_FLAGS_DRAWTITLE) == 0)) return FALSE;

	// 좌표가 제목 표시줄 영역에 있는지 비교
	if((win->area.x1 <= x) && (x <= win->area.x2) && (win->area.y1 <= y) && (y <= win->area.y1 + WINDOW_TITLE_HEIGHT)) return TRUE;
	return FALSE;
}

// X, Y좌표가 윈도우 닫기 버튼 위치에 있는지 반환
BOOL kIsCloseBtn(QWORD id, int x, int y) {
	WINDOW *win;

	// 윈도우 검색
	win = kGetWin(id);

	// 윈도우가 없거나 제목 표시줄을 안갖으면 미처리
	if((win == NULL) && ((win->flag & WINDOW_FLAGS_DRAWTITLE) == 0)) return FALSE;

	// 좌표가 닫기 버튼 영역에 있는지 비교
	if(((win->area.x2 - WINDOW_XBTN_SIZE - 1) <= x) && (x <= (win->area.x2 - 1)) && ((win->area.y1 + 1) <= y) && (y <= (win->area.y1 + 1 + WINDOW_XBTN_SIZE))) return TRUE;
	return FALSE;
}

// 윈도우 크기 변경
BOOL kResizeWin(QWORD id, int x, int y, int width, int height) {
	WINDOW *win;
	COLOR *new, *old;
	RECT preArea;

	// 윈도우 검색과 동기화 처리
	win = kFindWinLock(id);
	if(win == NULL) return FALSE;

	// 윈도우에 제목 표시줄 있으면 버튼 표시 자리가 있어야 함
	if(win->flag & WINDOW_FLAGS_DRAWTITLE) {
		if(width < WINDOW_WIDTH_MIN) width = WINDOW_WIDTH_MIN;
		if(height < WINDOW_HEIGHT_MIN) height = WINDOW_HEIGHT_MIN;
	}

	// 새로운 크기 화면 버퍼 할당
	new = (COLOR*)kAllocMem(width * height * sizeof(COLOR));
	if(new == NULL) {
		// 메모리 할당 실패 시 종료
		kUnlock(&(win->lock));
		return FALSE;
	}

	// 새로운 화면 버퍼 설정 후 이전 버퍼 해제
	old = win->winBuf;
	win->winBuf = new;
	kFreeMem(old);

	// 윈도우 크기 정보 저장 후 새로운 크기로 변경
	kMemCpy(&preArea, &(win->area), sizeof(RECT));
	win->area.x1 = x;
	win->area.y1 = y;
	win->area.x2 = x + width - 1;
	win->area.y2 = y + height - 1;

	// 윈도우 배경 그리기
	kDrawWinBackground(id);

	// 윈도우 테두리 그리기
	if(win->flag & WINDOW_FLAGS_DRAWFRAME) kDrawWinFrame(id);

	// 윈도우 제목 표시줄 그리기
	if(win->flag & WINDOW_FLAGS_DRAWTITLE) kDrawWinTitle(id, win->title, TRUE);

	// 동기화 처리
	kUnlock(&(win->lock));

	// 윈도우 화면에 표시하는 속성 있으면 업데이트
	if(win->flag & WINDOW_FLAGS_SHOW) {
		// 이전 윈도우 영역 다시 그림
		kUpdateMonArea(&preArea);

		// 새로운 영역 다시 그림
		kShowWin(id, TRUE);
	}

	return TRUE;
}

// X, Y좌표가 윈도우 크기 변경 버튼 위에 있는지 반환
BOOL kIsResizeBtn(QWORD id, int x, int y) {
	WINDOW *win;

	// 윈도우 검색
	win = kGetWin(id);

	// 윈도우 또는 제목 표시줄이 없거나 크기 변경 속성 없으면 빠져나옴
	if((win == NULL) || ((win->flag & WINDOW_FLAGS_DRAWTITLE) == 0) || ((win->flag & WINDOW_FLAGS_RESIZABLE) == 0)) return FALSE;

	// 좌표가 윈도우 크기 변경 버튼 영역에 있는지 비교
	if(((win->area.x2 - (WINDOW_XBTN_SIZE * 2) - 2) <= x) && (x <= (win->area.x2 - WINDOW_XBTN_SIZE - 2)) && ((win->area.y1 + 1) <= y) && (y <= (win->area.y1 + 1 + WINDOW_XBTN_SIZE))) return TRUE;

	return FALSE;
}

// 윈도우를 해당 위치로 이동
BOOL kMoveWin(QWORD id, int x, int y) {
	WINDOW *win;
	RECT preArea;
	int width, height;
	EVENT event;

	// 윈도우 검색과 동기화 처리
	win = kFindWinLock(id);
	if(win == NULL) return FALSE;

	// 이전 윈도우가 있던 곳 저장
	kMemCpy(&preArea, &(win->area), sizeof(RECT));

	// 높이와 너비 계산해 현재 윈도우 위치 변경
	width = kGetRectWidth(&preArea);
	height = kGetRectHeight(&preArea);
	kSetRectData(x, y, x + width - 1, y + height - 1, &(win->area));

	// 동기화 처리
	kUnlock(&(win->lock));

	// 이전 윈도우가 있던 화면 영역 업데이트
	kUpdateMonArea(&preArea);

	// 현재 윈도우 영역 화면 업데이트
	kUpdateMonID(id);

	// 윈도우 이동 메시지 전송
	kSetWinEvent(id, EVENT_WINDOW_MOVE, &event);
	kEventToWin(id, &event);

	return TRUE;
}

// 윈도우 제목 표시줄 새로 그림
static BOOL kUpdateWinTitle(QWORD id, BOOL select) {
	WINDOW *win;
	RECT titleArea;

	// 윈도우 검색
	win = kGetWin(id);

	// 윈도우가 존재하면 윈도우 제목 표시줄 다시 그려 화면에 업데이트
	if((win != NULL) && (win->flag & WINDOW_FLAGS_DRAWTITLE)) {
		// 선택 및 선택 해제 여부에 따라 윈도우 제목 표시줄 다시 그림
		kDrawWinTitle(win->link.id, win->title, select);
		// 윈도우 제목 표시줄 위치를 윈도우 내부 좌표로 저장
		titleArea.x1 = 0;
		titleArea.y1 = 0;
		titleArea.x2 = kGetRectWidth(&(win->area)) - 1;
		titleArea.y2 = WINDOW_TITLE_HEIGHT;

		// 윈도우 영역만큼 화면에 업데이트
		kUpdateMonWinArea(id, &titleArea);
		return TRUE;
	}
	return FALSE;
}

// 윈도우 영역 반환
BOOL kGetWinArea(QWORD id, RECT *area) {
	WINDOW *win;

	// 윈도우 검색과 동기화 처리
	win = kFindWinLock(id);
	if(win == NULL) return FALSE;

	// 윈도우 영역을 복사해 반환
	kMemCpy(area, &(win->area), sizeof(RECT));

	// 동기화 처리
	kUnlock(&(win->lock));
	return TRUE;
}

// 전체 화면을 기준으로 X, Y좌표를 윈도우 내부 좌표로 변환
BOOL kMonitorToPoint(QWORD id, const POINT *xy, POINT *xyWin) {
	RECT area;

	// 윈도우 영역 반환
	if(kGetWinArea(id, &area) == FALSE) return FALSE;

	xyWin->x = xy->x - area.x1;
	xyWin->y = xy->y - area.y1;
	return TRUE;
}

// 윈도우 내부를 기준으로 X, Y좌표를 화면 좌표로 변환
BOOL kPointToMon(QWORD id, const POINT *xy, POINT *xyMon) {
	RECT area;

	// 윈도우 영역 반환
	if(kGetWinArea(id, &area) == FALSE) return FALSE;

	xyMon->x = xy->x + area.x1;
	xyMon->y = xy->y + area.y1;
	return TRUE;
}

// 전체 화면을 기준으로 사각형 좌표를 윈도우 내부 좌표로 변환
BOOL kMonitorToRect(QWORD id, const RECT *area, RECT *areaWin) {
	RECT winArea;

	// 윈도우 영역 반환
	if(kGetWinArea(id, &winArea) == FALSE) return FALSE;

	areaWin->x1 = area->x1 - winArea.x1;
	areaWin->y1 = area->y1 - winArea.y1;
	areaWin->x2 = area->x2 - winArea.x1;
	areaWin->y2 = area->y2 - winArea.y1;
	return TRUE;
}

// 윈도우 내부를 기준으로 사각형 좌표를 화면 좌표로 변환
BOOL kRectToMon(QWORD id, const RECT *area, RECT *areaMon) {
	RECT winArea;

	// 윈도우 영역 반환
	if(kGetWinArea(id, &winArea) == FALSE) return FALSE;

	areaMon->x1 = area->x1 + winArea.x1;
	areaMon->y1 = area->y1 + winArea.y1;
	areaMon->x2 = area->x2 + winArea.x1;
	areaMon->y2 = area->y2 + winArea.y1;
	return TRUE;
}

// 윈도우를 화면에 업데이트. 태스크에서 사용
BOOL kUpdateMonID(QWORD id) {
	EVENT event;
	WINDOW *win;

	// 윈도우 검색
	win = kGetWin(id);
	// 윈도우가 없거나 보이지 않으면 이벤트 전달 미필요
	if((win == NULL) && ((win->flag & WINDOW_FLAGS_SHOW) == 0)) return FALSE;

	// 이벤트 자료구조 채움. 윈도우 ID 저장
	event.type = EVENT_WINDOWMANAGER_UPDATEBYID;
	event.winEvent.id = id;

	return kEventToWinManager(&event);
}

// 윈도우 내부를 화면에 업데이트. 태스크에서 사용
BOOL kUpdateMonWinArea(QWORD id, const RECT *area) {
	EVENT event;
	WINDOW *win;

	// 윈도우 검색
	win = kGetWin(id);
	// 윈도우가 없거나 보이지 않으면 이벤트 전달 미필요
	if((win == NULL) && ((win->flag & WINDOW_FLAGS_SHOW) == 0)) return FALSE;

	// 이벤트 자료구조 채움. 윈도우 ID와 윈도우 내부 영역 저장
	event.type = EVENT_WINDOWMANAGER_UPDATEBYWINAREA;
	event.winEvent.id = id;
	kMemCpy(&(event.winEvent.area), area, sizeof(RECT));

	return kEventToWinManager(&event);
}

// 화면 좌표로 화면 업데이트. 태스크에서 사용
BOOL kUpdateMonArea(const RECT *area) {
	EVENT event;

	// 이벤트 자료구조 채움. 윈도우 ID와 화면 영역 저장
	event.type = EVENT_WINDOWMANAGER_UPDATEBYMONAREA;
	event.winEvent.id = WINDOW_INVALID_ID;
	kMemCpy(&(event.winEvent.area), area, sizeof(RECT));

	return kEventToWinManager(&event);
}

// 윈도우로 이벤트 전송
BOOL kEventToWin(QWORD id, const EVENT *event) {
	WINDOW *win;
	BOOL res;

	// 윈도우 검색과 동기화 처리
	win = kFindWinLock(id);
	if(win == NULL) return FALSE;

	res = kAddQData(&(win->queue), event);

	// 동기화 처리
	kUnlock(&(win->lock));
	return res;
}

// 윈도우의 이벤트 큐에 저장된 이벤트 수신
BOOL kWinToEvent(QWORD id, EVENT *event) {
	WINDOW *win;
	BOOL res;

	// 윈도우 검색과 동기화 처리
	win = kFindWinLock(id);
	if(win == NULL) return FALSE;

	res = kRmQData(&(win->queue), event);

	// 동기화 처리
	kUnlock(&(win->lock));
	return res;
}

// 윈도우 매니저로 이벤트 전송
BOOL kEventToWinManager(const EVENT *event) {
	BOOL res = FALSE;

	// 큐에 여분이 있으면 데이터 넣을 수 있음
	if(kIsQFull(&(gs_winManager.queue)) == FALSE) {
		// 동기화 처리
		kLock(&(gs_winManager.lock));

		res = kAddQData(&(gs_winManager.queue), event);

		// 동기화 처리
		kUnlock(&(gs_winManager.lock));
	}
	return res;
}

// 윈도우 매니저의 이벤트 큐에 저장된 이벤트 수신
BOOL kWinManagerToEvent(EVENT *event) {
	BOOL res = FALSE;

	// 큐가 비어 있지 않으면 데이터 꺼낼 수 있음
	if(kIsQEmpty(&(gs_winManager.queue)) == FALSE) {
		// 동기화 처리
		kLock(&(gs_winManager.lock));

		res = kRmQData(&(gs_winManager.queue), event);

		// 동기화 처리
		kUnlock(&(gs_winManager.lock));
	}
	return res;
}

// 마우스 이벤트 자료구조 설정
BOOL kSetMouseEvent(QWORD id, QWORD type, int x, int y, BYTE btnStat, EVENT *event) {
	POINT xyWin, xy;

	// 이벤트 종류 확인 후 마우스 이벤트 생성
	switch(type) {
	// 마우스 이벤트 처리
	case EVENT_MOUSE_MOVE:
	case EVENT_MOUSE_LCLICK_ON:
	case EVENT_MOUSE_LCLICK_OFF:
	case EVENT_MOUSE_RCLICK_ON:
	case EVENT_MOUSE_RCLICK_OFF:
	case EVENT_MOUSE_WHEEL_ON:
	case EVENT_MOUSE_WHEEL_OFF:
		// 마우스 X, Y좌표 설정
		xy.x = x;
		xy.y = y;

		// 마우스 X, Y좌표를 윈도우 내부 좌표로 변환
		if(kMonitorToPoint(id, &xy, &xyWin) == FALSE) return FALSE;

		// 이벤트 타입 설정
		event->type = type;
		// 윈도우 ID 설정
		event->mouseEvent.id = id;
		// 마우스 버튼 상태 설정
		event->mouseEvent.btnStat = btnStat;
		// 마우스 커서 좌표를 윈도우 내부 좌표로 변환한 값 설정
		kMemCpy(&(event->mouseEvent.point), &xyWin, sizeof(POINT));
		break;
	default:
		return FALSE;
		break;
	}
	return TRUE;
}

// 윈도우 이벤트 자료구조 설정
BOOL kSetWinEvent(QWORD id, QWORD type, EVENT *event) {
	RECT area;

	// 이벤트 종류 확인해 윈도우 이벤트 생성
	switch(type) {
	// 윈도우 이벤트 처리
	case EVENT_WINDOW_SELECT:
	case EVENT_WINDOW_DESELECT:
	case EVENT_WINDOW_MOVE:
	case EVENT_WINDOW_RESIZE:
	case EVENT_WINDOW_CLOSE:
		// 이벤트 타입 설정
		event->type = type;
		// 윈도우 ID 설정
		event->winEvent.id = id;
		// 윈도우 영역 반환
		if(kGetWinArea(id, &area) == FALSE) return FALSE;

		// 윈도우 현재 좌표 설정
		kMemCpy(&(event->winEvent.area), &area, sizeof(RECT));
		break;
	default:
		return FALSE;
		break;
	}
	return TRUE;
}

// 키 이벤트 자료구조 설정
void kSetKeyEvent(QWORD win, const KEYDATA *key, EVENT *event) {
	// 눌림 또는 떨어짐 처리
	if(key->flag & KEY_FLAGS_DOWN) event->type = EVENT_KEY_DOWN;
	else event->type = EVENT_KEY_UP;

	// 키의 각 정보 설정
	event->keyEvent.ascii = key->ascii;
	event->keyEvent.scanCode = key->scanCode;
	event->keyEvent.flag = key->flag;
}

// 윈도우 내부에 그리는 함수 및 마우스 커서 관련. 윈도우 화면 버퍼에 윈도우 테두리 그리기
BOOL kDrawWinFrame(QWORD id) {
	WINDOW *win;
	RECT area;
	int width, height;

	// 윈도우 검색과 동기화 처리
	win = kFindWinLock(id);
	if(win == NULL) return FALSE;

	// 윈도우 너비와 높이 계산
	width = kGetRectWidth(&(win->area));
	height = kGetRectHeight(&(win->area));
	// 클리핑 영역 설정
	kSetRectData(0, 0, width - 1, height - 1, &area);

	// 윈도우 프레임의 가장자리 그림. 2픽셀 두께
	kInDrawRect(&area, win->winBuf, 0, 0, width - 1, height - 1, WINDOW_COLOR_FRAME, FALSE);
	kInDrawRect(&area, win->winBuf, 1, 1, width - 2, height - 2, WINDOW_COLOR_FRAME, FALSE);

	// 동기화 처리
	kUnlock(&(win->lock));
	return TRUE;
}

// 윈도우 화면 버퍼에 배경 그리기
BOOL kDrawWinBackground(QWORD id) {
	WINDOW *win;
	int width, height, x, y;
	RECT area;

	// 윈도우 검색과 동기화 처리
	win = kFindWinLock(id);
	if(win == NULL) return FALSE;

	// 윈도우 너비와 높이 계산
	width = kGetRectWidth(&(win->area));
	height = kGetRectHeight(&(win->area));
	// 클리핑 영역 설정
	kSetRectData(0, 0, width - 1, height - 1, &area);

	// 윈도우에 제목 표시줄이 있으면 그 아래부터 채움
	if(win->flag & WINDOW_FLAGS_DRAWTITLE) y = WINDOW_TITLE_HEIGHT;
	else y = 0;

	// 윈도우 테두리를 그리는 옵션이 설정되어 있으면 테두리 제외 영역 채움
	if(win->flag & WINDOW_FLAGS_DRAWFRAME) x = 2;
	else x = 0;

	// 윈도우 내부 채움
	kInDrawRect(&area, win->winBuf, x, y, width - 1 - x, height - 1 - x, WINDOW_COLOR_BACKGROUND, TRUE);

	// 동기화 처리
	kUnlock(&(win->lock));
	return TRUE;
}

// 윈도우 화면 버퍼에 윈도우 제목 표시줄 그리기
BOOL kDrawWinTitle(QWORD id, const char *title, BOOL select) {
	WINDOW *win;
	int width, height, x, y;
	RECT area, btnArea;
	COLOR titleColor;

	// 윈도우 검색과 동기화 처리
	win = kFindWinLock(id);
	if(win == NULL) return FALSE;

	// 윈도우 너비와 높이 계산
	width = kGetRectWidth(&(win->area));
	height = kGetRectHeight(&(win->area));
	// 클리핑 영역 설정
	kSetRectData(0, 0, width - 1, height - 1, &area);

	// 제목 표시줄 채움
	if(select == TRUE) titleColor = WINDOW_COLOR_TITLE_ONBACKGROUND;
	else titleColor = WINDOW_COLOR_TITLE_OFFBACKGROUND;
	kInDrawRect(&area, win->winBuf, 0, 3, width - 1, WINDOW_TITLE_HEIGHT - 1, titleColor, TRUE);

	// 윈도우 제목 표시
	kInDrawText(&area, win->winBuf, 6, 3, WINDOW_COLOR_TITLE_TEXT, titleColor, title, kStrLen(title));

	// 제목 표시줄을 입체로 보이게 위쪽의 선 그림. 2픽셀 두께
	kInDrawLine(&area, win->winBuf, 1, 1, width - 1, 1, WINDOW_COLOR_TITLE_FIRSTOUT);
	kInDrawLine(&area, win->winBuf, 1, 2, width - 1, 2, WINDOW_COLOR_TITLE_SECONDOUT);
	kInDrawLine(&area, win->winBuf, 1, 2, 1, WINDOW_TITLE_HEIGHT - 1, WINDOW_COLOR_TITLE_FIRSTOUT);
	kInDrawLine(&area, win->winBuf, 2, 2, 2, WINDOW_TITLE_HEIGHT - 1, WINDOW_COLOR_TITLE_SECONDOUT);

	// 제목 표시줄 아래쪽 선 그림
	kInDrawLine(&area, win->winBuf, 2, WINDOW_TITLE_HEIGHT - 2, width - 2, WINDOW_TITLE_HEIGHT - 2, WINDOW_COLOR_TITLE_UNDERLINE);
	kInDrawLine(&area, win->winBuf, 2, WINDOW_TITLE_HEIGHT - 1, width - 2, WINDOW_TITLE_HEIGHT - 1, WINDOW_COLOR_TITLE_UNDERLINE);

	// 동기화 처리
	kUnlock(&(win->lock));

	// 닫기 버튼 그리기. 오른쪽 위에 표시
	btnArea.x1 = width - WINDOW_XBTN_SIZE - 1;
	btnArea.y1 = 1;
	btnArea.x2 = width - 2;
	btnArea.y2 = WINDOW_XBTN_SIZE - 1;
	kDrawBtn(id, &btnArea, WINDOW_COLOR_BACKGROUND, "", WINDOW_COLOR_BACKGROUND);

	// 윈도우 검색과 동기화 처리
	win = kFindWinLock(id);
	if(win == NULL) return FALSE;

	// 닫기 버튼 내부에 대각선 X를 3픽셀로 그림
	kInDrawLine(&area, win->winBuf, width - 2 - 18 + 4, 1 + 4, width - 2 - 4, WINDOW_TITLE_HEIGHT - 6, WINDOW_COLOR_XLINE);
	kInDrawLine(&area, win->winBuf, width - 2 - 18 + 5, 1 + 4, width - 2 - 4, WINDOW_TITLE_HEIGHT - 7, WINDOW_COLOR_XLINE);
	kInDrawLine(&area, win->winBuf, width - 2 - 18 + 4, 1 + 5, width - 2 - 5, WINDOW_TITLE_HEIGHT - 6, WINDOW_COLOR_XLINE);
	kInDrawLine(&area, win->winBuf, width - 2 - 18 + 4, 19 - 4, width - 2 - 4, 1 + 4, WINDOW_COLOR_XLINE);
	kInDrawLine(&area, win->winBuf, width - 2 - 18 + 5, 19 - 4, width - 2 - 4, 1 + 5, WINDOW_COLOR_XLINE);
	kInDrawLine(&area, win->winBuf, width - 2 - 18 + 4, 19 - 5, width - 2 - 5, 1 + 4, WINDOW_COLOR_XLINE);

	// 동기화 처리
	kUnlock(&(win->lock));

	// 윈도우 크기 변경 버튼 그리기
	if(win->flag & WINDOW_FLAGS_RESIZABLE) {
		// 크기 변경 버튼 그림.
		btnArea.x1 = width - (WINDOW_XBTN_SIZE * 2) - 2;
		btnArea.y1 = 1;
		btnArea.x2 = width - WINDOW_XBTN_SIZE - 2;
		btnArea.y2 = WINDOW_XBTN_SIZE - 1;
		kDrawBtn(id, &btnArea, WINDOW_COLOR_BACKGROUND, "", WINDOW_COLOR_BACKGROUND);

		// 윈도우 검색과 동기화 처리
		win = kFindWinLock(id);
		if(win == NULL) return FALSE;

		// 크기 변경 버튼 내부에 대각선 화살표 3픽셀로 그림
		// 가운데 선
		kInDrawLine(&area, win->winBuf, btnArea.x1 + 4, btnArea.y2 - 4, btnArea.x2 - 5, btnArea.y1 + 3, WINDOW_COLOR_XLINE);
		kInDrawLine(&area, win->winBuf, btnArea.x1 + 4, btnArea.y2 - 3, btnArea.x2 - 4, btnArea.y1 + 3, WINDOW_COLOR_XLINE);
		kInDrawLine(&area, win->winBuf, btnArea.x1 + 5, btnArea.y2 - 3, btnArea.x2 - 4, btnArea.y1 + 4, WINDOW_COLOR_XLINE);

		// 오른쪽 위 화살표
		kInDrawLine(&area, win->winBuf, btnArea.x1 + 9, btnArea.y1 + 3, btnArea.x2 - 4, btnArea.y1 + 3, WINDOW_COLOR_XLINE);
		kInDrawLine(&area, win->winBuf, btnArea.x1 + 9, btnArea.y1 + 4, btnArea.x2 - 4, btnArea.y1 + 4, WINDOW_COLOR_XLINE);
		kInDrawLine(&area, win->winBuf, btnArea.x2 - 4, btnArea.y1 + 5, btnArea.x2 - 4, btnArea.y1 + 9, WINDOW_COLOR_XLINE);
		kInDrawLine(&area, win->winBuf, btnArea.x2 - 5, btnArea.y1 + 5, btnArea.x2 - 5, btnArea.y1 + 9, WINDOW_COLOR_XLINE);

		// 왼쪽 아래 화살표
		kInDrawLine(&area, win->winBuf, btnArea.x1 + 4, btnArea.y1 + 8, btnArea.x1 + 4, btnArea.y2 - 3, WINDOW_COLOR_XLINE);
		kInDrawLine(&area, win->winBuf, btnArea.x1 + 5, btnArea.y1 + 8, btnArea.x1 + 5, btnArea.y2 - 3, WINDOW_COLOR_XLINE);
		kInDrawLine(&area, win->winBuf, btnArea.x1 + 6, btnArea.y2 - 4, btnArea.x1 + 10, btnArea.y2 - 4, WINDOW_COLOR_XLINE);
		kInDrawLine(&area, win->winBuf, btnArea.x1 + 6, btnArea.y2 - 3, btnArea.x1 + 10, btnArea.y2 - 3, WINDOW_COLOR_XLINE);

		// 동기화 처리
		kUnlock(&(win->lock));
	}

	return TRUE;
}

// 윈도우 내부에 버튼 그리기
BOOL kDrawBtn(QWORD id, RECT *btnArea, COLOR background, const char *buf, COLOR text) {
	WINDOW *win;
	RECT area;
	int winWidth, winHeight, textLen, textWidth, btnWidth, btnHeight, x, y;

	// 윈도우 검색과 동기화 처리
	win = kFindWinLock(id);
	if(win == NULL) return FALSE;

	// 윈도우 너비와 높이 계산
	winWidth = kGetRectWidth(&(win->area));
	winHeight = kGetRectHeight(&(win->area));
	// 클리핑 영역 설정
	kSetRectData(0, 0, winWidth - 1, winHeight - 1, &area);

	// 버튼 배경색 표시
	kInDrawRect(&area, win->winBuf, btnArea->x1, btnArea->y1, btnArea->x2, btnArea->y2, background, TRUE);

	// 버튼과 텍스트 너비와 높이 계산
	btnWidth = kGetRectWidth(btnArea);
	btnHeight = kGetRectHeight(btnArea);
	textLen = kStrLen(buf);
	textWidth = textLen * FONT_ENG_WIDTH;

	// 텍스트가 버튼 가운데에 위치하게 출력
	x = (btnArea->x1 + btnWidth / 2) - textWidth / 2;
	y = (btnArea->y1 + btnHeight / 2) - FONT_ENG_HEIGHT / 2;
	kInDrawText(&area, win->winBuf, x, y, text, background, buf, textLen);

	// 버튼을 입체로 보이게 테두리 그림. 2픽셀 두께. 버튼 왼쪽과 위는 밝게 표시
	kInDrawLine(&area, win->winBuf, btnArea->x1, btnArea->y1, btnArea->x2, btnArea->y1, WINDOW_COLOR_BTN_OUT);
	kInDrawLine(&area, win->winBuf, btnArea->x1, btnArea->y1 + 1, btnArea->x2 - 1, btnArea->y1 + 1, WINDOW_COLOR_BTN_OUT);
	kInDrawLine(&area, win->winBuf, btnArea->x1, btnArea->y1, btnArea->x1, btnArea->y2, WINDOW_COLOR_BTN_OUT);
	kInDrawLine(&area, win->winBuf, btnArea->x1 + 1, btnArea->y1, btnArea->x1 + 1, btnArea->y2 - 1, WINDOW_COLOR_BTN_OUT);

	// 버튼 오른쪽과 아래는 어둡게 표시
	kInDrawLine(&area, win->winBuf, btnArea->x1 + 1, btnArea->y2, btnArea->x2, btnArea->y2, WINDOW_COLOR_BTN_DARK);
	kInDrawLine(&area, win->winBuf, btnArea->x1 + 2, btnArea->y2 - 1, btnArea->x2, btnArea->y2 - 1, WINDOW_COLOR_BTN_DARK);
	kInDrawLine(&area, win->winBuf, btnArea->x2, btnArea->y1 + 1, btnArea->x2, btnArea->y2, WINDOW_COLOR_BTN_DARK);
	kInDrawLine(&area, win->winBuf, btnArea->x2 - 1, btnArea->y1 + 2, btnArea->x2 - 1, btnArea->y2, WINDOW_COLOR_BTN_DARK);

	// 동기화 처리
	kUnlock(&(win->lock));
	return TRUE;
}


// 마우스 커서 이미지 저장 데이터
BYTE gs_mouseBuf[MOUSE_CURSOR_WIDTH * MOUSE_CURSOR_HEIGHT] = {
1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
1, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
1, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 1, 2, 3, 3, 3, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 1, 2, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0,
0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 0, 0, 0, 0, 0,
0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 0, 0, 0, 0,
0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 0, 0,
0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2,
0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 0, 0, 0, 0, 0,
0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 2, 2, 0, 0, 0, 0, 0,
0, 0, 0, 0, 1, 2, 3, 3, 3, 2, 2, 3, 3, 3, 2, 2, 0, 0, 0, 0,
0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 1, 2, 3, 3, 2, 2, 2, 0, 0, 0,
0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 1, 2, 2, 2, 2, 2, 2, 0, 0,
0, 0, 0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 1, 2, 2, 2, 2, 2, 2, 0,
0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1,
0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 2, 2, 2, 1, 0,
0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 2, 1, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,};

// X, Y 위치에 마우스 커서 출력
static void kDrawCursor(int x, int y) {
	int i, j;
	BYTE *position;

	// 커서 데이터 시작 위치 설정
	position = gs_mouseBuf;

	// 커서 너비와 높이만큼 루프를 돌며 픽셀을 화면에 출력
	for(j = 0; j < MOUSE_CURSOR_HEIGHT; j++) for(i = 0; i < MOUSE_CURSOR_WIDTH; i++) {
		switch(*position) {
		// 0은 출력하지 않음
		case 0:
			break;
		case 1:
			kInDrawPixel(&(gs_winManager.area), gs_winManager.memAddr, i + x, j + y, MOUSE_CURSOR_OUTLINE);
			break;
		case 2:
			kInDrawPixel(&(gs_winManager.area), gs_winManager.memAddr, i + x, j + y, MOUSE_CURSOR_OUTER);
			break;
		case 3:
			kInDrawPixel(&(gs_winManager.area), gs_winManager.memAddr, i + x, j + y, MOUSE_CURSOR_INNER);
			break;
		}

		// 커서 픽셀이 표시됨에 따라 커서 데이터 위치도 같이 이동
		position++;
	}
}

// 마우스 커서를 해당 위치로 이동해 그려줌
void kMoveCursor(int x, int y) {
	RECT preArea;

	// 마우스 커서가 화면을 벗어나지 못하도록 보정
	if(x < gs_winManager.area.x1) x = gs_winManager.area.x1;
	else if(x > gs_winManager.area.x2) x = gs_winManager.area.x2;

	if(y < gs_winManager.area.y1) y = gs_winManager.area.y1;
	else if(y > gs_winManager.area.y2) y = gs_winManager.area.y2;

	// 동기화 처리
	kLock(&(gs_winManager.lock));

	// 이전에 마우스 커서가 있던 자리 저장
	preArea.x1 = gs_winManager.xMouse;
	preArea.y1 = gs_winManager.yMouse;
	preArea.x2 = gs_winManager.xMouse + MOUSE_CURSOR_WIDTH - 1;
	preArea.y2 = gs_winManager.yMouse + MOUSE_CURSOR_HEIGHT - 1;

	// 마우스 커서 새 위치 저장
	gs_winManager.xMouse = x;
	gs_winManager.yMouse = y;

	// 동기화 처리
	kUnlock(&(gs_winManager.lock));

	// 마우스가 이전에 있던 영역 다시 그림
	kUpdateWinArea(&preArea, WINDOW_INVALID_ID);

	// 새로운 위치에 마우스 커서 출력
	kDrawCursor(x, y);
}

// 현재 마우스 커서 위치 반환
void kGetWinCursor(int *x, int *y) {
	*x = gs_winManager.xMouse;
	*y = gs_winManager.yMouse;
}

// 윈도우 내부에 점 그리기
BOOL kDrawPixel(QWORD id, int x, int y, COLOR color) {
	WINDOW *win;
	RECT area;

	// 윈도우 검색과 동기화 처리
	win = kFindWinLock(id);
	if(win == NULL) return FALSE;

	// 윈도우 시작 좌표를 0, 0으로 하는 좌표로 영역 반환
	kSetRectData(0, 0, win->area.x2 - win->area.x1, win->area.y2 - win->area.y1, &area);

	// 내부 함수 호출
	kInDrawPixel(&area, win->winBuf, x, y, color);

	// 동기화 처리
	kUnlock(&(win->lock));
	return TRUE;
}

// 윈도우 내부에 직선 그리기
BOOL kDrawLine(QWORD id, int x1, int y1, int x2, int y2, COLOR color) {
	WINDOW *win;
	RECT area;

	// 윈도우 검색과 동기화 처리
	win = kFindWinLock(id);
	if(win == NULL) return FALSE;

	// 윈도우 시작 좌표를 0, 0으로 하는 윈도우 기준 좌표로 영역 반환
	kSetRectData(0, 0, win->area.x2 - win->area.x1, win->area.y2 - win->area.y1, &area);

	// 내부 함수 호출
	kInDrawLine(&area, win->winBuf, x1, y1, x2, y2, color);

	// 동기화 처리
	kUnlock(&(win->lock));
	return TRUE;
}

// 윈도우 내부에 사각형 그리기
BOOL kDrawRect(QWORD id, int x1, int y1, int x2, int y2, COLOR color, BOOL fill) {
	WINDOW *win;
	RECT area;

	// 윈도우 검색과 동기화 처리
	win = kFindWinLock(id);
	if(win == NULL) return FALSE;

	// 윈도우 시작 좌표를 0, 0으로 하는 윈도우 기준 좌표로 영역 반환
	kSetRectData(0, 0, win->area.x2 - win->area.x1, win->area.y2 - win->area.y1, &area);

	// 내부 함수 호출
	kInDrawRect(&area, win->winBuf, x1, y1, x2, y2, color, fill);

	// 동기화 처리
	kUnlock(&(win->lock));
	return TRUE;
}

// 윈도우 내부에 원 그리기
BOOL kDrawCircle(QWORD id, int x, int y, int rad, COLOR color, BOOL fill) {
	WINDOW *win;
	RECT area;

	// 윈도우 검색과 동기화 처리
	win = kFindWinLock(id);
	if(win == NULL) return FALSE;

	// 윈도우 시작 좌표를 0, 0으로 하는 윈도우 기준 좌표로 영역 반환
	kSetRectData(0, 0, win->area.x2 - win->area.x1, win->area.y2 - win->area.y1, &area);

	// 내부 함수 호출
	kInDrawCircle(&area, win->winBuf, x, y, rad, color, fill);

	// 동기화 처리
	kUnlock(&(win->lock));
	return TRUE;
}

// 윈도우 내부에 문자 출력
BOOL kDrawText(QWORD id, int x, int y, COLOR text, COLOR background, const char *buf, int len) {
	WINDOW *win;
	RECT area;

	// 윈도우 검색과 동기화 처리
	win = kFindWinLock(id);
	if(win == NULL) return FALSE;

	// 윈도우 시작 좌표를 0, 0으로 하는 윈도우 기준 좌표로 영역 반환
	kSetRectData(0, 0, win->area.x2 - win->area.x1, win->area.y2 - win->area.y1, &area);

	// 내부 함수 호출
	kInDrawText(&area, win->winBuf, x, y, text, background, buf, len);

	// 동기화 처리
	kUnlock(&(win->lock));
	return TRUE;
}

// 화면 업데이트에 사용하는 화면 업데이트 비트맵 관련
BOOL kCreateBitmap(const RECT *area, DRAWBITMAP *bitmap) {
	// 화면 영역과 겹치는 부분이 없으면 비트맵 생성 필요 없음
	if(kGetRectCross(&(gs_winManager.area), area, &(bitmap->area)) == FALSE) return FALSE;

	// 윈도우 매니저에 있는 화면 업데이트 비트맵 버퍼 설정
	bitmap->bitmap = gs_winManager.bitmap;

	return kFillBitmap(bitmap, &(bitmap->area), TRUE);
}

// 화면에 업데이트할 비트맵 영역과 현재 영역이 겹치는 부분 값 0, 1로 채움
static BOOL kFillBitmap(DRAWBITMAP *bitmap, RECT *area, BOOL fill) {
	RECT crossArea;
	int byteOffset, bitOffset, width, height, i, xOffset, yOffset, bulkCnt, lastOffset;
	BYTE tmpBitmap;

	// 업데이트할 영역과 겹치는 부분이 없으면 비트맵 버퍼에 값 안채움
	if(kGetRectCross(&(bitmap->area), area, &crossArea) == FALSE) return FALSE;

	// 겹치는 영역 너비와 높이 계산
	width = kGetRectWidth(&crossArea);
	height = kGetRectHeight(&crossArea);

	// 겹치는 영역 높이만큼 출력하는 루프 반복
	for(yOffset = 0; yOffset < height; yOffset++) {
		// 비트맵 버퍼 내 라인 시작 위치 반환
		if(kGetStartBitmap(bitmap, crossArea.x1, crossArea.y1 + yOffset, &byteOffset, &bitOffset) == FALSE) break;

		// 겹친 영역 너비만큼 출력하는 루프 반복
		for(xOffset = 0; xOffset < width;) {
			// 8픽셀 단위로 처리할 수 있는 크기 계산해 한 번에 처리
			if((bitOffset == 0x00) && ((width - xOffset) >= 8)) {
				// 현재 위치에서 8픽셀 단위로 처리할 수 있는 최대 크기 계산
				bulkCnt = (width - xOffset) >> 3;

				// 8픽셀 단위로 한 번에 차리
				if(fill == TRUE) kMemSet(bitmap->bitmap + byteOffset, 0xFF, bulkCnt);
				else kMemSet(bitmap->bitmap + byteOffset, 0x00, bulkCnt);

				// 전체 개수에서 개별적으로 설정한 비트맵 수만큼 값 변경
				xOffset += bulkCnt << 3;

				// 비트맵 오프셋 변경
				byteOffset += bulkCnt;
				bitOffset = 0;
			} else {
				// 현재 비트맵에서 출력해야 할 마지막 픽셀 비트 오프셋 계산
				lastOffset = _MIN(8, width - xOffset + bitOffset);

				// 비트맵 생성
				tmpBitmap = 0;
				for(i = bitOffset; i < lastOffset; i++) tmpBitmap |= (0x01 << i);

				// 전체 개수에서 8픽셀씩 설정한 비트맵 수만큼 값 변경
				xOffset += (lastOffset - bitOffset);

				// 비트맵 정보를 변경된 것으로 업데이트
				if(fill == TRUE) bitmap->bitmap[byteOffset] |= tmpBitmap;
				else bitmap->bitmap[byteOffset] &= ~(tmpBitmap);
				byteOffset++;
				bitOffset = 0;
			}
		}
	}
	return TRUE;
}

// 화면 좌표가 화면 업데이트 비트맵 내부에서 시작하는 바이트 오프셋과 비트 오프셋 반환
BOOL kGetStartBitmap(const DRAWBITMAP *bitmap, int x, int y, int *byteOffset, int *bitOffset) {
	int width, xOffset, yOffset;

	// 비트맵 영역 내부에 좌표가 포함되지 않으면 안찾음
	if(kIsInRect(&(bitmap->area), x, y) == FALSE) return FALSE;

	// 업데이트 영역 내부 오프셋 계산
	xOffset = x - bitmap->area.x1;
	yOffset = y - bitmap->area.y1;
	// 업데이트할 영역 너비 계산
	width = kGetRectWidth(&(bitmap->area));

	// 바이트 오프셋은 X, Y가 그릴 영역에서 위치한 곳을 8(바이트당 8픽셀)로 나눠 계산
	*byteOffset = (yOffset * width + xOffset) >> 3;
	// 위에서 계산한 바이트 내 비트 오프셋은 8로 나눈 나머지로 계산
	*bitOffset = (yOffset * width + xOffset) & 0x7;

	return TRUE;
}

// 화면에 그릴 비트맵이 모두 0으로 설정되어 더 이상 업데이트 할 것이 없는지 반환
BOOL kIsBitmapFin(const DRAWBITMAP *bitmap) {
	int cnt, idx, width, height, i, size;
	BYTE *tmp;

	// 업데이트할 영역 너비와 높이 계산
	width = kGetRectWidth(&(bitmap->area));
	height = kGetRectHeight(&(bitmap->area));

	// 비트맵 바이트 수 계산
	size = width * height;
	cnt = size >> 3;

	// 8바이트씩 한 번에 비교
	tmp = bitmap->bitmap;
	for(i = 0; i < (cnt >> 3); i++) {
		if(*(QWORD*)(tmp) != 0) return FALSE;
		tmp += 8;
	}

	// 8바이트 단위로 떨어지지 않는 나머지 비교
	for(i = 0; i < (cnt & 0x7); i++) {
		if(*tmp != 0) return FALSE;
		tmp++;
	}

	// 전체 크기가 8로 나누어 떨어지지 않는다면 한 바이트가 가득 차지 않은 마지막 바이트가 있으므로 이를 검사
	idx = size & 0x7;
	for(i = 0; i < idx; i++) if(*tmp & (0x01 << i)) return FALSE;

	return TRUE;
}

// 윈도우 화면 버퍼에 버퍼 내용 한 번에 전송. X, Y좌표는 윈도우 내부 버퍼 기준
BOOL kBitBlt(QWORD winID, int x, int y, COLOR *buf, int width, int height) {
	WINDOW *win;
	RECT winArea, bufArea, crossArea;
	int winWidth, crossWidth, crossHeight, i, j, winPosition, bufPosition, startX, startY;

	// 윈도우 검색 동기화 처리
	win = kFindWinLock(winID);
	if(win == NULL) return FALSE;

	// 윈도우 시작 좌표를 0,0 으로 하는 윈도우 기준 좌표로 영역 반환
	kSetRectData(0, 0, win->area.x2 - win->area.x1, win->area.y2 - win->area.y1, &winArea);

	// 버퍼 영역 좌표 설정
	kSetRectData(x, y, x + width - 1, y + height - 1, &bufArea);

	// 윈도우 역역과 버퍼 영역 겹치는 좌표 계산
	if(kGetRectCross(&winArea, &bufArea, &crossArea) == FALSE) {
		kUnlock(&win->lock);
		return FALSE;
	}

	// 윈도우 영역과 겹치는 영역 너비, 높이 계산
	winWidth = kGetRectWidth(&winArea);
	crossWidth = kGetRectWidth(&crossArea);
	crossHeight = kGetRectHeight(&crossArea);

	// 이미지 출력 시작 위치 결정. 윈도우 시작 좌표 기준 출력 시작 좌표가 음수면 버퍼 이미지를 그만큼 cut
	if(x < 0) startX = x;
	else startX = 0;

	if(y < 0) startY = y;
	else startY = 0;

	// 너비, 높이 계산
	for(j = 0; j < crossHeight; j++) {
		// 화면 버퍼와 전송 버퍼 시작 오프셋 계산
		winPosition = (winWidth * (crossArea.y1 + j)) + crossArea.x1;
		bufPosition = (width * j + startY) + startX;

		// 한 줄씩 복사
		kMemCpy(win->winBuf + winPosition, buf + bufPosition, crossWidth * sizeof(COLOR));
	}

	// 동기화 처리
	kUnlock(&win->lock);
	return TRUE;
}

// 배경 화면 이미지 파일 저장된 데이터 버퍼와 버퍼 크기
extern unsigned char g_img[0];
extern unsigned int g_img_size;

// 배경 화면 윈도우에 배경 화면 이미지 출력
void kDrawBackgroundImg(void) {
	JPEG *jpg;
	COLOR *outBuf;
	WINDOWMANAGER *winManager;
	int i, j, midX, midY, monWidth, monHeight;

	// 윈도우 매니저 반환
	winManager = kGetWinManager();

	// JPEG 자료구조 할당
	jpg = (JPEG*)kAllocMem(sizeof(JPEG));

	// JPEG 초기화
	if(kJpgInit(jpg, g_img, g_img_size) == FALSE) return;

	// 디코딩할 메모리 할당
	outBuf = (COLOR*)kAllocMem(jpg->width * jpg->height * sizeof(COLOR));
	if(outBuf == NULL) {
		kFreeMem(jpg);
		return;
	}

	// 디코딩 처리
	if(kJpgDecode(jpg, outBuf) == FALSE) {
		// 디코딩 실패시 할당받은 버퍼 모두 반환
		kFreeMem(outBuf);
		kFreeMem(jpg);
		return;
	}

	// 디코딩 된 이미지 윈도우 화면 가운데 표시
	monWidth = kGetRectWidth(&(winManager->area));
	monHeight = kGetRectHeight(&(winManager->area));

	midX = (monWidth - jpg->width) / 2;
	midY = (monHeight - jpg->height) / 2;

	// 메모리에서 메모리로 한꺼번에 복사
	kBitBlt(winManager->backgroundID, midX, midY, outBuf, jpg->width, jpg->height);

	// 할당받았던 버퍼 모두 반환
	kFreeMem(outBuf);
	kFreeMem(jpg);
}
