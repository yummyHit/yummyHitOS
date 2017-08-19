/*
 * MPConfig.h
 *
 *  Created on: 2017. 8. 18.
 *      Author: Yummy
 */

#ifndef __MPCONFIG_H__
#define __MPCONFIG_H__

#include <Types.h>

#pragma once

// 매크로, MP 플로팅 포인터 특성 바이트
#define MP_FLOATINGPTR_FEATUREBYTE1_USEMPTBL	0x00
#define MP_FLOATINGPTR_FEATUREBYTE2_PICMODE	0x80

// 엔트리 타입
#define MP_ENTRYTYPE_PROCESSOR			0
#define MP_ENTRYTYPE_BUS			1
#define MP_ENTRYTYPE_IOAPIC			2
#define MP_ENTRYTYPE_IOINTERRUPT		3
#define MP_ENTRYTYPE_LOCALINTERRUPT		4

// 프로세서 CPU 플래그
#define MP_CPUFLAG_ON				0x01
#define MP_CPUFLAG_BSP				0x02

// 버스 타입 스트링
#define MP_BUS_TYPESTR_ISA			"ISA"
#define MP_BUS_TYPESTR_PCI			"PCI"
#define MP_BUS_TYPESTR_PCMCIA			"PCMCIA"
#define MP_BUS_TYPESTR_VESALOCALBUS		"VL"

// 인터럽트 타입
#define MP_INTERRUPTTYPE_INT			0
#define MP_INTERRUPTTYPE_NMI			1
#define MP_INTERRUPTTYPE_SMI			2
#define MP_INTERRUPTTYPE_EXTINT			3

// 인터럽트 플래그
#define MP_INTERRUPTFLAG_CONFORMPOLARITY	0x00
#define MP_INTERRUPTFLAG_ACTIVEHIGH		0x01
#define MP_INTERRUPTFLAG_ACTIVELOW		0x03
#define MP_INTERRUPTFLAG_CONFORMTRIGGER		0x00
#define MP_INTERRUPTFLAG_EDGETRIGGER		0x04
#define MP_INTERRUPTFLAG_LEVELTRIGGER		0x0C

// 구조체, 1바이트 정렬
#pragma pack(push, 1)

// MP 플로팅 포인터 자료구조
typedef struct mpFloatingPointer {
	// 시그너처, _MP_
	char sign[4];
	// MP 설정 테이블 위치 주소
	DWORD tblAddr;
	// MP 플로팅 포인터 자료구조 길이, 16바이트
	BYTE len;
	// MultiProcessor Specification 버전
	BYTE revision;
	// 체크섬
	BYTE checkSum;
	// MP 특성 바이트 1~5
	BYTE featureByte[5];
} MPFLOATINGPTR;

// MP 설정 테이블 헤더 자료구조
typedef struct mpConfigHeader {
	// 시그너처, PCMP
	char sign[4];
	// 기본 테이블 길이
	WORD len;
	// MultiProcessor Specification 버전
	BYTE revision;
	// 체크섬
	BYTE checkSum;
	// OEM ID
	char oemIDStr[8];
	// Product ID
	char productIDStr[12];
	// OEM이 정의한 테이블 주소
	DWORD oemPtrAddr;
	// OEM이 정의한 테이블 크기
	WORD oemSize;
	// 기본 MP 설정 테이블 엔트리 수
	WORD entryCnt;
	// 로컬 APIC 메모리 맵 IO 주소
	DWORD localAPICAddr;
	// 확장 테이블 길이
	WORD extLen;
	// 확장 테이블 체크섬
	BYTE extCheckSum;
	// 예약됨
	BYTE reserved;
} MPCONFIGHEADER;

// 프로세서 엔트리 자료구조
typedef struct processorEntry {
	// 엔트리 타입 코드, 0
	BYTE entryType;
	// 프로세서에 포함된 로컬 APIC ID
	BYTE localAPICID;
	// 로컬 APIC 버전
	BYTE localAPICVersion;
	// CPU Flag
	BYTE cpuFlag;
	// CPU Signature
	BYTE cpuSign[4];
	// 특성 플래그
	DWORD featureFlag;
	// 예약됨
	DWORD reserved[2];
} PROCESSORENTRY;

// 버스 엔트리 자료구조
typedef struct busEntry {
	// 엔트리 타입 코드, 1
	BYTE entryType;
	// 버스 ID
	BYTE id;
	// 버스 타입
	char typeStr[6];
} BUSENTRY;

// IO APIC 엔트리 자료구조
typedef struct ioAPICEntry {
	// 엔트리 타입 코드
	BYTE entryType;
	// IO APIC ID
	BYTE id;
	// IO APIC Version
	BYTE version;
	// IO APIC Flag
	BYTE flag;
	// 메모리 맵 IO 주소
	DWORD memAddr;
} IOAPICENTRY;

// IO 인터럽트 지정 엔트리 자료구조
typedef struct ioInterruptEntry {
	// 엔트리 타입 코드, 3
	BYTE entryType;
	// 인터럽트 타입
	BYTE type;
	// IO 인터럽트 플래그
	WORD flag;
	// 발생한 버스 ID
	BYTE srcID;
	// 발생한 버스 IRQ
	BYTE srcIRQ;
	// 전달할 IO APIC ID
	BYTE destIOID;
	// 전달할 IO APIC INTIN
	BYTE destIOINTIN;
} IOINTERRUPTENTRY;

// 로컬 인터럽트 지정 엔트리 자료구조
typedef struct localInterruptEntry {
	// 엔트리 타입 코드, 4
	BYTE entryType;
	// 인터럽트 타입
	BYTE type;
	// 로컬 인터럽트 플래그
	WORD flag;
	// 발생한 버스 ID
	BYTE srcID;
	// 발생한 버스 IRQ
	BYTE srcIRQ;
	// 전달할 로컬 APIC ID
	BYTE destLocalID;
	// 전달할 로컬 APIC LINTIN
	BYTE destLocalLINTIN;
} LOCALINTERRUPTENTRY;

#pragma pack(pop)

// MP 설정 테이블 관리 자료구조
typedef struct mpConfigManager {
	// MP 플로팅 테이블
	MPFLOATINGPTR *floatingPtr;
	// MP 설정 테이블 헤더
	MPCONFIGHEADER *tblHeader;
	// 기본 MP 설정 테이블 엔트리 시작 주소
	QWORD startAddr;
	// 프로세서 또는 코어 수
	int processorCnt;
	// PIC 모드 지원 여부
	BOOL usePICMode;
	// ISA 버스 ID
	BYTE isaBusID;
} MPCONFIGMANAGER;

BOOL findFloatingAddress(QWORD *addr);
BOOL analysisMPConfig(void);
MPCONFIGMANAGER *getMPConfigManager(void);
void printMPConfig(void);
int getProcessorCnt(void);

#endif /*__MPCONFIG_H__*/
