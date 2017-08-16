/*
 * SerialPort.c
 *
 *  Created on: 2017. 8. 16.
 *      Author: Yummy
 */

#include <SerialPort.h>
#include <Util.h>

// 시리얼 포트를 담당하는 자료구조
static SERIALMANAGER gs_serialManager;

// 시리얼 포트 초기화
void initSerial(void) {
	WORD baseAddr;

	// 뮤텍스 초기화
	initMutex(&(gs_serialManager.lock));

	// COM1 시리얼 포트(0x3F8) 선택해 초기화
	baseAddr = SERIAL_PORT_COM1;

	// 인터럽트 활성화 레지스터(0x3F9)에 0을 전송해 모든 인터럽트 비활성화
	outByte(baseAddr + SERIAL_PORT_IDX_ONINTERRUPT, 0);

	// 통신 속도 115,200 설정, 라인 제어 레지스터(0x3FB)의 DLAB 비트(비트 7)를 1로 설정해 제수 래치 레지스터에 접근
	outByte(baseAddr + SERIAL_PORT_IDX_LINECTRL, SERIAL_LINECTRL_DLAB);
	// LSB 제수 래치 레지스터(0x3F8)에 하위 8비트 전송
	outByte(baseAddr + SERIAL_PORT_IDX_DIVLATCH_LSB, SERIAL_DIVLATCH_115200);
	// MSB 제수 래치 레지스터(0x3F9)에 상위 8비트 전송
	outByte(baseAddr + SERIAL_PORT_IDX_DIVLATCH_MSB, SERIAL_DIVLATCH_115200 >> 8);

	// 송수신 방법 설정, 라인 제어 레지스터(0x3FB)에 Data Bit(8비트), 패리티 없음, Stop Bit(1비트)로 설정하고 DLAB비트 0으로 설정
	outByte(baseAddr + SERIAL_PORT_IDX_LINECTRL, SERIAL_LINECTRL_DATA | SERIAL_LINECTRL_NOPARITY | SERIAL_LINECTRL_STOP);

	// FIFO의 인터럽트 발생 시점을 14바이트로 설정
	outByte(baseAddr + SERIAL_PORT_IDX_FIFOCTRL, SERIAL_FIFOCTRL_ON | SERIAL_FIFOCTRL_14BYTE);
}

// 송신 FIFO가 비어있는지 반환
static BOOL isSerialTBE(void) {
	BYTE data;

	// 라인 상태 레지스터(0x3FD)를 읽은 후 TBE Bit(비트 1) 확인 후 송신 FIFO가 비어있는지 확인
	data = inByte(SERIAL_PORT_COM1 + SERIAL_PORT_IDX_LINESTAT);
	if((data & SERIAL_LINESTAT_TBE) == SERIAL_LINESTAT_TBE) return TRUE;
	return FALSE;
}

// 시리얼 포트로 데이터 송신
void sendSerialData(BYTE *buf, int size) {
	int data, _size, i;

	// 동기화 처리
	_lock(&(gs_serialManager.lock));

	// 요청한 바이트 수만큼 보낼 때까지 반복
	data = 0;
	while(data < size) {
		// 송신 FIFO에 데이터가 남아있다면 다 전송될 때까지 대기
		while(isSerialTBE() == FALSE) _sleep(0);

		// 전송할 데이터 중 남은 크기와 FIFO 최대 크기(16바이트) 비교 후 작은 것 선택해 송신 시리얼 포트 채움
		_size = _MIN(size - data, SERIAL_FIFO_MAXSIZE);
		for(i = 0; i < _size; i++) outByte(SERIAL_PORT_COM1 + SERIAL_PORT_IDX_SENDBUF, buf[data + i]);
		data += _size;
	}

	// 동기화 처리
	_unlock(&(gs_serialManager.lock));
}

// 수신 FIFO에 데이터 있는지 반환
static BOOL isSerialRxBF(void) {
	BYTE data;

	// 라인 상태 레지스터(0x3FD)를 읽은 뒤 RxRD 비트(비트 0)를 확인해 수신 FIFO에 데이터 있는지 확인
	data = inByte(SERIAL_PORT_COM1 + SERIAL_PORT_IDX_LINESTAT);
	if((data & SERIAL_LINESTAT_RXRD) == SERIAL_LINESTAT_RXRD) return TRUE;
	return FALSE;
}

// 시리얼 포트에서 데이터 읽음
int recvSerialData(BYTE *buf, int size) {
	int i;

	// 동기화 처리
	_lock(&(gs_serialManager.lock));

	// 루프를 돌며 현재 버퍼에 있는 데이터 반환
	for(i = 0; i < size; i++) {
		// 버퍼에 데이터가 없으면 중지
		if(isSerialRxBF() == FALSE) break;
		// 수신 버퍼 레지스터(0x3F8)에서 한 바이트 읽음
		buf[i] = inByte(SERIAL_PORT_COM1 + SERIAL_PORT_IDX_RECVBUF);
	}

	// 동기화 처리
	_unlock(&(gs_serialManager.lock));

	// 읽은 데이터 개수 반환
	return i;
}

// 시리얼 포트 컨트롤러 FIFO 초기화
void clearSerialFIFO(void) {
	// 동기화 처리
	_lock(&(gs_serialManager.lock));

	// 송수신 FIFO 모두 비우고 버퍼에 데이터가 14바이트 찼을 때 인터럽트 발생토록 FIFO 제어 레지스터(0x3FA)에 설정 값 전송
	outByte(SERIAL_PORT_COM1 + SERIAL_PORT_IDX_FIFOCTRL, SERIAL_FIFOCTRL_ON | SERIAL_FIFOCTRL_14BYTE | SERIAL_FIFOCTRL_CLEARRECV | SERIAL_FIFOCTRL_CLEARSEND);

	// 동기화 처리
	_unlock(&(gs_serialManager.lock));
}
