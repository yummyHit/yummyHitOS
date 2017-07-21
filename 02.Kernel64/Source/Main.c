/*
 * Main.c
 *
 *  Created on: 2017. 7. 18.
 *      Author: Yummy
 */

#include "Types.h"
#include "Keyboard.h"
#include "Descriptor.h"
#include "PIC.h"

// 함수 선언
void setPrint(int x, int y, BYTE color, const char *str);

// 아래 함수는 C언어 커널 시작 부분
void Main(void) {
	char temp[2] = {0,};
	BYTE flag, tmp;
	int i = 0;

	setPrint(53, 8, 0x0A, "[  Hit  ]");

	setPrint(3, 9, 0x0F, "GDT Initailiza And Switch For IA-32e Mode.........");
	initGDTNTSS();
	loadGDTR(GDTR_STARTADDRESS);
	setPrint(53, 9, 0x0A, "[  Hit  ]");

	setPrint(3, 9, 0x0F, "TSS Segment Load..................................");
	loadTSS(GDT_TSSSEGMENT);
	setPrint(53, 9, 0x0A, "[  Hit  ]");

	setPrint(3, 9, 0x0F, "IDT Initialize....................................");
	initIDT();
	loadIDTR(IDTR_STARTADDRESS);
	setPrint(53, 9, 0x0A, "[  Hit  ]");

	setPrint(3, 9, 0x0F, "Keyboard Activate.................................");
	if(activeKeyboard() == TRUE) {
		setPrint(53, 9, 0x0A, "[  Hit  ]");
		changeLED(FALSE, FALSE, FALSE);
	} else {
		setPrint(53, 9, 0x0C, "[  Err  ]");
		while(1);
	}

	setPrint(3, 10, 0x0F, "PIC Controller And Interrupt Initialize...........");
	initPIC();
	maskPIC(0);
	onInterrupt();
	setPrint(53, 10, 0x0A, "[  Hit  ]");

	while(1) if(outputBufCheck() == TRUE) {
			tmp = getScanCode();
			if(convertCode(tmp, &(temp[0]), &flag) == TRUE) if(flag & KEY_FLAGS_DOWN) {
				setPrint(i++, 11, 0x0D, temp);
				// 0이 입력되면 변수를 0으로 나누어 Divide Error 예외 발생
				if(temp[0] == '0') tmp = tmp / 0;
			}
	}
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

