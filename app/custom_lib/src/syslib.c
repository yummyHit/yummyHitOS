/*
 * syslib.c
 *
 *  Created on: 2018. 6. 16.
 *      Author: Yummy
 */

#include "syslib.h"

// 콘솔에 문자열 출력. printf() 함수 내부에서 사용
int printCS(const char *buf) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)buf;

	// 시스템 콜 호출
	return ExecSysCall(SYSCALL_CSPRINT, &param);
}

// 커서 위치 설정. 문자 출력 위치도 같이 설정
BOOL setCursor(int x, int y) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD) x;
	PARAM(1) = (QWORD) y;

	// 시스템 콜 호출
	return ExecSysCall(SYSCALL_SETCURSOR, &param);
}

// 현재 커서 위치 반환
BOOL getCursor(int *x, int *y) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD) x;
	PARAM(1) = (QWORD) y;

	// 시스템 콜 호출
	return ExecSysCall(SYSCALL_GETCURSOR, &param);
}

// 전체 화면 삭제
BOOL clearMonitor(void) {
	// 시스템 콜 호출
	return ExecSysCall(SYSCALL_CLEARMONITOR, NULL);
}

// getch() 함수 구현
BYTE getch(void) {
	// 시스템 콜 호출
	return ExecSysCall(SYSCALL_GETCH, NULL);
}

// 메모리 할당
void *malloc(QWORD size) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = size;

	// 시스템 콜 호출
	return (void*)ExecSysCall(SYSCALL_MALLOC, &param);
}

// 메모리 해제
BOOL free(void *addr) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)addr;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_FREE, &param);
}

// 파일 열기
FILE *fopen(const char *file, const char *mode) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)file;
	PARAM(1) = (QWORD)mode;

	// 시스템 콜 호출
	return (FILE*)ExecSysCall(SYSCALL_FOPEN, &param);
}

// 파일 읽기
DWORD fread(void *buf, DWORD size, DWORD cnt, FILE *file) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)buf;
	PARAM(1) = (QWORD)size;
	PARAM(2) = (QWORD)cnt;
	PARAM(3) = (QWORD)file;

	// 시스템 콜 호출
	return ExecSysCall(SYSCALL_FREAD, &param);
}

// 파일 쓰기
DWORD fwrite(const void *buf, DWORD size, DWORD cnt, FILE *file) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)buf;
	PARAM(1) = (QWORD)size;
	PARAM(2) = (QWORD)cnt;
	PARAM(3) = (QWORD)file;

	// 시스템 콜 호출
	return ExecSysCall(SYSCALL_FWRITE, &param);
}

// 파일 포인터 위치 이동
int fseek(FILE *file, int offset, int origin) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)file;
	PARAM(1) = (QWORD)offset;
	PARAM(2) = (QWORD)origin;

	// 시스템 콜 호출
	return (int)ExecSysCall(SYSCALL_FSEEK, &param);
}

// 파일 닫기
int fclose(FILE *file) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)file;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_FCLOSE, &param);
}

// 파일 삭제
int remove(const char *file) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)file;

	// 시스템 콜 호출
	return (int)ExecSysCall(SYSCALL_FREMOVE, &param);
}

// 디렉터리 열기
DIR *opendir(const char *dir) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)dir;

	// 시스템 콜 호출
	return (DIR*)ExecSysCall(SYSCALL_DOPEN, &param);
}

// 디렉터리 읽기
struct dirent *readdir(DIR *dir) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)dir;

	// 시스템 콜 호출
	return (struct dirent*)ExecSysCall(SYSCALL_DREAD, &param);
}

// 디렉터리 포인터 이동
BOOL rewinddir(DIR *dir) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)dir;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_DREWIND, &param);
}

// 디렉터리 닫음
int closedir(DIR *dir) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)dir;

	// 시스템 콜 호출
	return (int)ExecSysCall(SYSCALL_DCLOSE, &param);
}

// 핸들 풀을 검사해 파일 열림 확인
BOOL isfopen(const struct dirent *entry) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)entry;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_ISFOPEN, &param);
}

// 하드디스크 섹터 읽음. 최대 256개 읽을 수 있음.
int readHDDSector(BOOL pri, BOOL master, DWORD lba, int sectorCnt, char *buf) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)pri;
	PARAM(1) = (QWORD)master;
	PARAM(2) = (QWORD)lba;
	PARAM(3) = (QWORD)sectorCnt;
	PARAM(4) = (QWORD)buf;

	// 시스템 콜 호출
	return (int)ExecSysCall(SYSCALL_READ_HDDSECTOR, &param);
}

// 하드디스크 섹터 쓰기. 최대 256개 쓸 수 있음.
int writeHDDSector(BOOL pri, BOOL master, DWORD lba, int sectorCnt, char *buf) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)pri;
	PARAM(1) = (QWORD)master;
	PARAM(2) = (QWORD)lba;
	PARAM(3) = (QWORD)sectorCnt;
	PARAM(4) = (QWORD)buf;

	// 시스템 콜 호출
	return (int)ExecSysCall(SYSCALL_WRITE_HDDSECTOR, &param);
}

// 태스크 생성
QWORD makeTask(QWORD flag, void *memAddr, QWORD memSize, QWORD epAddr, BYTE affinity) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)flag;
	PARAM(1) = (QWORD)memAddr;
	PARAM(2) = (QWORD)memSize;
	PARAM(3) = (QWORD)epAddr;
	PARAM(4) = (QWORD)affinity;

	// 시스템 콜 호출
	return ExecSysCall(SYSCALL_CREATETASK, &param);
}

// 태스크 전환
BOOL schedule(void) {
	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_SCHEDULE, NULL);
}

// 태스크 우선순위 변경
BOOL alterPriority(QWORD id, BYTE priority, BOOL interrupt) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = id;
	PARAM(1) = (QWORD)priority;
	PARAM(2) = (QWORD)interrupt;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_ALTERPRIORITY, &param);
}

// 태스크 종료
BOOL taskFin(QWORD id) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = id;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_TASKFIN, &param);
}

// 태스크 종료
void exit(int val) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)val;

	// 시스템 콜 호출
	ExecSysCall(SYSCALL_TASKEXIT, &param);
}

// 전체 태스크 수 반환
int getTaskCnt(BYTE id) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)id;

	// 시스템 콜 호출
	return (int)ExecSysCall(SYSCALL_GETTASKCNT, &param);
}

// 태스크 존재 여부
BOOL istask(QWORD id) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = id;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_ISTASKEXIST, &param);
}

// 프로세서 사용률
QWORD getProcLoad(BYTE id) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)id;

	// 시스템 콜 호출
	return ExecSysCall(SYSCALL_GETPROCLOAD, &param);
}

// 프로세서 친화도 변경
BOOL alterAffinity(QWORD id, BYTE affinity) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = id;
	PARAM(1) = (QWORD)affinity;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_ALTERAFFINITY, &param);
}

// 응용프로그램 생성
QWORD ExecProg(const char *fileName, const char *argv, BYTE affinity) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)fileName;
	PARAM(1) = (QWORD)argv;
	PARAM(2) = (QWORD)affinity;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_EXECPROGRAM, &param);
}

// 쓰레드 생성
QWORD CreateThread(QWORD ep, QWORD arg, BYTE affinity) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)ep;
	PARAM(1) = (QWORD)arg;
	PARAM(2) = (QWORD)affinity;
	// 종료할 때 호출되는 함수에 exit을 지정하여 쓰레드가 스스로 종료하도록 함
	PARAM(3) = (QWORD)exit;

	// 시스템 콜 호출
	return ExecSysCall(SYSCALL_EXECPROGRAM, &param);
}

// 배경 윈도우 ID
QWORD getBackgroundID(void) {
	// 시스템 콜 호출
	return ExecSysCall(SYSCALL_GETBACKGROUNDID, NULL);
}

// 화면 영역 크기
void getMonArea(RECT *area) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)area;

	// 시스템 콜 호출
	ExecSysCall(SYSCALL_GETMONAREA, &param);
}

// 윈도우 생성
QWORD makeWin(int x, int y, int width, int height, DWORD flag, const char *title) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)x;
	PARAM(1) = (QWORD)y;
	PARAM(2) = (QWORD)width;
	PARAM(3) = (QWORD)height;
	PARAM(4) = (QWORD)flag;
	PARAM(5) = (QWORD)title;

	// 시스템 콜 호출
	return ExecSysCall(SYSCALL_CREATEWIN, &param);
}

// 윈도우 삭제
BOOL delWin(QWORD id) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = id;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_DELWIN, &param);
}

// 윈도우 화면에 나타냄
BOOL showWin(QWORD id, BOOL show) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = id;
	PARAM(1) = (QWORD)show;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_SHOWWIN, &param);
}

// 특정 위치 포함하는 윈도우 중 가장 위의 윈도우
QWORD findWinPoint(int x, int y) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)x;
	PARAM(1) = (QWORD)y;

	// 시스템 콜 호출
	return ExecSysCall(SYSCALL_FINDWINPOINT, &param);
}

// 윈도우 제목이 일치하는 윈도우
QWORD findWinTitle(const char *title) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)title;

	// 시스템 콜 호출
	return ExecSysCall(SYSCALL_FINDWINTITLE, &param);
}

// 윈도우 존재 여부
BOOL isWin(QWORD id) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = id;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_ISWINEXIST, &param);
}

// 최상위 윈도우 ID
QWORD getTopWin(void) {
	// 시스템 콜 호출
	return ExecSysCall(SYSCALL_GETTOPWIN, NULL);
}

// 윈도우 Z 순서 변경
BOOL moveWinTop(QWORD id) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = id;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_MOVEWINTOP, &param);
}

// X, Y좌표가 윈도우 제목 표시줄 위치에 있는가
BOOL isInTitle(QWORD id, int x, int y) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = id;
	PARAM(1) = (QWORD)x;
	PARAM(2) = (QWORD)y;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_ISINTITLE, &param);
}

// X, Y좌표가 윈도우 닫기 버튼 위치에 있는가
BOOL isCloseBtn(QWORD id, int x, int y) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = id;
	PARAM(1) = (QWORD)x;
	PARAM(2) = (QWORD)y;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_ISCLOSEBTN, &param);
}

// 윈도우 이동
BOOL moveWin(QWORD id, int x, int y) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = id;
	PARAM(1) = (QWORD)x;
	PARAM(2) = (QWORD)y;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_MOVEWIN, &param);
}

// 윈도우 크기 변경
BOOL resizeWin(QWORD id, int x, int y, int width, int height) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = id;
	PARAM(1) = (QWORD)x;
	PARAM(2) = (QWORD)y;
	PARAM(3) = (QWORD)width;
	PARAM(4) = (QWORD)height;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_RESIZEWIN, &param);
}

// 윈도우 영역
BOOL getWinArea(QWORD id, RECT *area) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = id;
	PARAM(1) = (QWORD)area;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_GETWINAREA, &param);
}

// 윈도우 화면 업데이트
BOOL updateMonID(QWORD id) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = id;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_UPDATEMONID, &param);
}

// 윈도우 내부 화면 업데이트
BOOL updateMonWinArea(QWORD id, const RECT *area) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = id;
	PARAM(1) = (QWORD)area;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_UPDATEMONWINAREA, &param);
}

// 화면 좌표로 화면 업데이트
BOOL updateMonArea(const RECT *area) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)area;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_UPDATEMONAREA, &param);
}

// 윈도우로 이벤트 전송
BOOL eventToWin(QWORD id, const EVENT *event) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = id;
	PARAM(1) = (QWORD)event;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_EVENTTOWIN, &param);
}

// 윈도우 이벤트 큐에 저장된 이벤트 수신
BOOL winToEvent(QWORD id, EVENT *event) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = id;
	PARAM(1) = (QWORD)event;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_WINTOEVENT, &param);
}

// 윈도우 화면 버퍼에 윈도우 테두리 그림
BOOL drawWinFrame(QWORD id) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = id;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_DRAWWINFRAME, &param);
}

// 윈도우 화면 버퍼에 배경 그림
BOOL drawWinBackground(QWORD id) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = id;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_DRAWWINBACKGROUND, &param);
}

// 윈도우 화면 버퍼에 윈도우 제목 표시줄 그림
BOOL drawWinTitle(QWORD id, const char *title, BOOL select) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = id;
	PARAM(1) = (QWORD)title;
	PARAM(2) = (QWORD)select;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_DRAWWINTITLE, &param);
}

// 윈도우 내부에 버튼 그림
BOOL drawBtn(QWORD id, RECT *btnArea, COLOR background, const char *text, COLOR textColor) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = id;
	PARAM(1) = (QWORD)btnArea;
	PARAM(2) = (QWORD)background;
	PARAM(3) = (QWORD)text;
	PARAM(4) = (QWORD)textColor;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_DRAWBTN, &param);
}

// 윈도우 내부에 점 그림
BOOL drawPixel(QWORD id, int x, int y, COLOR color) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = id;
	PARAM(1) = (QWORD)x;
	PARAM(2) = (QWORD)y;
	PARAM(3) = (QWORD)color;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_DRAWPIXEL, &param);
}

// 윈도우 내부에 직선 그림
BOOL drawLine(QWORD id, int x1, int y1, int x2, int y2, COLOR color) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = id;
	PARAM(1) = (QWORD)x1;
	PARAM(2) = (QWORD)y1;
	PARAM(3) = (QWORD)x2;
	PARAM(4) = (QWORD)y2;
	PARAM(5) = (QWORD)color;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_DRAWLINE, &param);
}

// 윈도우 내부에 사각형 그림
BOOL drawRect(QWORD id, int x1, int y1, int x2, int y2, COLOR color, BOOL fill) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = id;
	PARAM(1) = (QWORD)x1;
	PARAM(2) = (QWORD)y1;
	PARAM(3) = (QWORD)x2;
	PARAM(4) = (QWORD)y2;
	PARAM(5) = (QWORD)color;
	PARAM(6) = (QWORD)fill;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_DRAWRECT, &param);
}

// 윈도우 내부에 원 그림
BOOL drawCircle(QWORD id, int x, int y, int rad, COLOR color, BOOL fill) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = id;
	PARAM(1) = (QWORD)x;
	PARAM(2) = (QWORD)y;
	PARAM(3) = (QWORD)rad;
	PARAM(4) = (QWORD)color;
	PARAM(5) = (QWORD)fill;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_DRAWCIRCLE, &param);
}

// 윈도우 내부에 문자 출력
BOOL drawText(QWORD id, int x, int y, COLOR text, COLOR background, const char *str, int len) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = id;
	PARAM(1) = (QWORD)x;
	PARAM(2) = (QWORD)y;
	PARAM(3) = (QWORD)text;
	PARAM(4) = (QWORD)background;
	PARAM(5) = (QWORD)str;
	PARAM(6) = (QWORD)len;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_DRAWTEXT, &param);
}

// 마우스 커서 이동
void moveCursor(int x, int y) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)x;
	PARAM(1) = (QWORD)y;

	// 시스템 콜 호출
	ExecSysCall(SYSCALL_MOVECURSOR, &param);
}

// 현재 마우스 커서 위치
void getWinCursor(int *x, int *y) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)x;
	PARAM(1) = (QWORD)y;

	// 시스템 콜 호출
	ExecSysCall(SYSCALL_GETWINCURSOR, &param);
}

// 윈도우 화면 버퍼에 버퍼 내용 전송
BOOL bitblt(QWORD id, int x, int y, COLOR *buf, int width, int height) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = id;
	PARAM(1) = (QWORD)x;
	PARAM(2) = (QWORD)y;
	PARAM(3) = (QWORD)buf;
	PARAM(4) = (QWORD)width;
	PARAM(5) = (QWORD)height;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_BITBLT, &param);
}

// 파일 버퍼 내용 분석하여 이미지 전체 크기 및 정보를 JPEG 자료구조에 삽입
BOOL jpgInit(JPEG *jpg, BYTE *buf, DWORD size) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)jpg;
	PARAM(1) = (QWORD)buf;
	PARAM(2) = (QWORD)size;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_JPGINIT, &param);
}

// JPEG 자료구조 저장된 정보를 디코딩하여 출력 버퍼에 저장
BOOL jpgDecode(JPEG *jpg, COLOR *rgb) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)jpg;
	PARAM(1) = (QWORD)rgb;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_JPGDECODE, &param);
}

// CMOS 메모리에서 RTC 컨트롤러가 저장한 현재 시간 읽기
BOOL readTime(BYTE *hour, BYTE *min, BYTE *sec) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)hour;
	PARAM(0) = (QWORD)min;
	PARAM(0) = (QWORD)sec;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_READTIME, &param);
}

// CMOS 메모리에서 RTC 컨트롤러가 저장한 날짜 읽기
BOOL readDate(WORD *year, BYTE *month, BYTE *day, BYTE *week) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)year;
	PARAM(0) = (QWORD)month;
	PARAM(0) = (QWORD)day;
	PARAM(0) = (QWORD)week;

	// 시스템 콜 호출
	return (BOOL)ExecSysCall(SYSCALL_READDATE, &param);
}

// 시리얼 포트로 데이터 송신
void sendSerialData(BYTE *buf, int size) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)buf;
	PARAM(1) = (QWORD)size;

	// 시스템 콜 호출
	ExecSysCall(SYSCALL_SENDSERIALDATA, &param);
}

// 시리얼 포트에서 데이터 읽기
int recvSerialData(BYTE *buf, int size) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = (QWORD)buf;
	PARAM(1) = (QWORD)size;

	// 시스템 콜 호출
	return (int)ExecSysCall(SYSCALL_RECVSERIALDATA, &param);
}

// 시리얼 포트 컨트롤러 FIFO 초기화
void clearSerialFIFO(void) {
	// 시스템 콜 호출
	ExecSysCall(SYSCALL_CLEARSERIALFIFO, NULL);
}

// RAM 크기
QWORD getTotalMemSize(void) {
	// 시스템 콜 호출
	return ExecSysCall(SYSCALL_GETTOTALMEMSIZE, NULL);
}

// Tick Count
QWORD getTickCnt(void) {
	// 시스템 콜 호출
	return ExecSysCall(SYSCALL_GETTICKCNT, NULL);
}

// 밀리세컨드 sleep
void sleep(QWORD millisec) {
	PARAMTBL param;

	// 파라미터 삽입
	PARAM(0) = millisec;

	// 시스템 콜 호출
	ExecSysCall(SYSCALL_SLEEP, &param);
}

// 그래픽 모드 여부
BOOL isGUIMode(void) {
	// 시스템 콜 호출
	ExecSysCall(SYSCALL_ISGUIMODE, NULL);
}
