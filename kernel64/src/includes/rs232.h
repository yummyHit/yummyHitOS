/*
 * rs232.h
 *
 *  Created on: 2017. 8. 16.
 *      Author: Yummy
 */

#ifndef __RS232_H__
#define __RS232_H__

#include <types.h>
#include <queue.h>
#include <sync.h>

#pragma once

// 매크로, 시리얼 포트의 IO 포트 기준 주소
#define SERIAL_PORT_COM1		0x3F8
#define SERIAL_PORT_COM2		0x2F8
#define SERIAL_PORT_COM3		0x3E8
#define SERIAL_PORT_COM4		0x2E8

// 각 레지스터 오프셋
#define SERIAL_PORT_IDX_RECVBUF		0x00
#define SERIAL_PORT_IDX_SENDBUF		0x00
#define SERIAL_PORT_IDX_ONINTERRUPT	0x01
#define SERIAL_PORT_IDX_DIVLATCH_LSB	0x00
#define SERIAL_PORT_IDX_DIVLATCH_MSB	0x01
#define SERIAL_PORT_IDX_IDINTERRUPT	0x02
#define SERIAL_PORT_IDX_FIFOCTRL	0x02
#define SERIAL_PORT_IDX_LINECTRL	0x03
#define SERIAL_PORT_IDX_MODEMCTRL	0x04
#define SERIAL_PORT_IDX_LINESTAT	0x05
#define SERIAL_PORT_IDX_MODEMSTAT	0x06

// 인터럽트 활성화 레지스터 관련
#define SERIAL_ONINTERRUPT_RXBF		0x01
#define SERIAL_ONINTERRUPT_TBE		0x02
#define SERIAL_ONINTERRUPT_LINESTAT	0x04
#define SERIAL_ONINTERRUPT_DELTASTAT	0x08

// FIFO 제어 레지스터 관련
#define SERIAL_FIFOCTRL_ON		0x01
#define SERIAL_FIFOCTRL_CLEARRECV	0x02
#define SERIAL_FIFOCTRL_CLEARSEND	0x04
#define SERIAL_FIFOCTRL_ONDMA		0x08
#define SERIAL_FIFOCTRL_1BYTE		0x00
#define SERIAL_FIFOCTRL_4BYTE		0x40
#define SERIAL_FIFOCTRL_8BYTE		0x80
#define SERIAL_FIFOCTRL_14BYTE		0xC0

// 라인 제어 레지스터 관련
#define SERIAL_LINECTRL_DATA		0x03
#define SERIAL_LINECTRL_STOP		0x00
#define SERIAL_LINECTRL_NOPARITY	0x00
#define SERIAL_LINECTRL_ODDPARITY	0x08
#define SERIAL_LINECTRL_EVENPARITY	0x18
#define SERIAL_LINECTRL_MARKPARITY	0x28
#define SERIAL_LINECTRL_SPACEPARITY	0x38
#define SERIAL_LINECTRL_DLAB		0x80

// 라인 상태 레지스터 관련
#define SERIAL_LINESTAT_RXRD		0x01
#define SERIAL_LINESTAT_OVRE		0x02
#define SERIAL_LINESTAT_PARE		0x04
#define SERIAL_LINESTAT_FRME		0x08
#define SERIAL_LINESTAT_BREK		0x10
#define SERIAL_LINESTAT_TBE		0x20
#define SERIAL_LINESTAT_TXE		0x40
#define SERIAL_LINESTAT_RXCHARE		0x80

// 제수 래치 레지스터 관련
#define SERIAL_DIVLATCH_115200		1
#define SERIAL_DIVLATCH_57600		2
#define SERIAL_DIVLATCH_38400		3
#define SERIAL_DIVLATCH_19200		6
#define SERIAL_DIVLATCH_9600		12
#define SERIAL_DIVLATCH_4800		24
#define SERIAL_DIVLATCH_2400		48

// FIFO 최대 크기
#define SERIAL_FIFO_MAXSIZE		16

#pragma pack(push, 1)

// 구조체, 시리얼 포트 담당 자료구조
typedef struct kSerialPortManager {
	// 동기화 객체
	MUTEX lock;
} SERIALMANAGER;

#pragma pack(pop)

void kInitSerial(void);
void kSendSerialData(BYTE *buf, int size);
int kRecvSerialData(BYTE *buf, int size);
void kClearSerialFIFO(void);

static BOOL kIsSerialTBE(void);
static BOOL kIsSerialRxBF(void);

#endif /*__RS232_H__*/
