/*
 * PIT.c
 *
 *  Created on: 2017. 7. 29.
 *      Author: Yummy
 */

#include <PIT.h>

// PIT 초기화
void initPIT(WORD cnt, BOOL term) {
	// PIT 컨트롤 레지스터(포트 0x43)에 값 초기화로 카운트를 멈추고 모드 0에 바이너리 카운터 설정
	outByte(PIT_PORT_CTRL, PIT_COUNTER_ZERO_ONCE);

	// 만약 일정 주기로 반복하는 타이머라면 모드 2로 설정
	if(term == TRUE) outByte(PIT_PORT_CTRL, PIT_COUNTER_ZERO_TERM); // PIT 컨트롤 레지스터(포트 0x43)에 모드 2에 바이너리 카운터 설정

	// 카운터 0(포트 0x40)에 LSB->MSB 순서로 카운터 초기 값 설정
	outByte(PIT_PORT_COUNTER_ZERO, cnt);
	outByte(PIT_PORT_COUNTER_ZERO, cnt >> 8);
}

// 카운터 0의 현재 값 반환
WORD readCntZero(void) {
	BYTE high, low;
	WORD tmp = 0;

	// PIT 컨트롤 레지스터(포트 0x43)에 래치 커맨드 전송해 카운터 0에 있는 현재 값 읽음
	outByte(PIT_PORT_CTRL, PIT_COUNTER_ZERO_LATCH);

	// 카운터 0(포트 0x40)에서 LSB->MSB 순서로 카운터 값 읽음
	low = inByte(PIT_PORT_COUNTER_ZERO);
	high = inByte(PIT_PORT_COUNTER_ZERO);

	// 읽은 값을 16비트로 합해 반환
	tmp = high;
	tmp = (tmp << 8) | low;
	return tmp;
}

// 카운터 0을 직접 설정해 일정 시간 이상 대기. 함수 호출시 PIT 컨트롤러 설정이 변하여 이후 PIT 컨트롤러 재설정
// 정확히 측정하려면 함수 사용 전 인터럽트를 비활성화 해야 함. 약 50ms까지 측정 가능
void waitPIT(WORD cnt) {
	WORD last, now;

	// PIT 컨트롤러를 0x0000 ~ 0xFFFF까지 반복하여 카운팅하도록 설정
	initPIT(0, TRUE);

	// 지금부터 cnt 이상 증가할 때까지 대기
	last = readCntZero();
	while(1) {
		// 현재 카운터 0값 반환
		now = readCntZero();
		if(((last - now) & 0xFFFF) >= cnt) break;
	}
}
