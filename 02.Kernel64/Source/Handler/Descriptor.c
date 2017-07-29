/*
 * Descriptor.c
 *
 *  Created on: 2017. 7. 22.
 *      Author: Yummy
 */

#include <Descriptor.h>
#include <Util.h>
#include <ISR.h>

/* GDT, TSS */
// GDT 테이블 초기화
void initGDTNTSS(void) {
	GDTR *gdtr;
	ENTRY8 *entry;
	TSS *tss;
	int i;

	// GDTR 설정
	gdtr = (GDTR*) GDTR_STARTADDRESS;
	entry = (ENTRY8*)(GDTR_STARTADDRESS + sizeof(GDTR));
	gdtr->limit = GDT_TABLESIZE - 1;
	gdtr->baseAddr = (QWORD)entry;
	// TSS 영역 설정
	tss = (TSS*)((QWORD)entry + GDT_TABLESIZE);

	// NULL, 64bit Code|Data, TSS를 위한 4개의 세그먼트 생성
	setEntry8(&(entry[0]), 0, 0, 0, 0, 0);
	setEntry8(&(entry[1]), 0, 0xFFFFF, GDT_FLAGS_UPPER_CODE, GDT_FLAGS_LOWER_KERNELCODE, GDT_TYPE_CODE);
	setEntry8(&(entry[2]), 0, 0xFFFFF, GDT_FLAGS_UPPER_DATA, GDT_FLAGS_LOWER_KERNELDATA, GDT_TYPE_DATA);
	setEntry16((ENTRY16*)&(entry[3]), (QWORD)tss, sizeof(TSS) - 1, GDT_FLAGS_UPPER_TSS, GDT_FLAGS_LOWER_TSS, GDT_TYPE_TSS);

	// TSS 초기화 GDT 이하 영역 사용
	initTSS(tss);
}

// 8byte 크기의 GDT 엔트리에 값 설정, 코드와 데이터 세그먼트 디스크립터를 설정하는데 사용
void setEntry8(ENTRY8 *entry, DWORD baseAddr, DWORD limit, BYTE highFlag, BYTE lowFlag, BYTE type) {
	entry->lowLimit = limit & 0xFFFF;
	entry->lowBaseAddr = baseAddr & 0xFFFF;
	entry->highBaseAddr_A = (baseAddr >> 16) & 0xFF;
	entry->typeNLowFlag = lowFlag | type;
	entry->highLimitNFlag = ((limit >> 16) & 0xFF) | highFlag;
	entry->highBaseAddr_B = (baseAddr >> 24) & 0xFF;
}

// 16byte 크기의 GDT 엔트리에 값 설정, TSS 세그먼트 디스크립터 설정하는데 사용
void setEntry16(ENTRY16 *entry, QWORD baseAddr, DWORD limit, BYTE highFlag, BYTE lowFlag, BYTE type) {
	entry->lowLimit = limit & 0xFFFF;
	entry->lowBaseAddr = baseAddr & 0xFFFF;
	entry->midBaseAddr_A = (baseAddr >> 16) & 0xFF;
	entry->typeNLowFlag = lowFlag | type;
	entry->highLimitNFlag = ((limit >> 16) & 0xFF) | highFlag;
	entry->midBaseAddr_B = (baseAddr >> 24) & 0xFF;
	entry->highBaseAddr = baseAddr >> 32;
	entry->reserved = 0;
}

// TSS 세그먼트 정보 초기화
void initTSS(TSS *tss) {
	memSet(tss, 0, sizeof(TSS));
	tss->ist[0] = IST_STARTADDRESS + IST_SIZE;
	// IO를 TSS의 limit 값보다 크게 설정함으로써 IO Map을 사용하지 않도록 함
	tss->ioMapBaseAddr = 0xFFFF;
}

/* IDT */
// IDT 테이블 초기화
void initIDT(void) {
	IDTR *idtr;
	ENTRY *entry;
	int i;

	// IDTR의 시작 어드레스
	idtr = (IDTR*)IDTR_STARTADDRESS;
	// IDT 테이블의 정보 생성
	entry = (ENTRY*)(IDTR_STARTADDRESS + sizeof(IDTR));
	idtr->baseAddr = (QWORD)entry;
	idtr->limit = IDT_TABLESIZE - 1;

	// 예외 ISR 등록
	setEntry(&(entry[0]), ISRDivErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[1]), ISRDebug, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[2]), ISRNMI, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[3]), ISRBP, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[4]), ISROF, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[5]), ISRExceed, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[6]), ISROPErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[7]), ISRDevErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[8]), ISRDoubleErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[9]), ISRSegmentOverrun, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[10]), ISRTSSErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[11]), ISRSegmentErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[12]), ISRStackErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[13]), ISRProtectErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[14]), ISRPageErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[15]), ISR15, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[16]), ISRFPUErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[17]), ISRAlignChk, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[18]), ISRMachChk, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[19]), ISRSIMDErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);

	for(i = 20; i < 32; i++) setEntry(&(entry[i]), ISRETCErr, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);

	// 인터럽트 ISR 등록
	setEntry(&(entry[32]), ISRTimer, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[33]), ISRKeyboard, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[34]), ISRSlavePIC, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[35]), ISRSerial2, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[36]), ISRSerial1, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[37]), ISRParallel2, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[38]), ISRFloppy, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[39]), ISRParallel1, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[40]), ISRRTC, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[41]), ISRReserved, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[42]), ISRNotUsed1, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[43]), ISRNotUsed2, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[44]), ISRMouse, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[45]), ISRCoprocessor, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[46]), ISRHDD1, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setEntry(&(entry[47]), ISRHDD2, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);

	for(i = 48; i < IDT_MAXENTRYCOUNT; i++) setEntry(&(entry[i]), ISRETC, 0x08, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
}

// IDT 게이트 디스크립터에 값 설정
void setEntry(ENTRY *entry, void *handle, WORD selector, BYTE ist, BYTE flag, BYTE type) {
	entry->lowBaseAddr = (QWORD)handle & 0xFFFF;
	entry->selector = selector;
	entry->ist = ist & 0x3;
	entry->typeNFlag = type | flag;
	entry->midBaseAddr = ((QWORD)handle >> 16) & 0xFFFF;
	entry->highBaseAddr = ((QWORD)handle >> 32);
	entry->reserved = 0;
}

// 임시 예외 또는 인터럽트 핸들러
/*	인터럽트 핸들러 생성 후 필요없어진 임시 함수
void dummyHandler(void) {
	printXY(3, 0, 0x0F, "=====================================================");
	printXY(3, 1, 0x0B, "		Dummy Interrupt Handler Execute		  ");
	printXY(3, 2, 0x0E, "		    Interrupt or Exception		  ");
	printXY(3, 3, 0x0F, "=====================================================");
	while(1);
}
*/
