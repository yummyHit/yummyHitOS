/*
 * ioapic.h
 *
 *  Created on: 2017. 8. 24.
 *      Author: Yummy
 */

#ifndef __IOAPIC_H__
#define __IOAPIC_H__

#include <types.h>

#pragma once

// 매크로, IO APIC 레지스터 오프셋 관련
#define IO_APIC_REG_SELECTOR			0x00
#define IO_APIC_REG_WINDOW			0x10

// 위의 두 레지스터로 접근할 때 사용하는 레지스터 인덱스
#define IO_APIC_REGIDX_APICID			0x00
#define IO_APIC_REGIDX_APICVERSION		0x01
#define IO_APIC_REGIDX_APICARBID		0x02
#define IO_APIC_REGIDX_LOWTBL			0x10
#define IO_APIC_REGIDX_HIGHTBL			0x11

// IO 리다이렉션 테이블 최대 갯수
#define IO_APIC_MAXREDIRECT_TBLCNT		24

// 인터럽트 마스크 관련
#define IO_APIC_INTERRUPT_MASK			0x01

// 트리거 모드 관련
#define IO_APIC_TRIGGERMODE_LVL			0x80
#define IO_APIC_TRIGGERMODE_EDGE		0x00

// 리모트 IRR 관련
#define IO_APIC_REMOTEIRR_EOI			0x40
#define IO_APIC_REMOTEIRR_ACCEPT		0x00

// 인터럽트 입력 핀 극성 관련
#define IO_APIC_POLARITY_ACTIVELOW		0x20
#define IO_APIC_POLARITY_ACTIVEHIGH		0x00

// 전달 상태 관련
#define IO_APIC_DELIVERYSTAT_SENDPENDING	0x10
#define IO_APIC_DELIVERYSTAT_IDLE		0x00

// 목적지 모드 관련
#define IO_APIC_DESTMODE_LOGICAL		0x08
#define IO_APIC_DESTMODE_PHYSICAL		0x00

// 전달 모드 관련
#define IO_APIC_DELIVERYMODE_FIXED		0x00
#define IO_APIC_DELIVERYMODE_LOWPRIORITY	0x01
#define IO_APIC_DELIVERYMODE_SMI		0x02
#define IO_APIC_DELIVERYMODE_NMI		0x04
#define IO_APIC_DELIVERYMODE_INIT		0x05
#define IO_APIC_DELIVERYMODE_EXTINT		0x07

// IRQ를 IO APIC의 인터럽트 입력 핀으로 대응시키는 테이블 최대 크기
#define IO_APIC_MAXIRQ_MAPCNT			16

// 구조체, 1바이트로 정렬
#pragma pack(push, 1)

// IO 리다이렉션 테이블 자료구조
typedef struct kIORedirectTable {
	// 인터럽트 벡터
	BYTE vec;

	// 트리거 모드, 리모트 IRR, 인터럽트 입력 핀 극성, 전달 상태, 목적지 모드, 전달 모드 담당 필드
	BYTE mode;

	// 인터럽트 마스크
	BYTE interruptMask;

	// 예약된 영역
	BYTE reserved[4];

	// 인터럽트 전달할 APIC ID
	BYTE dest;
} IOREDIRECTTBL;

// IO APIC 관리 자료구조
typedef struct kIOAPICManager {
	// ISA 버스가 연결된 IO APIC 메모리 맵 어드레스
	QWORD ioAPICAddr;

	// IRQ와 IO APIC의 인터럽트 입력 핀 간 연결 관계 저장 테이블
	BYTE irqMap[IO_APIC_MAXIRQ_MAPCNT];
} IOAPICMANAGER;

#pragma pack(pop)

QWORD kGetIO_APICAddr(void);
void kSetIO_APICRedirect(IOREDIRECTTBL *entry, BYTE id, BYTE interruptMask, BYTE mode, BYTE vec);
void kReadIO_APICRedirect(int intin, IOREDIRECTTBL *entry);
void kWriteIO_APICRedirect(int intin, IOREDIRECTTBL *entry);
void kMaskInterruptIO_APIC(void);
void kInitIORedirect(void);
void kPrintIRQMap(void);
void kRoutingIRQ(int irq, BYTE id);

#endif /*__IOAPIC_H__*/
