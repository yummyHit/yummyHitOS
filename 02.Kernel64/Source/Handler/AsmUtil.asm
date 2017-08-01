;
; AsmUtil.asm
;
;  Created on: 2017. 7. 22.
;      Author: Yummy
;

[BITS 64]			; 이하의 코드는 64비트 코드로 설정

SECTION .text			; text 섹션(세그먼트)을 정의

; C언어에서 호출할 수 있도록 이름 노출
global inByte, outByte, loadGDTR, loadTSS, loadIDTR
global onInterrupt, offInterrupt, readRFLAGS		; 인터럽트 추가
global readTSC, switchContext, hlt

; 포트로부터 1바이트 읽음(PARAM: 포트 번호)
inByte:
	push rdx	; 함수에서 임시로 사용하는 레지스터를 스택에 저장
			; 함수의 마지막 부분에서 스택에 삽입된 값을 꺼내 복원
	mov rdx, rdi	; RDX 레지스터에 파라미터 1(포트 번호)을 저장
	mov rax, 0	; RAX 레지스터를 초기화
	in al, dx	; DX 레지스터에 저장된 포트 어드레스에서 한 바이트를 읽어 AL 레지스터에 저장
			; AL 레지스터는 함수의 반환 값으로 사용
	pop rdx		; 함수에서 사용이 끝난 레지스터 복원
	ret		; 함수를 호출한 다음 코드의 위치로 복귀

; 포트에 1바이트 씀(PARAM: 포트 번호, 데이터)
outByte:
	push rdx	; 함수에서 임시로 사용하는 레지스터를 스택에 저장
	push rax	; 함수의 마지막 부분에서 스택에 삽입된 값을 꺼내 복원
	mov rdx, rdi	; RDX 레지스터에 파라미터 1(포트 번호)을 저장
	mov rax, rsi	; RAX 레지스터에 파라미터 2(데이터)를 저장
	out dx, al	; DX 레지스터에 저장된 포트 어드레스에 AL 레지스터에 저장된 한 바이트 사용
	pop rax
	pop rdx
	ret

; GDTR 레지스터에 GDT테이블 설정(PARAM: GDT테이블 정보 저장하는 자료구조 어드레스)
loadGDTR:
	lgdt [ rdi ]	; 파라미터 1(GDTR 어드레스)을 프로세서에 로드해 GDT테이블 설정
	ret

; TR 레지스터에 TSS 세그먼트 디스크립터 설정(PARAM: TSS 세그먼트 디스크립터 오프셋)
loadTSS:
	ltr di		; 파라미터 1(TSS 세그먼트 디스크립터 오프셋)을 프로세서에 설정해 TSS 세그먼트 로드
	ret

; IDTR 레지스터에 IDT 테이블 설정(PARAM: IDT 테이블 정보 저장하는 자료구조 어드레스)
loadIDTR:
	lidt [ rdi ]	; 파라미터 1(IDTR 어드레스)을 프로세서에 로드해 IDT테이블 설정
	ret

; 인터럽트 활성화
onInterrupt:
	sti
	ret

; 인터럽트 비활성화
offInterrupt:
	cli
	ret

; RFLAGS 레지스터 읽고 되돌려줌
readRFLAGS:
	pushfq		; RFLAGS 레지스터를 스택에 저장
	pop rax		; 스택에 저장된 RFLAGS 레지스터를 RAX 레지스터에 저장해 함수 반환값으로 설정
	ret

; 타임 스탬프 카운터를 읽어서 반환
readTSC:
	push rdx	; RDX 레지스터를 스택에 저장
	rdtsc		; 타임 스탬프 카운터를 읽어서 RDX:RAX에 저장
	shl rdx, 32	; RDX 레지스터에 있는 상위 32비트 TSC 값과 RAX 레지스터에 있는 하위 32비트 TSC 값을 OR하여
	or rax, rdx	; RAX 레지스터에 64비트 TSC 값을 저장
	pop rdx
	ret

; 콘텍스트 저장, 셀렉터 교체 매크로
%macro SAVE 0		; 파라미터 전달받지 않는 SAVE 매크로 정의, RBP 레지스터부터 GS 세그먼트 셀렉터까지 모두 스택에 삽입
	push rbp
	push rax
	push rbx
	push rcx
	push rdx
	push rdi
	push rsi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15

	mov ax, ds	; DS 세그먼트 셀렉터와 ES 세그먼트 셀렉터는 스택에 직접 삽입할 수 없으므로
	push rax	; RAX 레지스터에 저장 후 스택에 삽입
	mov ax, es
	push rax
	push fs
	push gs
%endmacro		; 매크로 끝

; 콘텍스트 복원 매크로
%macro LOAD 0		; 파라미터 전달받지 않는 LOAD 매크로 정의, GS 세그먼트 셀렉터부터 RAX 레지스터까지 모두 스택에서 꺼내 복원
	pop gs
	pop fs
	pop rax
	mov es, ax	; ES 세그먼트 셀렉터와 DS 세그먼트 셀렉터는 스택에서 직접 꺼내 복원할 수 없으므로
	pop rax		; RAX 레지스터에 저장한 뒤 복원
	mov ds, ax

	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rsi
	pop rdi
	pop rdx
	pop rcx
	pop rbx
	pop rax
	pop rbp
%endmacro		; 매크로 끝

; Current Context에 현재 콘텍스트 저장하고 Next Task에서 콘텍스트 복구(PARAM: Current Context(curContext), Next Context(nextContext))
switchContext:
	push rbp	; 스택에 RBP 레지스터 저장 후 RSP 레지스터를 RBP에 저장
	mov rbp, rsp

	pushfq		; 아래의 cmp 결과로 RFLAGS 레지스터가 변하지 않게 스택에 저장
	cmp rdi, 0	; Current Context가 NULL이면 콘텍스트 복원으로 점프
	je .Load
	popfq		; 스택에 저장한 RFLAGS 레지스터 복원
	
	push rax	; 콘텍스트 영역 오프셋으로 사용할 RAX 레지스터 스택에 저장

	; SS, RSP, RFLAGS, CS, RIP 레지스터 순서대로 삽입
	mov ax, ss
	mov qword[ rdi + (23 * 8) ], rax

	mov rax, rbp	; RBP에 저장된 RSP 레지스터 저장
	add rax, 16	; RSP 레지스터는 push rbp와 return address를 제외한 값으로 저장
	mov qword[ rdi + (22 * 8) ], rax

	pushfq		; RFLAGS 레지스터 저장
	pop rax
	mov qword[ rdi + (21 * 8) ], rax

	mov ax, cs	; CS 레지스터 저장
	mov qword[ rdi + (20 * 8) ], rax

	mov rax, qword[ rbp + 8 ]	; RIP 레지스터를 return address로 설정해 다음 콘텍스트 복원 시 이 함수 호출한 위치로 이동
	mov qword[ rdi + (19 * 8) ], rax

	; 저장한 레지스터를 복구한 후 인터럽트가 발생했을 때처럼 나머지 콘텍스트 모두 저장
	pop rax
	pop rbp

	; 가장 끝부분에 SS, RSP, RFLAGS, CS, RIP 레지스터 저장했으니 이전 영역에 push 명령어로 콘텍스트 저장하기 위한 스택 변경
	add rdi, (19 * 8)
	mov rsp, rdi
	sub rdi, (19 * 8)

	; 나머지 레지스터를 모두 Context 자료구조에 저장
	SAVE

; 다음 태스크의 콘텍스트 복원
.Load:
	mov rsp, rsi

	; Context 자료구조에서 레지스터 복원
	LOAD
	iretq

; 프로세서 쉬게 함
hlt:
	hlt	; 프로세서를 대기 상태로 진입
	hlt
	ret
