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
void initGDTNTSS(void) {
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
	setGDTEntry8(&(entry[0]), 0, 0, 0, 0, 0);
	setGDTEntry8(&(entry[1]), 0, 0xFFFFF, GDT_FLAGS_UPPER_CODE, GDT_FLAGS_LOWER_KERNELCODE, GDT_TYPE_CODE);
	setGDTEntry8(&(entry[2]), 0, 0xFFFFF, GDT_FLAGS_UPPER_DATA, GDT_FLAGS_LOWER_KERNELDATA, GDT_TYPE_DATA);

	// 16개 코어 지원을 위해 16개 TSS 디스크립터 생성
	// TSS는 16바이트이므로 setGDTEntry16() 함수 사용, entry는 8바이트이므로 2개를 합쳐 하나로 사용
	for(i = 0; i < MAXPROCESSORCNT; i++) setGDTEntry16((GDTENTRY16*)&(entry[GDT_MAXENTRY8CNT + (i * 2)]), (QWORD)tss + (i * sizeof(TSS)), sizeof(TSS) - 1, GDT_FLAGS_UPPER_TSS, GDT_FLAGS_LOWER_TSS, GDT_TYPE_TSS);

	// TSS 초기화 GDT 이하 영역 사용
	initTSS(tss);
}

// 8byte 크기의 GDT 엔트리에 값 설정, 코드와 데이터 세그먼트 디스크립터를 설정하는데 사용
void setGDTEntry8(GDTENTRY8 *entry, DWORD baseAddr, DWORD limit, BYTE highFlag, BYTE lowFlag, BYTE type) {
	entry->lowLimit = limit & 0xFFFF;
	entry->lowBaseAddr = baseAddr & 0xFFFF;
	entry->highBaseAddr_A = (baseAddr >> 16) & 0xFF;
	entry->typeNLowFlag = lowFlag | type;
	entry->highLimitNFlag = ((limit >> 16) & 0x0F) | highFlag;
	entry->highBaseAddr_B = (baseAddr >> 24) & 0xFF;
}

// 16byte 크기의 GDT 엔트리에 값 설정, TSS 세그먼트 디스크립터 설정하는데 사용
void setGDTEntry16(GDTENTRY16 *entry, QWORD baseAddr, DWORD limit, BYTE highFlag, BYTE lowFlag, BYTE type) {
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
void initTSS(TSS *tss) {
	int i;

	// 최대 프로세서 또는 코어 수만큼 루프를 돌며 생성
	for(i = 0; i < MAXPROCESSORCNT; i++) {
		// 0으로 초기화
		memSet(tss, 0, sizeof(TSS));

		// IST의 뒤에서부터 잘라서 할당(주의 : IST는 16바이트 단위로 정렬해야 함)
		tss->ist[0] = IST_STARTADDR + IST_SIZE - (IST_SIZE / MAXPROCESSORCNT * i);

		// IO를 TSS의 limit 값보다 크게 설정함으로써 IO Map을 사용하지 않도록 함
		tss->ioMapBaseAddr = 0xFFFF;

		// 다음 엔트리로 이동
		tss++;
	}
}

// IDT 테이블 초기화
void initIDT(void) {
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
	setIDTEntry(&(entry[0]), ISRDivErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[1]), ISRDebug, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[2]), ISRNMI, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[3]), ISRBP, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[4]), ISROF, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[5]), ISRExceed, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[6]), ISROPErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[7]), ISRDevErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[8]), ISRDoubleErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[9]), ISRSegmentOverrun, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[10]), ISRTSSErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[11]), ISRSegmentErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[12]), ISRStackErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[13]), ISRProtectErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[14]), ISRPageErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[15]), ISR15, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[16]), ISRFPUErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[17]), ISRAlignChk, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[18]), ISRMachChk, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[19]), ISRSIMDErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);

	for(i = 20; i < 32; i++) setIDTEntry(&(entry[i]), ISRETCErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);

	// 인터럽트 ISR 등록
	setIDTEntry(&(entry[32]), ISRTimer, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[33]), ISRKeyboard, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[34]), ISRSlavePIC, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[35]), ISRSerial2, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[36]), ISRSerial1, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[37]), ISRParallel2, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[38]), ISRFloppy, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[39]), ISRParallel1, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[40]), ISRRTC, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[41]), ISRReserved, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[42]), ISRNotUsed1, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[43]), ISRNotUsed2, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[44]), ISRMouse, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[45]), ISRCoprocessor, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[46]), ISRHDD1, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&(entry[47]), ISRHDD2, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);

	for(i = 48; i < IDT_MAXENTRYCNT; i++) setIDTEntry(&(entry[i]), ISRETC, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
}

// IDT 게이트 디스크립터에 값 설정
void setIDTEntry(IDTENTRY *entry, void *handle, WORD selector, BYTE ist, BYTE flag, BYTE type) {
	entry->lowBaseAddr = (QWORD)handle & 0xFFFF;
	entry->selector = selector;
	entry->ist = ist & 0x3;
	entry->typeNFlag = type | flag;
	entry->midBaseAddr = ((QWORD)handle >> 16) & 0xFFFF;
	entry->highBaseAddr = (QWORD)handle >> 32;
	entry->reserved = 0;
}
