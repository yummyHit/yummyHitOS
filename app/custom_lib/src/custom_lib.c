#include "types.h"
#include "custom_lib.h"
#include <stdarg.h>

// 메모리를 특정 값으로 채움
void memset(void *dest, BYTE data, int size) {
	int i, remainOffset;
	QWORD _data = 0;
	// 8바이트 데이터 채움
	for(i = 0; i < 8; i++) _data = (_data << 8) | data;
	// 8바이트씩 먼저 채움
	for(i = 0; i < (size / 8); i++) ((QWORD*)dest)[i] = _data;
	// 8바이트씩 채우고 남은 부분 마무리
	remainOffset = i * 8;
	for(i = 0; i < (size % 8); i++) ((char*)dest)[remainOffset++] = data;
}

// 메모리 복사
int memcpy(void *dest, const void *src, int size) {
	int i, remainOffset;
	// 8바이트씩 먼저 복사
	for(i = 0; i < (size / 8); i++) ((QWORD*)dest)[i] = ((QWORD*)src)[i];
	// 8바이트씩 채우고 남은 부분 마무리
	remainOffset = i * 8;
	for(i = 0; i < (size % 8); i++) {
		((char*)dest)[remainOffset] = ((char*)src)[remainOffset];
		remainOffset++;
	}
	return size;
}

// 메모리 비교
int memcmp(const void *dest, const void *src, int size) {
	int i, j, remainOffset;
	char value;
	QWORD _value;
	// 8바이트씩 먼저 비교
	for(i = 0; i < (size / 8); i++) {
		_value = ((QWORD*)dest)[i] - ((QWORD*)src)[i];
		if(_value != 0) for(i = 0; i < 8; i++) if(((_value >> (i * 8)) & 0xFF) != 0) return (_value >> (i * 8)) & 0xFF;
	}
	// 8바이트씩 채우고 남은 부분 마무리
	remainOffset = i * 8;
	for(i = 0; i < (size % 8); i++) {
		value = ((char*)dest)[remainOffset] - ((char*)src)[remainOffset];
		if(value != 0) return value;
		remainOffset++;
	}
	return 0;
}

// 문자열 복사
int strcpy(char *dest, const char *src) {
	int i;
	for(i = 0; src[i] != 0; i++) dest[i] = src[i];
	return i;
}

// 문자열 비교
int strcmp(char *dest, const char *src) {
	int i;
	for(i = 0; (dest[i] != 0) && (src[i] != 0); i++) if((dest[i] - src[i]) != 0) break;
	return (dest[i] - src[i]);
}

// 문자열 길이
int strlen(const char *buf) {
	int i;
	for(i = 0;;i++) if(buf[i] == '\0') break;
	return i;
}

// atoi() 함수 내부 구현
long atoi(const char *buf, int radix) {
	long ret;

	switch(radix) {
	case 16:	// 16진수
		ret = hex2qword(buf);
		break;
	case 10:
	default:
		ret = int2long(buf);
		break;
	}
	return ret;
}

// 16진수 문자열을 QWORD로 반환
QWORD hex2qword(const char *buf) {
	QWORD v = 0;
	int i;

	// 문자열을 돌면서 차례로 변환
	for(i = 0; buf[i] != '\0'; i++) {
		v *= 16;
		if(('A' <= buf[i]) && (buf[i] <= 'Z')) v += (buf[i] - 'A') + 10;
		else if(('a' <= buf[i]) && (buf[i] <= 'z')) v += (buf[i] - 'a') + 10;
		else v += buf[i] - '0';
	}
	return v;
}

// 10진수 문자열을 Long으로 변환
long int2long(const char *buf) {
	long v = 0;
	int i;

	// 음수면 -를 제외하고 나머지를 먼저 Long으로 변환
	if(buf[0] == '-') i = 1;
	else i = 0;

	// 문자열을 돌면서 차례로 변환
	for(; buf[i] != '\0'; i++) {
		v *= 10;
		v += buf[i] - '0';
	}

	// 음수면 - 추가
	if(buf[0] == '-') v = -v;
	return v;
}

// itoa() 함수 내부 구현
int itoa(long v, char *buf, int radix) {
	int ret;

	switch(radix) {
	case 16:
		ret = hex2str(v, buf);
		break;
	case 10:
	default:
		ret = int2str(v, buf);
		break;
	}
	return ret;
}

// 16진수 값을 문자열로 변환
int hex2str(QWORD v, char *buf) {
	QWORD i, nowValue;

	// 0이 들어오면 처리
	if(v == 0) {
		buf[0] = '0';
		buf[1] = '\0';
		return 1;
	}

	// 버퍼에 1의 자리부터 16, 256, ...로 16의 배수로 숫자 삽입
	for(i = 0; v > 0; i++) {
		nowValue = v % 16;
		if(nowValue >= 10) buf[i] = 'A' + (nowValue - 10);
		else buf[i] = '0' + nowValue;
		v = v / 16;
	}
	buf[i] = '\0';

	// 버퍼에 들어있는 문자열을 뒤집어 ..., 256, 16, 1로 자리 순서 변경
	strrev(buf);
	return i;
}

// 10진수 값을 문자열로 변환
int int2str(long v, char *buf) {
	long i;

	// 0이 들어오면 처리
	if(v == 0) {
		buf[0] = '0';
		buf[1] = '\0';
		return 1;
	}

	// 만약 음수면 출력 버퍼에 '-' 추가 후 양수로 변환
	if(v < 0) {
		i = 1;
		buf[0] = '-';
		v = -v;
	} else i = 0;

	// 버퍼에 1의 자리부터 10, 100, ...로 10의 배수로 숫자 삽입
	for(; v > 0; i++) {
		buf[i] = '0' + v % 10;
		v = v / 10;
	}
	buf[i] = '\0';

	// 버퍼에 들어 있는 문자열을 뒤집어 ..., 100, 10, 1로 자리 순서 변경
	if(buf[0] == '-') strrev(&(buf[1]));
	else strrev(buf);
	return i;
}

// 문자열 순서 뒤집음
void strrev(char *buf) {
	int len, i;
	char tmp;

	// 문자열 가운데를 중심 좌|우를 바꿔 순서 뒤집음
	len = strlen(buf);
	for(i = 0; i < len / 2; i++) {
		tmp = buf[i];
		buf[i] = buf[len - 1 - i];
		buf[len - 1 - i] = tmp;
	}
}

// sprintf() 함수 내부 구현
int sprintf(char *buf, const char *format, ...) {
	va_list v;
	int ret;

	// 가변 인자를 꺼내 vsprintf() 함수에 넘김
	va_start(v, format);
	ret = vsprintf(buf, format, v);
	va_end(v);

	return ret;
}

// vsprintf() 함수 내부 구현. 버퍼에 포맷 문자열에 따라 데이터 복사
int vsprintf(char *buf, const char *format, va_list v) {
	QWORD i, j, k, qwV;
	int bufIdx = 0, fmLen, cpLen, iV;
	char *cpStr;
	double dV;

	// 포맷 문자열 길이를 읽어 문자열 길이만큼 데이터를 출력 버퍼에 출력
	fmLen = strlen(format);
	for(i = 0; i < fmLen; i++) {
		if(format[i] == '%') {
			i++;
			switch(format[i]) {
			case 's':
				// 가변 인자에 들어있는 파라미터를 문자열 타입으로 변환
				cpStr = (char*)(va_arg(v, char*));
				cpLen = strlen(cpStr);
				// 문자열 길이만큼 출력 버퍼로 복사 후 출력한 길이만큼 버퍼 인덱스 이동
				memcpy(buf + bufIdx, cpStr, cpLen);
				bufIdx += cpLen;
				break;
			case 'c':
				// 가변 인자에 들어있는 파라미터를 문자타입으로 변환해 출력 버퍼에 복사 후 버퍼 인덱스 1 이동
				buf[bufIdx] = (char)(va_arg(v, int));
				bufIdx++;
				break;
			case 'd':
			case 'i':
				// 가변 인자에 들어있는 파라미터를 정수타입으로 변환 후 출력 버퍼에 복사, 그 길이만큼 버퍼 인덱스 이동
				iV = (int)(va_arg(v, int));
				bufIdx += itoa(iV, buf + bufIdx, 10);
				break;
			case 'x':
			case 'X':
				// 가변 인자에 들어있는 파라미터를 DWORD타입으로 변환해 출력 버퍼에 복사, 그 길이만큼 버퍼 인덱스 이동
				qwV = (DWORD)(va_arg(v, DWORD)) & 0xFFFFFFFF;
				bufIdx += itoa(qwV, buf + bufIdx, 16);
				break;
			case 'q':
			case 'Q':
			case 'p':
				// 가변 인자에 들어있는 파라미터를 QWORD타입으로 변환해 출력 버퍼에 복사, 그 길이만큼 버퍼 인덱스 이동
				qwV = (QWORD)(va_arg(v, QWORD));
				bufIdx += itoa(qwV, buf + bufIdx, 16);
				break;
				// 소수점 둘째 자리까지 실수 출력
			case 'f':
				dV = (double)(va_arg(v, double));
				// 셋째 자리에서 반올림 처리
				dV += 0.005;
				// 소수점 둘째 자리부터 차례로 저장해 버퍼를 뒤집음
				buf[bufIdx] = '0' + (QWORD)(dV * 100) % 10;
				buf[bufIdx + 1] = '0' + (QWORD)(dV * 10) % 10;
				buf[bufIdx + 2] = '.';
				for(k = 0;; k++) {
					// 정수 부분이 0이면 종료
					if(((QWORD)dV == 0) && (k != 0)) break;
					buf[bufIdx + 3 + k] = '0' + ((QWORD)dV % 10);
					dV = dV / 10;
				}
				buf[bufIdx + 3 + k] = '\0';
				// 값이 저장된 길이만큼 뒤집고 길이를 증가시킴
				strrev(buf + bufIdx);
				bufIdx += 3 + k;
				break;
			case 'o':
				// 가변 인자에 들어있는 파라미터를 정수타입으로 변환 후 출력 버퍼에 복사, 그 길이만큼 버퍼 인덱스 이동
				iV = (int)(va_arg(v, int));
				bufIdx += itoa(iV, buf + bufIdx, 8);
				break;
			default:
				buf[bufIdx] = format[i];
				bufIdx++;
				break;
			}
		} else {
			// 일반 문자열 처리. 문자 그대로 출력 후 버퍼 인덱스 1 이동
			buf[bufIdx] = format[i];
			bufIdx++;
		}
	}

	// NULL을 추가해 완전한 문자열로 만들고, 출력 문자 길이 반환
	buf[bufIdx] = '\0';
	return bufIdx;
}

// printf() 함수 내부 구현
int printf(const char *format, ...) {
	va_list v;
	char buf[1024];
	int nextPrintOffset, tmp = 0;

	// 가변 인자 리스트를 사용해 vsprintf()로 처리
	va_start(v, format);
	tmp = vsprintf(buf, format, v);
	va_end(v);

	// 포맷 문자열을 화면에 출력
	nextPrintOffset = printCS(buf);

	// 커서 위치 업데이트
	setCursor(nextPrintOffset % CONSOLE_WIDTH, nextPrintOffset / CONSOLE_WIDTH);

	return tmp;
}

// 난수 발생을 위한 변수
static volatile QWORD gs_matrixValue = 0;

// 난수초기값(Seed) 설정
void srand(QWORD seed) {
	gs_matrixValue = seed;
}

// 임의의 난수 반환
QWORD rand(void) {
	gs_matrixValue = (gs_matrixValue * 412153 + 5571031) >> 16;
	return gs_matrixValue;
}

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

// 전체 화면을 기준으로 X, Y좌표를 윈도우 내부 좌표로 변환
BOOL monitorToPoint(QWORD id, const POINT *xy, POINT *xyWin) {
	RECT area;

	// 윈도우 영역 반환
	if(getWinArea(id, &area) == FALSE) return FALSE;

	xyWin->x = xy->x - area.x1;
	xyWin->y = xy->y - area.y1;
	return TRUE;
}

// 윈도우 내부를 기준으로 X, Y좌표를 화면 좌표로 변환
BOOL pointToMon(QWORD id, const POINT *xy, POINT *xyMon) {
	RECT area;

	// 윈도우 영역 반환
	if(getWinArea(id, &area) == FALSE) return FALSE;

	xyMon->x = xy->x + area.x1;
	xyMon->y = xy->y + area.y1;
	return TRUE;
}

// 전체 화면을 기준으로 사각형 좌표를 윈도우 내부 좌표로 변환
BOOL monitorToRect(QWORD id, const RECT *area, RECT *areaWin) {
	RECT winArea;

	// 윈도우 영역 반환
	if(getWinArea(id, &winArea) == FALSE) return FALSE;

	areaWin->x1 = area->x1 - winArea.x1;
	areaWin->y1 = area->y1 - winArea.y1;
	areaWin->x2 = area->x2 - winArea.x1;
	areaWin->y2 = area->y2 - winArea.y1;
	return TRUE;
}

// 윈도우 내부를 기준으로 사각형 좌표를 화면 좌표로 변환
BOOL rectToMon(QWORD id, const RECT *area, RECT *areaMon) {
	RECT winArea;

	// 윈도우 영역 반환
	if(getWinArea(id, &winArea) == FALSE) return FALSE;

	areaMon->x1 = area->x1 + winArea.x1;
	areaMon->y1 = area->y1 + winArea.y1;
	areaMon->x2 = area->x2 + winArea.x1;
	areaMon->y2 = area->y2 + winArea.y1;
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

// 마우스 이벤트 자료구조 설정
BOOL setMouseEvent(QWORD id, QWORD type, int x, int y, BYTE btnStat, EVENT *event) {
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
		if(monitorToPoint(id, &xy, &xyWin) == FALSE) return FALSE;

		// 이벤트 타입 설정
		event->type = type;
		// 윈도우 ID 설정
		event->mouseEvent.id = id;
		// 마우스 버튼 상태 설정
		event->mouseEvent.btnStat = btnStat;
		// 마우스 커서 좌표를 윈도우 내부 좌표로 변환한 값 설정
		memcpy(&(event->mouseEvent.point), &xyWin, sizeof(POINT));
		break;
	default:
		return FALSE;
		break;
	}
	return TRUE;
}

// 윈도우 이벤트 자료구조 설정
BOOL setWinEvent(QWORD id, QWORD type, EVENT *event) {
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
		if(getWinArea(id, &area) == FALSE) return FALSE;

		// 윈도우 현재 좌표 설정
		memcpy(&(event->winEvent.area), &area, sizeof(RECT));
		break;
	default:
		return FALSE;
		break;
	}
	return TRUE;
}

// 키 이벤트 자료구조 설정
void setKeyEvent(QWORD win, const KEYDATA *key, EVENT *event) {
	// 눌림 또는 떨어짐 처리
	if(key->flag & KEY_FLAGS_DOWN) event->type = EVENT_KEY_DOWN;
	else event->type = EVENT_KEY_UP;

	// 키의 각 정보 설정
	event->keyEvent.ascii = key->ascii;
	event->keyEvent.scanCode = key->scanCode;
	event->keyEvent.flag = key->flag;
}
