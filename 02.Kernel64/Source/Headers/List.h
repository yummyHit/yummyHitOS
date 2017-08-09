/*
 * List.h
 *
 *  Created on: 2017. 7. 31.
 *      Author: Yummy
 */

#ifndef __LIST_H__
#define __LIST_H__

#include <Types.h>

#pragma once
// 구조체, 1바이트로 정렬
#pragma pack(push, 1)

// 데이터 연결 자료구조. 반드시 데이터 가장 앞부분에 위치해야 함.
typedef struct listLink {
	void *next;
	QWORD id;
} LISTLINK;

// 리스트 관리 자료구조
typedef struct listManager {
	// 리스트 데이터 수
	int itemCnt;
	// 리스트 첫 번째와 마지막 데이터 주소
	void *head;
	void *tail;
} LIST;

#pragma pack(pop)

void initList(LIST *list);
int getListCnt(const LIST *list);
void addListTail(LIST *list, void *item);
void addListHead(LIST *list, void *item);
void *delList(LIST *list, QWORD id);
void *delListHead(LIST *list);
void *delListTail(LIST *list);
void *findList(const LIST *list, QWORD id);
void *getListHead(const LIST *list);
void *getListTail(const LIST *list);
void *getNextList(const LIST *list, void *now);

#endif /*__LIST_H__*/
