/*
 * Task.c
 *
 *  Created on: 2017. 7. 30.
 *      Author: Yummy
 */

#include <Task.h>
#include <Descriptor.h>

// 스케쥴러 관련 자료구조
static SCHEDULER gs_scheduler;
static TCBPOOLMANAGER gs_TCBPoolManager;

// 태스크 풀 초기화
void initTCBPool(void) {
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
TCB *allocTCB(void) {
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
void freeTCB(QWORD id) {
	int i;

	// 태스크 ID의 하위 32비트가 인덱스 역할 함
	i = id & 0xFFFFFFFF;

	// TCB 초기화 후 ID 설정
	memSet(&(gs_TCBPoolManager.startAddr[i].context), 0, sizeof(CONTEXT));
	gs_TCBPoolManager.startAddr[i].link.id = i;

	gs_TCBPoolManager.useCnt--;
}

// 태스크 생성. 태스크 ID에 따라 스택 풀에서 스택 자동 할당
TCB *createTask(QWORD flag, QWORD epAddr) {
	TCB *task;
	void *stackAddr;

	task = allocTCB();
	if(task == NULL) return NULL;

	// 태스크 ID로 스택 어드레스 계산, 하위 32비트가 스택 풀의 오프셋 역할 수행
	stackAddr = (void*)(TASK_STACKPOOLADDR + (TASK_STACKSIZE * (task->link.id & 0xFFFFFFFF)));

	// TCB를 설정한 후 준비 리스트에 삽입해 스케줄링 될 수 있도록 함
	setTask(task, flag, epAddr, stackAddr, TASK_STACKSIZE);
	addReadyList(task);

	return task;
}

// 파라미터를 이용해 TCB설정
void setTask(TCB *tcb, QWORD flag, QWORD epAddr, void *stackAddr, QWORD stackSize) {
	// 콘텍스트 초기화
	memSet(tcb->context.reg, 0, sizeof(tcb->context.reg));

	// 스택 관련 RSP, RBP 레지스터 설정
	tcb->context.reg[TASK_RSPOFFSET] = (QWORD)stackAddr + stackSize;
	tcb->context.reg[TASK_RBPOFFSET] = (QWORD)stackAddr + stackSize;

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
	// 태스크 풀 초기화
	initTCBPool();

	// 준비 리스트 초기화
	initList(&(gs_scheduler.readyList));

	// TCB를 할당받아 실행 중 태스크로 설정하여 부팅을 수행한 태스크 저장할 TCB 준비
	gs_scheduler.runningTask = allocTCB();
}

// 현재 수행 중 태스크 설정
void setRunningTask(TCB *task) {
	gs_scheduler.runningTask = task;
}

// 현재 수행 중 태스크 반환
TCB *getRunningTask(void) {
	return gs_scheduler.runningTask;
}

// 태스크 리스트에서 다음으로 실행할 태스크 얻음
TCB *getNextTask(void) {
	if(getListCnt(&(gs_scheduler.readyList)) == 0) return NULL;

	return (TCB*)delListHead(&(gs_scheduler.readyList));
}

// 태스크를 스케줄러 준비 리스트에 삽입
void addReadyList(TCB *task) {
	addListTail(&(gs_scheduler.readyList), task);
}

// 다른 태스크 찾아 전환. 인터럽트나 예외 발생시 호출하면 안됨
void schedule(void) {
	TCB *runningTask, *nextTask;
	BOOL prevFlag;

	// 전환할 태스크가 있어야 함
	if(getListCnt(&(gs_scheduler.readyList)) == 0) return;

	// 전환 도중 인터럽트가 발생해 태스크 전환이 또 일어나면 곤란하므로 전환하는 동안 인터럽트 발생 X
	prevFlag = setInterruptFlag(FALSE);
	// 실행할 다음 태스크 얻음
	nextTask = getNextTask();
	if(nextTask == NULL) {
		setInterruptFlag(prevFlag);
		return;
	}

	runningTask = gs_scheduler.runningTask;
	addReadyList(runningTask);

	// 다음 태스크를 현재 수행 중 태스크로 설정한 후 콘텍스트 전환
	gs_scheduler.runningTask = nextTask;
	switchContext(&(runningTask->context), &(nextTask->context));

	// 프로세서 사용 시간 업데이트
	gs_scheduler.time = TASK_PROCESSORTIME;

	setInterruptFlag(prevFlag);
}

// 인터럽트 발생 시 다른 태스크를 찾아 전환. 반드시 인터럽트나 예외 발생 시 호출해야함
BOOL scheduleInterrupt(void) {
	TCB *runningTask, *nextTask;
	char *contextAddr;

	// 전환할 태스크 없으면 종료
	nextTask = getNextTask();
	if(nextTask == NULL) return FALSE;

	// 태스크 전환 처리. 인터럽트 핸들러에서 저장한 콘텍스트를 다른 콘텍스트로 덮어쓰는 방법
	contextAddr = (char*)IST_STARTADDR + IST_SIZE - sizeof(CONTEXT);

	// 현재 태스크를 얻어 IST에 있는 콘텍스트 복사 후 현재 태스크를 준비 리스트로 옮김
	runningTask = gs_scheduler.runningTask;
	memCpy(&(runningTask->context), contextAddr, sizeof(CONTEXT));
	addReadyList(runningTask);

	// 전환해서 실행할 태스크를 Running Task로 설정하고, 콘텍스트를 IST에 복사해 자동으로 태스크 전환하게 함
	gs_scheduler.runningTask = nextTask;
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
