/*
 * List.c
 *
 *  Created on: 2017. 7. 31.
 *      Author: Yummy
 */

#include <List.h>

// 리스트 초기화
void initList(LIST *list) {
	list->itemCnt = 0;
	list->head = NULL;
	list->tail = NULL;
}

// 리스트에 포함된 아이템 수 반환
int getListCnt(const LIST *list) {
	return list->itemCnt;
}

// 리스트에 데이터 추가
void addListTail(LIST *list, void *item) {
	LISTLINK *link;

	// 다음 뎅터 어드레스를 NULL로 설정
	link = (LISTLINK*)item;
	link->next = NULL;

	// 리스트가 비어있으면 head와 tail을 추가한 데이터로 설정
	if(list->head == NULL) {
		list->head = item;
		list->tail = item;
		list->itemCnt = 1;
		return;
	}

	// 마지막 데이터의 LISTLINK 위치 구해서 다음 데이터를 추가한 데이터로 설정
	link = (LISTLINK*)list->tail;
	link->next = item;

	// 리스트의 마지막 데이터를 추가한 데이터로 변경
	list->tail = item;
	list->itemCnt++;
}

// 리스트 첫 부분에 데이터 추가
void addListHead(LIST *list, void *item) {
	LISTLINK *link;

	// 다음 데이터의 어드레스를 head로 설정
	link = (LISTLINK*)item;
	link->next = list->head;

	// 리스트가 비어있으면 head와 tail을 추가한 데이터로 설정
	if(list->head == NULL) {
		list->head = item;
		list->tail = item;
		list->itemCnt = 1;
		return;
	}

	// 리스트의 첫 번째 데이터를 추가한 데이터로 변경
	list->head = item;
	list->itemCnt++;
}

// 리스트에서 데이터 제거 후 데이터 포인터 반환
void *delList(LIST *list, QWORD id) {
	LISTLINK *link, *preLink;

	preLink = (LISTLINK*)list->head;
	for(link = preLink; link != NULL; link = link->next) {
		if(link->id == id) {
			if((link == list->head) && (link == list->tail)) {
				list->head = NULL;
				list->tail = NULL;
			} else if(link == list->head) list->head = link->next;
			else if(link == list->tail) list->tail = preLink;
			else preLink->next = link->next;

			list->itemCnt--;
			return link;
		}
		preLink = link;
	}
	return NULL;
}

void *delListHead(LIST *list) {
	LISTLINK *link;

	if(list->itemCnt == 0) return NULL;

	// 헤더 제거 후 반환
	link = (LISTLINK*)list->head;
	return delList(list, link->id);
}

// 리스트 마지막 데이터 제거 후 반환
void *delListTail(LIST *list) {
	LISTLINK *link;

	if(list->itemCnt == 0) return NULL;

	// 테일 제거 후 반환
	link = (LISTLINK*)list->tail;
	return delList(list, link->id);
}

// 리스트에서 아이템 검색
void *findList(const LIST *list, QWORD id) {
	LISTLINK *link;

	for(link = (LISTLINK*)list->head; link != NULL; link = link->next) if(link->id == id) return link;
	return NULL;
}

// 리스트 헤더 반환
void *getListHead(const LIST *list) {
	return list->head;
}

// 리스트 테일 반환
void *getListTail(const LIST *list) {
	return list->tail;
}

// 다음 아이템 반환
void *getNextList(const LIST *list, void *now) {
	LISTLINK *link;

	link = (LISTLINK*)now;
	return link->next;
}
