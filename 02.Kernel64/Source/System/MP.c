/*
 * MP.c
 *
 *  Created on: 2017. 8. 19.
 *      Author: Yummy
 */

#include <MP.h>
#include <MPConfig.h>
#include <AsmUtil.h>
#include <LocalAPIC.h>
#include <PIT.h>

// 활성화 된 Application Processor 개수
volatile int g_awakeAPCnt = 0;
// APIC ID 레지스터 어드레스
volatile QWORD g_APICIDAddr = 0;

// 로컬 APIC 활성화 후 AP(Application Processor) 활성화
BOOL startUpAP(void) {
	// MP 설정 테이블 분석
	if(analysisMPConfig() == FALSE) return FALSE;

	// 모든 프로세서에서 로컬 APIC 사용토록 활성화
	onLocalAPIC();

	// BSP(Bootstrap Processor)의 로컬 APIC 활성화
	onSWLocalAPIC();

	// AP 깨움
	if(awakeAP() == FALSE) return FALSE;

	return TRUE;
}

// AP(Application Processor) 활성화
static BOOL awakeAP(void) {
	MPCONFIGMANAGER *manager;
	MPCONFIGHEADER *head;
	QWORD localAddr;
	BOOL interruptFlag;
	int i;

	// 인터럽트 불가로 설정
	interruptFlag = setInterruptFlag(FALSE);

	// MP 설정 테이블 헤더에 저장된 로컬 APIC 메모리 맵 IO 어드레스 사용
	manager = getMPConfigManager();
	head = manager->tblHeader;
	localAddr = head->localAPICAddr;

	// APIC ID 레지스터의 어드레스(0xFEE00020)를 저장해 AP가 자신의 APIC ID 읽을 수 있게 함
	g_APICIDAddr = localAddr + APIC_REG_APICID;

	// 하위 인터럽트 커맨드 레지스터(Lower Interrupt Command Register, 0xFEE00300)에 초기화 IPI와 시작 IPI를 전송해 AP 깨움
	// 하위 인터럽트 커맨드 레지스터를 사용해 BSP를 제외한 나머지 코어에 INIT IPI 전송
	// AP는 보호 모드 커널(0x10000)에서 시작
	// All Excluding Self, Edge Trigger, Assert, Phisical Destination, INIT
	*(DWORD*)(localAddr + APIC_REG_LOWICR) = APIC_DESTABBR_ALLEXCLUDINGSELF | APIC_TRIGGERMODE_EDGE | APIC_LVL_ASSERT | APIC_DESTMODE_PHYSICAL | APIC_DELIVERYMODE_INIT;

	// PIT를 직접 제어해 10ms 대기
	waitPIT(MSTOCNT(10));

	// 하위 인터럽트 커맨드 레지스터에서 전달 상태 비트(비트 12)를 확인해 성공 여부 판별
	if(*(DWORD*)(localAddr + APIC_REG_LOWICR) & APIC_DELIVERYSTAT_PENDING) {
		// 타이머 인터럽트가 1초에 1000번 발생하도록 재설정
		initPIT(MSTOCNT(1), TRUE);

		// 인터럽트 플래그 복원
		setInterruptFlag(interruptFlag);
		return FALSE;
	}

	// 시작 IPI 전송, 2회 반복 전송
	for(i = 0; i < 2; i++) {
		// 하위 인터럽트 커맨드 레지스터를 사용해 BSP 제외 나머지 코어에 Start Up IPI 전송
		// 보호 모드 커널이 시작하는 0x10000에서 실행시키려고 0x10(0x10000 / 4KB)를 인터럽트 벡터로 설정
		// ALL Excluding Self, Edge Trigger, Assert, Physical Destination, Start Up
		*(DWORD*)(localAddr + APIC_REG_LOWICR) = APIC_DESTABBR_ALLEXCLUDINGSELF | APIC_TRIGGERMODE_EDGE | APIC_LVL_ASSERT | APIC_DESTMODE_PHYSICAL | APIC_DELIVERYMODE_STARTUP | 0x10;

		// PIT를 직접 제어해 200us 동안 대기
		waitPIT(USTOCNT(200));

		// 하위 인터럽트 커맨드 레지스터에서 전달 상태 비트(비트 12)를 확인해 성공 여부 판별
		if(*(DWORD*)(localAddr + APIC_REG_LOWICR) & APIC_DELIVERYSTAT_PENDING) {
			// 타이머 인터럽트가 1초에 1000번 발생하도록 설정
			initPIT(MSTOCNT(1), TRUE);

			// 인터럽트 플래그 복원
			setInterruptFlag(interruptFlag);
			return FALSE;
		}
	}

	// 타이머 인터럽트가 1초에 1000번 발생하도록 설정
	initPIT(MSTOCNT(1), TRUE);

	// 인터럽트 플래그 복원
	setInterruptFlag(interruptFlag);

	// AP가 모두 깨어날 때까지 대기
	while(g_awakeAPCnt < (manager->procCnt - 1)) _sleep(50);

	return TRUE;
}

// APIC ID 레지스터에서 APIC ID 반환
BYTE getAPICID(void) {
	MPCONFIGHEADER *head;
	QWORD localAddr;

	// APIC ID 어드레스 값이 설정되지 않았으면 MP 설정 테이블에서 값 읽어 설정
	if(g_APICIDAddr == 0) {
		// MP 설정 테이블 헤더에 저장된 로컬 APIC 메모리 맵 IO 어드레스 사용
		head = getMPConfigManager()->tblHeader;
		if(head == NULL) return 0;

		// APIC ID 레지스터 어드레스(0xFEE00020)를 저장해 자신의 APIC ID를 읽을 수 있게 함
		localAddr = head->localAPICAddr;
		g_APICIDAddr = localAddr + APIC_REG_APICID;
	}

	// APIC ID 레지스터의 Bit 24~31 값 반환
	return *((DWORD*)g_APICIDAddr) >> 24;
}
