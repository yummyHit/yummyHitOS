/*
 * BaseGraph.c
 *
 *  Created on: 2017. 8. 31.
 *      Author: Yummy
 */

#include <BaseGraph.h>
#include <VBE.h>
#include <Font.h>
#include <Util.h>

// 점 그리기
inline void drawPixel(int x, int y, COLOR color) {
	VBEMODEINFO *mode;

	// 모드 정보 블록 반환
	mode = getVBEModeInfo();

	// Physical Address를 COLOR 타입으로 하면 픽셀 오프셋으로 계산 가능
	*(((COLOR*)(QWORD)mode->linearBaseAddr) + mode->xPixel * y + x) = color;
}

// 직선 그리기에 이용되는 반복 구간 함수
void lineLoop(int dCenter, int dOther, int dC, int dC1, int dC2, int dO, int dO1, int cNext, int oNext, int errA, int errB, COLOR color, int chk) {
	// 기울기로 픽셀마다 더해줄 오차, 중심축이 아닌 반대 축의 변화량 2배. 시프트 연산으로 * 2 대체
	errB = dOther << 1;
	dO = dO1;
	for(dC = dC1; dC != dC2; dC += cNext) {
		// 점 그리기
		if(!chk) drawPixel(dC, dO, color);
		else drawPixel(dO, dC, color);

		// 오차 누적
		errA += errB;

		// 누적된 오차가 중심축 변화량보다 크면 위에 점을 선택, 오차를 위에 점 기준으로 갱신
		if(errA >= dCenter) {
			dO += oNext;
			// 중심축 변화량의 2배 감소. 시프트 연산으로 * 2 대체
			errA -= dCenter << 1;
		}
	}
	// dC == dC2인 최종 위치에 점 그리기
	if(!chk) drawPixel(dC, dO, color);
	else drawPixel(dO, dC, color);
}

// 직선 그리기
void drawLine(int x1, int y1, int x2, int y2, COLOR color) {
	int dx, dy, err = 0, _err, x, y, nextX, nextY;

	// 변화량 계산
	dx = x2 - x1;
	dy = y2 - y1;

	// X축 변화량에 따라 X축 증감 방향 계산
	if(dx < 0) {
		dx = -dx;
		nextX = -1;
	} else nextX = 1;

	// Y축 변화량에 따라 Y축 증감 방향 계산
	if(dy < 0) {
		dy = -dy;
		nextY = -1;
	} else nextY = 1;

	// X축 변화량이 Y축 변화량보다 크다면 X축을 중심으로, Y축 변화량이 X축 변화량보다 크다면 Y축을 중심으로 직선 그림
	if(dx > dy) lineLoop(dx, dy, x, x1, x2, y, y1, nextX, nextY, err, _err, color, 0);
	else lineLoop(dy, dx, y, y1, y2, x, x1, nextY, nextX, err, _err, color, 1);
}

// 사각형 그리기
void drawRect(int x1, int y1, int x2, int y2, COLOR color, BOOL fill) {
	int width, tmp, y, nextY;
	COLOR *memAddr;
	VBEMODEINFO *mode;

	// 채움 여부에 따라 코드 분리
	if(fill == FALSE) {
		// 네 점을 이웃한 것끼리 직선 연결
		drawLine(x1, y1, x2, y1, color);
		drawLine(x1, y1, x1, y2, color);
		drawLine(x2, y1, x2, y2, color);
		drawLine(x1, y2, x2, y2, color);
	} else {
		// VBE 모드 정보 블록 반환
		mode = getVBEModeInfo();

		// 비디오 메모리 어드레스 계산
		memAddr = (COLOR*)((QWORD)mode->linearBaseAddr);

		// memSetWord() 함수는 X 값 증가 방향으로 데이터를 채움. x1, x2 두 점을 비교해 교환
		if(x2 < x1) {
			tmp = x1;
			x1 = x2;
			x2 = tmp;

			tmp = y1;
			y1 = y2;
			y2 = tmp;
		}

		// 사각형 X축 길이 계산
		width = x2 - x1 + 1;

		// y1, y2 두 점 비교해 매 회 증가시킬 Y값 저장
		if(y1 <= y2) nextY = 1;
		else nextY = -1;

		// 출력 시작할 비디오 메모리 어드레스 계산
		memAddr += y1 * mode->xPixel + x1;

		// 루프를 돌며 Y축마다 값 채움
		for(y = y1; y != y2; y += nextY) {
			// 비디오 메모리 사각형 너비만큼 출력
			memSetWord(memAddr, color, width);

			// 출력할 비디오 메모리 어드레스 갱신. X, Y좌표로 매번 비디오 메모리 어드레스 계산을 피하기 위해 X축 해상도 이용해 Y 어드레스 계산
			if(nextY >= 0) memAddr += mode->xPixel;
			else memAddr -= mode->xPixel;
		}

		// 비디오 메모리 사각형 너비만큼 출력 후 마지막 줄 출력
		memSetWord(memAddr, color, width);
	}
}

// 원 그리기
void drawCircle(int _x, int _y, int rad, COLOR color, BOOL fill) {
	int x, y, distance;

	// 반지름이 0보다 작으면 그릴 필요 없음
	if(rad < 0) return;

	// (0, R)인 좌표에서 시작
	y = rad;

	// 채움 여부에 따라 시작점 그림
	if(fill == FALSE) {
		// 시작점은 네 접점 모두 그림
		drawPixel(0 + _x, rad + _y, color);
		drawPixel(0 + _x, -rad + _y, color);
		drawPixel(rad + _x, 0 + _y, color);
		drawPixel(-rad + _x, 0 + _y, color);
	} else {
		// 시작 직선은 X축과 Y축 모두 그림
		drawLine(0 + _x, rad + _y, 0 + _x, -rad + _y, color);
		drawLine(rad + _x, 0 + _y, -rad + _x, 0 + _y, color);
	}

	// 최초 시작점의 중심점과 원의 거리
	distance = -rad;

	// 원 그리기
	for(x = 1; x <= y; x++) {
		// 원에서 떨어진 거리 계산. 시프트 연산으로 * 2 대체
		distance += (x << 1) - 1;

		// 중심점이 원의 외부에 있으면 아래의 점 선택
		if(distance >= 0) {
			y--;

			// 새로운 점에서 다시 원과 거리 계산. 시프트 연산으로 * 2 대체
			distance += (-y << 1) + 2;
		}

		// 채움 여부에 따라 그림
		if(fill == FALSE) {
			// 8방향 모두 점 그림
			drawPixel(x + _x, y + _y, color);
			drawPixel(x + _x, -y + _y, color);
			drawPixel(-x + _x, y + _y, color);
			drawPixel(-x + _x, -y + _y, color);
			drawPixel(y + _x, x + _y, color);
			drawPixel(y + _x, -x + _y, color);
			drawPixel(-y + _x, x + _y, color);
			drawPixel(-y + _x, -x + _y, color);
		} else {
			// 대칭되는 점을 찾아 X축에 평행한 직선을 그어 채워진 원 그림. 평행선 그리는 것은 사각형 그리기 함수로 처리
			drawRect(-x + _x, y + _y, x + _x, y + _y, color, TRUE);
			drawRect(-x + _x, -y + _y, x + _x, -y + _y, color, TRUE);
			drawRect(-y + _x, x + _y, y + _x, x + _y, color, TRUE);
			drawRect(-y + _x, -x + _y, y + _x, -x + _y, color, TRUE);
		}
	}
}

// 문자 출력
void drawText(int x, int y, COLOR text, COLOR background, const char *buf, int len) {
	int nowX, nowY, i, j, k, bitStartIdx;
	BYTE bitMask;
	VBEMODEINFO *mode;
	COLOR *memAddr;

	// VBE 모드 정보 블록 반환
	mode = getVBEModeInfo();

	// 비디오 메모리 어드레스 계산
	memAddr = (COLOR*)((QWORD)mode->linearBaseAddr);

	// 문자를 출력하는 X좌표
	nowX = x;

	// 문자 개수만큼 반복
	for(k = 0; k < len; k++) {
		// 문자 출력 위치의 Y좌표 구함
		nowY = y * mode->xPixel;

		// 비트맵 폰트 데이터에서 문자 비트맵 시작 위치 계산. 1바이트 * FONT_HEIGHT로 구성되어 있으니 문자 비트맵 위치는 아래처럼 계산
		bitStartIdx = buf[k] * FONT_ENG_HEIGHT;

		// 문자 출력
		for(j = 0; j < FONT_ENG_HEIGHT; j++) {
			// 이 라인에서 출력할 폰트 비트맵
			bitMask = g_engFont[bitStartIdx++];

			// 현재 라인 출력. 비트가 설정되어 있으면 화면에 문자색 표시, 아니면 배경색 표시
			for(i = 0; i < FONT_ENG_WIDTH; i++)
				if(bitMask & (0x01 << (FONT_ENG_WIDTH - i - 1))) memAddr[nowY + nowX + i] = text;
				else memAddr[nowY + nowX + i] = background;

			// 다음 라인으로 이동해야 하니 현재 Y좌표에 화면 너비만큼 더해줌
			nowY += mode->xPixel;
		}

		// 문자 하나를 다 출력했으면 폰트 너비만큼 X좌표 이동해 다음 문자 출력
		nowX += FONT_ENG_WIDTH;
	}
}
