/*
 * MPConfig.c
 *
 *  Created on: 2017. 8. 16.
 *      Author: Yummy
 */

#include <MPConfig.h>
#include <Console.h>

// MP 설정 테이블 관리 자료구조
static MPCONFIGMANAGER gs_mpConfigManager = {0,};

// BIOS 영역에서 MP Floating Header 찾아 주소 반환
BOOL findFloatingAddress(QWORD *addr) {
	char *ptr;
	QWORD ebdaAddr, baseMem;	// EBDA : Extended BIOS Data Area

	// 확장 BIOS 데이터 영역과 시스템 기본 메모리 출력
	printF("Extended BIOS Data Area = [0x%X]\n", (DWORD)(*(WORD*)0x040E) * 16);
	printF("System Bas Address = [0x%X]\n", (DWORD)(*(WORD*)0x0413) * 1024);

	// 확장 BIOS 데이터 영역을 검색해 MP 플로팅 포인터를 찾고, 이 영역은 0x040E에서 세그먼트 시작 주소를 찾을 수 있음
	ebdaAddr = *(WORD*)(0x040E);

	// 세그먼트 시작 주소이므로 16을 곱해 실제 물리 어드레스로 변환
	ebdaAddr *= 16;
	for(ptr = (char*)ebdaAddr; (QWORD)ptr <= (ebdaAddr + 1024); ptr++) if(memCmp(ptr, "_MP_", 4) == 0) {
		printF("MP Floating Pointer is in EBDA, [0x%X] Address\n", (QWORD)ptr);
		*addr = (QWORD)ptr;
		return TRUE;
	}

	// 시스템 기본 메모리 끝부분에서 1KB 미만 영역 검색해 MP 플로팅 포인터 찾고, 이 메모리는 0x0413에서 KB단위로 정렬된 값 찾을 수 있음
	baseMem = *(WORD*)0x0413;
	// KB단위로 저장된 값이므로 1024를 곱해 실제 물리 어드레스로 변환
	baseMem *= 1024;
	for(ptr = (char*)(baseMem - 1024); (QWORD)ptr <= baseMem; ptr++) if(memCmp(ptr, "_MP_", 4) == 0) {
		printF("MP Floating Pointer is in System Base Memory, [0x%X] Address\n", (QWORD)ptr);
		*addr = (QWORD)ptr;
		return TRUE;
	}

	// BIOS의 ROM영역을 검색해 MP 플로팅 포인터 찾음
	for(ptr = (char*)0x0F0000; (QWORD)ptr < 0x0FFFFF; ptr++) if(memCmp(ptr, "_MP_", 4) == 0) {
		printF("MP Floating Pointer is in ROM, [0x%X] Address\n", (QWORD)ptr);
		*addr = (QWORD)ptr;
		return TRUE;
	}

	return FALSE;
}

// MP 설정 테이블을 분석해 필요 정보를 저장
BOOL analysisMPConfig(void) {
	QWORD ptrAddr, entryAddr;
	MPFLOATINGPTR *ptr;
	MPCONFIGHEADER *head;
	BYTE type;
	WORD i;
	PROCESSORENTRY *processorEntry;
	BUSENTRY *busEntry;

	// 초기화
	memSet(&gs_mpConfigManager, 0, sizeof(MPCONFIGMANAGER));
	gs_mpConfigManager.isaBusID = 0xFF;

	// MP 플로팅 포인터 어드레스를 구함
	if(findFloatingAddress(&ptrAddr) == FALSE) return FALSE;

	// MP 플로팅 테이블 설정
	ptr = (MPFLOATINGPTR*)ptrAddr;
	gs_mpConfigManager.floatingPtr = ptr;
	head = (MPCONFIGHEADER*)((QWORD)ptr->tblAddr & 0xFFFFFFFF);

	// PIC 모드 지원 여부 저장
	if(ptr->featureByte[1] & MP_FLOATINGPTR_FEATUREBYTE2_PICMODE) gs_mpConfigManager.usePICMode = TRUE;

	// MP 설정 테이블 헤더와 기본 MP 설정 테이블 엔트리의 시작 어드레스 설정
	gs_mpConfigManager.tblHeader = head;
	gs_mpConfigManager.startAddr = ptr->tblAddr + sizeof(MPCONFIGHEADER);

	// 모든 엔트리 돌며 프로세서 코어 수 계산하고, ISA 버스를 검색해 ID 저장
	entryAddr = gs_mpConfigManager.startAddr;
	for(i = 0; i < head->entryCnt; i++) {
		type = *(BYTE*)entryAddr;
		switch(type) {
		// 프로세서 엔트리면 프로세서 수 하나 증가
		case MP_ENTRYTYPE_PROCESSOR:
			processorEntry = (PROCESSORENTRY*)entryAddr;
			if(processorEntry->cpuFlag & MP_CPUFLAG_ON) gs_mpConfigManager.processorCnt++;
			entryAddr += sizeof(PROCESSORENTRY);
			break;
		// 버스 엔트리면 ISA 버스인지 확인해 저장
		case MP_ENTRYTYPE_BUS:
			busEntry = (BUSENTRY*)entryAddr;
			if(memCmp(busEntry->typeStr, MP_BUS_TYPESTR_ISA, strLen(MP_BUS_TYPESTR_ISA)) == 0) gs_mpConfigManager.isaBusID = busEntry->id;
			entryAddr += sizeof(BUSENTRY);
			break;
		// 기타 엔트리는 무시
		case MP_ENTRYTYPE_IOAPIC:
		case MP_ENTRYTYPE_IOINTERRUPT:
		case MP_ENTRYTYPE_LOCALINTERRUPT:
		default:
			entryAddr += 8;
			break;
		}
	}
	return TRUE;
}

// MP 설정 테이블을 관리하는 자료구조 반환
MPCONFIGMANAGER *getMPConfigManager(void) {
	return &gs_mpConfigManager;
}

// MP 설정 테이블 정보 모두 화면에 출력
void printMPConfig(void) {
	MPCONFIGMANAGER *manager;
	QWORD ptrAddr, entryAddr;
	MPFLOATINGPTR *ptr;
	MPCONFIGHEADER *head;
	PROCESSORENTRY *processorEntry;
	BUSENTRY *busEntry;
	IOAPICENTRY *ioAPICEntry;
	IOINTERRUPTENTRY *ioInterruptEntry;
	LOCALINTERRUPTENTRY *localInterruptEntry;
	char buf[20];
	WORD i;
	BYTE type;
	char *interruptType[4] = {"INT", "NMI", "SMI", "ExtINT"};
	char *interruptFlagPO[4] = {"Conform", "Active High", "Reserved", "Active Low"};
	char *interruptFlagEL[4] = {"Conform", "Edge-Trigger", "Reserved", "Level-Trigger"};

	// MP 설정 테이블 처리 함수를 먼저 호출해 시스템 처리에 필요한 정보 저장
	printF("   ============== MP Configuration Table Summary ==============\n");
	manager = getMPConfigManager();
	if((manager->startAddr == 0) && (analysisMPConfig() == FALSE)) {
		printF("MP Configuration Table Analysis Fail...\n");
		return;
	}
	printF("MP Configuration Table Analysis Success !!\n");

	printF("MP Floating Pointer Address : 0x%Q\n", manager->floatingPtr);
	printF("PIC Mode Support : %d\n", manager->usePICMode);
	printF("MP Configuration Table Header Address : 0x%Q\n", manager->tblHeader);
	printF("Base MP Configuration Table Entry Start Address : 0x%Q\n", manager->startAddr);
	printF("Processor Count : %d\n", manager->processorCnt);
	printF("ISA Bus ID : %d\n", manager->isaBusID);

	printF("Press any key to continue... ('q' is exit) : ");
	if(getCh() == 'q') {
		printF("\n");
		return;
	}
	printF("\n");

	// MP 플로팅 포인터 정보 출력
	printF("   =================== MP Floating Pointer ====================\n");
	ptr = manager->floatingPtr;
	memCpy(buf, ptr->sign, 4);
	buf[4] = '\0';
	printF("Signature : %s\n", buf);
	printF("MP Configuration Table Address : 0x%Q\n", ptr->tblAddr);
	printF("Length : %d * 16 Byte\n", ptr->len);
	printF("Version : %d\n", ptr->revision);
	printF("CheckSum : 0x%X\n", ptr->checkSum);

	// MP 설정 테이블 사용 여부 출력
	printF("Feature Byte 1 : 0x%X ", ptr->featureByte[0]);
	if(ptr->featureByte[0] == 0) printF("(Use MP Configuration Table)\n");
	else printF("(Use Default Configuration)\n");

	// PIC 모드 지원 여부 출력
	printF("Feature Byte 2 : 0x%X ", ptr->featureByte[1]);
	if(ptr->featureByte[2] & MP_FLOATINGPTR_FEATUREBYTE2_PICMODE) printF("(PIC Mode Support)\n");
	else printF("(Virtual Wire Mode Support)\n");

	// MP 설정 테이블 헤더 정보 출력
	printF("\n   =============== MP Configuration Table Header ==============\n");
	head = manager->tblHeader;
	memCpy(buf, head->sign, 4);
	buf[4] = '\0';
	printF("Signature : %s\n", buf);
	printF("Length : %d Byte\n", head->len);
	printF("Version : %d\n", head->revision);
	printF("CheckSUm : 0x%X\n", head->checkSum);
	memCpy(buf, head->oemIDStr, 8);
	buf[8] = '\0';
	printF("OEM ID String : %s\n", buf);
	memCpy(buf, head->productIDStr, 12);
	buf[12] = '\0';
	printF("Product ID String : %s\n", buf);
	printF("OEM Table Pointer : 0x%X\n", head->oemPtrAddr);
	printF("OEM Table Size : %d Byte\n", head->oemSize);
	printF("Entry Count : %d\n", head->entryCnt);
	printF("Memory Mapped I/O Address Of Local APIC : 0x%X\n", head->localAPICAddr);
	printF("Extended Table Length : %d Byte\n", head->extLen);
	printF("Extended Table CheckSum : 0x%X\n", head->extCheckSum);

	printF("Press any key to continue... ('q' is exit) : ");
	if(getCh() == 'q') {
		printF("\n");
		return;
	}
	printF("\n");

	// 기본 MP 설정 테이블 엔트리 정보 출력
	printF("\n   ============= Base MP Configuration Table Entry ============\n");
	entryAddr = ptr->tblAddr + sizeof(MPCONFIGHEADER);
	for(i = 0; i < head->entryCnt; i++) {
		type = *(BYTE*)entryAddr;
		switch(type) {
		// 프로세스 엔트리 정보 출력
		case MP_ENTRYTYPE_PROCESSOR:
			processorEntry = (PROCESSORENTRY*)entryAddr;
			printF("Entry Type : Processor\n");
			printF("Local APIC ID : %d\n", processorEntry->localAPICID);
			printF("Local APIC Version : 0x%X\n", processorEntry->localAPICVersion);
			printF("CPU Flags : 0x%X ", processorEntry->cpuFlag);
			// Enable, Disable
			if(processorEntry->cpuFlag & MP_CPUFLAG_ON) printF("(Enable, ");
			else printF("(Disable, ");
			// BSP, AP
			if(processorEntry->cpuFlag & MP_CPUFLAG_BSP) printF("BSP)\n");
			else printF("AP)\n");
			printF("CPU Signature : 0x%X\n", processorEntry->cpuSign);
			printF("Feature Flags : 0x%X\n\n", processorEntry->featureFlag);

			// 프로세스 엔트리 크기만큼 어드레스를 증가시켜 다음 엔트리로 이동
			entryAddr += sizeof(PROCESSORENTRY);
			break;
		// 버스 엔트리 정보 출력
		case MP_ENTRYTYPE_BUS:
			busEntry = (BUSENTRY*)entryAddr;
			printF("Entry Type : BUS\n");
			printF("Bus ID : %d\n", busEntry->id);
			memCpy(buf, busEntry->typeStr, 6);
			buf[6] = '\0';
			printF("Bus Type String : %s\n\n", buf);

			// 버스 엔트리 크기만큼 어드레스를 증가시켜 다음 엔트리로 이동
			entryAddr += sizeof(BUSENTRY);
			break;
		// IO APIC 엔트리
		case MP_ENTRYTYPE_IOAPIC:
			ioAPICEntry = (IOAPICENTRY*)entryAddr;
			printF("Entry Type : I/O APIC\n");
			printF("I/O APIC ID : %d\n", ioAPICEntry->id);
			printF("I/O APIC Version : 0x%X\n", ioAPICEntry->version);
			printF("I/O APIC Flags : 0x%X ", ioAPICEntry->flag);
			// Enable, Disable
			if(ioAPICEntry->flag == 1) printF("(Enable)\n");
			else printF("(Disable)\n");
			printF("Memory Mapped I/O Address : 0x%X\n\n", ioAPICEntry->memAddr);

			// IO APIC 엔트리 크기만큼 어드레스를 증가시켜 다음 엔트리로 이동
			entryAddr += sizeof(IOAPICENTRY);
			break;
		// IO 인터럽트 지정 엔트리
		case MP_ENTRYTYPE_IOINTERRUPT:
			ioInterruptEntry = (IOINTERRUPTENTRY*)entryAddr;
			printF("Entry Type : I/O Interrupt Assignment\n");
			printF("Interrupt Type : 0x%X ", ioInterruptEntry->type);
			// 인터럽트 타입 출력
			printF("(%s)\n", interruptType[ioInterruptEntry->type]);
			printF("I/O Interrupt Flags : 0x%X ", ioInterruptEntry->flag);
			// 극성과 트리거 모드 출력
			printF("(%s, %s)\n", interruptFlagPO[ioInterruptEntry->flag & 0x03], interruptFlagEL[(ioInterruptEntry->flag >> 2) & 0x03]);
			printF("Source BUS ID : %d\n", ioInterruptEntry->srcID);
			printF("Source BUS IRQ : %d\n", ioInterruptEntry->srcIRQ);
			printF("Destination I/O APIC ID : %d\n", ioInterruptEntry->destIOID);
			printF("Destination I/O APIC INTIN : %d\n\n", ioInterruptEntry->destIOINTIN);

			// IO 인터럽트 지정 엔트리 크기만큼 어드레스를 증가시켜 다음 엔트리로 이동
			entryAddr += sizeof(IOINTERRUPTENTRY);
			break;
		// 로컬 인터럽트 지정 엔트리
		case MP_ENTRYTYPE_LOCALINTERRUPT:
			localInterruptEntry = (LOCALINTERRUPTENTRY*)entryAddr;
			printF("Entry Type : Local Interrupt Assignment\n");
			printF("Interrupt Type : 0x%X ", localInterruptEntry->type);
			// 인터럽트 타입 출력
			printF("(%s)\n", interruptType[localInterruptEntry->type]);
			printF("I/O Interrupt Flags : 0x%X ", localInterruptEntry->flag);
			// 극성과 트리거 모드 출력
			printF("(%s, %s)\n", interruptFlagPO[localInterruptEntry->flag & 0x03], interruptFlagEL[(localInterruptEntry->flag >> 2) & 0x03]);
			printF("Source BUS ID : %d\n", localInterruptEntry->srcID);
			printF("Source BUS IRQ : %d\n", localInterruptEntry->srcIRQ);
			printF("Destination Local APIC ID : %d\n", localInterruptEntry->destLocalID);
			printF("Destination Local APIC LINTIN : %d\n\n", localInterruptEntry->destLocalLINTIN);

			// 로컬 인터럽트 지정 엔트리 크기만큼 어드레스를 증가시켜 다음 엔트리로 이동
			entryAddr += sizeof(LOCALINTERRUPTENTRY);
			break;
		default:
			printF("Unknown Entry Type. %d\n", type);
			break;
		}

		// 3개 출력 후 키 입력 대기
		if((i != 0) && (((i + 1) % 3) == 0)) {
			printF("Press any key to continue... ('q' is exit) : ");
			if(getCh() == 'q') {
				printF("\n");
				return;
			}
			printF("\n");
		}
	}
}

// 프로세서 또는 코어 개수 반환
int getProcessorCnt(void) {
	// MP 설정 테이블이 없을 수도 있으니 0으로 설정되면 1 반환
	if(gs_mpConfigManager.processorCnt == 0) return 1;
	return gs_mpConfigManager.processorCnt;
}
