[BITS 64]			; 이하의 코드는 64비트 코드로 설정

SECTION .text			; text 섹션(세그먼트)을 정의

; C언어에서 호출할 수 있도록 이름 노출
global inByte, outByte, loadGDTR, loadTSS, loadIDTR

; 포트로부터 1바이트 읽음
; PARAM: 포트 번호
inByte:
	push rdx	; 함수에서 임시로 사용하는 레지스터를 스택에 저장
			; 함수의 마지막 부분에서 스택에 삽입된 값을 꺼내 복원
	mov rdx, rdi	; RDX 레지스터에 파라미터 1(포트 번호)을 저장
	mov rax, 0	; RAX 레지스터를 초기화
	in al, dx	; DX 레지스터에 저장된 포트 어드레스에서 한 바이트를 읽어 AL 레지스터에 저장
			; AL 레지스터는 함수의 반환 값으로 사용
	pop rdx		; 함수에서 사용이 끝난 레지스터 복원
	ret		; 함수를 호출한 다음 코드의 위치로 복귀

; 포트에 1바이트 씀
; PARAM: 포트 번호, 데이터
outByte:
	push rdx	; 함수에서 임시로 사용하는 레지스터를 스택에 저장
	push rax	; 함수의 마지막 부분에서 스택에 삽입된 값을 꺼내 복원
	mov rdx, rdi	; RDX 레지스터에 파라미터 1(포트 번호)을 저장
	mov rax, rsi	; RAX 레지스터에 파라미터 2(데이터)를 저장
	out dx, al	; DX 레지스터에 저장된 포트 어드레스에 AL 레지스터에 저장된 한 바이트 사용
	pop rax
	pop rdx
	ret

; GDTR 레지스터에 GDT테이블 설정
; PARAM: GDT테이블 정보 저장하는 자료구조 어드레스
loadGDTR:
	lgdt [ rdi ]	; 파라미터 1(GDTR 어드레스)을 프로세서에 로드해 GDT테이블 설정
	ret

; TR 레지스터에 TSS 세그먼트 디스크립터 설정
; PARAM: TSS 세그먼트 디스크립터 오프셋
loadTSS:
	ltr di		; 파라미터 1(TSS 세그먼트 디스크립터 오프셋)을 프로세서에 설정해 TSS 세그먼트 로드
	ret

; IDTR 레지스터에 IDT 테이블 설정
; PARAM: IDT 테이블 정보 저장하는 자료구조 어드레스
loadIDTR:
	lidt [ rdi ]	; 파라미터 1(IDTR 어드레스)을 프로세서에 로드해 IDT테이블 설정
	ret
