;
; ISR.asm
;
;  Created on: 2017. 7. 22.
;      Author: Yummy
;

[BITS 64]		; 이하 코드는 64비트 코드로 설정

SECTION .text		; text 섹션(세그먼트)을 정의

; 외부에서 정의된 함수를 쓸 수 있도록 선언(Import)
extern exceptionHandler, interruptHandler, keyboardHandler

; C언어에서 호출할 수 있도록 이름 노출(Export). 예외(Exception) 처리를 위한 ISR
global ISRDivErr, ISRDebug, ISRNMI, ISRBP, ISROF, ISRExceed, ISROPErr, ISRDevErr, ISRDoubleErr, ISRSegmentOverrun, ISRTSSErr
global ISRSegmentErr, ISRStackErr, ISRProtectErr, ISRPageErr, ISR15, ISRFPUErr, ISRAlignChk, ISRMachChk, ISRSIMDErr, ISRETCErr

; 인터럽트(Interrupt) 처리를 위한 ISR
global ISRTimer, ISRKeyboard, ISRSlavePIC, ISRSerial2, ISRSerial1, ISRParallel2, ISRFloppy, ISRParallel1
global ISRRTC, ISRReserved, ISRNotUsed1, ISRNotUsed2, ISRMouse, ISRCoprocessor, ISRHDD1, ISRHDD2, ISRETC

; 콘텍스트를 저장하고 셀렉터를 교체하는 매크로
%macro SAVE 0	; 파라미터를 전달받지 않는 매크로 정의
	; RBP 레지스터부터 GS 세그먼트 셀렉터까지 모두 스택에 삽입
	push rbp
	mov rbp, rsp
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

	mov ax, ds	; DS 세그먼트 셀렉터와 ES 세그먼트 셀렉터는 스택에 직접 삽입할 수 없으므로 RAX 레지스터에 저장 후 스택에 삽입
	push rax
	mov ax, es
	push rax
	push fs
	push gs

	; 세그먼트 셀렉터 교체
	mov ax, 0x10	; AX 레지스터에 커널 데이터 세그먼트 디스크립터 저장
	mov ds, ax	; DS 세그먼트 셀렉터부터 FS 세그먼트 셀렉터까지 모두 커널 데이터 세그먼트로 교체
	mov es, ax
	mov gs, ax
	mov fs, ax
%endmacro

; 콘텍스트 복원 매크로
%macro LOAD 0	; 파라미터 전달받지 않는 매크로 정의
	; GS 세그먼트 셀렉터부터 RBP 레지스터까지 모두 스택에서 꺼내 복원
	pop gs
	pop fs
	pop rax
	mov es, ax	; ES, DS 세그먼트 셀렉터는 스택에서 직접 꺼내 복원할 수 없으니 RAX 레지스터에 저장후 복원
	pop rax
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
%endmacro

; 예외 핸들러
; #0, Divide Error ISR
ISRDivErr:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 0
	call exceptionHandler

	LOAD
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #1, Debug ISR
ISRDebug:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 1
	call exceptionHandler

	LOAD
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #2, NMI ISR
ISRNMI:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 2
	call exceptionHandler

	LOAD
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #3, BreakPoint ISR
ISRBP:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 3
	call exceptionHandler

	LOAD
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #4, Overflow ISR
ISROF:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 4
	call exceptionHandler

	LOAD
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #5, Bound Range Exceeded ISR
ISRExceed:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 5
	call exceptionHandler

	LOAD
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #6, Invalid Opcode ISR
ISROPErr:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 6
	call exceptionHandler

	LOAD
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #7, Device Not Available ISR
ISRDevErr:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 7
	call exceptionHandler

	LOAD
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #8, Double Fault ISR
ISRDoubleErr:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 8
	mov rsi, qword [ rbp + 8 ]
	call exceptionHandler

	LOAD
	add rsp, 8	; 에러 코드 스택에서 제거
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #9, Coprocessor Segment Overrun ISR
ISRSegmentOverrun:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 9
	call exceptionHandler

	LOAD
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #10, Invalid TSS ISR
ISRTSSErr:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 10
	mov rsi, qword [ rbp + 8 ]
	call exceptionHandler

	LOAD
	add rsp, 8
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #11, Segment Not Present ISR
ISRSegmentErr:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 11
	mov rsi, qword [ rbp + 8 ]
	call exceptionHandler

	LOAD
	add rsp, 8
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #12, Stack Segment Fault ISR
ISRStackErr:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 12
	mov rsi, qword [ rbp + 8 ]
	call exceptionHandler

	LOAD
	add rsp, 8
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #13, General Protection ISR
ISRProtectErr:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 13
	mov rsi, qword [ rbp + 8 ]
	call exceptionHandler

	LOAD
	add rsp, 8
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #14, Page Fault ISR
ISRPageErr:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 14
	mov rsi, qword [ rbp + 8 ]
	call exceptionHandler

	LOAD
	add rsp, 8
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #15, Reserved ISR
ISR15:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 15
	call exceptionHandler

	LOAD
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #16, FPU Error ISR
ISRFPUErr:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 16
	call exceptionHandler

	LOAD
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #17, Alignment Check ISR
ISRAlignChk:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 17
	mov rsi, qword [ rbp + 8 ]
	call exceptionHandler

	LOAD
	add rsp, 8
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #18, Machine Check ISR
ISRMachChk:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 18
	call exceptionHandler

	LOAD
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #19, SIMD Floating Point Exception ISR
ISRSIMDErr:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 19
	call exceptionHandler

	LOAD
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #20 ~ 31, Reserved ISR
ISRETCErr:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 20
	call exceptionHandler

	LOAD
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; 인터럽트 핸들러
; #32, Timer ISR
ISRTimer:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 32
	call interruptHandler

	LOAD
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #33, Keyboard ISR
ISRKeyboard:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 33
	call keyboardHandler

	LOAD
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #34, Slave PIC ISR
ISRSlavePIC:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 34
	call interruptHandler

	LOAD
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #35, Serial Port2 ISR
ISRSerial2:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 35
	call interruptHandler

	LOAD
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #36, Serial Port1 ISR
ISRSerial1:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 36
	call exceptionHandler

	LOAD
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #37, Parallel Port2 ISR
ISRParallel2:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 37
	call exceptionHandler

	LOAD
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #38, Floppy Disk Controller ISR
ISRFloppy:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 38
	call exceptionHandler

	LOAD
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #39, Parallel Port1 ISR
ISRParallel1:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 39
	call interruptHandler

	LOAD
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #40, RTC ISR
ISRRTC:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 40
	call interruptHandler

	LOAD
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #41, Reserved Interrupt ISR
ISRReserved:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 41
	call interruptHandler

	LOAD
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #42, Not Use 1
ISRNotUsed1:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 42
	call interruptHandler

	LOAD
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #43, Not Use 2
ISRNotUsed2:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 43
	call interruptHandler

	LOAD
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #44, Mouse ISR
ISRMouse:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 44
	call interruptHandler

	LOAD
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #45, Coprocessor ISR
ISRCoprocessor:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 45
	call interruptHandler

	LOAD
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #46, Hard Disk Drive 1 ISR
ISRHDD1:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 46
	call interruptHandler

	LOAD
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #47, Hard Disk Drive 2 ISR
ISRHDD2:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 47
	call interruptHandler

	LOAD
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #48 etc ISR
ISRETC:
	SAVE	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 48
	call interruptHandler

	LOAD
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원
