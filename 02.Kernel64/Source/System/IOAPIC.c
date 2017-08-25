/*
 * IOAPIC.c
 *
 *  Created on: 2017. 8. 24.
 *      Author: Yummy
 */

#include <IOAPIC.h>
#include <MPConfig.h>
#include <PIC.h>

// IO APIC를 관리하는 자료구조
static IOAPICMANAGER gs_ioAPICManager;

// ISA 버스가 연결된 IO APIC의 기준 주소 반환
QWORD getIO_APICAddr(void) {
	MPCONFIGMANAGER *manager;
	IOAPICENTRY *entry;

	// IO APIC의 주소가 저장되어 있지 않으면 엔트리 찾아 저장
	if(gs_ioAPICManager.ioAPICAddr == NULL) {
		entry = findIO_APICEntry();
		if(entry != NULL) gs_ioAPICManager.ioAPICAddr = entry->memAddr & 0xFFFFFFFF;
	}

	// IO APIC의 기준 주소를 찾아 저장한 후 반환
	return gs_ioAPICManager.ioAPICAddr;
}

// IO 리다이렉션 테이블 자료구조에 값 설정
void setIO_APICRedirect(IOREDIRECTTBL *entry, BYTE id, BYTE interruptMask, BYTE mode, BYTE vec) {
	memSet(entry, 0, sizeof(IOREDIRECTTBL));

	entry->dest = id;
	entry->mode = mode;
	entry->interruptMask = interruptMask;
	entry->vec = vec;
}

// 인터럽트 입력  핀에 해당하는 IO 리다이렉션 테이블에서 값 읽음
void readIO_APICRedirect(int intin, IOREDIRECTTBL *entry) {
	QWORD *data, baseAddr;

	// ISA 버스가 연결된 IO APIC 메모리 맵 IO 어드레스
	baseAddr = getIO_APICAddr();

	// IO 리다이렉션 테이블은 8바이트. 형변환 처리
	data = (QWORD*)entry;

	// IO 리다이렉션 테이블의 상위 4바이트 읽음. 상위 레지스터와 하위 레지스터가 한 쌍이니 INTIN에 2를 곱해 인덱스 계산
	// IO 레지스터 선택 레지스터(0xFEC00000)에 상위 IO 리다이렉션 테이블 레지스터 인덱스 전송
	*(DWORD*)(baseAddr + IO_APIC_REG_SELECTOR) = IO_APIC_REGIDX_HIGHTBL + intin * 2;
	// IO 윈도우 레지스터(0xFEC00010)에서 상위 IO 리다이렉션 테이블 레지스터 값 읽음
	*data = *(DWORD*)(baseAddr + IO_APIC_REG_WINDOW);
	*data = *data << 32;

	// IO 레지스터 선택 레지스터(0xFEC00000)에 하위 IO 리다이렉션 테이블 레지스터 인덱스 전송
	*(DWORD*)(baseAddr + IO_APIC_REG_SELECTOR) = IO_APIC_REGIDX_LOWTBL + intin * 2;
	// IO 윈도우 레지스터(0xFEC00010)에서 하위 IO 리다이렉션 테이블 레지스터 값 읽음
	*data |= *(DWORD*)(baseAddr + IO_APIC_REG_WINDOW);
}

// 인터럽트 입력 핀에 해당하는 IO 리다이렉션 테이블에 값 씀
void writeIO_APICRedirect(int intin, IOREDIRECTTBL *entry) {
	QWORD *data, baseAddr;

	// ISA 버스가 연결된 IO APIC 메모리 맵 IO 어드레스
	baseAddr = getIO_APICAddr();

	// IO 리다이렉션 테이블은 8바이트. 형변환 처리
	data = (QWORD*)entry;

	// IO 리다이렉션 테이블 상위 4바이트 씀. 상위 레지스터와 하위 레지스터가 한 쌍이니 INTIN에 2를 곱해 인덱스 계산
	// IO 레지스터 선택 레지스터(0xFEC00000)에 상위 IO 리다이렉션 테이블 레지스터 인덱스 전송
	*(DWORD*)(baseAddr + IO_APIC_REG_SELECTOR) = IO_APIC_REGIDX_HIGHTBL + intin * 2;
	// IO 윈도우 레지스터(0xFEC00010)에 상위 IO 리다이렉션 테이블 레지스터 값 씀
	*(DWORD*)(baseAddr + IO_APIC_REG_WINDOW) = *data >> 32;

	// IO 레지스터 선택 레지스터(0xFEC00000)에 하위 IO 리다이렉션 테이블 레지스터 인덱스 전송
	*(DWORD*)(baseAddr + IO_APIC_REG_SELECTOR) = IO_APIC_REGIDX_LOWTBL + intin * 2;
	// IO 윈도우 레지스터(0xFEC00010)에 하위 IO 리다이렉션 테이블 레지스터 값 씀
	*(DWORD*)(baseAddr + IO_APIC_REG_WINDOW) = *data;
}

// IO APIC에 연결된 모든 인터럽트 핀을 마스크해 인터럽트가 전달되지 않게 함
void maskInterruptIO_APIC(void) {
	IOREDIRECTTBL entry;
	int i;

	// 모든 인터럽트 비활성화
	for(i = 0; i < IO_APIC_MAXREDIRECT_TBLCNT; i++) {
		// IO 리다이렉션 테이블을 읽어 인터럽트 마스크 필드(비트 0)를 1로 설정해 저장
		readIO_APICRedirect(i, &entry);
		entry.interruptMask = IO_APIC_INTERRUPT_MASK;
		writeIO_APICRedirect(i, &entry);
	}
}

// IO APIC의 IO 리다이렉션 테이블 초기화
void initIORedirect(void) {
	MPCONFIGMANAGER *manager;
	MPCONFIGHEADER *head;
	IOINTERRUPTENTRY *ioInterruptEntry;
	IOREDIRECTTBL redirectEntry;
	QWORD addr;
	BYTE type, dest;
	int i;

	// IO APIC 자료구조 초기화
	memSet(&gs_ioAPICManager, 0, sizeof(gs_ioAPICManager));

	// IO APIC 메모리 맵 IO 어드레스 저장, 내부적 처리
	getIO_APICAddr();

	// IRQ를 IO APIC의 INTIN 핀과 연결한 테이블 초기화
	for(i = 0; i < IO_APIC_MAXIRQ_MAPCNT; i++) gs_ioAPICManager.irqMap[i] = 0xFF;

	// IO APIC를 마스크해 인터럽트가 발생하지 않도록 하고 IO 리다이렉션 테이블 초기화
	// 먼저 IO APIC의 인터럽트 마스크해 인터럽트 발생하지 않도록 함
	maskInterruptIO_APIC();

	// IO 인터럽트 지정 엔트리 중 ISA 버스와 관련된 인터럽트만 IO 리다이렉션 테이블에 설정
	// MP 설정 테이블 헤더의 시작 주소와 엔트리 시작 주소 저장
	manager = getMPConfigManager();
	head = manager->tblHeader;
	addr = manager->startAddr;

	// 모든 엔트리 확인해 ISA 버스와 관련된 IO 인터럽트 지정 엔트리 검색
	for(i = 0; i < head->entryCnt; i++) {
		type = *(BYTE*)addr;
		switch(type) {
		// IO 인터럽트 지정 엔트리면 ISA 버스인지 확인해 IO 리다이렉션 테이블에 설정하고 IRQ->INTIN 매핑 테이블 구성
		case MP_ENTRYTYPE_IOINTERRUPT:
			ioInterruptEntry = (IOINTERRUPTENTRY*)addr;
			if((ioInterruptEntry->srcID == manager->isaBusID) && (ioInterruptEntry->type == MP_INTERRUPTTYPE_INT)) {
				// 목적지 필드는 IRQ 0을 제외하고 0x00으로 설정해 Bootstrap Processor만 전달. IRQ 0은 스케줄러에 사용, 0xFF로 설정
				if(ioInterruptEntry->srcIRQ == 0) dest = 0xFF;
				else dest = 0x00;

				// ISQ 버스는 엣지 트리거와 1일 때 Active High 사용, 목적지 모드는 물리 모드, 전달 모드는 고정
				// 인터럽트 벡터는 PIC 컨트롤러의 벡터와 같이 0x20 + IRQ로 설정
				setIO_APICRedirect(&redirectEntry, dest, 0x00, IO_APIC_TRIGGERMODE_EDGE | IO_APIC_POLARITY_ACTIVEHIGH | IO_APIC_DESTMODE_PHYSICAL | IO_APIC_DELIVERYMODE_FIXED, PIC_IRQ_STARTVEC + ioInterruptEntry->srcIRQ);

				// ISA 버스에서 전달된 IRQ는 IO APIC의 INTIN 핀에 있으므로 INTIN 값 이용해 처리
				writeIO_APICRedirect(ioInterruptEntry->destINTIN, &redirectEntry);

				// IRQ와 인터럽트 입력 핀(INTIN)의 관계 저장(IRQ->INTIN 매핑 테이블 구성)
				gs_ioAPICManager.irqMap[ioInterruptEntry->srcIRQ] = ioInterruptEntry->destINTIN;
			}
			addr += sizeof(IOINTERRUPTENTRY);
			break;
		// 프로세스 엔트리 무시
		case MP_ENTRYTYPE_PROCESSOR:
			addr += sizeof(PROCESSORENTRY);
			break;
		// 버스 엔트리, IO APIC 엔트리, 로컬 인터럽트 지정 엔트리는 무시
		case MP_ENTRYTYPE_BUS:
		case MP_ENTRYTYPE_IOAPIC:
		case MP_ENTRYTYPE_LOCALINTERRUPT:
			addr += 8;
			break;
		}
	}
}

// IRQ와 IO APIC의 인터럽트 핀 간 매핑 관계 출력
void printIRQMap(void) {
	int i;

	printF("   =========== IRQ to I/O APIC INT IN Mapping Table ===========\n");

	for(i = 0; i < IO_APIC_MAXIRQ_MAPCNT; i++) printF("IRQ[%d] -> INTIN [%d]\n", i, gs_ioAPICManager.irqMap[i]);
}

// IRQ를 로컬 APIC ID로 전달하도록 변경
void routingIRQ(int irq, BYTE id) {
	int i;
	IOREDIRECTTBL entry;

	// 범위 검사
	if(irq > IO_APIC_MAXIRQ_MAPCNT) return;

	// 설정된 IO 리다이렉션 테이블을 읽어 목적지 필드만 수정
	readIO_APICRedirect(gs_ioAPICManager.irqMap[irq], &entry);
	entry.dest = id;
	writeIO_APICRedirect(gs_ioAPICManager.irqMap[irq], &entry);
}
