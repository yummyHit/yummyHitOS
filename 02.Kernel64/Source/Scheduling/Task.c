/*
 * Task.c
 *
 *  Created on: 2017. 7. 30.
 *      Author: Yummy
 */

#include <Task.h>
#include <Descriptor.h>
#include <Synchronize.h>

// 스케쥴러 관련 자료구조
static SCHEDULER gs_scheduler;
static TCBPOOLMANAGER gs_TCBPoolManager;

// 태스크 풀 초기화
static void initTCBPool(void) {
	int i;

	memSet(&(gs_TCBPoolManager), 0, sizeof(gs_TCBPoolManager));

	// 태스크 풀 어드레스 지정 후 초기화
	gs_TCBPoolManager.startAddr = (TCB*)TASK_TCBPOOLADDR;
	memSet(TASK_TCBPOOLADDR, 0, sizeof(TCB) * TASK_MAXCNT);

	// TCB에 ID 할당
	for(i = 0; i < TASK_MAXCNT; i++) gs_TCBPoolManager.startAddr[i].link.id = i;

	// TCB의 최대 개수와 할당된 횟수 초기화
	gs_TCBPoolManager.maxCnt = TASK_MAXCNT;
	gs_TCBPoolManager.allocCnt = 1;
}

// TCB 할당받음
static TCB *allocTCB(void) {
	TCB *empty;
	int i;

	if(gs_TCBPoolManager.useCnt == gs_TCBPoolManager.maxCnt) return NULL;

	for(i = 0; i < gs_TCBPoolManager.maxCnt; i++) if((gs_TCBPoolManager.startAddr[i].link.id >> 32) == 0) {
		empty = &(gs_TCBPoolManager.startAddr[i]);
		break;
	}

	// 상위 32비트를 0이 아닌 값으로 설정해 할당된 TCB로 설정
	empty->link.id = ((QWORD)gs_TCBPoolManager.allocCnt << 32) | i;
	gs_TCBPoolManager.useCnt++;
	gs_TCBPoolManager.allocCnt++;
	if(gs_TCBPoolManager.allocCnt == 0) gs_TCBPoolManager.allocCnt = 1;
	return empty;
}

// TCB 해제
static void freeTCB(QWORD id) {
	int i;

	// 태스크 ID의 하위 32비트가 인덱스 역할 함
	i = GETTCBOFFSET(id);

	// TCB 초기화 후 ID 설정
	memSet(&(gs_TCBPoolManager.startAddr[i].context), 0, sizeof(CONTEXT));
	gs_TCBPoolManager.startAddr[i].link.id = i;

	gs_TCBPoolManager.useCnt--;
}

// 태스크 생성. 태스크 ID에 따라 스택 풀에서 스택 자동 할당
TCB *createTask(QWORD flag, void *memAddr, QWORD memSize, QWORD epAddr) {
	TCB *task, *proc;
	void *stackAddr;
	BOOL preFlag;

	task = allocTCB();
	if(task == NULL) return NULL;

	stackAddr = allocMem(TASK_STACKSIZE);
	if(stackAddr == NULL) {
		freeTCB(task->link.id);
		return NULL;
	}

	// 임계 영역 시작
	preFlag = lockData();

	// 현재 프로세스 또는 쓰레드가 속한 프로세스 검색
	proc = getProcThread(getRunningTask());
	// 만약 프로세스가 없다면 아무 작업도 안함
	if(proc == NULL) {
		freeTCB(task->link.id);
		freeMem(stackAddr);
		// 임계 영역 끝
		unlockData(preFlag);
		return NULL;
	}

	// 쓰레드를 생성하는 경우라면 내가 속한 프로세스의 자식 쓰레드 리스트에 연결
	if(flag & TASK_FLAGS_THREAD) {
		//현재 쓰레드의 프로세스를 찾아 생성할 쓰레드에 프로세스 정보 상속
		task->parentProcID = proc->link.id;
		task->memAddr = proc->memAddr;
		task->memSize = proc->memSize;

		// 부모 프로세스의 자식 스레드 리스트에 추가
		addListTail(&(proc->childThreadList), &(task->threadLink));
	} else {	// 프로세스는 파라미터로 넘어온 값 그대로 설정
		task->parentProcID = proc->link.id;
		task->memAddr = memAddr;
		task->memSize = memSize;
	}

	// 스레드 ID를 태스크 ID와 동일하게 설정
	task->threadLink.id = task->link.id;

	// 임계 영역 끝
	unlockData(preFlag);

	// 태스크 ID로 스택 어드레스 계산, 하위 32비트가 스택 풀의 오프셋 역할 수행
//	stackAddr = (void*)(TASK_STACKPOOLADDR + (TASK_STACKSIZE * (GETTCBOFFSET(task->link.id))));

	// TCB를 설정한 후 준비 리스트에 삽입해 스케줄링 될 수 있도록 함
	setTask(task, flag, epAddr, stackAddr, TASK_STACKSIZE);

	// 자식 쓰레드 리스트 초기화
	initList(&(task->childThreadList));

	// FPU 사용 여부를 사용하지 않은 것으로 초기화
	task->fpuUsed = FALSE;

	// 임계 영역 시작
	preFlag = lockData();

	// 태스크를 준비 리스트에 삽입
	addReadyList(task);

	// 임계 영역 끝
	unlockData(preFlag);

	return task;
}

// 파라미터를 이용해 TCB설정
static void setTask(TCB *tcb, QWORD flag, QWORD epAddr, void *stackAddr, QWORD stackSize) {
	// 콘텍스트 초기화
	memSet(tcb->context.reg, 0, sizeof(tcb->context.reg));

	// 스택 관련 RSP, RBP 레지스터 설정
	tcb->context.reg[TASK_RSPOFFSET] = (QWORD)stackAddr + stackSize - 8;
	tcb->context.reg[TASK_RBPOFFSET] = (QWORD)stackAddr + stackSize - 8;

	// Return Address 영역에 taskExit()함수의 어드레스 삽입. 엔트리 포인트 함수 빠져나가며 taskExit()함수로 이동
	*(QWORD*)((QWORD)stackAddr + stackSize - 8) = (QWORD)taskExit;

	// 세그먼트 셀렉터 설정
	tcb->context.reg[TASK_CSOFFSET] = GDT_KERNELCODESEGMENT;
	tcb->context.reg[TASK_DSOFFSET] = GDT_KERNELDATASEGMENT;
	tcb->context.reg[TASK_ESOFFSET] = GDT_KERNELDATASEGMENT;
	tcb->context.reg[TASK_FSOFFSET] = GDT_KERNELDATASEGMENT;
	tcb->context.reg[TASK_GSOFFSET] = GDT_KERNELDATASEGMENT;
	tcb->context.reg[TASK_SSOFFSET] = GDT_KERNELDATASEGMENT;

	// RIP 레지스터와 인터럽트 플래그 설정
	tcb->context.reg[TASK_RIPOFFSET] = epAddr;

	// RFLAGS 레지스터의 IF 비트(비트 9)를 1로 설정해 인터럽트 활성화
	tcb->context.reg[TASK_RFLAGSOFFSET] |= 0x0200;

	// ID 및 스택, 그리고 플래그 저장
	tcb->stackAddr = stackAddr;
	tcb->stackSize = stackSize;
	tcb->flag = flag;
}

// 스케줄러 초기화. 스케줄러 초기화에 필요한 TCB 풀과 init Task도 같이 초기화
void initScheduler(void) {
	int i;
	TCB *task;

	// 태스크 풀 초기화
	initTCBPool();

	// 준비 리스트와 우선순위별 실행 횟수를 초기화하고 대기 리스트도 초기화
	for(i = 0; i < TASK_MAXREADYCNT; i++) {
		initList(&(gs_scheduler.readyList[i]));
		gs_scheduler.exeCnt[i] = 0;
	}
	initList(&(gs_scheduler.waitList));

	// TCB를 할당받아 부팅을 수행한 태스크를 커널 최초 프로세스로 설정
	task = allocTCB();
	gs_scheduler.runningTask = task;
	task->flag = TASK_FLAGS_HIGHEST | TASK_FLAGS_PROCESS | TASK_FLAGS_SYSTEM;
	task->parentProcID = task->link.id;
	task->memAddr = (void*)0x100000;
	task->memSize = 0x500000;
	task->stackAddr = (void*)0x600000;
	task->stackSize = 0x100000;

	// 프로세서 사용률 계산하는데 사용하는 자료구조 초기화
	gs_scheduler.loopIdleTask = 0;
	gs_scheduler.processorLoad = 0;

	// FPU를 사용한 태스크 ID를 유효하지 않은 값으로 초기화
	gs_scheduler.lastFPU = TASK_INVALID_ID;
}

// 현재 수행 중 태스크 설정
void setRunningTask(TCB *task) {
	BOOL preFlag;

	// 임계 영역 시작
	preFlag = lockData();

	gs_scheduler.runningTask = task;

	// 임계 영역 끝
	unlockData(preFlag);
}

// 현재 수행 중 태스크 반환
TCB *getRunningTask(void) {
	BOOL preFlag;
	TCB *runningTask;

	// 임계 영역 시작
	preFlag = lockData();

	runningTask = gs_scheduler.runningTask;

	// 임계 영역 끝
	unlockData(preFlag);

	return runningTask;
}

// 태스크 리스트에서 다음으로 실행할 태스크 얻음
static TCB *getNextTask(void) {
	TCB *target = NULL;
	int cnt, i, j;

	// 큐에 태스크가 있으나 모든 태스크가 1회씩 실행된 경우 프로세서를 양보해 태스크를 선택 안 할 수 있으니 NULL일 경우 1회 더 수행
	for(j = 0; j < 2; j++) {
		for(i = 0; i < TASK_MAXREADYCNT; i++) {
			cnt = getListCnt(&(gs_scheduler.readyList[i]));

			// 만약 실행한 횟수보다 리스트 태스크 수가 더 많으면 우선순위 태스크 실행
			if(gs_scheduler.exeCnt[i] < cnt) {
				target = (TCB*)delListHead(&(gs_scheduler.readyList[i]));
				gs_scheduler.exeCnt[i]++;
				break;
			} else gs_scheduler.exeCnt[i] = 0;
		}

		// 만약 수행할 태스크 찾았으면 종료
		if(target != NULL) break;
	}
	return target;
}

// 태스크를 스케줄러 준비 리스트에 삽입
static BOOL addReadyList(TCB *task) {
	BYTE priority;

	priority = GETPRIORITY(task->flag);
	if(priority == TASK_FLAGS_WAIT) {
		addListTail(&(gs_scheduler.waitList), task);
		return TRUE;
	}
	else if(priority >= TASK_MAXREADYCNT) return FALSE;
	addListTail(&(gs_scheduler.readyList[priority]), task);
	return TRUE;
}

// 준비 큐에서 태스크 제거
static TCB *delReadyList(QWORD id) {
	TCB *target;
	BYTE priority;

	// 태스크 ID가 유효하지 않으면 실패
	if(GETTCBOFFSET(id) >= TASK_MAXCNT) return NULL;

	// TCB 풀에서 해당 태스크의 TCB를 찾아 실제로 ID가 일치하는지 확인
	target = &(gs_TCBPoolManager.startAddr[GETTCBOFFSET(id)]);
	if(target->link.id != id) return NULL;

	// 태스크가 존재하는 준비 리스트에서 태스크 제거
	priority = GETPRIORITY(target->flag);

	target = delList(&(gs_scheduler.readyList[priority]), id);
	return target;
}

// 태스크 우선순위 변경
BOOL changePriority(QWORD id, BYTE priority) {
	TCB *target;
	BOOL preFlag;

	if(priority > TASK_MAXREADYCNT) return FALSE;

	// 임계 영역 시작
	preFlag = lockData();

	// 현재 실행 중인 태스크면 우선순위만 변경, PIT 컨트롤러 인터럽트(IRQ 0)가 발생해 태스크 전환 수행시 변경된 우선순위 리스트로 이동
	target = gs_scheduler.runningTask;
	if(target->link.id == id) SETPRIORITY(target->flag, priority);
	else {	// 실행 중인 태스크가 아니면 준비 리스트에서 찾아 해당 우선순위 리스트로 이동
		// 준비 리스트에서 태스크를 찾지 못하면 직접 우선순위 설정
		target = delReadyList(id);
		if(target == NULL) {
			// 태스크 ID로 직접 찾아 설정
			target = getTCB(GETTCBOFFSET(id));
			if(target != NULL) SETPRIORITY(target->flag, priority);
		} else {
				SETPRIORITY(target->flag, priority);
				addReadyList(target);
		}
	}

	// 임계 영역 끝
	unlockData(preFlag);
	return TRUE;
}

// 다른 태스크 찾아 전환. 인터럽트나 예외 발생시 호출하면 안됨
BOOL schedule(void) {
	TCB *runningTask, *nextTask;
	BOOL preFlag;

	// 전환할 태스크가 있어야 함
	if(getReadyCnt() < 1) return FALSE;

	// 전환 도중 인터럽트가 발생해 태스크 전환이 또 일어나면 곤란하므로 전환하는 동안 인터럽트 발생 X
	// 임계 영역 시작
	preFlag = lockData();

	// 실행할 다음 태스크 얻음
	nextTask = getNextTask();
	if(nextTask == NULL) {
		// 임계 영역 끝
		unlockData(preFlag);
		return FALSE;
	}

	// 현재 수행 중인 태스크 정보 수정한 뒤 콘텍스트 전환
	runningTask = gs_scheduler.runningTask;
	gs_scheduler.runningTask = nextTask;

	// 유휴 태스크에서 전환되었다면 사용한 프로세서 시간 증가시킴
	if((runningTask->flag & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE) gs_scheduler.loopIdleTask += TASK_PROCESSORTIME - gs_scheduler.time;

	// 다음에 수행할 태스크가 FPU를 쓴 태스크가 아니라면 TS 비트 설정
	if(gs_scheduler.lastFPU != nextTask->link.id) setTS();
	else clearTS();

	// 프로세서 사용 시간 업데이트
	gs_scheduler.time = TASK_PROCESSORTIME;

	// 태스크 종료 플래그가 설정된 경우 콘텍스트를 저장할 필요가 없으니 대기 리스트에 삽입하고 콘텍스트 전환
	if(runningTask->flag & TASK_FLAGS_FIN) {
		addListTail(&(gs_scheduler.waitList), runningTask);
		// 임계 영역 끝
		unlockData(preFlag);
		switchContext(NULL, &(nextTask->context));
	} else {
		addReadyList(runningTask);
		// 임계 영역 끝
		unlockData(preFlag);
		switchContext(&(runningTask->context), &(nextTask->context));
	}

	unlockData(preFlag);
	return FALSE;
}

// 인터럽트 발생 시 다른 태스크를 찾아 전환. 반드시 인터럽트나 예외 발생 시 호출해야함
BOOL scheduleInterrupt(void) {
	TCB *runningTask, *nextTask;
	char *contextAddr;
	BOOL preFlag;

	// 임계 영역 시작
	preFlag = lockData();

	// 전환할 태스크 없으면 종료
	nextTask = getNextTask();
	if(nextTask == NULL) {
		// 임계 영역 끝
		unlockData(preFlag);
		return FALSE;
	}

	// 태스크 전환 처리. 인터럽트 핸들러에서 저장한 콘텍스트를 다른 콘텍스트로 덮어쓰는 방법
	contextAddr = (char*)IST_STARTADDR + IST_SIZE - sizeof(CONTEXT);

	// 현재 수행 중 태스크 정보 수정 후 콘텍스트 전환
	runningTask = gs_scheduler.runningTask;
	gs_scheduler.runningTask = nextTask;

	// 유휴 태스크에서 전환되었으면 사용한 프로세서 시간 증가
	if((runningTask->flag & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE) gs_scheduler.loopIdleTask += TASK_PROCESSORTIME;

	// 태스크 종료 플래그가 설정된 경우 콘텍스트를 저장하지 않고 대기 리스트에만 삽입
	if(runningTask->flag & TASK_FLAGS_FIN) addListTail(&(gs_scheduler.waitList), runningTask);
	else {	// 태스크가 종료되지 않으면 IST에 있는 콘텍스트 복사 후 현재 태스크를 준비 리스트로 옮김
		memCpy(&(runningTask->context), contextAddr, sizeof(CONTEXT));
		addReadyList(runningTask);
	}

	// 다음에 수행할 태스크가 FPU를 쓴 태스크가 아니라면 TS 비트 설정
	if(gs_scheduler.lastFPU != nextTask->link.id) setTS();
	else clearTS();

	// 임계 영역 끝
	unlockData(preFlag);

	// 전환해서 실행할 태스크를 Running Task로 설정하고, 콘텍스트를 IST에 복사해 자동으로 태스크 전환하게 함
	memCpy(contextAddr, &(nextTask->context), sizeof(CONTEXT));

	// 프로세서 사용 시간 업데이트
	gs_scheduler.time = TASK_PROCESSORTIME;
	return TRUE;
}

// 프로세서 사용할 수 있는 시간 하나 줄임
void reduceProcessorTime(void) {
	if(gs_scheduler.time > 0) gs_scheduler.time--;
}

// 프로세서 사용할 수 있는 시간이 다 되었는지 여부 반환
BOOL isProcessorTime(void) {
	if(gs_scheduler.time <= 0) return TRUE;
	return FALSE;
}

// 태스크 종료
BOOL taskFin(QWORD id) {
	TCB *target;
	BYTE priority;
	BOOL preFlag;

	// 임계 영역 시작
	preFlag = lockData();

	// 현재 실행 중 태스크이면 Fin 비트 설정 후 태스크 전환
	target = gs_scheduler.runningTask;
	if(target->link.id == id) {
		target->flag |= TASK_FLAGS_FIN;
		SETPRIORITY(target->flag, TASK_FLAGS_WAIT);
		// 임계 영역 끝
		unlockData(preFlag);

		schedule();
	} else {	// 실행 중 태스크가 아니면 준비 큐에서 직접 찾아 대기 리스트에 연결
		target = delReadyList(id);
		if(target == NULL) {
			target = getTCB(GETTCBOFFSET(id));
			if(target != NULL) {
				target->flag |= TASK_FLAGS_FIN;
				SETPRIORITY(target->flag, TASK_FLAGS_WAIT);
			}
			// 임계 영역 끝
			unlockData(preFlag);
			return TRUE;
		}
		target->flag |= TASK_FLAGS_FIN;
		SETPRIORITY(target->flag, TASK_FLAGS_WAIT);
		addListTail(&(gs_scheduler.waitList), target);
	}

	// 임계 영역 끝
	unlockData(preFlag);
	return TRUE;
}

// 태스크가 자신을 종료
void taskExit(void) {
	taskFin(gs_scheduler.runningTask->link.id);
}

// 준비 큐에 있는 모든 태스크 수 반환
int getReadyCnt(void) {
	int cnt = 0, i;
	BOOL preFlag;

	// 임계 영역 시작
	preFlag = lockData();

	// 모든 준비 큐를 확인해 태스크 개수 구함
	for(i = 0; i < TASK_MAXREADYCNT; i++) cnt += getListCnt(&(gs_scheduler.readyList[i]));

	// 임계 영역 끝
	unlockData(preFlag);
	return cnt;
}

// 전체 태스크 수 반환
int getTaskCnt(void) {
	int cnt;
	BOOL preFlag;

	// 준비 큐 태스크 수 구한 후 대기 큐 태스크 수와 현재 수행 중 태스크 수 더함
	cnt = getReadyCnt();

	// 임계 영역 시작
	preFlag = lockData();
	cnt += getListCnt(&(gs_scheduler.waitList)) + 1;

	// 임계 영역 끝
	unlockData(preFlag);
	return cnt;
}

// TCB 풀에서 해당 오프셋 TCB 반환
TCB *getTCB(int offset) {
	if((offset < -1) && (offset > TASK_MAXCNT)) return NULL;
	return &(gs_TCBPoolManager.startAddr[offset]);
}

// 태스크가 존해자는지 여부 반환
BOOL isTaskExist(QWORD id) {
	TCB *tcb;

	// ID로 TCB 반환
	tcb = getTCB(GETTCBOFFSET(id));
	// TCB가 없거나 ID가 일치하지 않으면 존재하지 않음
	if((tcb == NULL) || (tcb->link.id != id)) return FALSE;
	return TRUE;
}

// 프로세서 사용률 반환
QWORD getProcessorLoad(void) {
	return gs_scheduler.processorLoad;
}

static TCB *getProcThread(TCB *thread) {
	TCB *proc;

	// 만약 내가 프로세스면 자신을 반환
	if(thread->flag & TASK_FLAGS_PROCESS) return thread;

	// 내가 프로세스가 아니면 부모 프로세스로 설정된 태스크 ID를 통해 TCB풀에서 태스크 자료구조 추출
	proc = getTCB(GETTCBOFFSET(thread->parentProcID));

	// 만약 프로세스가 없거나 태스크 ID가 일치하지 않으면 NULL반환
	if((proc == NULL) || (proc->link.id != thread->parentProcID)) return NULL;

	return proc;
}

// 대기 큐에 삭제 대기 중인 태스크 정리
void idleTask(void) {
	TCB *task, *childThread, *proc;
	QWORD lastTickCnt, lastIdleTask, nowTickCnt, nowIdleTask, taskID;
	BOOL preFlag;
	int i, cnt;
	void *threadLink;

	// 프로세서 사용량 계산을 위해 기준 정보 저장
	lastIdleTask = gs_scheduler.loopIdleTask;
	lastTickCnt = getTickCnt();

	while(1) {
		// 현재 상태 저장
		nowTickCnt = getTickCnt();
		nowIdleTask = gs_scheduler.loopIdleTask;

		// 프로세서 사용량 계산(100 - 유휴 태스크가 사용한 프로세서 시간) * 100 / 시스템 전체에서 사용한 프로세서 시간)
		if(nowTickCnt - lastTickCnt == 0) gs_scheduler.processorLoad = 0;
		else gs_scheduler.processorLoad = 100 - (nowIdleTask - lastIdleTask) * 100 / (nowTickCnt - lastTickCnt);

		// 현재 상태를 이전 상태에 보관
		lastTickCnt = nowTickCnt;
		lastIdleTask = nowIdleTask;

		// 프로세서의 부하에 따라 쉬게 함
		haltProcessor();

		// 대기 큐에 대기 중인 태스크가 있으면 태스크 종료
		if(getListCnt(&(gs_scheduler.waitList)) > 0) {
			while(1) {
				// 임계 영역 시작
				preFlag = lockData();
				task = delListHead(&(gs_scheduler.waitList));
				// 임계 영역 끝
				unlockData(preFlag);
				if(task == NULL) break;

				if(task->flag & TASK_FLAGS_PROCESS) {
					// 임계 영역 시작
					preFlag = lockData();
					// 프로세스 종료시 자식 쓰레드가 존재하면 쓰레드 모두 종료 후 다시 자식 쓰레드 리스트에 삽입
					cnt = getListCnt(&(task->childThreadList));
					for(i = 0; i < cnt; i++) {
						// 쓰레드 링크의 어드레스에서 꺼내 쓰레드 종료시킴
						threadLink = (TCB*)delListHead(&(task->childThreadList));
						if(threadLink == NULL) {
							// 임계 영역 끝
							unlockData(preFlag);
							break;
						}
						
						// 자식 쓰레드 리스트에 연결된 정보는 태스크 자료구조에 있는 threadLink의 시작 어드레스이므로
						// 태스크 자료구조 시작 어드레스를 구하려면 별도 계산이 필요
						childThread = GETTCBTHREAD(threadLink);

						// 다시 자식 쓰레드 리스트에 삽입해 해당 쓰레드 종료시 자식 쓰레드가 프로세스를 찾아
						// 스스로 리스트에서 제거하도록 함
						addListTail(&(task->childThreadList), &(childThread->threadLink));

						// 임계 영역 끝
						unlockData(preFlag);

						// 자식 쓰레드 찾아서 종료
						taskFin(childThread->link.id);
					}

					// 아직 자식 쓰레드가 남아있다면 자식 쓰레드가 다 종료될 때까지 기다려야 하므로 다시 대기 리스트에 삽입
					if(getListCnt(&(task->childThreadList)) > 0) {
						// 임계 영역 시작
						preFlag = lockData();
						addListTail(&(gs_scheduler.waitList), task);

						// 임계 영역 끝
						unlockData(preFlag);
						continue;
					} else { // 프로세스를 종료해야 하므로 할당받은 메모리 영역 삭제
						if(task->flag & TASK_FLAGS_USERLV) freeMem(task->memAddr);
					}
				} else if(task->flag & TASK_FLAGS_THREAD) {
					// 쓰레드라면 프로세스의 자식 쓰레드 리스트에서 제거
					proc = getProcThread(task);
					if(proc != NULL) delList(&(proc->childThreadList), task->link.id);
				}

				taskID = task->link.id;
				freeMem(task->stackAddr);
				freeTCB(taskID);

				printF("### IDLE: Task ID[0x%q] is Completely Finished.\n", task->link.id);
			}
		}
		schedule();
	}
}

// 측정된 프로세서 부하에 따라 프로세서를 쉬게 함
void haltProcessor(void) {
	if(gs_scheduler.processorLoad < 40) {
		_hlt(); _hlt(); _hlt();
	} else if(gs_scheduler.processorLoad < 80) {
		_hlt(); _hlt();
	} else if(gs_scheduler.processorLoad < 95) _hlt();
}

// 마지막으로 FPU를 사용한 태스크 ID 반환
QWORD getLastFPU(void) {
	return gs_scheduler.lastFPU;
}

// 마지막으로 FPU를 사용한 태스크 ID 설정
void setLastFPU(QWORD id) {
	gs_scheduler.lastFPU = id;
}
