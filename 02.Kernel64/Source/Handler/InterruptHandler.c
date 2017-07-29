/*
 * InterruptHandler.c
 *
 *  Created on: 2017. 7. 22.
 *      Author: Yummy
 */

#include <InterruptHandler.h>
#include <PIC.h>
#include <Keyboard.h>
#include <Console.h>

void printDebug(int vec, int cnt, int handler) {
	char buf[] = "[INT:  ,  ]";

	// 인터럽트가 발생했음을 알리기 위해 메시지 출력하는 부분. 화면 오른쪽 위 2자리 정수로 출력
	buf[5] = '0' + vec / 10;
	buf[6] = '0' + vec % 10;
	// 발생 횟수 출력
	buf[8] = '0' + cnt / 10;
	buf[9] = '0' + cnt % 10;
	if(handler == 1) printXY(69, 0, 0x1E, buf);
	else printXY(0, 0, 0x1E, buf);
	printXY(34, 0, 0xE5, " YummyHitOS ");
}

// 공통으로 사용하는 예외 핸들러
void exceptionHandler(int vecNum, QWORD errCode) {
	char buf[3] = {0,};

	// 인터럽트 벡터를 화면 오른쪽 위 2자리 정수로 출력
	buf[0] = '0' + vecNum / 10;
	buf[1] = '0' + vecNum % 10;

	printXY(3, 3, 0x1F, "=============================================================           ");
	printXY(3, 4, 0x1F, "                                                                        ");
	printXY(3, 5, 0x1B, "                Interrupt Handler Execute                               ");
	printXY(3, 6, 0x1F, "                                                                        ");
	printXY(3, 7, 0x1E, "                It is Exception : ");
	printXY(37, 7, 0x1C, buf);
	printXY(39, 7, 0x1F, "                                 ");
	printXY(3, 8, 0x1F, "                                                                        ");
	printXY(3, 9, 0x1F, "=============================================================           ");

	while(1);
}

// 공통으로 사용하는 인터럽트 핸들러
void interruptHandler(int vecNum) {
	static int ls_interruptCnt = 0;

	ls_interruptCnt = (ls_interruptCnt + 1) % 100;
	printDebug(vecNum, ls_interruptCnt, 1);

	// EOI 전송
	sendEOI(vecNum - PIC_IRQSTARTVECTOR);
}

// 키보드 인터럽트 핸들러
void keyboardHandler(int vecNum) {
	static int ls_keyboardCnt = 0;
	BYTE tmp;

	ls_keyboardCnt = (ls_keyboardCnt + 1) % 100;
	printDebug(vecNum, ls_keyboardCnt, 2);

	// 키보드 컨트롤러에서 데이터 읽고 ASCII로 변환해 큐에 삽입
	if(outputBufCheck() == TRUE) {
		tmp = getCode();
		convertNPutCode(tmp);
	}

	// EOI 전송
	sendEOI(vecNum - PIC_IRQSTARTVECTOR);
}
