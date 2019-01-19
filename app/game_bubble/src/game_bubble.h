/*
 * game_bubble.h
 *
 *  Created on: 2019. 1. 17.
 *      Author: Yummy
 */

#ifndef __GAME_BUBBLE_H__
#define __GAME_BUBBLE_H__

#include "yummyHitLib.h"

#pragma once

// 물방울 최대 개수
#define MAX_BUBBLE_CNT		50
// 물방울 반지름
#define RADIUS				16
// 물방울 기본 속도
#define DEFAULT_SPEED		3
// 플레이어 최대 생명
#define MAX_LIFE			20
// 물방울 생성 시간
#define DEFAULT_TIME		50

// 윈도우 너비와 높이
#define BUBBLE_WIDTH		250
#define BUBBLE_HEIGHT		350

// 게임 정보 영역 높이
#define INFO_HEIGHT			20

#pragma pack(push, 1)

// 물방울 정보 저장하는 구조체
typedef struct Bubble {
	QWORD x;
	QWORD y;

	// 떨어진느 속도(Y축 변화량)
	QWORD speed;

	// 물방울 색
	COLOR color;

	// 살아있는지 여부
	BOOL alive;
} BUBBLE;

// 게임 정보 저장하는 구조체
typedef struct BubbleInfo {
	// 물방울 정보 저장 버퍼
	BUBBLE *buf;

	// 살아있는 물방울 수
	int cnt;

	// 플레이어 생명
	int life;

	// 유저 점수
	QWORD score;

	// 게임 시작 여부
	BOOL start;
} BUBBLEINFO;

#pragma pack(pop)

BOOL bubbleInit(void);
BOOL makeBubble(void);
void moveBubble(void);
void delBubble(POINT *mouse);
void drawInfo(QWORD winID);
void drawGameArea(QWORD winID, POINT *mouse);

#endif /*__GAME_BUBBLE_H__*/
