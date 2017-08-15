/*
 * Util.c
 *
 *  Created on: 2017. 7. 22.
 *      Author: Yummy
 */

#include <Util.h>
#include <AsmUtil.h>
#include <stdarg.h>

volatile QWORD g_tickCnt = 0;

void memSet(void *dest, BYTE data, int size) {
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

inline void memSetWord(void *dest, WORD data, int size) {
	int i, remainOffset;
	QWORD _data = 0;

	for(i = 0; i < 4; i++) _data = (_data << 16) | data;
	for(i = 0; i < (size / 4); i++) ((QWORD*)dest)[i] = _data;

	remainOffset = i * 4;
	for(i = 0; i < (size % 4); i++) ((WORD*)dest)[remainOffset++] = data;
}

int memCpy(void *dest, const void *src, int size) {
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

int memCmp(const void *dest, const void *src, int size) {
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

// RFLAGS 레지스터의 인터럽트 플래그를 변경하고 이전 인터럽트 플래그의 상태를 반환
BOOL setInterruptFlag(BOOL _onInterrupt) {
	QWORD rflags;

	// 이전 RFLAGS 레지스터 값을 읽은 뒤 인터럽트 가능|불가능 처리
	rflags = readRFLAGS();
	if(_onInterrupt == TRUE) onInterrupt();
	else offInterrupt();

	// 이전 RFLAGS 레지스터의 IF 비트(비트 9)를 확인해 이전 인터럽트 상태 반환
	if(rflags & 0x0200) return TRUE;
	return FALSE;
}

// 문자열 길이 반환
int strLen(const char *buf) {
	int i;

	for(i = 0;;i++) if(buf[i] == '\0') break;
	return i;
}

// 메모리 총 크기(MB)
static gs_totalMemSize = 0;

// 64MB 이상 위치부터 크기 체크, 최초 부팅 과정에서 한 번만 호출해야 함
void chkTotalMemSize(void) {
	DWORD *nowAddr, preValue;

	// 64MB(0x4000000)부터 4MB 단위로 검사 시작
	nowAddr = (DWORD*)0x4000000;
	while(1) {
		// 이전 메모리에 있던 값 저장
		preValue = *nowAddr;

		// 0x12345678을 써서 읽었을 때 문제가 없는 곳까지를 유효 메모리 영역으로 인정
		*nowAddr = 0x12345678;
		if(*nowAddr != 0x12345678) break;

		// 이전 메모리 값으로 복원
		*nowAddr = preValue;
		nowAddr += (0x400000 / 4);
	}
	// 체크가 성공한 어드레스를 1MB로 나누어 MB단위 계산
	gs_totalMemSize = (QWORD)nowAddr / 0x100000;
}

// Memory 크기 반환
QWORD getTotalMemSize(void) {
	return gs_totalMemSize;
}

// atoi() 함수 내부 구현
long aToi(const char *buf, int radix) {
	long ret;

	switch(radix) {
	case 16:	// 16진수
		ret = hexToQW(buf);
		break;
	case 10:
	default:
		ret = decimalToL(buf);
		break;
	}
	return ret;
}

// 16진수 문자열을 QWORD로 반환
QWORD hexToQW(const char *buf) {
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
long decimalToL(const char *buf) {
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
int iToa(long v, char *buf, int radix) {
	int ret;

	switch(radix) {
	case 16:
		ret = hexToStr(v, buf);
		break;
	case 10:
	default:
		ret = decimalToStr(v, buf);
		break;
	}
	return ret;
}

// 16진수 값을 문자열로 변환
int hexToStr(QWORD v, char *buf) {
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
	revStr(buf);
	return i;
}

// 10진수 값을 문자열로 변환
int decimalToStr(long v, char *buf) {
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
	if(buf[0] == '-') revStr(&(buf[1]));
	else revStr(buf);
	return i;
}

// 문자열 순서 뒤집음
void revStr(char *buf) {
	int len, i;
	char tmp;

	// 문자열 가운데를 중심 좌|우를 바꿔 순서 뒤집음
	len = strLen(buf);
	for(i = 0; i < len / 2; i++) {
		tmp = buf[i];
		buf[i] = buf[len - 1 - i];
		buf[len - 1 - i] = tmp;
	}
}

// sprintf() 함수 내부 구현
int sprintF(char *buf, const char *format, ...) {
	va_list v;
	int ret;

	// 가변 인자를 꺼내 vsprintf() 함수에 넘김
	va_start(v, format);
	ret = vsprintF(buf, format, v);
	va_end(v);

	return ret;
}

// vsprintf() 함수 내부 구현. 버퍼에 포맷 문자열에 따라 데이터 복사
int vsprintF(char *buf, const char *format, va_list v) {
	QWORD i, j, k, qwV;
	int bufIdx = 0, fmLen, cpLen, iV;
	char *cpStr;
	double dV;

	// 포맷 문자열 길이를 읽어 문자열 길이만큼 데이터를 출력 버퍼에 출력
	fmLen = strLen(format);
	for(i = 0; i < fmLen; i++) {
		if(format[i] == '%') {
			i++;
			switch(format[i]) {
			case 's':
				// 가변 인자에 들어있는 파라미터를 문자열 타입으로 변환
				cpStr = (char*)(va_arg(v, char*));
				cpLen = strLen(cpStr);
				// 문자열 길이만큼 출력 버퍼로 복사 후 출력한 길이만큼 버퍼 인덱스 이동
				memCpy(buf + bufIdx, cpStr, cpLen);
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
				bufIdx += iToa(iV, buf + bufIdx, 10);
				break;
			case 'x':
			case 'X':
				// 가변 인자에 들어있는 파라미터를 DWORD타입으로 변환해 출력 버퍼에 복사, 그 길이만큼 버퍼 인덱스 이동
				qwV = (DWORD)(va_arg(v, DWORD)) & 0xFFFFFFFF;
				bufIdx += iToa(qwV, buf + bufIdx, 16);
				break;
			case 'q':
			case 'Q':
			case 'p':
				// 가변 인자에 들어있는 파라미터를 QWORD타입으로 변환해 출력 버퍼에 복사, 그 길이만큼 버퍼 인덱스 이동
				qwV = (QWORD)(va_arg(v, QWORD));
				bufIdx += iToa(qwV, buf + bufIdx, 16);
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
				revStr(buf + bufIdx);
				bufIdx += 3 + k;
				break;
			case 'o':
				// 가변 인자에 들어있는 파라미터를 정수타입으로 변환 후 출력 버퍼에 복사, 그 길이만큼 버퍼 인덱스 이동
				iV = (int)(va_arg(v, int));
				bufIdx += iToa(iV, buf + bufIdx, 8);
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

// Tick Count 반환
QWORD getTickCnt(void) {
	return g_tickCnt;
}

// ms 동안 대기
void _sleep(QWORD ms) {
	QWORD lastTickCnt;

	lastTickCnt = g_tickCnt;

	while((g_tickCnt - lastTickCnt) <= ms) schedule();
}
