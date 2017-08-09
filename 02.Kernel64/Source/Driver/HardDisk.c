/*
 * HardDisk.c
 *
 *  Created on: 2017. 8. 8.
 *      Author: Yummy
 */

#include <HardDisk.h>

// 하드 디스크 관리하는 자료구조
static HDDMANAGER gs_hddManager;

// 하드 디스크 디바이스 드라이버 초기화
BOOL initHDD(void) {
	// 뮤텍스 초기화
	initMutex(&(gs_hddManager.mut));

	// 인터럽트 플래그 초기화
	gs_hddManager.priInterruptOccur = FALSE;
	gs_hddManager.secInterruptOccur = FALSE;

	// 첫 번째와 두 번째 PATA 포트의 디지털 출력 레지스터(포트 0x3F6, 0x376)에 0을 출력해 하드 디스크 컨트롤러의 인터럽트 활성화
	outByte(HDD_PORT_PRIBASE + HDD_PORT_IDX_DIGITOUT, 0);
	outByte(HDD_PORT_SECBASE + HDD_PORT_IDX_DIGITOUT, 0);

	// 하드 디스크 정보 요청
	if(readHDDInfo(TRUE, TRUE, &(gs_hddManager.hddInfo)) == FALSE) {
		gs_hddManager.hddDetect = FALSE;
		gs_hddManager.isWrite = FALSE;
		return FALSE;
	}

	// 하드 디스크가 검색되었으면 QEMU에서만 쓸 수 있도록 설정
	gs_hddManager.hddDetect = TRUE;
	if(memCmp(gs_hddManager.hddInfo.modelNum, "QEMU", 4) == 0) gs_hddManager.isWrite = TRUE;
	else gs_hddManager.isWrite = FALSE;
	return TRUE;
}

// 하드 디스크 상태 반환
static BYTE readHDDStat(BOOL pri) {
	if(pri == TRUE) return inByte(HDD_PORT_PRIBASE + HDD_PORT_IDX_STAT);	// 첫 번째 PATA 포트 상태 레지스터(포트 0x1F7)에서 값 반환
	return inByte(HDD_PORT_SECBASE + HDD_PORT_IDX_STAT);	// 두 번째 PATA 포트 상태 레지스터(포트 0x177)에서 값 반환
}

// 하드 디스크의 Busy가 해제될 때까지 대기
static BOOL waitHDDBusy(BOOL pri) {
	QWORD startTickCnt;
	BYTE stat;

	// 대기를 시작한 시간 저장
	startTickCnt = getTickCnt();

	// 일정 시간 동안 하드 디스크의 Busy가 해제될 때까지 대기
	while((getTickCnt() - startTickCnt) <= HDD_WAIT_TIME) {
		// HDD 상태 반환
		stat = readHDDStat(pri);

		// Busy 비트(7비트)가 설정되어 있지 않으면 Busy가 해제된 것이므로 종료
		if((stat & HDD_STAT_BUSY) != HDD_STAT_BUSY) return TRUE;
		_sleep(1);
	}
	return FALSE;
}

// 하드 디스크가 Ready될 때까지 일정 시간 대기
static BOOL waitHDDReady(BOOL pri) {
	QWORD startTickCnt;
	BYTE stat;

	// 대기를 시작한 시간 저장
	startTickCnt = getTickCnt();

	// 일정 시간 동안 하드 디스크가 Ready가 될 때까지 대기
	while((getTickCnt() - startTickCnt) <= HDD_WAIT_TIME) {
		// HDD 상태 반환
		stat = readHDDStat(pri);

		// Ready 비트(6비트)가 설정되어 있으면 데이터를 받을 준비가 된 것이므로 종료
		if((stat & HDD_STAT_READY) == HDD_STAT_READY) return TRUE;
		_sleep(1);
	}
	return FALSE;
}

// 인터럽트 발생 여부 설정
void setHDDInterruptFlag(BOOL pri, BOOL flag) {
	if(pri == TRUE) gs_hddManager.priInterruptOccur = flag;
	else gs_hddManager.secInterruptOccur = flag;
}

// 인터럽트 발생시까지 대기
static BOOL waitHDDInterrupt(BOOL pri) {
	QWORD tickCnt;

	// 대기 시작 시간 저장
	tickCnt = getTickCnt();

	// 일정 시간 동안 하드 디스크의 인터럽트가 발생할 때까지 대기
	while(getTickCnt() - tickCnt <= HDD_WAIT_TIME) {
		// 하드 디스크 자료구조에 인터럽트 발생 플래그 확인
		if((pri == TRUE) && (gs_hddManager.priInterruptOccur == TRUE)) return TRUE;
		else if((pri == FALSE) && (gs_hddManager.secInterruptOccur == TRUE)) return TRUE;
	}
	return FALSE;
}

// 하드 디스크 정보 읽음
BOOL readHDDInfo(BOOL pri, BOOL master, HDDINFO *hddInfo) {
	WORD portBase, tmp;
	QWORD lastTickCnt;
	BYTE stat, driveFlag;
	int i;
	BOOL waitRes;

	// PATA 포트에 따라 I/O 포트의 기본 어드레스 설정
	if(pri == TRUE) portBase = HDD_PORT_PRIBASE;	// 첫 번째 PATA 포트면 포트 0x1F0을 저장
	else portBase = HDD_PORT_SECBASE;	// 두 번째 PATA 포트면 포트 0x170을 저장

	// 동기화 처리
	_lock(&(gs_hddManager.mut));

	// 아직 수행중 커맨드가 있다면 일정 시간동안 끝나길 대기
	if(waitHDDBusy(pri) == FALSE) {
		// 동기화 처리
		_unlock(&(gs_hddManager.mut));
		return FALSE;
	}

	// 드라이브와 헤드 데이터 설정
	if(master == TRUE) driveFlag = HDD_DRIVENHEAD_LBA;	// 마스터면 LBA 플래그만 설정
	else driveFlag = HDD_DRIVENHEAD_LBA | HDD_DRIVENHEAD_SLAVE;

	// 드라이브와 헤드 레지스터(포트 0x1F6 또는 0x176)에 설정된 값 전송
	outByte(portBase + HDD_PORT_IDX_DRIVENHEAD, driveFlag);

	// 커맨드 받아들일 준비가 될 때까지 일정 시간 대기
	if(waitHDDReady(pri) == FALSE) {
		// 동기화 처리
		_unlock(&(gs_hddManager.mut));
		return FALSE;
	}

	// 인터럽트 플래그 초기화
	setHDDInterruptFlag(pri, FALSE);

	// 커맨드 레지스터(포트 0x1F7 또는 0x177)에 드라이브 인식 커맨드(0xEC) 전송
	outByte(portBase + HDD_PORT_IDX_CMD, HDD_CMD_ID);

	// 처리가 완료될 때까지 인터럽트 발생 대기
	waitRes = waitHDDInterrupt(pri);
	// 에러가 발생하거나 인터럽트가 발생하지 않으면 문제가 발생한 것이니 종료
	stat = readHDDStat(pri);
	if((waitRes == FALSE) || (stat & HDD_STAT_ERR)) {
		// 동기화 처리
		_unlock(&(gs_hddManager.mut));
		return FALSE;
	}

	// 한 섹터 읽음
	for(i = 0; i < 512 / 2; i++) ((WORD*)hddInfo)[i] = inWord(portBase + HDD_PORT_IDX_DATA);

	// 문자열은 바이트 순서로 다시 변환
	swapByte(hddInfo->modelNum, sizeof(hddInfo->modelNum) / 2);
	swapByte(hddInfo->serialNum, sizeof(hddInfo->serialNum) / 2);

	// 동기화 처리
	_unlock(&(gs_hddManager.mut));
	return TRUE;
}

// WORD 내 바이트 순서 바꿈
static void swapByte(WORD *data, int cnt) {
	int i;
	WORD tmp;

	for(i = 0; i < cnt; i++) {
		tmp = data[i];
		data[i] = (tmp >> 8) | (tmp << 8);
	}
}

// 하드 디스크 섹터 읽음. 최대 256개 섹터 읽을 수 있고, 실제로 읽은 섹터 수 반환
int readHDDSector(BOOL pri, BOOL master, DWORD lba, int sectorCnt, char *buf) {
	WORD portBase;
	int i, j;
	BYTE driveFlag, stat;
	long readCnt = 0;
	BOOL waitRes;

	// 범위 검사. 최대 256섹터 처리 가능
	if((gs_hddManager.hddDetect == FALSE) || (sectorCnt <= 0) || (256 < sectorCnt) || ((lba + sectorCnt) >= gs_hddManager.hddInfo.totalSector)) return 0;

	// PATA 포트에 따라 I/O 포트의 기본 어드레스 설정
	if(pri == TRUE) portBase = HDD_PORT_PRIBASE;	// 첫 번째 PATA 포트면 포트 0x1F0 저장
	else portBase = HDD_PORT_SECBASE;	// 두 번째 PATA 포트면 포트 0x170 저장

	// 동기화 처리
	_lock(&(gs_hddManager.mut));

	// 아직 수행중 커맨드가 있으면 일정시간 끝날때까지 대기
	if(waitHDDBusy(pri) == FALSE) {
		// 동기화 처리
		_unlock(&(gs_hddManager.mut));
		return FALSE;
	}

	// 섹터 수 레지스터(포트 0x1F2 또는 0x172)에 읽을 섹터 수 전송
	outByte(portBase + HDD_PORT_IDX_SECTORCNT, sectorCnt);
	// 섹터 번호 레지스터(포트 0x1F3 또는 0x173)에 읽을 섹터 위치(LBA 0~7비트) 전송
	outByte(portBase + HDD_PORT_IDX_SECTORNUM, lba);
	// 실린더 LSB 레지스터(포트 0x1F4 또는 0x174)에 읽을 섹터 위치(LBA 8~15비트) 전송
	outByte(portBase + HDD_PORT_IDX_CYLINDERLSB, lba >> 8);
	// 실린더 MSB 레지스터(포트 0x1F5 또는 0x175)에 읽을 섹터 위치(LBA 16~23비트) 전송
	outByte(portBase + HDD_PORT_IDX_CYLINDERMSB, lba >> 16);
	// 드라이브와 헤드 데이터 설정
	if(master == TRUE) driveFlag = HDD_DRIVENHEAD_LBA;
	else driveFlag = HDD_DRIVENHEAD_LBA | HDD_DRIVENHEAD_SLAVE;
	// 드라이브와 헤드 레지스터(포트 0x1F6 또는 0x176)에 읽을 섹터 위치(LBA 24~27비트)와 설정된 값 같이 전송
	outByte(portBase + HDD_PORT_IDX_DRIVENHEAD, driveFlag | ((lba >> 24) & 0x0F));

	// 커맨드를 받아들일 준비가 될 때까지 일정시간 대기
	if(waitHDDReady(pri) == FALSE) {
		// 동기화 처리
		_unlock(&(gs_hddManager.mut));
		return FALSE;
	}

	// 인터럽트 플래그 초기화
	setHDDInterruptFlag(pri, FALSE);

	// 커맨드 레지스터(포트 0x1F7 또는 0x177)에 읽기(0x20) 전송
	outByte(portBase + HDD_PORT_IDX_CMD, HDD_CMD_READ);

	// 섹터 수만큼 루프 돌며 데이터 수신
	for(i = 0; i < sectorCnt; i++) {
		// 에러 발생시 종료
		stat = readHDDStat(pri);
		if((stat & HDD_STAT_ERR) == HDD_STAT_ERR) {
			printF("Error Occur !!\n");
			// 동기화 처리
			_unlock(&(gs_hddManager.mut));
			return i;
		}

		// DATAREQUEST 비트가 설정되지 않았으면 데이터가 수신되기를 기다림
		if((stat & HDD_STAT_DATAREQ) != HDD_STAT_DATAREQ) {
			// 처리가 완료될 때까지 일정시간 인터럽트 대기
			waitRes = waitHDDInterrupt(pri);
			setHDDInterruptFlag(pri, FALSE);
			// 인터럽트가 발생하지 않으면 문제가 발생했으니 종료
			if(waitRes == FALSE) {
				printF("Interrupt Not Occur !!\n");
				// 동기화 처리
				_unlock(&(gs_hddManager.mut));
				return FALSE;
			}
		}

		// 한 섹터 읽음
		for(j = 0; j < 512 / 2; j++) ((WORD*)buf)[readCnt++] = inWord(portBase + HDD_PORT_IDX_DATA);
	}

	// 동기화 처리
	_unlock(&(gs_hddManager.mut));
	return i;
}

// 하드 디스크에 섹터 쓰며 최대 256개까지 가능. 실제로 쓴 섹터 수 반환
int writeHDDSector(BOOL pri, BOOL master, DWORD lba, int sectorCnt, char *buf) {
	WORD portBase, tmp;
	int i, j;
	BYTE driveFlag, stat;
	long readCnt = 0;
	BOOL waitRes;

	// 범위 검사. 최대 256섹터 처리 가능
	if((gs_hddManager.isWrite == FALSE) || (sectorCnt <= 0) || (256 < sectorCnt) || ((lba + sectorCnt) >= gs_hddManager.hddInfo.totalSector)) return 0;

	// PATA 포트에 따라 I/O 포트의 기본 주소 설정
	if(pri == TRUE) portBase = HDD_PORT_PRIBASE;	// 첫 번째 PATA 포트면 포트 0x1F0 저장
	else portBase = HDD_PORT_SECBASE;	// 두 번째 PATA 포트면 포트 0x170 저장

	// 아직 수행중 커맨드가 있으면 일정시간 대기
	if(waitHDDBusy(pri) == FALSE) return FALSE;

	// 동기화 처리
	_lock(&(gs_hddManager.mut));

	// 섹터 수 레지스터(포트 0x1F2 또는 0x172)에 쓸 섹터 수 전송
	outByte(portBase + HDD_PORT_IDX_SECTORCNT, sectorCnt);
	// 섹터 번호 레지스터(포트 0x1F3 또는 0x173)에 쓸 섹터 위치(LBA 0~7비트) 전송
	outByte(portBase + HDD_PORT_IDX_SECTORNUM, lba);
	// 실린더 LSB 레지스터(포트 0x1F4 또는 0x174)에 쓸 섹터 위치(LBA 8~15비트) 전송
	outByte(portBase + HDD_PORT_IDX_CYLINDERLSB, lba >> 8);
	// 실린더 MSB 레지스터(포트 0x1F5 또는 0x175)에 쓸 섹터 위치(LBA 16~23비트) 전송
	outByte(portBase + HDD_PORT_IDX_CYLINDERMSB, lba >> 16);
	// 드라이브와 헤드 데이터 설정
	if(master == TRUE) driveFlag = HDD_DRIVENHEAD_LBA;
	else driveFlag = HDD_DRIVENHEAD_LBA | HDD_DRIVENHEAD_SLAVE;
	// 드라이브와 헤드 레지스터(포트 0x1F6 또는 0x176)에 쓸 섹터 위치(LBA 24~27비트)와 설정된 값 같이 전송
	outByte(portBase + HDD_PORT_IDX_DRIVENHEAD, driveFlag | ((lba >> 24) & 0x0F));

	// 커맨드 받아들일 준비가 될 때까지 일정시간 대기
	if(waitHDDReady(pri) == FALSE) {
		// 동기화 처리
		_unlock(&(gs_hddManager.mut));
		return FALSE;
	}

	// 커맨드 전송
	outByte(portBase + HDD_PORT_IDX_CMD, HDD_CMD_WRITE);

	// 데이터 송신 가능시까지 대기
	while(1) {
		stat = readHDDStat(pri);
		// 에러 발생시 종료
		if((stat & HDD_STAT_ERR) == HDD_STAT_ERR) {
			// 동기화 처리
			_unlock(&(gs_hddManager.mut));
			return 0;
		}

		// DATAREQUEST 비트가 설정되었다면 데이터 송신 가능
		if((stat & HDD_STAT_DATAREQ) == HDD_STAT_DATAREQ) break;

		_sleep(1);
	}

	// 섹터 수만큼 루프 돌며 데이터 송신
	for(i = 0; i < sectorCnt; i++) {
		// 인터럽트 플래그 초기화 후 한 섹터 씀
		setHDDInterruptFlag(pri, FALSE);
		for(j = 0; j < 512 / 2; j++) outWord(portBase + HDD_PORT_IDX_DATA, ((WORD*)buf)[readCnt++]);

		// 에러 발생시 종료
		stat = readHDDStat(pri);
		if((stat & HDD_STAT_ERR) == HDD_STAT_ERR) {
			// 동기화 처리
			_unlock(&(gs_hddManager.mut));
			return i;
		}

		// DATAREQUEST 비트가 설정되지 않았으면 데이터 처리 완료를 대기
		if((stat & HDD_STAT_DATAREQ) != HDD_STAT_DATAREQ) {
			// 처리가 완료될 때까지 일정 시간 인터럽트 대기
			waitRes = waitHDDInterrupt(pri);
			setHDDInterruptFlag(pri, FALSE);
			// 인터럽트가 발생하지 않으면 문제가 발생했으니 종료
			if(waitRes == FALSE) {
				// 동기화 처리
				_unlock(&(gs_hddManager.mut));
				return FALSE;
			}
		}
	}

	// 동기화 처리
	_unlock(&(gs_hddManager.mut));
	return i;
}
