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

// (x, y)가 사각형 영역 안에 있는지 여부
BOOL isInRect(const RECT *area, int x, int y) {
	// 화면에 표시되는 영역을 벗어났다면 안그림
	if((x < area->x1) || (area->x2 < x) || (y < area->y1) || (area->y2 < y)) return FALSE;
	return TRUE;
}

// 사각형 너비 반환
int getRectWidth(const RECT *area) {
	int width;

	width = area->x2 - area->x1 + 1;

	if(width < 0) return -width;
	return width;
}

// 사각형 높이 반환
int getRectHeight(const RECT *area) {
	int height;

	height = area->y2 - area->y1 + 1;

	if(height < 0) return -height;
	return height;
}

// 두 개 사각형이 교차하는가 판단
BOOL isRectCross(const RECT *area1, const RECT *area2) {
	// 영역 1의 끝점이 영역 2의 시작점보다 작거나 영역 1의 시작점이 영역 2의 끝점보다 크면 겹치는 부분 없음
	if((area1->x1 > area2->x2) || (area1->x2 < area2->x1) || (area1->y1 > area2->y2) || (area1->y2 < area2->y1)) return FALSE;
	return TRUE;
}

// 영역 1과 영역 2 겹치는 영역 반환
BOOL getRectCross(const RECT *area1, const RECT *area2, RECT *inter) {
	int xMax, xMin, yMax, yMin;

	// X축 시작점은 두 점 중 큰 것 찾음
	xMax = _MAX(area1->x1, area2->x1);
	// X축 끝점은 두 점 중 작은 것 찾음
	xMin = _MIN(area1->x2, area2->x2);
	// 계산한 시작점 위치가 끝점 위치보다 크면 두 사각형은 안겹침
	if(xMin < xMax) return FALSE;

	// Y축 시작점은 두 점 중 큰 것 찾음
	yMax = _MAX(area1->y1, area2->y1);
	// Y축 끝점은 두 점 중 작은 것 찾음
	yMin = _MIN(area1->y2, area2->y2);
	// 계산한 시작점 위치가 끝점 위치보다 크면 두 사각형은 안겹침
	if(yMin < yMax) return FALSE;

	// 겹치는 영역 정보 저장
	inter->x1 = xMax;
	inter->y1 = yMax;
	inter->x2 = xMin;
	inter->y2 = yMin;

	return TRUE;
}

// 사각형 자료구조 채움. x1과 x2, y1과 y2를 비교해 x1 < x2, y1 < y2가 되도록 저장
void setRectData(int x1, int y1, int x2, int y2, RECT *rect) {
	// x1 < x2가 되도록 RECT 자료구조에 X좌표 설정
	if(x1 < x2) {
		rect->x1 = x1;
		rect->x2 = x2;
	} else {
		rect->x1 = x2;
		rect->x2 = x1;
	}

	// y1 < y2가 되도록 RECT 자료구조에 Y좌표 설정
	if(y1 < y2) {
		rect->y1 = y1;
		rect->y2 = y2;
	} else {
		rect->y1 = y2;
		rect->y2 = y1;
	}
}

// 점 그리기
void inDrawPixel(const RECT *area, COLOR *addr, int x, int y, COLOR color) {
	int width;

	// 클리핑 처리. 화면에 표시되는 영역 벗어나면 안그림
	if(isInRect(area, x, y) == FALSE) return;

	// 출력할 메모리 영역 너비 구함
	width = getRectWidth(area);

	// 픽셀 오프셋으로 계산해 픽셀 출력
	*(addr + (width * y) + x) = color;
}

// 직선 그리기에 이용되는 반복 구간 함수
void lineLoop(const RECT *area, COLOR *addr, int dCenter, int dOther, int center, int center1, int center2, int other, int other1, int cNext, int oNext, int errA, int errB, COLOR color, int chk) {
	// 기울기로 픽셀마다 더해줄 오차, 중심축이 아닌 반대 축의 변화량 2배. 시프트 연산으로 * 2 대체
	errB = dOther << 1;
	other = other1;
	for(center = center1; center != center2; center += cNext) {
		// 점 그리기
		if(chk == 0) inDrawPixel(area, addr, center, other, color);
		else inDrawPixel(area, addr, other, center, color);

		// 오차 누적
		errA += errB;

		// 누적된 오차가 중심축 변화량보다 크면 위에 점을 선택, 오차를 위에 점 기준으로 갱신
		if(errA >= dCenter) {
			other += oNext;
			// 중심축 변화량의 2배 감소. 시프트 연산으로 * 2 대체
			errA -= dCenter << 1;
		}
	}
	// dC == dC2인 최종 위치에 점 그리기
	if(chk == 0) inDrawPixel(area, addr, center, other, color);
	else inDrawPixel(area, addr, other, center, color);
}

// 직선 그리기
void inDrawLine(const RECT *area, COLOR *addr, int x1, int y1, int x2, int y2, COLOR color) {
	int dx, dy, err = 0, dErr, x, y, nextX, nextY;
	RECT lineArea;

	// 클리핑 처리. 직선이 그려지는 영역과 메모리 영역이 겹치지 않으면 안그림
	setRectData(x1, y1, x2, y2, &lineArea);
	if(isRectCross(area, &lineArea) == FALSE) return;

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
	if(dx > dy) lineLoop(area, addr, dx, dy, x, x1, x2, y, y1, nextX, nextY, err, dErr, color, 0);
	else lineLoop(area, addr, dy, dx, y, y1, y2, x, x1, nextY, nextX, err, dErr, color, 1);
}

// 사각형 그리기
void inDrawRect(const RECT *area, COLOR *addr, int x1, int y1, int x2, int y2, COLOR color, BOOL fill) {
	int width, tmp, y, areaWidth;
	RECT rect, crossArea;

	// 채움 여부에 따라 코드 분리
	if(fill == FALSE) {
		// 네 점을 이웃한 것끼리 직선 연결
		inDrawLine(area, addr, x1, y1, x2, y1, color);
		inDrawLine(area, addr, x1, y1, x1, y2, color);
		inDrawLine(area, addr, x2, y1, x2, y2, color);
		inDrawLine(area, addr, x1, y2, x2, y2, color);
	} else {
		// 출력할 사각형 정보 RECT 자료구조에 저장
		setRectData(x1, y1, x2, y2, &rect);

		// 출력할 메모리 영역과 사각형 영역이 겹치는 부분을 계산해 클리핑 처리
		if(getRectCross(area, &rect, &crossArea) == FALSE) return;

		// 클리핑된 사각형 너비 계산
		width = getRectWidth(&crossArea);

		// 출력할 메모리 영역 너비 계산
		areaWidth = getRectWidth(area);

		// 출력할 메모리 어드레스 시작 위치 계산. 파라미터로 전달된 것이 아닌 클리핑 처리된 사각형 기준으로 그림
		addr += crossArea.y1 * areaWidth + crossArea.x1;

		// 루프를 돌며 각 Y축마다 값 채움
		for(y = crossArea.y1; y < crossArea.y2; y++) {
			// 메모리에 사각형 너비만큼 픽셀 채움
			memSetWord(addr, color, width);

			// 출력할 비디오 메모리 어드레스 갱신. X, Y좌표로 매번 비디오 메모리 어드레스 계산을 피해 X축 해상도를 이용, 다음 라인 y좌표 어드레스 계산
			addr += areaWidth;
		}

		// 메모리에 사각형 너비만큼 픽셀 채우고 마지막 줄 출력
		memSetWord(addr, color, width);
	}
}

// 원 그리기
void inDrawCircle(const RECT *area, COLOR *addr, int x, int y, int rad, COLOR color, BOOL fill) {
	int xCircle, yCircle, distance;

	// 반지름이 0보다 작으면 그릴 필요 없음
	if(rad < 0) return;

	// (0, R)인 좌표에서 시작
	yCircle = rad;

	// 채움 여부에 따라 시작점 그림
	if(fill == FALSE) {
		// 시작점은 네 접점 모두 그림
		inDrawPixel(area, addr, 0 + x, rad + y, color);
		inDrawPixel(area, addr, 0 + x, -rad + y, color);
		inDrawPixel(area, addr, rad + x, 0 + y, color);
		inDrawPixel(area, addr, -rad + x, 0 + y, color);
	} else {
		// 시작 직선은 X축과 Y축 모두 그림
		inDrawLine(area, addr, 0 + x, rad + y, 0 + x, -rad + y, color);
		inDrawLine(area, addr, rad + x, 0 + y, -rad + x, 0 + y, color);
	}

	// 최초 시작점의 중심점과 원의 거리
	distance = -rad;

	// 원 그리기
	for(xCircle = 1; xCircle <= yCircle; xCircle++) {
		// 원에서 떨어진 거리 계산. 시프트 연산으로 * 2 대체
		distance += (xCircle << 1) - 1;

		// 중심점이 원의 외부에 있으면 아래의 점 선택
		if(distance >= 0) {
			yCircle--;

			// 새로운 점에서 다시 원과 거리 계산. 시프트 연산으로 * 2 대체
			distance += (-yCircle << 1) + 2;
		}

		// 채움 여부에 따라 그림
		if(fill == FALSE) {
			// 8방향 모두 점 그림
			inDrawPixel(area, addr, xCircle + x, yCircle + y, color);
			inDrawPixel(area, addr, xCircle + x, -yCircle + y, color);
			inDrawPixel(area, addr, -xCircle + x, yCircle + y, color);
			inDrawPixel(area, addr, -xCircle + x, -yCircle + y, color);
			inDrawPixel(area, addr, yCircle + x, xCircle + y, color);
			inDrawPixel(area, addr, yCircle + x, -xCircle + y, color);
			inDrawPixel(area, addr, -yCircle + x, xCircle + y, color);
			inDrawPixel(area, addr, -yCircle + x, -xCircle + y, color);
		} else {
			// 대칭되는 점을 찾아 X축에 평행한 직선을 그어 채워진 원 그림. 평행선 그리는 것은 사각형 그리기 함수로 처리
			inDrawRect(area, addr, -xCircle + x, yCircle + y, xCircle + x, yCircle + y, color, TRUE);
			inDrawRect(area, addr, -xCircle + x, -yCircle + y, xCircle + x, -yCircle + y, color, TRUE);
			inDrawRect(area, addr, -yCircle + x, xCircle + y, yCircle + x, xCircle + y, color, TRUE);
			inDrawRect(area, addr, -yCircle + x, -xCircle + y, yCircle + x, -xCircle + y, color, TRUE);
		}
	}
}

// 문자 출력
void inDrawText(const RECT *area, COLOR *addr, int x, int y, COLOR text, COLOR background, const char *buf, int len) {
	int nowX, nowY, i, j, k, bitStartIdx, areaWidth, startY, startX, crossWidth, crossHeight;
	BYTE bit, bitMask;
	RECT fontArea, crossArea;

	// 문자 출력 X좌표
	nowX = x;

	// 메모리 영역 너비 계산
	areaWidth = getRectWidth(area);

	// 문자 개수만큼 반복
	for(k = 0; k < len; k++) {
		// 문자 출력 위치의 Y좌표 구함
		nowY = y * areaWidth;

		// 현재 폰트를 표시하는 영역 RECT 자료구조에 설정
		setRectData(nowX, y, nowX + FONT_ENG_WIDTH - 1, y + FONT_ENG_HEIGHT - 1, &fontArea);

		// 현재 그려야 할 문자가 메모리 영역과 겹치는 부분이 없으면 다음 문자로 이동
		if(getRectCross(area, &fontArea, &crossArea) == FALSE) {
			// 문자 하나를 뛰어넘었으니 폰트 너비만큼 X좌표 이동해 다음 문자 출력
			nowX += FONT_ENG_WIDTH;
			continue;
		}

		// 비트맵 폰트 데이터에서 문자 비트맵 시작 위치 계산. 1바이트 * FONT_HEIGHT로 구성되어 있으니 문자 비트맵 위치는 아래처럼 계산
		bitStartIdx = buf[k] * FONT_ENG_HEIGHT;

		// 문자 출력할 영역과 메모리 영역이 겹치는 부분을 이용해 X, Y 와프셋과 출력 너비, 높이 계산
		startX = crossArea.x1 - nowX;
		startY = crossArea.y1 - y;
		crossWidth = getRectWidth(&crossArea);
		crossHeight = getRectHeight(&crossArea);

		// 출력에서 제외된 Y오프셋 만큼 비트맵 데이터 제외
		bitStartIdx += startY;

		// 문자 출력
		for(j = startY; j < crossHeight; j++) {
			// 이 라인에서 출력할 폰트 비트맵과 비트 오프셋 계산
			bit = g_engFont[bitStartIdx++];
			bitMask = 0x01 << (FONT_ENG_WIDTH - 1 - startX);

			// 겹치는 영역의 X오프셋부터 너비만큼 현재 라인에 출력, 비트가 설정 되어 있으면 화면에 문자색, 아니면 배경색 표시
			for(i = startX; i < crossWidth; i++) {
				if(bit & bitMask) addr[nowY + nowX + i] = text;
				else addr[nowY + nowX + i] = background;

				bitMask = bitMask >> 1;
			}

			// 다음 라인으로 이동해야 하니 현재 Y좌표에 화면 너비만큼 더해줌
			nowY += areaWidth;
		}

		// 문자 하나를 다 출력했으면 폰트 너비만큼 X좌표 이동해 다음 문자 출력
		nowX += FONT_ENG_WIDTH;
	}
}
