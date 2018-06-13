/*
 * InterruptHandler.c
 *
 *  Created on: 2017. 7. 22.
 *      Author: Yummy
 */

#include <InterruptHandler.h>
#include <PIC.h>
#include <Keyboard.h>
#include <Console.h>
#include <Util.h>
#include <CLITask.h>
#include <Descriptor.h>
#include <AsmUtil.h>
#include <HardDisk.h>
#include <Mouse.h>

// 인터럽트 핸들러 자료구조
static INTERRUPTMANAGER gs_interruptManager;

// 인터럽트 자료구조 초기화
void kInitHandler(void) {
	kMemSet(&gs_interruptManager, 0, sizeof(gs_interruptManager));
}

// 인터럽트 처리 모드 설정
void kSetSymmetricIOMode(BOOL mode) {
	gs_interruptManager.mode = mode;
}

// 인터럽트 부하 분산 기능 사용할지 여부 설정
void kSetInterruptLoadBalancing(BOOL isLoadBalancing) {
	gs_interruptManager.isLoadBalancing = isLoadBalancing;
}

// 코어별 인터럽트 처리 횟수 증가
void kIncInterruptCnt(int irq) {
	gs_interruptManager.coreInterruptCnt[kGetAPICID()][irq]++;
}

// 현재 인터럽트 모드에 맞춰 EOI 전송
void kSendEOI(int irq) {
	// 대칭 IO 모드가 아니면 PIC 모드
	if(gs_interruptManager.mode == FALSE) kSendEOI_PIC(irq);
	else kSendEOI_LocalAPIC();
}

// 인터럽트 핸들러 자료구조 반환
INTERRUPTMANAGER *kGetInterruptManager(void) {
	return &gs_interruptManager;
}

// 인터럽트 부하 분산 처리
void kProcLoadBalancing(int irq) {
	QWORD minCnt = 0xFFFFFFFFFFFFFFFF;
	int minCntIdx, coreCnt, i;
	BOOL resetCnt = FALSE;
	BYTE id;

	id = kGetAPICID();

	// 부하 분산 기능이 꺼져 있거나 부하 분산 처리할 시점이 아니면 할 필요가 없음
	if((gs_interruptManager.coreInterruptCnt[id][irq] == 0) || ((gs_interruptManager.coreInterruptCnt[id][irq] % INTERRUPT_LOADBALANCING_DIV) != 0) || (gs_interruptManager.isLoadBalancing == FALSE)) return;

	// 코어 개수를 구해 루프를 수행하며 인터럽트 처리 횟수가 가장 작은 코어 선택
	minCntIdx = 0;
	coreCnt = kGetProcCnt();
	for(i = 0; i < coreCnt; i++) {
		if((gs_interruptManager.coreInterruptCnt[i][irq] < minCnt)) {
			minCnt = gs_interruptManager.coreInterruptCnt[i][irq];
			minCntIdx = i;
		// 전체 카운트가 거의 최댓값에 근접했다면 나중에 카운트 모두 0으로 설정
		} else if(gs_interruptManager.coreInterruptCnt[i][irq] >= 0xFFFFFFFFFFFE) resetCnt = TRUE;
	}

	// IO 리다이렉션 테이블을 변경해 가장 인터럽트를 처리한 횟수가 작은 로컬 APIC로 전달
	kRoutingIRQ(irq, minCntIdx);

	// 처리한 코어의 카운트가 최댓값에 근접했다면 전체 카운트를 다시 0에서 시작하도록 변경
	if(resetCnt == TRUE) for(i = 0; i < coreCnt; i++) gs_interruptManager.coreInterruptCnt[i][irq] = 0;
}

static void kPrintDebug(int vec, int cnt, int handler) {
	char int_buf[] = "[INT:  , ]", exc_buf[] = "[EXC:  , ]";

	// 인터럽트가 발생했음을 알리기 위해 메시지 출력하는 부분. 화면 오른쪽 위 2자리 정수로 출력
	exc_buf[5] = int_buf[5] = '0' + vec / 10;
	exc_buf[6] = int_buf[6] = '0' + vec % 10;
	// 발생 횟수 출력
	exc_buf[8] = int_buf[8] = '0' + cnt;
	if(handler == 1) kPrintXY(70, 0, 0x1E, int_buf);
	else if(handler == 2) kPrintXY(0, 0, 0x1E, int_buf);
	else if(handler == 3) kPrintXY(0, 0, 0x1E, exc_buf);
	else if(handler == 4) kPrintXY(10, 0, 0x1E, int_buf);
	kPrintXY(34, 0, 0xE5, " YummyHitOS ");
}

// 공통으로 사용하는 예외 핸들러
void kExceptionHandler(int vecNum, QWORD errCode) {
	char buf[100] = {0,};
	BYTE apicID;
	TCB *task;

	// 현재 예외가 발생한 코어 반환
	apicID = kGetAPICID();
	// 현재 코어 실행 중 태스크 반환
	task = kGetRunningTask(apicID);

	kPrintXY(7, 1, 0x1F, "=============================================================           ");
	kPrintXY(7, 2, 0x1F, "                                                                        ");
	kPrintXY(7, 3, 0x1B, "                Interrupt Handler Execute                               ");
	kSprintF(buf, "Vector:%d%d, ErrorCode:0x%X, Core ID:0x%X", vecNum / 10, vecNum % 10, errCode, apicID);
	kPrintXY(7, 4, 0x1C, buf);
	kSprintF(buf, "Task ID:0x%Q", task->link.id);
	kPrintXY(7, 5, 0x1C, buf);
	kPrintXY(7, 6, 0x1F, "                                                                        ");
	kPrintXY(7, 7, 0x1F, "=============================================================           ");

	// 유저 레벨 태스크는 무한 루프를 수행하지 않고 태스크 종료 후 다른 태스크로 전환
	if(task->flag & TASK_FLAGS_USERLV) {
		// 태스크 종료
		kTaskFin(task->link.id);
		// taskFin() 함수에서 다른 태스크로 전환하므로 수행 안 될 것
		while(1);
	} else {	// 커널 레벨이면 무한 루프
		while(1);
	}
}

// 공통으로 사용하는 인터럽트 핸들러
void kInterruptHandler(int vecNum) {
	static int ls_interruptCnt = 0;
	int irq;

	ls_interruptCnt = (ls_interruptCnt + 1) % 10;
	kPrintDebug(vecNum, ls_interruptCnt, 1);

	// 인터럽트 벡터에서 IRQ 번호 추출
	irq = vecNum - PIC_IRQ_STARTVEC;

	// EOI 전송
	kSendEOI(irq);

	// 인터럽트 발생 횟수 업데이트
	kIncInterruptCnt(irq);

	// 부하 분산 처리
	kProcLoadBalancing(irq);
}

// 키보드 인터럽트 핸들러
void kKeyboardHandler(int vecNum) {
	static int ls_keyboardCnt = 0;
	BYTE tmp;
	int irq;

	ls_keyboardCnt = (ls_keyboardCnt + 1) % 10;
	kPrintDebug(vecNum, ls_keyboardCnt, 2);

	// 키보드 컨트롤러에서 데이터 읽고 ASCII로 변환해 큐에 삽입
	if(kOutputBufChk() == TRUE) {
		// 마우스 데이터가 아니면 키 큐에 삽입, 마우스 데이터면 마우스 큐에 삽
		if(kIsMouseData() == FALSE) {
			// 출력 버퍼(포트 0x60)에서 키 스캔 코드를 읽는 용도지만, 키보드와 마우스 데이터는 공통 출력 버퍼를 사용하니 마우스 데이터 읽을 때도 사용
			tmp = kGetCode();
			kConvertNPutCode(tmp);
		} else {
			tmp = kGetCode();
			kGatherMouseData(tmp);
		}
	}

	// 인터럽트 벡터에서 IRQ 번호 추출
	irq = vecNum - PIC_IRQ_STARTVEC;

	// EOI 전송
	kSendEOI(irq);

	// 인터럽트 발생 횟수 업데이트
	kIncInterruptCnt(irq);

	// 부하 분산 처리
	kProcLoadBalancing(irq);
}

// 타이머 인터럽트 핸들러
void kTimerHandler(int vecNum) {
	static int ls_timerCnt = 0;
	int irq;
	BYTE _id;

	ls_timerCnt = (ls_timerCnt + 1) % 10;
	kPrintDebug(vecNum, ls_timerCnt, 1);

	// 인터럽트 벡터에서 IRQ 번호 추출
	irq = vecNum - PIC_IRQ_STARTVEC;

	// EOI 전송
	kSendEOI(irq);

	// 인터럽트 발생 횟수 업데이트
	kIncInterruptCnt(irq);

	// IRQ 0 인터럽트 처리는 Bootstrap Processor만 처리
	_id = kGetAPICID();
	if(_id == 0) g_tickCnt++;	// 타이머 발생 횟수 증가

	// 태스크가 사용한 프로세서 시간 줄임
	kDecProcTime(_id);

	// 프로세서가 사용할 수 있는 시간을 다썼으면 태스크 전환 수행
	if(kIsProcTime(_id) == TRUE) kScheduleInterrupt();
}

// Device Not Available 예외 핸들러
void kDevFPUHandler(int vecNum) {
	TCB *fpu, *nowTask;
	QWORD lastID;
	BYTE _id;
	static int ls_devCnt = 0;

	// FPU 예외가 발생했음을 알리려고 메시지 출력
	ls_devCnt = (ls_devCnt + 1) % 10;
	kPrintDebug(vecNum, ls_devCnt, 3);

	// 현재 코어의 로컬 APIC ID 확인
	_id = kGetAPICID();

	// CR0 컨트롤 레지스터의 TS 비트를 0으로 설정
	kClearTS();

	// 이전 FPU를 사용한 태스크가 있는지 확인해 있다면 FPU의 상태를 태스크에 저장
	lastID = kGetLastFPU(_id);
	nowTask = kGetRunningTask(_id);

	// 이전 FPU를 사용한 것이 자신이면 아무것도 안 함
	if(lastID == nowTask->link.id) return;
	else if(lastID != TASK_INVALID_ID) { // FPU를 사용한 태스크가 있으면 FPU 상태 저장
		fpu = kGetTCB(GETTCBOFFSET(lastID));
		if((fpu != NULL) && (fpu->link.id == lastID)) kSaveFPU(fpu->contextFPU);
	}

	// 현재 태스크가 FPU를 사용한 적이 있는지 확인해 FPU를 사용한 적이 없다면 초기화, 있다면 저장된 FPU 콘텍스트 복원
	if(nowTask->fpuUsed == FALSE) {
		kInitFPU();
		nowTask->fpuUsed = TRUE;
	} else kLoadFPU(nowTask->contextFPU);

	// FPU를 사용한 태스크 ID를 현재 태스크로 변경
	kSetLastFPU(_id, nowTask->link.id);
}

// 하드 디스크에서 발생하는 인터럽트 핸들러
void kHardDiskHandler(int vecNum) {
	static int ls_hddCnt = 0;
	BYTE tmp;
	int irq;

	ls_hddCnt = (ls_hddCnt + 1) % 10;
	kPrintDebug(vecNum, ls_hddCnt, 4);

	// 인터럽트 벡터에서 IRQ 번호 추출
	irq = vecNum - PIC_IRQ_STARTVEC;

	// 첫 번째 PATA 포트의 인터럽트 벡터(IRQ 14) 처리
	if(irq == 14) kSetHDDInterruptFlag(TRUE, TRUE);	// 첫 번째 PATA 포트 인터럽트 발생 여부 TRUE
	else kSetHDDInterruptFlag(FALSE, TRUE);	// 두 번째 PATA 포트 인터럽트 벡터(IRQ 15) 발생 여부 TRUE

	// EOI 전송
	kSendEOI(irq);

	// 인터럽트 발생 횟수 업데이트
	kIncInterruptCnt(irq);

	// 부하 분산 처리
	kProcLoadBalancing(irq);
}

// 마우스 인터럽트 핸들러
void kMouseHandler(int vecNum) {
	static int ls_mouseCnt = 0;
	BYTE tmp;
	int irq;

	ls_mouseCnt = (ls_mouseCnt + 1) % 10;
	kPrintDebug(vecNum, ls_mouseCnt, 2);

	// 출력 버퍼(포트 0x60)에 수신된 데이터가 있는지 여부 확인 후 읽은 데이터를 키 큐 또는 마우스 큐에 삽입
	if(kOutputBufChk() == TRUE) {
		// 마우스 데이터가 아니면 키 큐에 삽입
		if(kIsMouseData() == FALSE) {
			// 출력 버퍼(포트 0x60)에서 키 스캔 코드를 읽는 용도지만, 키보드와 마우스 데이터는 공통 출력 버퍼를 사용하니 마우스 데이터 읽을 때도 사용
			tmp = kGetCode();
			kConvertNPutCode(tmp);
		} else {
			tmp = kGetCode();
			kGatherMouseData(tmp);
		}
	}

	// 인터럽트 벡터에서 IRQ 번호 추출
	irq = vecNum - PIC_IRQ_STARTVEC;

	// EOI 전송
	kSendEOI(irq);

	// 인터럽트 발생 횟수 업데이트
	kIncInterruptCnt(irq);

	// 부하 분산 처리
	kProcLoadBalancing(irq);
}
