/*
 * Mouse.c
 *
 *  Created on: 2017. 9. 1.
 *      Author: Yummy
 */

#include <Mouse.h>
#include <Keyboard.h>
#include <Queue.h>
#include <AsmUtil.h>

// 마우스 상태 관리 매니저
static MOUSEMANAGER gs_mouseManager = {0,};

// 마우스 저장 큐, 버퍼
static QUEUE gs_mouseQ;
static MOUSEDATA gs_mouseQBuf[MOUSE_MAXQUEUECNT];

// 마우스 초기화
BOOL kInitMouse(void) {
	// 큐 초기화
	kInitQueue(&gs_mouseQ, gs_mouseQBuf, MOUSE_MAXQUEUECNT, sizeof(MOUSEDATA));

	// 스핀락 초기화
	kInitSpinLock(&(gs_mouseManager.spinLock));

	// 마우스 활성화
	if(kActiveMouse() == TRUE) {
		// 마우스 인터럽트 활성화
		kOnMouseInterrupt();
		return TRUE;
	}
	return FALSE;
}

// 마우스 활성화
BOOL kActiveMouse(void) {
	int i, j;
	BOOL preInterrupt, res;

	// 인터럽트 불가
	preInterrupt = kSetInterruptFlag(FALSE);

	// 컨트롤 레지스터(포트 0x64)에 마우스 활성화 커맨드(0xA8)를 전달해 마우스 디바이스 활성화
	kOutByte(0x64, 0xA8);

	// 컨트롤 레지스터(포트 0x64)에 마우스로 데이터 전송하는 커맨드(0xD4)를 전달해 입력 버퍼(포트 0x60)로 전달된 데이터 마우스로 전송
	kOutByte(0x64, 0xD4);

	// 입력 버퍼(포트 0x60)가 비어있으면 마우스에 활성화 커맨드 전송. 0xFFFF만큼 루프 수행하면 충분. 루프를 수행한 후에도 입력 버퍼가 차있으면 무시하고 전송
	for(i = 0; i < 0xFFFF; i++) if(kInputBufChk() == FALSE) break;

	// 입력 버퍼(포트 0x60)로 마우스 활성화(0xF4) 커맨드 전달해 마우스로 전송
	kOutByte(0x60, 0xF4);

	// ACK가 올 때까지 대기
	res = kAckForQ();

	// 이전 인터럽트 상태 복원
	kSetInterruptFlag(preInterrupt);

	return res;
}

// 마우스 인터럽트 활성화
void kOnMouseInterrupt(void) {
	BYTE portData;
	int i;

	// 커맨드 바이트 읽기. 컨트롤 레지스터(포트 0x64)에 키보드 컨트롤러 커맨드 바이트 읽는 커맨드(0x20) 전송
	kOutByte(0x64, 0x20);

	// 출력 포트 데이터를 기다렸다가 읽음
	for(i = 0; i < 0xFFFF; i++) if(kOutputBufChk() == TRUE) break;

	// 출력 포트(포트 0x60)에 수신된 커맨드 바이트 값 읽음
	portData = kInByte(0x60);

	// 마우스 인터럽트 비트 활성화 후 커맨드 바이트 전송.(비트 1 설정)
	portData |= 0x02;

	// 커맨드 레지스터(0x64)에 커맨드 바이트를 쓰는 커맨드(0x60) 전달
	kOutByte(0x64, 0x60);

	// 입력 버퍼(포트 0x60)에 데이터가 비어있으면 출력 포트에 값을 쓰는 커맨드와 커맨드 바이트 전송
	for(i = 0; i < 0xFFFF; i++) if(kInputBufChk() == FALSE) break;

	// 입력 버퍼(0x60)에 마우스 인터럽트 비트가 1로 설정된 값 전달
	kOutByte(0x60, portData);
}

// 마우스 데이터를 모아 큐에 삽입
BOOL kGatherMouseData(BYTE data) {
	BOOL res;

	// 수신된 바이트 수에 따라 마우스 데이터 설정
	switch(gs_mouseManager.byteCnt) {
	// 바이트 1에 데이터 설정
	case 0:
		gs_mouseManager.nowData.clickFlag = data;
		gs_mouseManager.byteCnt++;
		break;
	// 바이트 2에 데이터 설정
	case 1:
		gs_mouseManager.nowData.xMove = data;
		gs_mouseManager.byteCnt++;
		break;
	// 바이트 3에 데이터 설정
	case 2:
		gs_mouseManager.nowData.yMove = data;
		gs_mouseManager.byteCnt++;
		break;
	// 그외 경우는 수신 바이트 수 초기화
	default:
		gs_mouseManager.byteCnt = 0;
		break;
	}

	// 3바이트 모두 수신되면 마우스 큐에 삽입 후 수신 횟수 초기화
	if(gs_mouseManager.byteCnt >= 3) {
		// 임계 영역 시작
		kLock_spinLock(&(gs_mouseManager.spinLock));

		// 마우스 큐에 마우스 데이터 삽입
		res = kAddQData(&gs_mouseQ, &gs_mouseManager.nowData);
		// 임계 영역 끝
		kUnlock_spinLock(&(gs_mouseManager.spinLock));
		// 수신된 바이트 수 초기화
		gs_mouseManager.byteCnt = 0;
	}

	return res;
}

// 마우스 큐에서 마우스 데이터 꺼냄
BOOL kRmMouseData(BYTE *stat, int *x, int *y) {
	MOUSEDATA data;
	BOOL res;

	// 큐가 비어있으면 데이터 못 꺼냄
	if(kIsQEmpty(&(gs_mouseQ)) == TRUE) return FALSE;

	// 임계 영역 시작
	kLock_spinLock(&(gs_mouseManager.spinLock));
	// 큐에서 데이터 꺼냄
	res = kRmQData(&(gs_mouseQ), &data);
	// 임계 영역 끝
	kUnlock_spinLock(&(gs_mouseManager.spinLock));

	// 데이터 꺼내지 못하면 실패
	if(res == FALSE) return FALSE;

	// 마우스 데이터 분석. 마우스 클릭 플래그는 첫 번째 바이트 하위 3비트에 존재
	*stat = data.clickFlag & 0x7;

	// X, Y의 이동 거리 설정. X 부호 비트는 비트 4에 있고 1로 설정되면 음수
	*x = data.xMove & 0xFF;
	// 음수이면 아래 8비트에 X 이동 거리 설정 후 상위 비트를 모두 1로 만들어 부호 비트 확장
	if(data.clickFlag & 0x10) *x |= (0xFFFFFF00);

	// Y 부호 비트는 비트 5에 있고 1로 설정되면 음수. 아래 방향으로 갈수록 Y 값이 증가하는 화면 좌표와 달리 마우스는 위쪽이 값 증가이므로 Toggle해야함
	*y = data.yMove & 0xFF;
	// 음수이면 아래 8비트에 Y 이동 거리 설정 후 상위 비트를 모두 1로 만들어 부호 비트 확장
	if(data.clickFlag & 0x20) *y |= (0xFFFFFF00);

	// 마우스 Y축 증감 방향은 화면 좌표와 반대이므로 Y 이동 거리에 음수로 방향 바꿈
	*y = -*y;
	return TRUE;
}

// 마우스 데이터가 출력 버퍼에 있는지 반환
BOOL kIsMouseData(void) {
	// 출력 버퍼(포트 0x60)를 읽기 전 상태 레지스터(포트 0x64)를 읽어 마우스 데이터인가 확인. 마우스 데이터는 AUXB 비트(비트 5)가 1로 설정됨
	if(kInByte(0x64) & 0x20) return TRUE;
	return FALSE;
}
