/*
 * Mouse.h
 *
 *  Created on: 2017. 9. 1.
 *      Author: Yummy
 */

#ifndef __MOUSE_H__
#define __MOUSE_H__

#include <Types.h>
#include <Synchronize.h>

#pragma once

// 매크로, 마우스 큐에 대한 것
#define MOUSE_MAXQUEUECNT	100

// 클릭 상태
#define MOUSE_LCLICK_ON		0x01
#define MOUSE_RCLICK_ON		0x02
#define MOUSE_WHEEL_ON		0x04

// 구조체, 1바이트로 정렬
#pragma pack(push, 1)

// PS2 마우스 패킷 저장 자료구조, 마우스 큐에 삽입하는 데이터
typedef struct mousePacket {
	// 클릭 상태와 X, Y 값에 관련된 플래그
	BYTE clickFlag;
	// X축 이동 거리
	BYTE xMove;
	// Y축 이동 거리
	BYTE yMove;
} MOUSEDATA;

#pragma pack(pop)

// 마우스 상태 관리 자료구조
typedef struct mouseManager {
	// 자료구조 동기화 스핀락
	SPINLOCK spinLock;
	// 현재 수신된 데이터 개수, 마우스 데이터가 세 개이므로 0 ~ 2 범위 반복
	int byteCnt;
	// 현재 수신 중인 마우스 데이터
	MOUSEDATA nowData;
} MOUSEMANAGER;

BOOL kInitMouse(void);
BOOL kGatherMouseData(BYTE data);
BOOL kActiveMouse(void);
void kOnMouseInterrupt(void);
BOOL kIsMouseData(void);
BOOL kRmMouseData(BYTE *stat, int *x, int *y);

#endif /*__MOUSE_H__*/
