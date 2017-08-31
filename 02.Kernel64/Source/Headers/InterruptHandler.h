/*
 * InterruptHandler.h
 *
 *  Created on: 2017. 7. 22.
 *      Author: Yummy
 */

#ifndef __INTERRUPTHANDLER_H__
#define __INTERRUPTHANDLER_H__

#include <Types.h>
#include <MP.h>

#pragma once

// 매크로, 인터럽트 벡터 최대 개수, ISA 버스의 인터럽트만 처리하므로 16
#define INTERRUPT_MAX_VECCNT		16
// 인터럽트 부하 분산 수행하는 시점. 인터럽트 처리 횟수가 10의 배수가 되는 시점
#define INTERRUPT_LOADBALANCING_DIV	10

// 구조체, 인터럽트에 관련된 정보를 저장하는 자료구조
typedef struct interruptManager {
	// 코어 별 인터럽트 처리 횟수, 최대 코어 개수 * 최대 인터럽트 벡터 개수로 정의된 2차원 배열
	QWORD coreInterruptCnt[MAXPROCESSORCNT][INTERRUPT_MAX_VECCNT];

	// 부하 분산 기능 사용 여부
	BOOL isLoadBalancing;

	// 대칭 IO 모드 사용 여부
	BOOL mode;
} INTERRUPTMANAGER;

void setSymmetricIOMode(BOOL mode);
void setInterruptLoadBalancing(BOOL isLoadBalancing);
void incInterruptCnt(int irq);
void sendEOI(int irq);
INTERRUPTMANAGER *getInterruptManager(void);
void procLoadBalancing(int irq);

void exceptionHandler(int vecNum, QWORD errCode);
void interruptHandler(int vecNum);
void keyboardHandler(int vecNum);
void timerHandler(int verNum);
void devFPUHandler(int vecNum);
void hardDiskHandler(int vecNum);
void mouseHandler(int vecNum);

#endif /*__INTERRUPTHANDLER_H__*/
