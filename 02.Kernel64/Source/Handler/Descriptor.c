/*
 * Descriptor.c
 *
 *  Created on: 2017. 7. 22.
 *      Author: Yummy
 */

#include <Descriptor.h>
#include <Util.h>
#include <ISR.h>
#include <MP.h>

// GDT 테이블 초기화
void kInitGDTNTSS(void) {
	GDTR *gdtr;
	GDTENTRY8 *entry;
	TSS *tss;
	int i;

	// GDTR 설정
	gdtr = (GDTR*)GDTR_STARTADDR;
	entry = (GDTENTRY8*)(GDTR_STARTADDR + sizeof(GDTR));
	gdtr->limit = GDT_TABLESIZE - 1;
	gdtr->baseAddr = (QWORD)entry;
	// TSS 영역 설정, GDT 테이블 뒤쪽에 위치
	tss = (TSS*)((QWORD)entry + GDT_TABLESIZE);

	// NULL, 64bit Code & Data, TSS를 위해 총 3 + 16개의 세그먼트 생성
	kSetGDTEntry8(&(entry[0]), 0, 0, 0, 0, 0);
	kSetGDTEntry8(&(entry[1]), 0, 0xFFFFF, GDT_FLAGS_UPPER_CODE, GDT_FLAGS_LOWER_KERNELCODE, GDT_TYPE_CODE);
	kSetGDTEntry8(&(entry[2]), 0, 0xFFFFF, GDT_FLAGS_UPPER_DATA, GDT_FLAGS_LOWER_KERNELDATA, GDT_TYPE_DATA);
	// Make to User Level Code & Data Descriptor
	kSetGDTEntry8(&(entry[3]), 0, 0xFFFFF, GDT_FLAGS_UPPER_DATA, GDT_FLAGS_LOWER_USERDATA, GDT_TYPE_DATA);
	kSetGDTEntry8(&(entry[4]), 0, 0xFFFFF, GDT_FLAGS_UPPER_CODE, GDT_FLAGS_LOWER_USERCODE, GDT_TYPE_CODE);

	// 16개 코어 지원을 위해 16개 TSS 디스크립터 생성
	// TSS는 16바이트이므로 setGDTEntry16() 함수 사용, entry는 8바이트이므로 2개를 합쳐 하나로 사용
	for(i = 0; i < MAXPROCESSORCNT; i++) kSetGDTEntry16((GDTENTRY16*)&(entry[GDT_MAXENTRY8CNT + (i * 2)]), (QWORD)tss + (i * sizeof(TSS)), sizeof(TSS) - 1, GDT_FLAGS_UPPER_TSS, GDT_FLAGS_LOWER_TSS, GDT_TYPE_TSS);

	// TSS 초기화 GDT 이하 영역 사용
	kInitTSS(tss);
}

// 8byte 크기의 GDT 엔트리에 값 설정, 코드와 데이터 세그먼트 디스크립터를 설정하는데 사용
void kSetGDTEntry8(GDTENTRY8 *entry, DWORD baseAddr, DWORD limit, BYTE highFlag, BYTE lowFlag, BYTE type) {
	entry->lowLimit = limit & 0xFFFF;
	entry->lowBaseAddr = baseAddr & 0xFFFF;
	entry->highBaseAddr_A = (baseAddr >> 16) & 0xFF;
	entry->typeNLowFlag = lowFlag | type;
	entry->highLimitNFlag = ((limit >> 16) & 0x0F) | highFlag;
	entry->highBaseAddr_B = (baseAddr >> 24) & 0xFF;
}

// 16byte 크기의 GDT 엔트리에 값 설정, TSS 세그먼트 디스크립터 설정하는데 사용
void kSetGDTEntry16(GDTENTRY16 *entry, QWORD baseAddr, DWORD limit, BYTE highFlag, BYTE lowFlag, BYTE type) {
	entry->lowLimit = limit & 0xFFFF;
	entry->lowBaseAddr = baseAddr & 0xFFFF;
	entry->midBaseAddr_A = (baseAddr >> 16) & 0xFF;
	entry->typeNLowFlag = lowFlag | type;
	entry->highLimitNFlag = ((limit >> 16) & 0x0F) | highFlag;
	entry->midBaseAddr_B = (baseAddr >> 24) & 0xFF;
	entry->highBaseAddr = baseAddr >> 32;
	entry->reserved = 0;
}

// TSS 세그먼트 정보 초기화
void kInitTSS(TSS *tss) {
	int i;

	// 최대 프로세서 또는 코어 수만큼 루프를 돌며 생성
	for(i = 0; i < MAXPROCESSORCNT; i++) {
		// 0으로 초기화
		kMemSet(tss, 0, sizeof(TSS));

		// IST의 뒤에서부터 잘라서 할당(주의 : IST는 16바이트 단위로 정렬해야 함)
		tss->ist[0] = IST_STARTADDR + IST_SIZE - (IST_SIZE / MAXPROCESSORCNT * i);

		// IO를 TSS의 limit 값보다 크게 설정함으로써 IO Map을 사용하지 않도록 함
		tss->ioMapBaseAddr = 0xFFFF;

		// 다음 엔트리로 이동
		tss++;
	}
}

// IDT 테이블 초기화
void kInitIDT(void) {
	IDTR *idtr;
	IDTENTRY *entry;
	int i;

	// IDTR의 시작 어드레스
	idtr = (IDTR*)IDTR_STARTADDR;
	// IDT 테이블의 정보 생성
	entry = (IDTENTRY*)(IDTR_STARTADDR + sizeof(IDTR));
	idtr->baseAddr = (QWORD)entry;
	idtr->limit = IDT_TABLESIZE - 1;

	// 예외 ISR 등록
	kSetIDTEntry(&(entry[0]), kISRDivErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[1]), kISRDebug, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[2]), kISRNMI, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[3]), kISRBP, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[4]), kISROF, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[5]), kISRExceed, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[6]), kISROPErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[7]), kISRDevErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[8]), kISRDoubleErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[9]), kISRSegmentOverrun, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[10]), kISRTSSErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[11]), kISRSegmentErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[12]), kISRStackErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[13]), kISRProtectErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[14]), kISRPageErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[15]), kISR15, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[16]), kISRFPUErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[17]), kISRAlignChk, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[18]), kISRMachChk, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[19]), kISRSIMDErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);

	for(i = 20; i < 32; i++) kSetIDTEntry(&(entry[i]), kISRETCErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);

	// 인터럽트 ISR 등록
	kSetIDTEntry(&(entry[32]), kISRTimer, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[33]), kISRKeyboard, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[34]), kISRSlavePIC, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[35]), kISRSerial2, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[36]), kISRSerial1, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[37]), kISRParallel2, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[38]), kISRFloppy, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[39]), kISRParallel1, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[40]), kISRRTC, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[41]), kISRReserved, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[42]), kISRNotUsed1, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[43]), kISRNotUsed2, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[44]), kISRMouse, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[45]), kISRCoprocessor, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[46]), kISRHDD1, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(entry[47]), kISRHDD2, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);

	for(i = 48; i < IDT_MAXENTRYCNT; i++) kSetIDTEntry(&(entry[i]), kISRETC, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
}

// IDT 게이트 디스크립터에 값 설정
void kSetIDTEntry(IDTENTRY *entry, void *handle, WORD selector, BYTE ist, BYTE flag, BYTE type) {
	entry->lowBaseAddr = (QWORD)handle & 0xFFFF;
	entry->selector = selector;
	entry->ist = ist & 0x3;
	entry->typeNFlag = type | flag;
	entry->midBaseAddr = ((QWORD)handle >> 16) & 0xFFFF;
	entry->highBaseAddr = (QWORD)handle >> 32;
	entry->reserved = 0;
}
