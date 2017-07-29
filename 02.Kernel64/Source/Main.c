/*
 * Main.c
 *
 *  Created on: 2017. 7. 18.
 *      Author: Yummy
 */

#include <Types.h>
#include <Keyboard.h>
#include <Descriptor.h>
#include <PIC.h>
#include <Console.h>
#include <Shell.h>

/*	15장 진행 후 Console.c - printXY() 함수로 전부 교체
void setPrint(int x, int y, BYTE color, const char *str);
*/

// 아래 함수는 C언어 커널 시작 부분
void Main(void) {
	int x, y;

	// 콘솔 초기화 후 작업 수행
	initConsole(0, 5);

	printXY(57, 5, 0x1A, "[  Hit  ]");

	getCursor(&x, &y);	y++;
	printXY(7, 6, 0x1F, "GDT Initialize And Switch For IA-32e Mode ........");
	initGDTNTSS();
	loadGDTR(GDTR_STARTADDRESS);
	printXY(57, 6, 0x1A, "[  Hit  ]");

	printXY(7, 6, 0x1F, "TSS Segment Load .................................");
	loadTSS(GDT_TSSSEGMENT);
	printXY(57, 6, 0x1A, "[  Hit  ]");

	printXY(7, 6, 0x1F, "IDT Initialize ...................................");
	initIDT();
	loadIDTR(IDTR_STARTADDRESS);
	printXY(57, 6, 0x1A, "[  Hit  ]");

	printXY(7, 6, 0x1F, "Memory Size Check ................................[     MB]");
	chkTotalMemSize();
	setCursor(58, y++);
	printF("%d", getTotalMemSize());

	printXY(7, 7, 0x1F, "Keyboard Activate And Queue Initialize ...........");
	if(initKeyboard() == TRUE) {
		printXY(57, 7, 0x1A, "[  Hit  ]");
		changeLED(FALSE, FALSE, FALSE);
	} else {
		printXY(57, 7, 0x1C, "[  Err  ]");
		printXY(7, 8, 0x1C, "Keyboard is Not active ! Check your keyboard port ...");
		while(1);
	}	y++;

	printXY(7, 7, 0x1F, "PIC Controller And Interrupt Initialize ..........");
	initPIC();
	maskPIC(0);
	onInterrupt();
	printXY(57, 7, 0x1A, "[  Hit  ]");

	printXY(12, 8, 0x1B, "### Welcome to YummyHitOS !! Please enjoy this !! ###");
	setCursor(0, ++y);

/*	15장 진행 후 Shell.c - startShell() 함수로 교체
	while(1) if(rmKeyData(&data) == TRUE) if(data.flag & KEY_FLAGS_DOWN) {
		temp[0] = data.ascii;
		setPrint(i++, 11, 0x0D, temp);
		// 0이 입력되면 변수를 0으로 나누어 Divide Error 예외 발생
		if(temp[0] == '0') tmp = tmp / 0;
	}
*/
	startShell();
}

// 문자열을 X, Y 위치에 출력
/*	15장 진행 후 Console.c - printXY() 함수로 전부 교체
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
*/
