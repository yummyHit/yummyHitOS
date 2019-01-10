/*
 * interrupt.h
 *
 *  Created on: 2017. 7. 22.
 *      Author: Yummy
 */

#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#include <types.h>
#include <mp.h>

#pragma once

// 매크로, 인터럽트 벡터 최대 개수, ISA 버스의 인터럽트만 처리하므로 16
#define INTERRUPT_MAX_VECCNT		16
// 인터럽트 부하 분산 수행하는 시점. 인터럽트 처리 횟수가 10의 배수가 되는 시점
#define INTERRUPT_LOADBALANCING_DIV	10

#pragma pack(push, 1)

// 구조체, 인터럽트에 관련된 정보를 저장하는 자료구조
typedef struct kInterruptManager {
	// 코어 별 인터럽트 처리 횟수, 최대 코어 개수 * 최대 인터럽트 벡터 개수로 정의된 2차원 배열
	QWORD coreInterruptCnt[MAXPROCESSORCNT][INTERRUPT_MAX_VECCNT];

	// 부하 분산 기능 사용 여부
	BOOL isLoadBalancing;

	// 대칭 IO 모드 사용 여부
	BOOL mode;
} INTERRUPTMANAGER;

#pragma pack(pop)

void kSetSymmetricIOMode(BOOL mode);
void kSetInterruptLoadBalancing(BOOL isLoadBalancing);
void kIncInterruptCnt(int irq);
void kSendEOI(int irq);
INTERRUPTMANAGER *kGetInterruptManager(void);
void kProcLoadBalancing(int irq);

void kExceptionHandler(int vecNum, QWORD errCode);
void kInterruptHandler(int vecNum);
void kKeyboardHandler(int vecNum);
void kTimerHandler(int verNum);
void kDevFPUHandler(int vecNum);
void kHardDiskHandler(int vecNum);
void kMouseHandler(int vecNum);

#endif /*__INTERRUPT_H__*/
