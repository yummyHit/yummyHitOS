/*
 * InterruptHandler.c
 *
 *  Created on: 2017. 7. 22.
 *      Author: Yummy
 */

#include "InterruptHandler.h"
#include "PIC.h"
#include "Keyboard.h"

// 공통으로 사용하는 예외 핸들러
void exceptionHandler(int vecNum, QWORD errCode) {
	char buf[3] = {0,};

	// 인터럽트 벡터를 화면 오른쪽 위 2자리 정수로 출력
	buf[0] = '0' + vecNum / 10;
	buf[1] = '0' + vecNum % 10;

	setPrint(3, 3, 0x0F, "=============================================================           ");
	setPrint(3, 4, 0x0F, "                                                                        ");
	setPrint(3, 5, 0x0B, "                Interrupt Handler Execute                               ");
	setPrint(3, 6, 0x0F, "                                                                        ");
	setPrint(3, 7, 0x0E, "                It is Exception : ");
	setPrint(37, 7, 0x0C, buf);
	setPrint(39, 7, 0x0F, "                                 ");
	setPrint(3, 8, 0x0F, "                                                                        ");
	setPrint(3, 9, 0x0F, "=============================================================           ");

	while(1);
}

// 공통으로 사용하는 인터럽트 핸들러
void interruptHandler(int vecNum) {
	char buf[] = "[INT:  , ]";
	static int ls_interruptCnt = 0;

	// 인터럽트가 발생했음을 알리기 위해 메시지 출력하는 부분. 화면 오른쪽 위 2자리 정수로 출력
	buf[5] = '0' + vecNum / 10;
	buf[6] = '0' + vecNum % 10;
	// 발생 횟수 출력
	buf[8] = '0' + ls_interruptCnt;
	ls_interruptCnt = (ls_interruptCnt + 1) % 10;
	setPrint(70, 0, buf);

	// EOI 전송
	sendEOI(vecNum - PIC_IRQSTARTVECTOR);
}

// 키보드 인터럽트 핸들러
void keyboardHandler(int vecNum) {
	char buf[] = "[INT:  , ]";
	static int ls_keyboardCnt = 0;
	BYTE tmp;

	// 인터럽트가 발생했음을 알리기 위해 메시지 출력하는 부분. 화면 오른쪽 위 2자리 정수로 출력
	buf[5] = '0' + vecNum / 10;
	buf[6] = '0' + vecNum % 10;
	// 발생 횟수 출력
	buf[8] = '0' + ls_keyboardCnt;
	ls_keyboardCnt = (ls_keyboardCnt + 1) % 10;
	setPrint(0, 0, buf);

	// 키보드 컨트롤러에서 데이터 읽고 ASCII로 변환해 큐에 삽입
	if(outputBufCheck() == TRUE) {
		tmp = getCode();
		convertNPutCode(tmp);
	}

	// EOI 전송
	sendEOI(vecNum - PIC_IRQSTARTVECTOR);
}
