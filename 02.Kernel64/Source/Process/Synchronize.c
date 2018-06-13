/*
 * Synchronize.c
 *
 *  Created on: 2017. 8. 4.
 *      Author: Yummy
 */

#include <Synchronize.h>
#include <Util.h>
#include <CLITask.h>
#include <AsmUtil.h>

// 스핀락 도입 이전에 사용하던 함수들
#if 0

// 시스템 전역에서 사용하는 데이터를 위한 잠금 함수
BOOL lockData(void) {
	return setInterruptFlag(FALSE);
}

// 시스템 전역에서 사용하는 데이터를 위한 잠금 해제 함수
void unlockData(BOOL interruptFlag) {
	setInterruptFlag(interruptFlag);
}

#endif

// 뮤텍스 초기화
void kInitMutex(MUTEX *mut) {
	// 잠김 플래그와 횟수, 그리고 태스크 ID 초기화
	mut->flag = FALSE;
	mut->cnt = 0;
	mut->id = TASK_INVALID_ID;
}

// 태스크 사이에서 사용하는 데이터를 위한 잠금 함수
void kLock(MUTEX *mut) {
	BYTE _id;
	BOOL interruptFlag;

	// 인터럽트 비활성화
	interruptFlag = kSetInterruptFlag(FALSE);

	// 현재 코어 로컬 APIC ID 확인
	_id = kGetAPICID();

	// 이미 잠겨있다면 내가 잠갔는지 확인, 잠근 횟수 증가 후 종료
	if(kTestNSet(&(mut->flag), 0, 1) == FALSE) {
		// 자신이 잠갔다면 횟수만 증가
		if(mut->id == kGetRunningTask(_id)->link.id) {
			// 인터럽트 복원
			kSetInterruptFlag(interruptFlag);
			mut->cnt++;
			return;
		}
		// 자신이 아닌 경우 잠긴 것이 해제될 때까지 대기
		while(kTestNSet(&(mut->flag), 0, 1) == FALSE) kSchedule();
	}

	// 잠금 설정, 잠긴 플래그는 위 testNSet() 함수에서 처리
	mut->cnt = 1;
	mut->id = kGetRunningTask(_id)->link.id;
	// 인터럽트 복원
	kSetInterruptFlag(interruptFlag);
}

// 태스크 사이에서 사용하는 데이터를 위한 잠금 해제 함수
void kUnlock(MUTEX *mut) {
	BOOL interruptFlag;

	// 인터럽트 비활성화
	interruptFlag = kSetInterruptFlag(FALSE);

	// 뮤텍스를 잠근 태스크가 아니면 실패
	if((mut->flag == FALSE) || (mut->id != kGetRunningTask(kGetAPICID())->link.id)) {
		// 인터럽트 복원
		kSetInterruptFlag(interruptFlag);
		return;
	}

	// 뮤텍스를 중복으로 잠갔으면 잠긴 횟수만 감소
	if(mut->cnt > 1) mut->cnt--;
	else {
		// 해제된 것으로 설정, 잠긴 플래그를 가장 나중에 해제
		mut->id = TASK_INVALID_ID;
		mut->cnt = 0;
		mut->flag = FALSE;
	}
	// 인터럽트 복원
	kSetInterruptFlag(interruptFlag);
}

// 스핀락 초기화
void kInitSpinLock(SPINLOCK *spinLock) {
	// 잠김 플래그와 횟수, APIC ID, 인터럽트 플래그 초기화
	spinLock->lockFlag = FALSE;
	spinLock->lockCnt = 0;
	spinLock->id = 0xFF;
	spinLock->interruptFlag = FALSE;
}

// 시스템 전역에서 사용하는 데이터를 위한 잠금 함수
void kLock_spinLock(SPINLOCK *spinLock) {
	BOOL interruptFlag;

	// 우선 인터럽트 비활성화
	interruptFlag = kSetInterruptFlag(FALSE);

	// 이미 잠겨 있다면 내가 잠갔는지 확인 후 잠근 횟수 증가시킨 뒤 종료
	if(kTestNSet(&(spinLock->lockFlag), 0, 1) == FALSE) {
		// 자신이 잠갔다면 횟수만 증가
		if(spinLock->id == kGetAPICID()) {
			spinLock->lockCnt++;
			return;
		}

		// 자신이 아니라면 잠금 해제까지 대기
		while(kTestNSet(&(spinLock->lockFlag), 0, 1) == FALSE) while(spinLock->lockFlag == TRUE) kPause();
	}

	// 잠김 설정, 잠김 플래그는 위 testNSet() 함수가 처리
	spinLock->lockCnt = 1;
	spinLock->id = kGetAPICID();

	// 인터럽트 플래그를 저장해 unlock 수행 시 복원
	spinLock->interruptFlag = interruptFlag;
}

// 시스템 전역에서 사용하는 데이터를 위한 잠금 해제 함수
void kUnlock_spinLock(SPINLOCK *spinLock) {
	BOOL interruptFlag;

	// 우선 인터럽트 비활성화
	interruptFlag = kSetInterruptFlag(FALSE);

	// 스핀락을 잠근 태스크가 아니면 실패
	if((spinLock->lockFlag == FALSE) || (spinLock->id != kGetAPICID())) {
		kSetInterruptFlag(interruptFlag);
		return;
	}

	// 스핀락을 중복으로 잠갔으면 잠긴 횟수만 감소
	if(spinLock->lockCnt > 1) {
		spinLock->lockCnt--;
		return;
	}

	// 스핀락을 해제된 것으로 설정 후 인터럽트 플래그 복원. 인터럽트 플래그는 미리 저장해두고 사용
	interruptFlag = spinLock->interruptFlag;

	spinLock->id = 0xFF;
	spinLock->lockCnt = 0;
	spinLock->interruptFlag = FALSE;
	spinLock->lockFlag = FALSE;

	kSetInterruptFlag(interruptFlag);
}
