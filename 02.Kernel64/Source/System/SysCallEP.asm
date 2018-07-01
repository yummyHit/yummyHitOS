;
; SysCallEP.asm
;
;  Created on: 2018. 6. 16.
;      Author: Yummy
;

[BITS 64]		; 이하 코드는 64비트 코드로 설정

SECTION .text	; text section

; 외부에서 정의된 함수 쓸 수 있도록 선언
extern kSysCallProc

; C언어에서 호출할 수 있도록 이름 노출
global kSysCallEntryPoint, kSysCallTaskTest

; 유저 레벨에서 SYSCALL 했을 때 호출되는 시스템 콜 엔트리 포인트(PARAM: 서비스 번호, 파라미터)
kSysCallEntryPoint:
	push rcx		; SYSRET으로 돌아갈 때 사용할 RIP 레지스터 값과 RFLAGS 레지스터 값 저장
	push r11
	mov cx, ds		; CS, SS 세그먼트 셀렉터 제외 나머지 세그먼트 셀렉터 값 저장
	push cx
	mov cx, es		; DS, ES 세그먼트 셀렉터는 스택에 바로 저장할 수 없으니 CX 레지스터로 옮긴 후 저장
	push cx
	push fs
	push gs

	mov cx, 0x10	; 커널 데이터 세그먼트 셀렉터의 인덱스를 CX 레지스터에 저장
	mov ds, cx		; 세그먼트 셀렉터를 커널 데이터 세그먼트 셀렉터로 교체
	mov es, cx
	mov fs, cx
	mov gs, cx

	call kSysCallProc	; 서비스 번호에 따라 커널 레벨 함수 호출

	pop gs			; 스택에 저장된 값으로 세그먼트 셀렉터의 값과 RCX, R11 레지스터 복원
	pop fs
	pop cx
	mov es, cx
	pop cx
	mov ds, cx
	pop r11
	pop rcx

	o64 sysret		; SYSCALL 명령어로 호출되었으므로 SYSRET 명령어로 복귀해야 함

; 시스템 콜 테스트 유저 레벨 태스크. 태스크 시스템 콜 서비스를 3번 연속으로 호출 후 exit() 서비스를 호출하여 종료(PARAM: 없음)
kSysCallTaskTest:
	mov rdi, 0xFFFFFFFF	; 서비스 번호에 테스트 시스템 콜(SYSCALL_TEST, 0xFFFFFFFF) 저장
	mov rsi, 0x00		; 시스템 콜 호출시 사용하는 파라미터 테이블 포인터에 NULL 저장
	syscall			; 시스템 콜 호출

	mov rdi, 0xFFFFFFFF
	mov rsi, 0x00
	syscall

	mov rdi, 0xFFFFFFFF
	mov rsi, 0x00
	syscall

	mov rdi, 24		; 서비스 번호에 태스크 종료(SYSCALL_EXITTASK, 24) 저장
	mov rsi, 0x00
	syscall
	jmp $
