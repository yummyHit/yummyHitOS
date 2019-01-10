;
; entry.asm
;
;  Created on: 2018. 6. 16.
;      Author: Yummy
;

[BITS 64]			; 이하의 코드는 64비트 코드로 설정

SECTION .text			; text 섹션(세그먼트)을 정의

; C언어에서 호출할 수 있도록 이름 노출
global Start, ExecSysCall

extern main, exit

Start:
	call main		; C 언어 엔트리 포인트 함수 호출

	mov rdi, rax	; 파라미터에 Main 함수 반환 값(RAX 레지스터) 저장
	call exit

	jmp $
	ret

; 시스템 콜 실행(PARAM: 서비스 번호, 파라미터)
ExecSysCall:
	push rcx		; SYSCALL 호출 시 RCX 레지스터에 RIP 레지스터 저장, R11 레지스터에 RFLAGS 레지스터 저장되니 스택에 보관
	push r11

	syscall

	pop r11			; 스택에 저장된 값으로 RCX 레지스터와 R11 레지스터 복원
	pop rcx
	ret
