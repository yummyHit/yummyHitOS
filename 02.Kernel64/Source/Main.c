/*
 * Main.c
 *
 *  Created on: 2017. 7. 18.
 *      Author: Yummy
 */

#include "Types.h"

// 함수 선언
void setPrint(int x, int y, BYTE color, const char *str);

// 아래 함수는 C언어 커널 시작 부분
void Main(void) {
	setPrint(53, 11, 0x0A, "[  Hit  ]");
	setPrint(3, 13, 0x0F, "IA-32e C Language Kernel Start....................");
	while(1);
}

// 문자열을 X, Y 위치에 출력
void setPrint(int x, int y, BYTE color, const char *str) {
	CHARACTER *p = (CHARACTER*) 0xB8000;
	int i;

	// X, Y 좌표를 이용해 문자열을 출력할 어드레스 계산
	p += (y * 80) + x;

	// NULL이 나올 때까지 문자열 출력
	for(i = 0; str[i] != 0; i++) {
		p[i].character = str[i];
		p[i].color = color;
	}
}

