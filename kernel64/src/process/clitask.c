/*
 * clitask.c
 *
 *  Created on: 2017. 7. 30.
 *      Author: Yummy
 */

#include <clitask.h>
#include <descriptor.h>
#include <sync.h>
#include <mp.h>
#include <dynmem.h>

// 스케쥴러 관련 자료구조
static SCHEDULER gs_scheduler[MAXPROCESSORCNT];
static TCBPOOLMANAGER gs_tcbPoolManager;

// 태스크 풀 초기화
static void kInitTCBPool(void) {
	int i;

	kMemSet(&(gs_tcbPoolManager), 0, sizeof(gs_tcbPoolManager));

	// 태스크 풀 어드레스 지정 후 초기화
	gs_tcbPoolManager.startAddr = (TCB*)TASK_TCBPOOLADDR;
	kMemSet(TASK_TCBPOOLADDR, 0, sizeof(TCB) * TASK_MAXCNT);

	// TCB에 ID 할당
	for(i = 0; i < TASK_MAXCNT; i++) gs_tcbPoolManager.startAddr[i].link.id = i;

	// TCB의 최대 개수와 할당된 횟수 초기화
	gs_tcbPoolManager.maxCnt = TASK_MAXCNT;
	gs_tcbPoolManager.allocCnt = 1;

	// 스핀락 초기화
	kInitSpinLock(&(gs_tcbPoolManager.spinLock));
}

// TCB 할당받음
static TCB *kAllocTCB(void) {
	TCB *empty;
	int i;

	// 동기화 처리
	kLock_spinLock(&(gs_tcbPoolManager.spinLock));

	if(gs_tcbPoolManager.useCnt == gs_tcbPoolManager.maxCnt) {
		// 동기화 처리
		kUnlock_spinLock(&(gs_tcbPoolManager.spinLock));
		return NULL;
	}

	for(i = 0; i < gs_tcbPoolManager.maxCnt; i++) if((gs_tcbPoolManager.startAddr[i].link.id >> 32) == 0) {
		empty = &(gs_tcbPoolManager.startAddr[i]);
		break;
	}

	// 상위 32비트를 0이 아닌 값으로 설정해 할당된 TCB로 설정
	empty->link.id = ((QWORD)gs_tcbPoolManager.allocCnt << 32) | i;
	gs_tcbPoolManager.useCnt++;
	gs_tcbPoolManager.allocCnt++;
	if(gs_tcbPoolManager.allocCnt == 0) gs_tcbPoolManager.allocCnt = 1;

	// 동기화 처리
	kUnlock_spinLock(&(gs_tcbPoolManager.spinLock));
	return empty;
}

// TCB 해제
static void kFreeTCB(QWORD id) {
	int i;

	// 태스크 ID의 하위 32비트가 인덱스 역할 함
	i = GETTCBOFFSET(id);

	// TCB 초기화 후 ID 설정
	kMemSet(&(gs_tcbPoolManager.startAddr[i].context), 0, sizeof(CONTEXT));

	// 동기화 처리
	kLock_spinLock(&(gs_tcbPoolManager.spinLock));

	gs_tcbPoolManager.startAddr[i].link.id = i;

	gs_tcbPoolManager.useCnt--;

	// 동기화 처리
	kUnlock_spinLock(&(gs_tcbPoolManager.spinLock));
}

// 태스크 생성. 태스크 ID에 따라 스택 풀에서 스택 자동 할당. 프로세스와 쓰레드 모두 생성 가능. affinity에 태스크를 수행하고 싶은 코어 ID 설정 가능
TCB *kCreateTask(QWORD flag, void *memAddr, QWORD memSize, QWORD epAddr, BYTE affinity) {
	TCB *task, *proc;
//	void *stackAddr;
	BYTE _id;

	// 현재 코어의 로컬 APIC ID 확인
	_id = kGetAPICID();

	task = kAllocTCB();
	if(task == NULL) return NULL;

	task->stackAddr = kAllocMem(TASK_STACKSIZE);
	if(task->stackAddr == NULL) {
//	stackAddr = kAllocMem(TASK_STACKSIZE);
//	if(stackAddr == NULL) {
		kFreeTCB(task->link.id);
		return NULL;
	}

	// 임계 영역 시작
	kLock_spinLock(&(gs_scheduler[_id].spinLock));

	// 현재 프로세스 또는 쓰레드가 속한 프로세스 검색
	proc = kGetProcThread(kGetRunningTask(_id));
	// 만약 프로세스가 없다면 아무 작업도 안함
	if(proc == NULL) {
		kFreeTCB(task->link.id);
		kFreeMem(task->stackAddr);
//		kFreeMem(stackAddr);
		// 임계 영역 끝
		kUnlock_spinLock(&(gs_scheduler[_id].spinLock));
		return NULL;
	}

	// 쓰레드를 생성하는 경우라면 내가 속한 프로세스의 자식 쓰레드 리스트에 연결
	if(flag & TASK_FLAGS_THREAD) {
		//현재 쓰레드의 프로세스를 찾아 생성할 쓰레드에 프로세스 정보 상속
		task->parentProcID = proc->link.id;
		task->memAddr = proc->memAddr;
		task->memSize = proc->memSize;

		// 부모 프로세스의 자식 스레드 리스트에 추가
		kAddListTail(&(proc->childThreadList), &(task->threadLink));
	} else {	// 프로세스는 파라미터로 넘어온 값 그대로 설정
		task->parentProcID = proc->link.id;
		task->memAddr = memAddr;
		task->memSize = memSize;
	}

	// 스레드 ID를 태스크 ID와 동일하게 설정
	task->threadLink.id = task->link.id;

	// 임계 영역 끝
	kUnlock_spinLock(&(gs_scheduler[_id].spinLock));

	// TCB를 설정한 후 준비 리스트에 삽입해 스케줄링 될 수 있도록 함
	kSetTask(task, flag, epAddr, task->stackAddr, TASK_STACKSIZE);
//	kSetTask(task, flag, epAddr, stackAddr, TASK_STACKSIZE);

	// 자식 쓰레드 리스트 초기화
	kInitList(&(task->childThreadList));

	// FPU 사용 여부를 사용하지 않은 것으로 초기화
	task->fpuUsed = FALSE;

	// 현재 코어의 로컬 APIC ID를 태스크에 설정
	task->_id = _id;

	// 프로세서 친화도 설정
	task->affinity = affinity;

	// 부하 분산을 고려해 스케줄러에 태스크 추가
	kAddTaskLoadBalancing(task);
	return task;
}

// 파라미터를 이용해 TCB설정
static void kSetTask(TCB *tcb, QWORD flag, QWORD epAddr, void *stackAddr, QWORD stackSize) {
	// 콘텍스트 초기화
	kMemSet(tcb->context.reg, 0, sizeof(tcb->context.reg));

	// 스택 관련 RSP, RBP 레지스터 설정
	tcb->context.reg[TASK_RSPOFFSET] = (QWORD)stackAddr + stackSize - 8;
	tcb->context.reg[TASK_RBPOFFSET] = (QWORD)stackAddr + stackSize - 8;

	// Return Address 영역에 kTaskExit()함수의 어드레스 삽입. 엔트리 포인트 함수 빠져나가며 kTaskExit()함수로 이동
	*(QWORD*)((QWORD)stackAddr + stackSize - 8) = (QWORD)kTaskExit;

	// 세그먼트 셀렉터 설정
	// 커널 태스크의 경우 커널 레벨 세그먼트 디스크립터 설정
	if((flag & TASK_FLAGS_USERLV) == 0) {
		tcb->context.reg[TASK_CSOFFSET] = GDT_KERNELCODE_SEGMENT | SELECTOR_RPL_0;
		tcb->context.reg[TASK_DSOFFSET] = GDT_KERNELDATA_SEGMENT | SELECTOR_RPL_0;
		tcb->context.reg[TASK_ESOFFSET] = GDT_KERNELDATA_SEGMENT | SELECTOR_RPL_0;
		tcb->context.reg[TASK_FSOFFSET] = GDT_KERNELDATA_SEGMENT | SELECTOR_RPL_0;
		tcb->context.reg[TASK_GSOFFSET] = GDT_KERNELDATA_SEGMENT | SELECTOR_RPL_0;
		tcb->context.reg[TASK_SSOFFSET] = GDT_KERNELDATA_SEGMENT | SELECTOR_RPL_0;
	} else {		// 유저 태스크의 경우 유저 레벨 세그먼트 디스크립터 설정
		tcb->context.reg[TASK_CSOFFSET] = GDT_USERCODE_SEGMENT | SELECTOR_RPL_3;
		tcb->context.reg[TASK_DSOFFSET] = GDT_USERDATA_SEGMENT | SELECTOR_RPL_3;
		tcb->context.reg[TASK_ESOFFSET] = GDT_USERDATA_SEGMENT | SELECTOR_RPL_3;
		tcb->context.reg[TASK_FSOFFSET] = GDT_USERDATA_SEGMENT | SELECTOR_RPL_3;
		tcb->context.reg[TASK_GSOFFSET] = GDT_USERDATA_SEGMENT | SELECTOR_RPL_3;
		tcb->context.reg[TASK_SSOFFSET] = GDT_USERDATA_SEGMENT | SELECTOR_RPL_3;
	}

	// RIP 레지스터와 인터럽트 플래그 설정
	tcb->context.reg[TASK_RIPOFFSET] = epAddr;

	// RFLAGS 레지스터의 IF 비트(비트 9)를 1로 설정해 인터럽트 활성화
	// IOPL 비트(비트 12-13)를 3으로 설정하여 유저 레벨에서도 IO 포트에 접근할 수 있도록 함
	tcb->context.reg[TASK_RFLAGSOFFSET] |= 0x3200;

	// ID 및 스택, 그리고 플래그 저장
	tcb->stackAddr = stackAddr;
	tcb->stackSize = stackSize;
	tcb->flag = flag;
}

// 스케줄러 초기화. 스케줄러 초기화에 필요한 TCB 풀과 init Task도 같이 초기화
void kInitScheduler(void) {
	int i, j;
	BYTE _id;
	TCB *task;

	// 현재 코어의 로컬 APIC ID 확인
	_id = kGetAPICID();

	// Bootstrap Processor만 태스크 풀과 스케줄러 자료구조를 모두 초기화
	if(_id == 0) {
		// 태스크 풀 초기화
		kInitTCBPool();

		// 준비 리스트와 우선순위별 실행 횟수를 초기화하고 대기 리스트와 스핀락 초기화
		for(j = 0; j < MAXPROCESSORCNT; j++) {
			for(i = 0; i < TASK_MAXREADYCNT; i++) {
				kInitList(&(gs_scheduler[j].readyList[i]));
				gs_scheduler[j].exeCnt[i] = 0;
			}
			// 대기 리스트 초기화
			kInitList(&(gs_scheduler[j].waitList));

			// 스핀락 초기화
			kInitSpinLock(&(gs_scheduler[j].spinLock));
		}
	}

	// TCB를 할당받아 부팅을 수행한 태스크를 커널 최초 프로세스로 설정
	task = kAllocTCB();
	gs_scheduler[_id].runningTask = task;
	// BSP의 콘솔 셸이나 AP의 유휴 태스크는 모두 현재 코어에서만 실행하도록 로컬 APIC ID와 프로세서 친화도를 현재 코어 로컬 APIC ID로 설정
	task->_id = _id;
	task->affinity = _id;

	// Bootstrap Processor는 콘솔 셸로 실행, Application Processor는 특별히 긴급한 태스크가 없으므로 유휴 태스크 실행
	if(_id == 0) task->flag = TASK_FLAGS_HIGHEST | TASK_FLAGS_PROC | TASK_FLAGS_SYSTEM;
	else task->flag = TASK_FLAGS_LOWEST | TASK_FLAGS_PROC | TASK_FLAGS_SYSTEM | TASK_FLAGS_IDLE;

	task->parentProcID = task->link.id;
	task->memAddr = (void*)0x100000;
	task->memSize = 0x500000;
	task->stackAddr = (void*)0x600000;
	task->stackSize = 0x100000;

	// 프로세서 사용률 계산하는데 사용하는 자료구조 초기화
	gs_scheduler[_id].loopIdleTask = 0;
	gs_scheduler[_id].procLoad = 0;

	// FPU를 사용한 태스크 ID를 유효하지 않은 값으로 초기화
	gs_scheduler[_id].lastFPU = TASK_INVALID_ID;
}

// 현재 수행 중 태스크 설정
void kSetRunningTask(BYTE _id, TCB *task) {
	// 임계 영역 시작
	kLock_spinLock(&(gs_scheduler[_id].spinLock));

	gs_scheduler[_id].runningTask = task;

	// 임계 영역 끝
	kUnlock_spinLock(&(gs_scheduler[_id].spinLock));
}

// 현재 수행 중 태스크 반환
TCB *kGetRunningTask(BYTE _id) {
	TCB *runningTask;

	// 임계 영역 시작
	kLock_spinLock(&(gs_scheduler[_id].spinLock));

	runningTask = gs_scheduler[_id].runningTask;

	// 임계 영역 끝
	kUnlock_spinLock(&(gs_scheduler[_id].spinLock));

	return runningTask;
}

// 태스크 리스트에서 다음으로 실행할 태스크 얻음
static TCB *kGetNextTask(BYTE _id) {
	TCB *target = NULL;
	int cnt, i, j;

	// 큐에 태스크가 있으나 모든 태스크가 1회씩 실행된 경우 프로세서를 양보해 태스크를 선택 안 할 수 있으니 NULL일 경우 1회 더 수행
	for(j = 0; j < 2; j++) {
		// 높은 우선순위에서 낮은 우선순위까지 리스트를 확인해 스케줄링할 태스크 선택
		for(i = 0; i < TASK_MAXREADYCNT; i++) {
			cnt = kGetListCnt(&(gs_scheduler[_id].readyList[i]));

			// 만약 실행한 횟수보다 리스트 태스크 수가 더 많으면 우선순위 태스크 실행
			if(gs_scheduler[_id].exeCnt[i] < cnt) {
				target = (TCB*)kDelListHead(&(gs_scheduler[_id].readyList[i]));
				gs_scheduler[_id].exeCnt[i]++;
				break;
			} else gs_scheduler[_id].exeCnt[i] = 0;
		}

		// 만약 수행할 태스크 찾았으면 종료
		if(target != NULL) break;
	}
	return target;
}

// 태스크를 스케줄러 준비 리스트에 삽입
static BOOL kAddReadyList(BYTE _id, TCB *task) {
	BYTE priority;

	priority = GETPRIORITY(task->flag);
	if(priority == TASK_FLAGS_WAIT) {
		kAddListTail(&(gs_scheduler[_id].waitList), task);
		return TRUE;
	}
	else if(priority >= TASK_MAXREADYCNT) return FALSE;
	kAddListTail(&(gs_scheduler[_id].readyList[priority]), task);
	return TRUE;
}

// 준비 큐에서 태스크 제거
static TCB *kDelReadyList(BYTE _id, QWORD id) {
	TCB *target;
	BYTE priority;

	// 태스크 ID가 유효하지 않으면 실패
	if(GETTCBOFFSET(id) >= TASK_MAXCNT) return NULL;

	// TCB 풀에서 해당 태스크의 TCB를 찾아 실제로 ID가 일치하는지 확인
	target = &(gs_tcbPoolManager.startAddr[GETTCBOFFSET(id)]);
	if(target->link.id != id) return NULL;

	// 태스크가 존재하는 준비 리스트에서 태스크 제거
	priority = GETPRIORITY(target->flag);
	if(priority >= TASK_MAXREADYCNT) return NULL;

	target = kDelList(&(gs_scheduler[_id].readyList[priority]), id);
	return target;
}

// 태스크가 포함된 스케줄러의 ID를 반환하고 해당 스케줄러 스핀락 잠금
static BOOL kFindSchedulerLock(QWORD id, BYTE *apicID) {
	TCB *target;
	BYTE _id;

	while(1) {
		// 태스크 ID로 태스크 자료구조를 찾아서 어느 스케줄러에서 실행 중인지 확인
		target = &(gs_tcbPoolManager.startAddr[GETTCBOFFSET(id)]);
		if((target == NULL) || (target->link.id != id)) return FALSE;

		// 현재 태스크가 실행되는 코어 ID 확인
		_id = target->_id;

		// 임계 영역 시작
		kLock_spinLock(&(gs_scheduler[_id].spinLock));

		// 스핀락을 획득한 이후 다시 확인해 같은 코어에서 실행되는지 확인. 태스크가 수행되는 코어를 찾은 후 정확히 스핀락을 걸기 위해 이중 검사
		target = &(gs_tcbPoolManager.startAddr[GETTCBOFFSET(id)]);
		if(target->_id == _id) break;

		// 태스크 자료구조에 저장된 로컬 APIC ID 값이 스핀락을 획득하기 전과 후가 다르면 스핀락을 획득하는 동안 태스크가 다른 코어로 옮겨간 것
		// 따라서 다시 스핀락을 해제하고 옮겨진 코어의 스핀락을 획득해야 함
		// 임계 영역 끝
		kUnlock_spinLock(&(gs_scheduler[_id].spinLock));
	}

	*apicID = _id;
	return TRUE;
}

// 태스크 우선순위 변경
BOOL kAlterPriority(QWORD id, BYTE priority) {
	TCB *target;
	BYTE _id;

	if(priority > TASK_MAXREADYCNT) return FALSE;

	// 태스크가 포함된 코어의 로컬 APIC ID를 찾은 후 스핀락 잠금
	if(kFindSchedulerLock(id, &_id) == FALSE) return FALSE;

	// 현재 실행 중인 태스크면 우선순위만 변경, PIT 컨트롤러 인터럽트(IRQ 0)가 발생해 태스크 전환 수행시 변경된 우선순위 리스트로 이동
	target = gs_scheduler[_id].runningTask;
	if(target->link.id == id) SETPRIORITY(target->flag, priority);
	else {	// 실행 중인 태스크가 아니면 준비 리스트에서 찾아 해당 우선순위 리스트로 이동
		// 준비 리스트에서 태스크를 찾지 못하면 직접 우선순위 설정
		target = kDelReadyList(_id, id);
		if(target == NULL) {
			// 태스크 ID로 직접 찾아 설정
			target = kGetTCB(GETTCBOFFSET(id));
			if(target != NULL) SETPRIORITY(target->flag, priority);
		} else {
			SETPRIORITY(target->flag, priority);
			kAddReadyList(_id, target);
		}
	}

	// 임계 영역 끝
	kUnlock_spinLock(&(gs_scheduler[_id].spinLock));
	return TRUE;
}

// 다른 태스크 찾아 전환. 인터럽트나 예외 발생시 호출하면 안됨
BOOL kSchedule(void) {
	TCB *runningTask, *nextTask;
	BOOL preFlag;
	BYTE _id;

	// 전환 도중 인터럽트가 발생해 태스크 전환이 또 일어나면 곤란하므로 전환하는 동안 인터럽트 발생 X
	preFlag = kSetInterruptFlag(FALSE);

	_id = kGetAPICID();

	// 전환할 태스크가 있어야 함
	if(kGetReadyCnt(_id) < 1) {
		kSetInterruptFlag(preFlag);
		return FALSE;
	}

	// 임계 영역 시작
	kLock_spinLock(&(gs_scheduler[_id].spinLock));

	// 실행할 다음 태스크 얻음
	nextTask = kGetNextTask(_id);
	if(nextTask == NULL) {
		// 임계 영역 끝
		kUnlock_spinLock(&(gs_scheduler[_id].spinLock));
		kSetInterruptFlag(preFlag);
		return FALSE;
	}

	// 현재 수행 중인 태스크 정보 수정한 뒤 콘텍스트 전환
	runningTask = gs_scheduler[_id].runningTask;
	gs_scheduler[_id].runningTask = nextTask;

	// 유휴 태스크에서 전환되었다면 사용한 프로세서 시간 증가시킴
	if((runningTask->flag & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE) gs_scheduler[_id].loopIdleTask += TASK_PROCESSORTIME - gs_scheduler[_id].time;

	// 다음에 수행할 태스크가 FPU를 쓴 태스크가 아니라면 TS 비트 설정
	if(gs_scheduler[_id].lastFPU != nextTask->link.id) kSetTS();
	else kClearTS();

	// 프로세서 사용 시간 업데이트
	gs_scheduler[_id].time = TASK_PROCESSORTIME;

	// 태스크 종료 플래그가 설정된 경우 콘텍스트를 저장할 필요가 없으니 대기 리스트에 삽입하고 콘텍스트 전환
	if(runningTask->flag & TASK_FLAGS_FIN) {
		kAddListTail(&(gs_scheduler[_id].waitList), runningTask);
		// 임계 영역 끝
		kUnlock_spinLock(&(gs_scheduler[_id].spinLock));
		kSwitchContext(NULL, &(nextTask->context));
	} else {
		kAddReadyList(_id, runningTask);
		// 임계 영역 끝
		kUnlock_spinLock(&(gs_scheduler[_id].spinLock));
		kSwitchContext(&(runningTask->context), &(nextTask->context));
	}

	kSetInterruptFlag(preFlag);
	return FALSE;
}

// 인터럽트 발생 시 다른 태스크를 찾아 전환. 반드시 인터럽트나 예외 발생 시 호출해야함
BOOL kScheduleInterrupt(void) {
	TCB *runningTask, *nextTask;
	char *contextAddr;
	BYTE _id;
	QWORD istAddr;

	// 현재 로컬 APIC ID 확인
	_id = kGetAPICID();

	// 임계 영역 시작
	kLock_spinLock(&(gs_scheduler[_id].spinLock));

	// 전환할 태스크 없으면 종료
	nextTask = kGetNextTask(_id);
	if(nextTask == NULL) {
		// 임계 영역 끝
		kUnlock_spinLock(&(gs_scheduler[_id].spinLock));
		return FALSE;
	}

	// 태스크 전환 처리. 인터럽트 핸들러에서 저장한 콘텍스트를 다른 콘텍스트로 덮어쓰는 방법
	// IST의 끝부분부터 코어 0 -> 코어 15 순으로 64KB씩 쓰고 있으므로 로컬 APIC ID를 이용해 IST 어드레스 계산
	istAddr = IST_STARTADDR + IST_SIZE - (IST_SIZE / MAXPROCESSORCNT * _id);
	contextAddr = (char*)istAddr - sizeof(CONTEXT);

	// 현재 수행 중 태스크 정보 수정 후 콘텍스트 전환
	runningTask = gs_scheduler[_id].runningTask;
	gs_scheduler[_id].runningTask = nextTask;

	// 유휴 태스크에서 전환되었으면 사용한 프로세서 시간 증가
	if((runningTask->flag & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE) gs_scheduler[_id].loopIdleTask += TASK_PROCESSORTIME;

	// 태스크 종료 플래그가 설정된 경우 콘텍스트를 저장하지 않고 대기 리스트에만 삽입. 태스크가 종료되지 않으면 IST에 있는 콘텍스트 복사
	if(runningTask->flag & TASK_FLAGS_FIN) kAddListTail(&(gs_scheduler[_id].waitList), runningTask);
	else kMemCpy(&(runningTask->context), contextAddr, sizeof(CONTEXT));

	// 다음에 수행할 태스크가 FPU를 쓴 태스크가 아니라면 TS 비트 설정
	if(gs_scheduler[_id].lastFPU != nextTask->link.id) kSetTS();
	else kClearTS();

	// 임계 영역 끝
	kUnlock_spinLock(&(gs_scheduler[_id].spinLock));

	// 전환해서 실행할 태스크를 Running Task로 설정하고, 콘텍스트를 IST에 복사해 자동으로 태스크 전환하게 함
	kMemCpy(contextAddr, &(nextTask->context), sizeof(CONTEXT));

	// 종료하는 태스크가 아니면 스케줄러에 태스크 추가
	if((runningTask->flag & TASK_FLAGS_FIN) != TASK_FLAGS_FIN) kAddTaskLoadBalancing(runningTask);

	// 프로세서 사용 시간 업데이트
	gs_scheduler[_id].time = TASK_PROCESSORTIME;
	return TRUE;
}

// 프로세서 사용할 수 있는 시간 하나 줄임
void kDecProcTime(BYTE _id) {
	gs_scheduler[_id].time--;
}

// 프로세서 사용할 수 있는 시간이 다 되었는지 여부 반환
BOOL kIsProcTime(BYTE _id) {
	if(gs_scheduler[_id].time <= 0) return TRUE;
	return FALSE;
}

// 태스크 종료
BOOL kTaskFin(QWORD id) {
	TCB *target;
	BYTE priority, _id;

	// 태스크가 포함된 코어의 로컬 APIC ID 찾은 후 스핀락 잠금
	if(kFindSchedulerLock(id, &_id) == FALSE) return FALSE;

	// 현재 실행 중 태스크이면 Fin 비트 설정 후 태스크 전환
	target = gs_scheduler[_id].runningTask;
	if(target->link.id == id) {
		target->flag |= TASK_FLAGS_FIN;
		SETPRIORITY(target->flag, TASK_FLAGS_WAIT);

		// 임계 영역 끝
		kUnlock_spinLock(&(gs_scheduler[_id].spinLock));

		// 현재 스케줄러에서 실행 중인 태스크의 경우만 아래 적용
		if(kGetAPICID() == _id) kSchedule();
		return TRUE;
	}

	// 실행 중 태스크가 아니면 준비 큐에서 직접 찾아 대기 리스트에 연결
	// 준비 리스트에서 태스크를 찾지 못하면 직접 태스크를 찾아 태스크 종료 비트 설정
	target = kDelReadyList(_id, id);
	if(target == NULL) {
		// 태스크 ID로 직접 찾아 설정
		target = kGetTCB(GETTCBOFFSET(id));
		if(target != NULL) {
			target->flag |= TASK_FLAGS_FIN;
			SETPRIORITY(target->flag, TASK_FLAGS_WAIT);
		}

		// 임계 영역 끝
		kUnlock_spinLock(&(gs_scheduler[_id].spinLock));
		return TRUE;
	}

	target->flag |= TASK_FLAGS_FIN;
	SETPRIORITY(target->flag, TASK_FLAGS_WAIT);
	kAddListTail(&(gs_scheduler[_id].waitList), target);

	// 임계 영역 끝
	kUnlock_spinLock(&(gs_scheduler[_id].spinLock));
	return TRUE;
}

// 태스크가 자신을 종료
void kTaskExit(void) {
	kTaskFin(gs_scheduler[kGetAPICID()].runningTask->link.id);
}

// 준비 큐에 있는 모든 태스크 수 반환
int kGetReadyCnt(BYTE _id) {
	int cnt = 0, i;

	// 임계 영역 시작
	kLock_spinLock(&(gs_scheduler[_id].spinLock));

	// 모든 준비 큐를 확인해 태스크 개수 구함
	for(i = 0; i < TASK_MAXREADYCNT; i++) cnt += kGetListCnt(&(gs_scheduler[_id].readyList[i]));

	// 임계 영역 끝
	kUnlock_spinLock(&(gs_scheduler[_id].spinLock));
	return cnt;
}

// 전체 태스크 수 반환
int kGetTaskCnt(BYTE _id) {
	int cnt;

	// 준비 큐 태스크 수 구한 후 대기 큐 태스크 수와 현재 수행 중 태스크 수 더함
	cnt = kGetReadyCnt(_id);

	// 임계 영역 시작
	kLock_spinLock(&(gs_scheduler[_id].spinLock));
	cnt += kGetListCnt(&(gs_scheduler[_id].waitList)) + 1;

	// 임계 영역 끝
	kUnlock_spinLock(&(gs_scheduler[_id].spinLock));
	return cnt;
}

// TCB 풀에서 해당 오프셋 TCB 반환
TCB *kGetTCB(int offset) {
	if((offset < -1) && (offset > TASK_MAXCNT)) return NULL;
	return &(gs_tcbPoolManager.startAddr[offset]);
}

// 태스크가 존해자는지 여부 반환
BOOL kIsTaskExist(QWORD id) {
	TCB *tcb;

	// ID로 TCB 반환
	tcb = kGetTCB(GETTCBOFFSET(id));
	// TCB가 없거나 ID가 일치하지 않으면 존재하지 않음
	if((tcb == NULL) || (tcb->link.id != id)) return FALSE;
	return TRUE;
}

// 프로세서 사용률 반환
QWORD kGetProcLoad(BYTE _id) {
	return gs_scheduler[_id].procLoad;
}

static TCB *kGetProcThread(TCB *thread) {
	TCB *proc;

	// 만약 내가 프로세스면 자신을 반환
	if(thread->flag & TASK_FLAGS_PROC) return thread;

	// 내가 프로세스가 아니면 부모 프로세스로 설정된 태스크 ID를 통해 TCB풀에서 태스크 자료구조 추출
	proc = kGetTCB(GETTCBOFFSET(thread->parentProcID));

	// 만약 프로세스가 없거나 태스크 ID가 일치하지 않으면 NULL반환
	if((proc == NULL) || (proc->link.id != thread->parentProcID)) return NULL;

	return proc;
}

// 각 스케줄러의 태스크 수를 이용해 적절한 스케줄러에 태스크 추가. 부하 분산 기능을 사용하지 않으면 현재 코어에 삽입, 사용하면 태스크가 현재 수행되는 코어에서 계속 수행하니 task 변수에 적어도 APIC ID가 설정되어 있어야 함
void kAddTaskLoadBalancing(TCB *task) {
	BYTE _id, target;

	// 태스크가 동작하던 코어의 APIC 확인
	_id = task->_id;

	// 부하 분산 기능을 사용하고 프로세서 친화도가 모든 코어(0xFF)로 설정되었으면 부하 분산 수행
	if((gs_scheduler[_id].isLoadBalancing == TRUE) && (task->affinity == TASK_LOADBALANCING_ID)) target = kFindSchedulerCnt(task);
	// 태스크 부하 분산 기능과 관계 없이 프로세서 친화도 필드에 다른 코어 APIC ID가 들어있으면 해당 스케줄러로 옮김
	else if((task->affinity != _id) && (task->affinity != TASK_LOADBALANCING_ID)) target = task->affinity;
	// 부하 분산 기능 사용하지 않는 경우 현재 스케줄러에 다시 삽입
	else target = _id;

	// 임계 영역 시작
	kLock_spinLock(&(gs_scheduler[_id].spinLock));
	// 태스크를 추가할 스케줄러가 현재 스케줄러와 다르면 태스크 이동. FPU는 공유되지 않으니 현재 태스크가 FPU를 마지막으로 썼다면 FPU 콘텍스트를 메모리에 저장
	if((_id != target) && (task->link.id == gs_scheduler[_id].lastFPU)) {
		// FPU를 저장하기 전 TS 비트를 끄지 않으면 Device Not Available 예외가 발생하니 주의
		kClearTS();
		kSaveFPU(task->contextFPU);
		gs_scheduler[_id].lastFPU = TASK_INVALID_ID;
	}
	// 임계 영역 끝
	kUnlock_spinLock(&(gs_scheduler[_id].spinLock));

	// 임계 영역 시작
	kLock_spinLock(&(gs_scheduler[_id].spinLock));

	// 태스크를 수행할 코어의 APIC ID를 설정하고 해당 스케줄러에 태스크 삽입
	task->_id = target;
	kAddReadyList(target, task);

	// 임계 영역 끝
	kUnlock_spinLock(&(gs_scheduler[_id].spinLock));
}

// 태스크를 추가할 스케줄러 ID 반환. 파라미터로 전달된 태스크 자료구조에 적어도 플래그와 프로세서 친화도 필드가 채워져 있어야 함
static BYTE kFindSchedulerCnt(const TCB *task) {
	BYTE priority, i, minIdx;
	int nowCnt, minCnt, tmpCnt, procCnt;

	// 코어 개수 확인
	procCnt = kGetProcCnt();

	// 코어가 하나면 현재 코어 계속 수행
	if(procCnt == 1) return task->_id;

	// 우선순위 추출
	priority = GETPRIORITY(task->flag);

	// 태스크가 포함된 스케줄러에서 태스크와 같은 우선순위 태스크 수 확인
	nowCnt = kGetListCnt(&(gs_scheduler[task->_id].readyList[priority]));

	// 나머지 코어에서 현재 태스크와 같은 레벨 검사. 자신과 태스크 수가 적어도 2 이상 차이 나는 것 중 가장 태스크 수가 작은 스케줄러 ID 반환
	minCnt = TASK_MAXCNT;
	minIdx = task->_id;
	for(i = 0; i < procCnt; i++) {
		if(i == task->_id) continue;

		// 모든 스케줄러를 돌며 확인
		tmpCnt = kGetListCnt(&(gs_scheduler[i].readyList[priority]));

		// 현재 코어와 태스크 수가 2개 이상 차이나고 이전까지 태스크 수가 가장 작았던 코어보다 더 작으면 정보 갱신
		if((tmpCnt + 2 <= nowCnt) && (tmpCnt < minCnt)) {
			minIdx = i;
			minCnt = tmpCnt;
		}
	}
	return minIdx;
}

// 파라미터로 전달된 코어에 태스크 부하 분산 기능 사용 여부 설정
BYTE kSetTaskLoadBalancing(BYTE _id, BOOL isLoadBalancing) {
	gs_scheduler[_id].isLoadBalancing = isLoadBalancing;
}

// 프로세서 친화도 변경
BOOL kAlterAffinity(QWORD id, BYTE affinity) {
	TCB *target;
	BYTE _id;

	// 태스크가 포함된 코어의 로컬 APIC ID를 찾은 후 스핀락 잠금
	if(kFindSchedulerLock(id, &_id) == FALSE) return FALSE;

	// 현재 실행 중 태스크라면 프로세서 친화도만 변경. 실제 태스크가 옮겨지는 시점은 태스크 전환 수행할 때
	target = gs_scheduler[_id].runningTask;
	if(target->link.id == id) {
		// 프로세서 친화도 변경
		target->affinity = affinity;

		// 임계 영역 끝
		kUnlock_spinLock(&(gs_scheduler[_id].spinLock));
	} else {	// 실행 중인 태스크가 아니면 준비 리스트에서 찾아 즉시 이동
		// 준비 리스트에서 태스크를 찾지 못하면 직접 태스크를 찾아 친화도 설정
		target = kDelReadyList(_id, id);
		if(target == NULL) {
			target = kGetTCB(GETTCBOFFSET(id));
			if(target != NULL) target->affinity = affinity;
		} else target->affinity = affinity;

		// 임계 영역 끝
		kUnlock_spinLock(&(gs_scheduler[_id].spinLock));

		// 프로세서 부하 분산을 고려해 스케줄러에 등록
		kAddTaskLoadBalancing(target);
	}
	return TRUE;
}
	
// 유휴 태스크. 대기 큐에 삭제 대기 중인 태스크 정리
void kIdleTask(void) {
	TCB *task, *childThread, *proc;
	QWORD lastTickCnt, lastIdleTask, nowTickCnt, nowIdleTask, id, childThreadID;
	BYTE _id, procID;
	int i, cnt;
	void *threadLink;

	// 현재 코어 로컬 APIC 확인
	_id = kGetAPICID();

	// 프로세서 사용량 계산을 위해 기준 정보 저장
	lastIdleTask = gs_scheduler[_id].loopIdleTask;
	lastTickCnt = kGetTickCnt();

	while(1) {
		// 현재 상태 저장
		nowTickCnt = kGetTickCnt();
		nowIdleTask = gs_scheduler[_id].loopIdleTask;

		// 프로세서 사용량 계산(100 - 유휴 태스크가 사용한 프로세서 시간) * 100 / 시스템 전체에서 사용한 프로세서 시간)
		if((nowTickCnt - lastTickCnt) == 0) gs_scheduler[_id].procLoad = 0;
		else gs_scheduler[_id].procLoad = 100 - (nowIdleTask - lastIdleTask) * 100 / (nowTickCnt - lastTickCnt);

		// 현재 상태를 이전 상태에 보관
		lastTickCnt = nowTickCnt;
		lastIdleTask = nowIdleTask;

		// 프로세서의 부하에 따라 쉬게 함
		kHaltProc(_id);

		// 대기 큐에 대기 중인 태스크가 있으면 태스크 종료
		if(kGetListCnt(&(gs_scheduler[_id].waitList)) > 0) {
			while(1) {
				// 임계 영역 시작
				kLock_spinLock(&(gs_scheduler[_id].spinLock));
				task = kDelListHead(&(gs_scheduler[_id].waitList));
				// 임계 영역 끝
				kUnlock_spinLock(&(gs_scheduler[_id].spinLock));
				if(task == NULL) break;

				if(task->flag & TASK_FLAGS_PROC) {
					// 프로세스 종료시 자식 쓰레드가 존재하면 쓰레드 모두 종료 후 다시 자식 쓰레드 리스트에 삽입
					cnt = kGetListCnt(&(task->childThreadList));
					for(i = 0; i < cnt; i++) {
						// 임계 영역 시작
						kLock_spinLock(&(gs_scheduler[_id].spinLock));
						// 쓰레드 링크의 어드레스에서 꺼내 쓰레드 종료시킴
						threadLink = (TCB*)kDelListHead(&(task->childThreadList));
						if(threadLink == NULL) {
							// 임계 영역 끝
							kUnlock_spinLock(&(gs_scheduler[_id].spinLock));
							break;
						}
						
						// 자식 쓰레드 리스트에 연결된 정보는 태스크 자료구조에 있는 threadLink의 시작 어드레스이므로
						// 태스크 자료구조 시작 어드레스를 구하려면 별도 계산이 필요
						childThread = GETTCBTHREAD(threadLink);

						// 다시 자식 쓰레드 리스트에 삽입해 해당 쓰레드 종료시 자식 쓰레드가 프로세스를 찾아
						// 스스로 리스트에서 제거하도록 함
						kAddListTail(&(task->childThreadList), &(childThread->threadLink));
						childThreadID = childThread->link.id;

						// 임계 영역 끝
						kUnlock_spinLock(&(gs_scheduler[_id].spinLock));

						// 자식 쓰레드 찾아서 종료
						kTaskFin(childThreadID);
					}

					// 아직 자식 쓰레드가 남아있다면 자식 쓰레드가 다 종료될 때까지 기다려야 하므로 다시 대기 리스트에 삽입
					if(kGetListCnt(&(task->childThreadList)) > 0) {
						// 임계 영역 시작
						kLock_spinLock(&(gs_scheduler[_id].spinLock));
						kAddListTail(&(gs_scheduler[_id].waitList), task);

						// 임계 영역 끝
						kUnlock_spinLock(&(gs_scheduler[_id].spinLock));
						continue;
					} else { // 프로세스를 종료해야 하므로 할당받은 메모리 영역 삭제
						if(task->flag & TASK_FLAGS_USERLV) kFreeMem(task->memAddr);
					}
				} else if(task->flag & TASK_FLAGS_THREAD) {
					// 쓰레드라면 프로세스의 자식 쓰레드 리스트에서 제거
					proc = kGetProcThread(task);
					if(proc != NULL) if(kFindSchedulerLock(proc->link.id, &procID) == TRUE) {
						// 프로세스 ID로 프로세스가 속한 스케줄러의 ID를 찾고 스핀락 잠금
						kDelList(&(proc->childThreadList), task->link.id);
						kUnlock_spinLock(&(gs_scheduler[procID].spinLock));
					}
				}

				// 태스크 정상 종료이므로 스택 및 자료구조 반환
				id = task->link.id;
				kFreeMem(task->stackAddr);
				kFreeTCB(id);

				kPrintf("### IDLE: Task ID[0x%q] is Completely Finished.\n", id);
			}
		}
	kSchedule();
	}
}

// 측정된 프로세서 부하에 따라 프로세서를 쉬게 함
void kHaltProc(BYTE _id) {
	if(gs_scheduler[_id].procLoad < 40) {
		kHlt(); kHlt(); kHlt();
	} else if(gs_scheduler[_id].procLoad < 80) {
		kHlt(); kHlt();
	} else if(gs_scheduler[_id].procLoad < 95) kHlt();
}

// 마지막으로 FPU를 사용한 태스크 ID 반환
QWORD kGetLastFPU(BYTE _id) {
	return gs_scheduler[_id].lastFPU;
}

// 마지막으로 FPU를 사용한 태스크 ID 설정
void kSetLastFPU(BYTE _id, QWORD id) {
	gs_scheduler[_id].lastFPU = id;
}
