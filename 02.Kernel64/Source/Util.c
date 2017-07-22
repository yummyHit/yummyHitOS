/*
 * Util.c
 *
 *  Created on: 2017. 7. 22.
 *      Author: Yummy
 */

#include "Util.h"
#include "Port.h"

// 메모리를 특정 값으로 채움
void memSet(void *dest, BYTE data, int size) {
	int i;
	for(i = 0; i < size; i++) ((char*)dest)[i] = data;
}

// 메모리 복사
int memCpy(void *dest, const void *src, int size) {
	int i;
	for(i = 0; i < size; i++) ((char*)dest)[i] = ((char*)src)[i];
	return size;
}

// 메모리 비교
int memCmp(const void *dest, const void *src, int size) {
	int i;
	char tmp;
	for(i = 0; i < size; i++) {
		tmp = ((char*)dest)[i] - ((char*)src)[i];
		if(tmp != 0) return (int)tmp;
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
