/*
 * CLITask.h
 *
 *  Created on: 2017. 7. 30.
 *      Author: Yummy
 */

#ifndef __CLITASK_H__
#define __CLITASK_H__

#include <Types.h>
#include <List.h>
#include <Synchronize.h>

#pragma once
// SS, RSP, RFLAGS, CS, RIP + ISR에서 저장하는 19개의 레지스터
#define TASK_REGCNT		(5 + 19)
#define TASK_REGSIZE		8

// Context 자료구조 레지스터 오프셋
#define TASK_GSOFFSET		0
#define TASK_FSOFFSET		1
#define TASK_ESOFFSET		2
#define TASK_DSOFFSET		3
#define TASK_R15OFFSET		4
#define TASK_R14OFFSET		5
#define TASK_R13OFFSET		6
#define TASK_R12OFFSET		7
#define TASK_R11OFFSET		8
#define TASK_R10OFFSET		9
#define TASK_R9OFFSET		10
#define TASK_R8OFFSET		11
#define TASK_RSIOFFSET		12
#define TASK_RDIOFFSET		13
#define TASK_RDXOFFSET		14
#define TASK_RCXOFFSET		15
#define TASK_RBXOFFSET		16
#define TASK_RAXOFFSET		17
#define TASK_RBPOFFSET		18
#define TASK_RIPOFFSET		19
#define TASK_CSOFFSET		20
#define TASK_RFLAGSOFFSET	21
#define TASK_RSPOFFSET		22
#define TASK_SSOFFSET		23

// 태스크 풀 어드레스
#define TASK_TCBPOOLADDR	0x800000
#define TASK_MAXCNT		1024

// 스택 풀과 스택 크기
#define TASK_STACKPOOLADDR	(TASK_TCBPOOLADDR + sizeof(TCB) * TASK_MAXCNT)
#define TASK_STACKSIZE		8192

// 유효하지 않은 태스크 ID
#define TASK_INVALID_ID		0xFFFFFFFFFFFFFFFF

// 태스크가 최대로 쓸 수 있는 프로세서 시간(5ms)
#define TASK_PROCESSORTIME	5

// 준비 리스트의 수
#define TASK_MAXREADYCNT	5

// 태스크의 우선순위
#define TASK_FLAGS_HIGHEST	0
#define TASK_FLAGS_HIGH		1
#define TASK_FLAGS_MEDIUM	2
#define TASK_FLAGS_LOW		3
#define TASK_FLAGS_LOWEST	4
#define TASK_FLAGS_WAIT		0xFF

// 태스크의 플래그
#define TASK_FLAGS_FIN		0x8000000000000000
#define TASK_FLAGS_SYSTEM	0x4000000000000000
#define TASK_FLAGS_PROC		0x2000000000000000
#define TASK_FLAGS_THREAD	0x1000000000000000
#define TASK_FLAGS_IDLE		0x0800000000000000
#define TASK_FLAGS_USERLV	0x0400000000000000

// 함수 매크로
#define GETPRIORITY(x)		((x) & 0xFF)
#define SETPRIORITY(x, priority)	((x) = ((x) & 0xFFFFFFFFFFFFFF00) | (priority))
#define GETTCBOFFSET(x)		((x) & 0xFFFFFFFF)

// 자식 쓰레드 링크에 연결된 threadLink 정보에서 태스크 자료구조(TCB) 위치를 계산해 반환하는 매크로
#define GETTCBTHREAD(x)		(TCB*)((QWORD)(x) - offsetof(TCB, threadLink))

// 프로세서 친화도 필드에 라애 값이 설정되면 해당 태스크는 특별 요구사항이 없는 것으로 판단하여 태스크 부하 분산 사용
#define TASK_LOADBALANCING_ID	0xFF

// 구조체, 1바이트로 정렬
#pragma pack(push, 1)

// 콘텍스트에 관련된 자료구조
typedef struct context{
	QWORD reg[TASK_REGCNT];
} CONTEXT;

// 태스크(프로세스와 쓰레드) 상태 관리 자료구조. FPU 콘텍스트 때문에 자료구조 크기를 16 배수로 정렬
typedef struct taskControlBlock {
	// 다음 데이터의 위치와 ID
	LISTLINK link;

	// 플래그
	QWORD flag;

	// 프로세스 메모리 영역 시작과 크기
	void *memAddr;
	QWORD memSize;

	// 자식 쓰레드의 위치와 ID
	LISTLINK threadLink;
	QWORD parentProcID;

	// FPU 콘텍스트를 16의 배수로 정렬해야 함
	QWORD contextFPU[512 / 8];

	LIST childThreadList;

	// 콘텍스트
	CONTEXT context;

	// 스택 주소 및 크기
	void *stackAddr;
	QWORD stackSize;

	// FPU 사용 여부
	BOOL fpuUsed;

	// 프로세서 친화도
	BYTE affinity;

	// 현재 탯크를 수행하는 코어의 로컬 APIC ID
	BYTE _id;

	// 패딩 비트
	char pad[9];
} TCB;

// TCB 풀 상태 관리 자료구조
typedef struct tcbPoolManager {
	// 자료구조 동기화를 위한 스핀락
	SPINLOCK *spinLock;

	// 태스크 풀에 대한 정보
	TCB *startAddr;
	int maxCnt;
	int useCnt;

	// TCB 할당 횟수
	int allocCnt;
} TCBPOOLMANAGER;

// 스케줄러 상태 관리 자료구조
typedef struct scheduler {
	// 자료구조 동기화를 위한 스핀락
	SPINLOCK spinLock;

	// 현재 수행중인 태스크
	TCB *runningTask;

	// 현재 수행중 태스크가 사용할 수 있는 프로세서 시간
	int time;

	// 실행할 태스크가 준비 중인 리스트, 태스크의 우선순위에 따라 구분
	LIST readyList[TASK_MAXREADYCNT];

	// 종료할 태스크가 대기 중인 리스트
	LIST waitList;

	// 각 우선순위별 태스크 실행 횟수를 저장하는 자료구조
	int exeCnt[TASK_MAXREADYCNT];

	// 프로세서 부하를 계산하기 위한 자료구조
	QWORD procLoad;

	// 유휴 태스크에서 사용한 프로세서 시간
	QWORD loopIdleTask;

	// 마지막으로 FPU를 사용한 태스크의 ID
	QWORD lastFPU;

	// 부하 분산 기능 사용 여부
	BOOL isLoadBalancing;
} SCHEDULER;

#pragma pack(pop)

static void initTBPool(void);
static TCB *allocTCB(void);
static void freeTCB(QWORD id);
TCB *createTask(QWORD flag, void *memAddr, QWORD memSize, QWORD epAddr, BYTE affinity);
static void setTask(TCB *tcb, QWORD flag, QWORD epAddr, void *stackAddr, QWORD stackSize);

void initScheduler(void);
void setRunningTask(BYTE _id, TCB *task);
TCB *getRunningTask(BYTE _id);
static TCB *getNextTask(BYTE _id);
static BOOL addReadyList(BYTE _id, TCB *task);
BOOL schedule(void);
BOOL scheduleInterrupt(void);
void decProcTime(BYTE _id);
BOOL isProcTime(BYTE _id);
static TCB *delReadyList(BYTE _id, QWORD id);
static BOOL findSchedulerLock(QWORD id, BYTE *_id);
BOOL alterPriority(QWORD id, BYTE priority);
BOOL taskFin(QWORD id);
void taskExit(void);
int getReadyCnt(BYTE _id);
int getTaskCnt(BYTE _id);
TCB *getTCB(int offset);
BOOL isTaskExist(QWORD id);
QWORD getProcLoad(BYTE _id);
static TCB *getProcThread(TCB *thread);
void addTaskLoadBalancing(TCB *task);
static BYTE findSchedulerCnt(const TCB *task);
BYTE setTaskLoadBalancing(BYTE _id, BOOL isLoadbalancing);
BOOL alterAffinity(QWORD id, BYTE affinity);

void idleTask(void);
void haltProc(BYTE _id);

QWORD getLastFPU(BYTE _id);
void setLastFPU(BYTE _id, QWORD id);

#endif /*__CLITASK_H__*/
