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

void kInitList(LIST *list);
int kGetListCnt(const LIST *list);
void kAddListTail(LIST *list, void *item);
void kAddListHead(LIST *list, void *item);
void *kDelList(LIST *list, QWORD id);
void *kDelListHead(LIST *list);
void *kDelListTail(LIST *list);
void *kFindList(const LIST *list, QWORD id);
void *kGetListHead(const LIST *list);
void *kGetListTail(const LIST *list);
void *kGetNextList(const LIST *list, void *now);

#endif /*__LIST_H__*/
