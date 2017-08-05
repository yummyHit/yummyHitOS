/*
 * Descriptor.h
 *
 *  Created on: 2017. 7. 22.
 *      Author: Yummy
 */

#ifndef __DESCRIPTOR_H__
#define __DESCRIPTOR_H__

#include <Types.h>

#pragma once
/* GDT */
// 조합에 사용할 기본 매크로
#define GDT_TYPE_CODE		0x0A
#define GDT_TYPE_DATA		0x02
#define GDT_TYPE_TSS		0x09
#define GDT_FLAGS_LOWER_S	0x10
#define GDT_FLAGS_LOWER_DPL0	0x00
#define GDT_FLAGS_LOWER_DPL1	0x20
#define GDT_FLAGS_LOWER_DPL2	0x40
#define GDT_FLAGS_LOWER_DPL3	0x60
#define GDT_FLAGS_LOWER_P	0x80
#define GDT_FLAGS_UPPER_L	0x20
#define GDT_FLAGS_UPPER_DB	0x40
#define GDT_FLAGS_UPPER_G	0x80

// 실제로 사용할 매크로, Lower Flags는 Code|Data|TSS, DPL0, Present로 설정
#define GDT_FLAGS_LOWER_KERNELCODE ( GDT_TYPE_CODE | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P )
#define GDT_FLAGS_LOWER_KERNELDATA ( GDT_TYPE_DATA | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P )
#define GDT_FLAGS_LOWER_TSS ( GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P )
#define GDT_FLAGS_LOWER_USERCODE ( GDT_TYPE_CODE | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P )
#define GDT_FLAGS_LOWER_USERDATA ( GDT_TYPE_DATA | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P )

// Upper Flags는 Granulaty로 설정하고 코드 및 데이터는 64비트 추가
#define GDT_FLAGS_UPPER_CODE ( GDT_FLAGS_UPPER_G | GDT_FLAGS_UPPER_L )
#define GDT_FLAGS_UPPER_DATA ( GDT_FLAGS_UPPER_G | GDT_FLAGS_UPPER_L )
#define GDT_FLAGS_UPPER_TSS ( GDT_FLAGS_UPPER_G )

// 세그먼트 디스크립터 오프셋
#define GDT_KERNELCODESEGMENT	0x08
#define GDT_KERNELDATASEGMENT	0x10
#define GDT_TSSSEGMENT		0x18

// 기타 GDT에 관련된 매크로, GDTR의 시작 어드레스, 1MB에서 264KB까지는 페이지 테이블 영역
#define GDTR_STARTADDR		0x142000
// 8바이트 엔트리 개수, 널 디스크립터|커널코드|커널데이터
#define GDT_MAXENTRY8CNT	3
// 16바이트 엔트리 개수, TSS
#define GDT_MAXENTRY16CNT	1
// GDT 테이블 크기
#define GDT_TABLESIZE		((sizeof(ENTRY8) * GDT_MAXENTRY8CNT) + (sizeof(ENTRY16) * GDT_MAXENTRY16CNT))
#define TSS_SEGMENTSIZE		(sizeof(TSS))

/* IDT */
// 조합에 사용할 기본 매크로
#define IDT_TYPE_INTERRUPT	0x0E
#define IDT_TYPE_TRAP		0x0F
#define IDT_FLAGS_DPL0		0x00
#define IDT_FLAGS_DPL1		0x20
#define IDT_FLAGS_DPL2		0x40
#define IDT_FLAGS_DPL3		0x60
#define IDT_FLAGS_P		0x80
#define IDT_FLAGS_IST0		0
#define IDT_FLAGS_IST1		1

// 실제로 사용할 매크로
#define IDT_FLAGS_KERNEL	( IDT_FLAGS_DPL0 | IDT_FLAGS_P )
#define IDT_FLAGS_USER		( IDT_FLAGS_DPL3 | IDT_FLAGS_P )

// 기타 IDT에 관련된 매크로, IDT 엔트리 개수
#define IDT_MAXENTRYCNT		100
// IDTR의 시작 어드레스, TSS 세그먼트 뒤쪽에 위치
#define IDTR_STARTADDR		( GDTR_STARTADDR + sizeof(GDTR) + GDT_TABLESIZE + TSS_SEGMENTSIZE)
// IDT 테이블 시작 어드레스
#define IDT_STARTADDR		( IDTR_STARTADDR + sizeof(IDTR) )
// IDT 테이블 전체 크기
#define IDT_TABLESIZE		( IDT_MAXENTRYCNT * sizeof(ENTRY) )
// IST 시작 어드레스
#define IST_STARTADDR		0x700000
// IST 크기
#define IST_SIZE		0x100000

// 구조체, 1바이트로 정렬
#pragma pack(push, 1)

// GDTR, IDTR 구조체
typedef struct GDTR {
	WORD limit;
	QWORD baseAddr;
	WORD padding_W;
	DWORD padding_DW;
} GDTR, IDTR;

// 8byte 크기 GDT 엔트리 구조
typedef struct GDTEntry8 {
	WORD lowLimit;
	WORD lowBaseAddr;
	BYTE highBaseAddr_A;
	// 4bit type, 1bit S, 2bit DPL, 1bit P
	BYTE typeNLowFlag;
	// 4bit segment limit, 1bit AVL, L, D/B, G
	BYTE highLimitNFlag;
	BYTE highBaseAddr_B;
} ENTRY8;

// 16byte 크기 GDT 엔트리 구조
typedef struct GDTEntry16 {
	WORD lowLimit;
	WORD lowBaseAddr;
	BYTE midBaseAddr_A;
	// 4bit type, 1bit 0, 2bit DPL, 1bit P
	BYTE typeNLowFlag;
	// 4bit segment limit, 1bit AVL, 0, 0, G
	BYTE highLimitNFlag;
	BYTE midBaseAddr_B;
	DWORD highBaseAddr;
	DWORD reserved;
} ENTRY16;

// TSS Data 구조체
typedef struct TSSData {
	DWORD reserved_A;
	QWORD rsp[3];
	QWORD reserved_B;
	QWORD ist[7];
	QWORD reserved_C;
	WORD reserved;
	WORD ioMapBaseAddr;
} TSS;

// IDT 게이트 디스크립터 구조체
typedef struct IDTEntry {
	WORD lowBaseAddr;
	WORD selector;
	// 3bit IST, 5bit 0
	BYTE ist;
	// 4bit type, 1bit 0, 2bit DPL, 1bit P
	BYTE typeNFlag;
	WORD midBaseAddr;
	DWORD highBaseAddr;
	DWORD reserved;
} ENTRY;

#pragma pack(pop)

void initGDTNTSS(void);
void setEntry8(ENTRY8 *entry, DWORD baseAddr, DWORD limit, BYTE highFlag, BYTE lowFlag, BYTE type);
void setEntry16(ENTRY16 *entry, QWORD baseAddr, DWORD limit, BYTE highFlag, BYTE lowFlag, BYTE type);
void initTSS(TSS *tss);
void initIDT(void);
void setEntry(ENTRY *entry, void *handle, WORD selector, BYTE ist, BYTE flag, BYTE type);
//void dummyHandler(void);

#endif /*__DESCRIPTOR_H__*/
