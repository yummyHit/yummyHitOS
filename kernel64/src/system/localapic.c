/*
 * localapic.c
 *
 *  Created on: 2017. 8. 19.
 *      Author: Yummy
 */

#include <localapic.h>
#include <mpconfig.h>

// 로컬 APIC의 메모리 맵 IO 어드레스 반환
QWORD kGetLocalAPICAddr(void) {
	MPCONFIGHEADER *head;

	// MP 설정 테이블 헤더에 저장된 로컬 APIC의 메모리 맵 IO 어드레스 사용
	head = kGetMPConfigManager()->tblHeader;
	return head->localAPICAddr;
}

// 의사 인터럽트 벡터 레지스터(Spurious Interrupt Vector Register)에 있는 APIC 소프트웨어 활성/비활성 필드를 1로 설정해 로컬 APIC 활성화
void kOnSWLocalAPIC(void) {
	QWORD localAddr;

	// MP 설정 테이블 헤더에 저장된 로컬 APIC 메모리 맵 IO 어드레스 사용
	localAddr = kGetLocalAPICAddr();

	// 의사 인터럽트 벡터 레지스터(0xFEE000F0)의 APIC 소프트웨어 활성/비활성 필드(비트 8)를 1로 설정해 로컬 APIC 활성화
	*(DWORD*)(localAddr + APIC_REG_SVR) |= 0x100;
}

// 로컬 APIC에 EOI(End of Interrupt) 전송
void kSendEOI_LocalAPIC(void) {
	QWORD addr;

	// MP 설정 테이블 헤더에 저장된 로컬 APIC 메모리 맵 IO 어드레스 사용
	addr = kGetLocalAPICAddr();

	// EOI 레지스터(0xFEE000B0)에 0x00을 출력해 EOI 전송
	*(DWORD*)(addr + APIC_REG_EOI) = 0;
}

// 태스크 우선순위 레지스터 설정
void kSetTaskPriority(BYTE priority) {
	QWORD addr;

	// MP 설정 테이블 헤더에 저장된 로컬 APIC 메모리 맵 IO 어드레스 사용
	addr = kGetLocalAPICAddr();

	// 태스크 우선순위 레지스터(0xFEE00080)에 우선순위 값 전송
	*(DWORD*)(addr + APIC_REG_TASKPRIORITY) = priority;
}

// 로컬 벡터 테이블 초기화
void kInitLocalVecTbl(void) {
	QWORD addr;
	DWORD tmp;

	// MP 설정 테이블 헤더에 저장된 로컬 APIC 메모리 맵 IO 어드레스 사용
	addr = kGetLocalAPICAddr();

	// 타이머 인터럽트가 발생하지 않도록 기존 값에 마스크 값을 더해 LVT 타이머 레지스터(0xFEE00320)에 저장
	*(DWORD*)(addr + APIC_REG_TIMER) |= APIC_INTERRUPT_MASK;

	// LINT0 인터럽트가 발생하지 않도록 기존 값에 마스크 값을 더해 LVT LINT0 레지스터(0xFEE00350)에 저장
	*(DWORD*)(addr + APIC_REG_LINT0) |= APIC_INTERRUPT_MASK;

	// LINT1 인터럽트는 NMI가 발생하도록 NMI로 설정해 LVT LINT1 레지스터(0xFEE00360)에 저장
	*(DWORD*)(addr + APIC_REG_LINT1) = APIC_TRIGGERMODE_EDGE | APIC_POLARITY_ACTIVEHIGH | APIC_DELIVERYMODE_NMI;

	// 에러 인터럽트가 발생하지 않도록 기존 값에 마스크 값을 더해 LVT 에러 레지스터(0xFEE00370)에 저장
	*(DWORD*)(addr + APIC_REG_ERR) |= APIC_INTERRUPT_MASK;

	// 성능 모니터링 카운터 인터럽트가 발생하지 않도록 기존 값에 마스크 값을 더해 LVT 성능 모니터링 카운터 레지스터(0xFEE00340)에 저장
	*(DWORD*)(addr + APIC_REG_MONITORINGCOUNTER) |= APIC_INTERRUPT_MASK;

	// 온도 센서 인터럽트가 발생하지 않도록 기존 값에 마스크 값을 더해 LVT 온도 센서 레지스터(0xFEE00330)에 저장
	*(DWORD*)(addr + APIC_REG_TEMPERATURESENSOR) |= APIC_INTERRUPT_MASK;
}
