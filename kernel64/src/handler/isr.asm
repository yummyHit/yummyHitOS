;
; isr.asm
;
;  Created on: 2017. 7. 22.
;      Author: Yummy
;

[BITS 64]		; 이하 코드는 64비트 코드로 설정

SECTION .text		; text 섹션(세그먼트)을 정의

; 외부에서 정의된 함수를 쓸 수 있도록 선언(Import)
extern kExceptionHandler, kInterruptHandler, kKeyboardHandler
extern kTimerHandler, kDevFPUHandler, kHardDiskHandler, kMouseHandler

; C언어에서 호출할 수 있도록 이름 노출(Export). 예외(Exception) 처리를 위한 ISR
global kISRDivErr, kISRDebug, kISRNMI, kISRBP, kISROF, kISRExceed, kISROPErr, kISRDevErr, kISRDoubleErr, kISRSegmentOverrun, kISRTSSErr
global kISRSegmentErr, kISRStackErr, kISRProtectErr, kISRPageErr, kISR15, kISRFPUErr, kISRAlignChk, kISRMachChk, kISRSIMDErr, kISRETCErr

; 인터럽트(Interrupt) 처리를 위한 ISR
global kISRTimer, kISRKeyboard, kISRSlavePIC, kISRSerial2, kISRSerial1, kISRParallel2, kISRFloppy, kISRParallel1
global kISRRTC, kISRReserved, kISRNotUsed1, kISRNotUsed2, kISRMouse, kISRCoprocessor, kISRHDD1, kISRHDD2, kISRETC

; 콘텍스트를 저장하고 셀렉터를 교체하는 매크로
%macro SAVE_ISR 0	; 파라미터를 전달받지 않는 매크로 정의. RBP 레지스터부터 GS 세그먼트 셀렉터까지 모두 스택에 삽입
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
%macro LOAD_ISR 0	; 파라미터 전달받지 않는 매크로 정의. GS 세그먼트 셀렉터부터 RBP 레지스터까지 모두 스택에서 꺼내 복원
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

; #0, Divide Error ISR
kISRDivErr:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 0
	call kExceptionHandler

	LOAD_ISR
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #1, Debug ISR
kISRDebug:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 1
	call kExceptionHandler

	LOAD_ISR
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #2, NMI ISR
kISRNMI:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 2
	call kExceptionHandler

	LOAD_ISR
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #3, BreakPoint ISR
kISRBP:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 3
	call kExceptionHandler

	LOAD_ISR
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #4, Overflow ISR
kISROF:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 4
	call kExceptionHandler

	LOAD_ISR
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #5, Bound Range Exceeded ISR
kISRExceed:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 5
	call kExceptionHandler

	LOAD_ISR
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #6, Invalid Opcode ISR
kISROPErr:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 6
	call kExceptionHandler

	LOAD_ISR
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #7, Device Not Available ISR
kISRDevErr:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 7
	call kDevFPUHandler

	LOAD_ISR
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #8, Double Fault ISR
kISRDoubleErr:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 8
	mov rsi, qword [ rbp + 8 ]
	call kExceptionHandler

	LOAD_ISR
	add rsp, 8	; 에러 코드 스택에서 제거
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #9, Coprocessor Segment Overrun ISR
kISRSegmentOverrun:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 9
	call kExceptionHandler

	LOAD_ISR
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #10, Invalid TSS ISR
kISRTSSErr:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 10
	mov rsi, qword [ rbp + 8 ]
	call kExceptionHandler

	LOAD_ISR
	add rsp, 8
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #11, Segment Not Present ISR
kISRSegmentErr:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 11
	mov rsi, qword [ rbp + 8 ]
	call kExceptionHandler

	LOAD_ISR
	add rsp, 8
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #12, Stack Segment Fault ISR
kISRStackErr:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 12
	mov rsi, qword [ rbp + 8 ]
	call kExceptionHandler

	LOAD_ISR
	add rsp, 8
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #13, General Protection ISR
kISRProtectErr:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 13
	mov rsi, qword [ rbp + 8 ]
	call kExceptionHandler

	LOAD_ISR
	add rsp, 8
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #14, Page Fault ISR
kISRPageErr:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 14
	mov rsi, qword [ rbp + 8 ]
	call kExceptionHandler

	LOAD_ISR
	add rsp, 8
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #15, Reserved ISR
kISR15:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 15
	call kExceptionHandler

	LOAD_ISR
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #16, FPU Error ISR
kISRFPUErr:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 16
	call kExceptionHandler

	LOAD_ISR
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #17, Alignment Check ISR
kISRAlignChk:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 17
	mov rsi, qword [ rbp + 8 ]
	call kExceptionHandler

	LOAD_ISR
	add rsp, 8
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #18, Machine Check ISR
kISRMachChk:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 18
	call kExceptionHandler

	LOAD_ISR
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #19, SIMD Floating Point Exception ISR
kISRSIMDErr:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 19
	call kExceptionHandler

	LOAD_ISR
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #20 ~ 31, Reserved ISR
kISRETCErr:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 20
	call kExceptionHandler

	LOAD_ISR
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; 인터럽트 핸들러
; #32, Timer ISR
kISRTimer:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 32
	call kTimerHandler

	LOAD_ISR
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #33, Keyboard ISR
kISRKeyboard:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 33
	call kKeyboardHandler

	LOAD_ISR
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #34, Slave PIC ISR
kISRSlavePIC:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 34
	call kInterruptHandler

	LOAD_ISR
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #35, Serial Port2 ISR
kISRSerial2:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 35
	call kInterruptHandler

	LOAD_ISR
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #36, Serial Port1 ISR
kISRSerial1:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 36
	call kExceptionHandler

	LOAD_ISR
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #37, Parallel Port2 ISR
kISRParallel2:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 37
	call kExceptionHandler

	LOAD_ISR
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #38, Floppy Disk Controller ISR
kISRFloppy:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 38
	call kExceptionHandler

	LOAD_ISR
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #39, Parallel Port1 ISR
kISRParallel1:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 39
	call kInterruptHandler

	LOAD_ISR
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #40, RTC ISR
kISRRTC:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 40
	call kInterruptHandler

	LOAD_ISR
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #41, Reserved Interrupt ISR
kISRReserved:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 41
	call kInterruptHandler

	LOAD_ISR
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #42, Not Use 1
kISRNotUsed1:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 42
	call kInterruptHandler

	LOAD_ISR
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #43, Not Use 2
kISRNotUsed2:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 43
	call kInterruptHandler

	LOAD_ISR
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #44, Mouse ISR
kISRMouse:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 44
	call kMouseHandler

	LOAD_ISR
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #45, Coprocessor ISR
kISRCoprocessor:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 45
	call kInterruptHandler

	LOAD_ISR
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #46, Hard Disk Drive 1 ISR
kISRHDD1:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 46
	call kHardDiskHandler

	LOAD_ISR
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #47, Hard Disk Drive 2 ISR
kISRHDD2:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 47
	call kHardDiskHandler

	LOAD_ISR
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원

; #48 etc ISR
kISRETC:
	SAVE_ISR	; 콘텍스트 저장 후 셀렉터를 커널 데이터 디스크립터로 교체

	; 핸들러에 예외 번호를 삽입하고 핸들러 호출
	mov rdi, 48
	call kInterruptHandler

	LOAD_ISR
	iretq		; 인터럽트 처리 완료 후 이전 수행 코드로 복원
