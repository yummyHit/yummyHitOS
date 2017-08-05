/*
 * Synchronize.c
 *
 *  Created on: 2017. 8. 4.
 *      Author: Yummy
 */

#include <Synchronize.h>
#include <Util.h>
#include <Task.h>

// 시스템 전역에서 사용하는 데이터를 위한 잠금 함수
BOOL lockData(void) {
	return setInterruptFlag(FALSE);
}

// 시스템 전역에서 사용하는 데이터를 위한 잠금 해제 함수
void unlockData(BOOL flag) {
	setInterruptFlag(flag);
}

// 뮤텍스 초기화
void initMutex(MUTEX *mut) {
	// 잠김 플래그와 횟수, 그리고 태스크 ID 초기화
	mut->flag = FALSE;
	mut->cnt = 0;
	mut->id = TASK_INVALID_ID;
}

// 태스크 사이에서 사용하는 데이터를 위한 잠금 함수
void _lock(MUTEX *mut) {
	// 이미 잠겨있다면 내가 잠갔는지 확인, 잠근 횟수 증가 후 종료
	if(testNSet(&(mut->flag), 0, 1) == FALSE) {
		// 자신이 잠갔다면 횟수만 증가
		if(mut->id == getRunningTask()->link.id) {
			mut->cnt++;
			return;
		}
		// 자신이 아닌 경우 잠긴 것이 해제될 때까지 대기
		while(testNSet(&(mut->flag), 0, 1) == FALSE) schedule();
	}

	// 잠금 설정, 잠긴 플래그는 위 testNSet() 함수에서 처리
	mut->cnt = 1;
	mut->id = getRunningTask()->link.id;
}

// 태스크 사이에서 사용하는 데이터를 위한 잠금 해제 함수
void _unlock(MUTEX *mut) {
	// 뮤텍스를 잠근 태스크가 아니면 실패
	if((mut->flag == FALSE) || (mut->id != getRunningTask()->link.id)) return;

	// 뮤텍스를 중복으로 잠갔으면 잠긴 횟수만 감소
	if(mut->cnt > 1) {
		mut->cnt--;
		return;
	}

	// 해제된 것으로 설정, 잠긴 플래그를 가장 나중에 해제
	mut->id = TASK_INVALID_ID;
	mut->cnt = 0;
	mut->flag = FALSE;
}
