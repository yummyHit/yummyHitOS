/*
 * localapic.h
 *
 *  Created on: 2017. 8. 19.
 *      Author: Yummy
 */

#ifndef __LOCALAPIC_H__
#define __LOCALAPIC_H__

#include <types.h>

#pragma once

// 매크로, 로컬 APIC 레지스터 오프셋 관련
#define APIC_REG_EOI			0x0000B0
#define APIC_REG_SVR			0x0000F0
#define APIC_REG_APICID			0x000020
#define APIC_REG_TASKPRIORITY		0x000080
#define APIC_REG_TIMER			0x000320
#define APIC_REG_TEMPERATURESENSOR	0x000330
#define APIC_REG_MONITORINGCOUNTER	0x000340
#define APIC_REG_LINT0			0x000350
#define APIC_REG_LINT1			0x000360
#define APIC_REG_ERR			0x000370
#define APIC_REG_LOWICR			0x000300
#define APIC_REG_HIGHICR		0x000310

// 전달 모드(Delivery Mode) 관련
#define APIC_DELIVERYMODE_FIXED		0x000000
#define APIC_DELIVERYMODE_LOWPRIORITY	0x000100
#define APIC_DELIVERYMODE_SMI		0x000200
#define APIC_DELIVERYMODE_NMI		0x000400
#define APIC_DELIVERYMODE_INIT		0x000500
#define APIC_DELIVERYMODE_STARTUP	0x000600
#define APIC_DELIVERYMODE_EXTINT	0x000700

// 목적지 모드(Destination Mode) 관련
#define APIC_DESTMODE_PHYSICAL		0x000000
#define APIC_DESTMODE_LOGICAL		0x000800

// 전달 상태(Delivery Status) 관련
#define APIC_DELIVERYSTAT_IDLE		0x000000
#define APIC_DELIVERYSTAT_PENDING	0x001000

// 레벨(Level) 관련
#define APIC_LVL_DEASSERT		0x000000
#define APIC_LVL_ASSERT			0x004000

// 트리거 모드(Trigger Mode) 관련
#define APIC_TRIGGERMODE_EDGE		0x000000
#define APIC_TRIGGERMODE_LVL		0x008000

// 목적지 약어(Destination Abbreviation) 관련
#define APIC_DESTABBR_NOABBR		0x000000
#define APIC_DESTABBR_SELF		0x040000
#define APIC_DESTABBR_ALLINCLUDINGSELF	0x080000
#define APIC_DESTABBR_ALLEXCLUDINGSELF	0x0C0000

// 인터럽트 마스크(Interrupt Mask) 관련
#define APIC_INTERRUPT_MASK		0x010000

// 타이머 모드(Timer Mode) 관련
#define APIC_TIMERMODE_TERM		0x020000
#define APIC_TIMERMODE_ONESHOT		0x000000

// 인터럽트 입력 핀 극성(Interrupt Input Pin Polarity) 관련
#define APIC_POLARITY_ACTIVELOW		0x002000
#define APIC_POLARITY_ACTIVEHIGH	0x000000

QWORD kGetLocalAPICAddr(void);
void kOnSWLocalAPIC(void);
void kSendEOI_LocalAPIC(void);
void kSetTaskPriority(BYTE priority);
void kInitLocalVecTbl(void);

#endif /*__LOCALAPIC_H__*/
