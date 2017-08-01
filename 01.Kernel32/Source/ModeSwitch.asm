;
; ModeSwitch.asm
;
;  Created on: 2017. 7. 16.
;      Author: Yummy
;

[BITS 32]			; 이하의 코드는 32비트 코드로 설정

; C 언어에서 호출할 수 있도록 이름을 노출함(Export)
global ReadCPUID, SwitchNExecKernel

SECTION .text		; text 섹션(세그먼트)을 정의

; CPUID를 반환(PARAM: DWORD ax, DWORD *eax, *ebx, *ecx, *edx)
ReadCPUID:
	push ebp		; 베이스 포인터 레지스터(EBP)를 스택에 삽입
	mov ebp, esp	; 베이스 포인터 레지스터에 스택 포인터 레지스터(ESP)의 값을 설정
	push eax		; 함수에서 임시로 사용하는 레지스터로 함수의 마지막 부분에서
	push ebx		; 스택에 삽입된 값을 꺼내 원래 값으로 복원
	push ecx
	push edx
	push esi

	mov eax, dword [ ebp + 8 ]	; 파라미터 1(ax)을 EAX 레지스터에 저장
	cpuid

	; *eax
	mov esi, dword [ ebp + 12 ]	; 파라미터 2(eax)를 ESI에 저장
	mov dword [ esi ], eax		; pdwEAX가 포인터이므로 포인터가 가리키는 어드레스에 EAX 레지스터 값 저장

	; *ebx
	mov esi, dword [ ebp + 16 ]	; 파라미터 3(ebc)을 ESI 레지스터에 저장
	mov dword [ esi ], ebx		; pdwEBX가 포인터이므로 포인터가 가리키는 어드레스에 EBX 레지스터 값 저장

	; *ecx
	mov esi, dword [ ebp + 20 ]	; 파라미터 4(ecx)를 ESI 레지스터에 저장
	mov dword [ esi ], ecx		; pdwECX가 포인터이므로 포인터가 가리키는 어드레스에 ECX 레지스터 값 저장

	; *edx
	mov esi, dword [ ebp + 24 ]	; 파라미터 5(edx)를 ESI 레지스터에 저장
	mov dword [ esi ], edx		; pdwEDX가 포인터이므로 포인터가 가리키는 어드레스에 EDX 레지스터 값 저장

	pop esi				; 함수에서 사용이 끝난 ESI 레지스터부터 EBP 레지스터까지를 스택에
	pop edx				; 삽입된 값을 이용해 복원
	pop ecx				; 스택은 가장 마지막에 들어간 데이터가 가장 먼저 나오는 자료구조이므로
	pop ebx				; 삽입의 역순으로 제거해야 함
	pop eax
	pop ebp				; 베이스 포인터 레지스터(EBP) 복원
	ret				; 함수를 호출한 다음 코드의 위치로 복귀

; IA-32e 모드로 전환하고 64비트 커널을 수행
SwitchNExecKernel:
	; CR4 컨트롤 레지스터의 PAE 비트를 1로 설정, OSXMMEXCPT 비트, OSFXSR 비트를 1로 설정
	mov eax, cr4		; CR4 컨트롤 레지스터의 값을 EAX 레지스터에 저장
	or eax, 0x620		; PAE 비트(비트 5)를 1로 설정
	mov cr4, eax		; PAE 비트가 1로 설정된 값을 CR4 컨트롤 레지스터에 저장

	; CR3 컨트롤 레지스터에 PML4 테이블의 어드레스와 캐시 활성화
	mov eax, 0x100000	; EAX 레지스터에 PML4 테이블이 존재하는 0x100000(1MB)를 저장
	mov cr3, eax		; CR3 컨트롤 레지스터에 0x100000(1MB)를 저장

	; IA32_EFER.LME를 1로 설정하여 IA-32e 모드를 활성화. SYSCALL, SYSRET 명령어 활성
	mov ecx, 0xC0000080	; IA32_EFER MSR 레지스터의 어드레스를 저장
	rdmsr			; MSR 레지스터를 읽기

	or eax, 0x0101		; EAX 레지스터에 저장된 IA32_EFER MSR의 하위 32비트에서 LME 비트(비트 8)와 SCE 비트(비트 0)를 1로 설정
	wrmsr			; MSR 레지스터에 쓰기

	; CR0 컨트롤 레지스터를 NW 비트(비트 29) = 0, CD 비트(비트 30) = 0, PG 비트(비트 31) = 1로 설정하여 캐시기능과 페이징 기능 활성화
	; TS 비트(비트 3) = 1, EM 비트(비트 2) = 0, MP 비트(비트 1) = 1로 설정하여 FPU 활성화
	mov eax, cr0		; EAX 레지스터에 CR0 컨트롤 레지스터를 저장
	or eax, 0xE000000E	; NW 비트(비트 29), CD 비트(비트 30), PG 비트(비트 31), TS 비트(비트 3), EM 비트(비트 2), MP 비트(비트 1)를 모두 1로 설정
	xor eax, 0x60000004	; NW 비트(비트 29), CD 비트(비트 30), EM 비트(비트 2)를 XOR하여 0으로 설정
	mov cr0, eax		; NW 비트 = 0, CD 비트 = 0, PG 비트 = 1로 설정한 값을 다시 CR0 컨트롤 레지스터에 저장

	jmp 0x08: 0x200000	; CS 세그먼트 셀렉터를 IA-32e 모드용 코드 세그먼트 디스크립터로 교체하고 0x200000(2MB) 어드레스로 이동
