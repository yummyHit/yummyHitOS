/*
 * Util.c
 *
 *  Created on: 2017. 7. 22.
 *      Author: Yummy
 */

#include "Util.h"

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
