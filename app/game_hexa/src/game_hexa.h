/*
 * game_hexa.h
 *
 *  Created on: 2019. 1. 17.
 *      Author: Yummy
 */

#ifndef __GAME_HEXA_H__
#define __GAME_HEXA_H__

#include "yummyHitLib.h"

#pragma once

// 게임판 너비와 높이
#define BOARD_WIDTH		8
#define BOARD_HEIGHT	12

// 블록 하나 크기
#define BLOCK_SIZE		32
// 움직이는 블록 수
#define BLOCK_CNT		3
// 빈 블록 값
#define EMPTY_BLOCK		0
// 지울 블록 값
#define ERASE_BLOCK		0xFF
// 블록 종류
#define BLOCK_ITEMS		5

// 게임 정보 영역 높이
#define INFO_HEIGHT		20

// 윈도우 너비와 높이
#define HEXA_WIDTH		(BOARD_WIDTH * BLOCK_SIZE)
#define HEXA_HEIGHT		(WINDOW_TITLE_HEIGHT + INFO_HEIGHT + BOARD_HEIGHT * BLOCK_SIZE)

#pragma pack(push, 1)

// 물방울 정보 저장하는 구조체
typedef struct HexaInfo {
	// 블록 종류에 따른 색깔(내부 색/테두리 색)
	COLOR color[BLOCK_ITEMS + 1];
	COLOR edge[BLOCK_ITEMS + 1];

	// 현재 블록 위치
	int x;
	int y;

	// 게임판 고정된 블록 상태 관리 영역
	BYTE board[BOARD_HEIGHT][BOARD_WIDTH];

	// 게임판 고정된 블록 중 삭제해야 할 블록 관리 영역
	BYTE erase[BOARD_HEIGHT][BOARD_WIDTH];

	// 현재 움직이는 블록 구성 저장 영역
	BYTE block[BLOCK_CNT];

	// 게임 시작 여부
	BOOL start;
	// 유저 점수
	QWORD score;
	// 게임 레벨
	QWORD level;
} HEXAINFO;

#pragma pack(pop)

void hexaInit(void);
void makeHexa(void);
BOOL ismovable(int x, int y);
BOOL islockable(int x, int y);

BOOL horizonBlock(void);
BOOL verticalBlock(void);
BOOL diagonalBlock(void);
void eraseBlock(void);
void fillBlock(void);
void delBlock(QWORD winID);

void drawInfo(QWORD winID);
void drawGameArea(QWORD winID);

#endif /*__GAME_HEXA_H__*/
