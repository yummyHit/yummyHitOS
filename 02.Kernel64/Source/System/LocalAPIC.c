/*
 * LocalAPIC.c
 *
 *  Created on: 2017. 8. 19.
 *      Author: Yummy
 */

#include <LocalAPIC.h>
#include <MPConfig.h>

// 로컬 APIC의 메모리 맵 IO 어드레스 반환
QWORD getLocalAPICAddr(void) {
	MPCONFIGHEADER *head;

	// MP 설정 테이블 헤더에 저장된 로컬 APIC의 메모리 맵 IO 어드레스 사용
	head = getMPConfigManager()->tblHeader;
	return head->localAPICAddr;
}

// 의사 인터럽트 벡터 레지스터(Spurious Interrupt Vector Register)에 있는 APIC 소프트웨어 활성/비활성 필드를 1로 설정해 로컬 APIC 활성화
void onSWLocalAPIC(void) {
	QWORD localAddr;

	// MP 설정 테이블 헤더에 저장된 로컬 APIC 메모리 맵 IO 어드레스 사용
	localAddr = getLocalAPICAddr();

	// 의사 인터럽트 벡터 레지스터(0xFEE000F0)의 APIC 소프트웨어 활성/비활성 필드(비트 8)를 1로 설정해 로컬 APIC 활성화
	*(DWORD*)(localAddr + APIC_REG_SVR) |= 0x100;
}
