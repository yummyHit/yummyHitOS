/*
 * Main.c
 *
 *  Created on: 2017. 7. 18.
 *      Author: Yummy
 */

#include "Types.h"

// 함수 선언
void kPrintString(int iX, int iY, const char *pcString);

// 아래 함수는 C언어 커널 시작 부분
void Main(void) {
	kPrintString(0, 12, "Switch To IA-32e Mode Success!!");
	kPrintString(0, 13, "IA-32e C Language Kernel Start.........[Pass]");
}

// 문자열을 X, Y 위치에 출력
void kPrintString(int iX, int iY, const char *pcString) {
	CHARACTER *pstScreen = (CHARACTER*) 0xB8000;
	int i;

	// X, Y 좌표를 이용해 문자열을 출력할 어드레스 계산
	pstScreen += (iY * 80) + iX;

	// NULL이 나올 때까지 문자열 출력
	for(i = 0; pcString[i] != 0; i++) pstScreen[i].bCharactor = pcString[i];
}

