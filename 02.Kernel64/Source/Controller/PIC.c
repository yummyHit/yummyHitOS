/*
 * PIC.c
 *
 *  Created on: 2017. 7. 22.
 *      Author: Yummy
 */

#include <PIC.h>

// PIC 초기화
void initPIC(void) {
	// ICW1(포트 0x20), IC4 비트(비트 0) = 1
	outByte(PIC_MASTER_PORT_A, 0x11);

	// ICW2(포트 0x21), 인터럽트 벡터(0x20)
	outByte(PIC_MASTER_PORT_B, PIC_IRQSTARTVECTOR);

	// ICW3(포트 0x21), 슬레이브 PIC 컨트롤러가 연결 위치(비트로 표현)
	// 마스터 PIC 컨트롤러의 2번 핀에 연결되어 있으므로 0x04(비트 2)로 설정
	outByte(PIC_MASTER_PORT_B, 0x04);

	// ICW4(포트 0x21), uPM 비트(비트 0) = 1
	outByte(PIC_MASTER_PORT_B, 0x01);

	/* 슬레이브 PIC 컨트롤러 초기화 */
	// ICW1(포트 0xA0), IC4 비트(비트 0) = 1
	outByte(PIC_SLAVE_PORT_A, 0x11);

	// ICW2(포트 0xA1), 인터럽트 벡터(0x20 + 8)
	outByte(PIC_SLAVE_PORT_B, PIC_IRQSTARTVECTOR + 8);

	// ICW3(포트 0xA1), 마스터 PIC 컨트롤러에 연결된 위치(정수로 표현)
	// 마스터 PIC 컨트롤러의 2번 핀에 연결되어 있으므로 0x02로 설정
	outByte(PIC_SLAVE_PORT_B, 0x02);

	// ICW4(포트 0xA1), uPM 비트(비트 0) = 1
	outByte(PIC_SLAVE_PORT_B, 0x01);
}

// 인터럽트를 마스크하여 해당 인터럽트가 발생하지 않게 처리
void maskPIC(WORD mask) {
	// 마스터 PIC 컨트롤러에 IMR 설정. OCW1(포트 0x21), IRQ 0 ~ 7
	outByte(PIC_MASTER_PORT_B, (BYTE)mask);

	// 슬레이브 PIC 컨트롤러에 IMR 설정. OCW1(포트 0xA1), IRQ 8 ~ 15
	outByte(PIC_SLAVE_PORT_B, (BYTE)(mask >> 8));
}

// 인터럽트 처리가 완료되었음을 전송(EOI). 마스터 PIC 컨트롤러는 마스터 PIC 컨트롤러에, 슬레이브 PIC 컨트롤러는 두 PIC 컨트롤러에 모두 전송
void sendEOI(int num) {
	// 마스터 PIC 컨트롤러에 EOI 전송. OCW2(포트 0x20), EOI 비트(비트 5) = 1
	outByte(PIC_MASTER_PORT_A, 0x20);

	// 슬레이브 PIC 컨트롤러의 인터럽트인 경우 슬레이브 PIC 컨트롤러에게도 EOI 전송. OCW2(포트 0xA0), EOI 비트(비트 5) = 1
	if(num >= 8) outByte(PIC_SLAVE_PORT_A, 0x20);
}
