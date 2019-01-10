/*
 * loader.h
 *
 *  Created on: 2018. 7. 9.
 *      Author: Yummy
 */

#ifndef __LOADER_H__
#define __LOADER_H__

#include <types.h>
#include <clitask.h>

#pragma once

// 기본 데이터 타입을 정의한 매크로
#define ELF64_ADDR		unsigned long
#define ELF64_OFF		unsigned long
#define ELF64_HALF		unsigned short int
#define ELF64_WORD		unsigned int
#define ELF64_SWORD		int
#define ELF64_XWORD		unsigned long
#define ELF64_SXWORD	long

// e_id[]의 index 리스트
#define EI_MAG0			0
#define EI_MAG1			1
#define EI_MAG2			2
#define EI_MAG3			3
#define EI_CLASS		4
#define EI_DATA			5
#define EI_VERSION		6
#define EI_OSABI		7
#define EI_ABIVERSION	8
#define EI_PAD			9
#define EI_NIDENT		16

// e_id[EI_MAGX]
#define ELFMAG0			0x7F
#define ELFMAG1			'E'
#define ELFMAG2			'L'
#define ELFMAG3			'F'

// e_id[EI_CLASS]
#define ELFCLASSNONE	0
#define ELFCLASS32		1
#define ELFCLASS64		2

// e_id[EI_DATA]
#define ELFDATANONE		0
#define ELFDATA2LSB		1
#define ELFDATA2MSB		2

// e_id[OSABI]
#define ELFOSABI_NONE		0
#define ELFOSABI_HPUX		1
#define ELFOSABI_NETBSD		2
#define ELFOSABI_LINUX		3
#define ELFOSABI_SOLARIS	6
#define ELFOSABI_AIX		7
#define ELFOSABI_FREEBSD	9

// e_type
#define ET_NONE			0
#define ET_REL			1
#define ET_EXEC			2
#define ET_DYN			3
#define ET_CORE			4
#define ET_LOOS			0xFE00
#define ET_HIOS			0xFEFF
#define ET_LOPROC		0xFF00
#define ET_HIPROC		0xFFFF

// e_machine
#define EM_NONE			0
#define EM_M32			1
#define EM_SPARC		2
#define EM_386			3
#define EM_PPC			20
#define EM_PPC64		21
#define EM_ARM			40
#define EM_IA_64		50
#define EM_X86_64		62
#define EM_AVR			83
#define EM_AVR32		185
#define EM_CUDA			190

// Special Section Index
#define SHI_UNDEF		0
#define SHI_LORESERVE	0xFF00
#define SHI_LOPROC		0xFF00
#define SHI_HIPROC		0xFF1F
#define SHI_LOOS		0xFF20
#define SHI_HIOS		0xFF3F
#define SHI_ABS			0xFFF1
#define SHI_COMMON		0xFFF2
#define SHI_XINDEX		0xFFFF
#define SHI_HIRESERVE	0xFFFF

// sh_type
#define SHT_NULL		0
#define SHT_PROGBITS	1
#define SHT_SYMTAB		2
#define SHT_STRTAB		3
#define SHT_RELA		4
#define SHT_HASH		5
#define SHT_DYNAMIC		6
#define SHT_NOTE		7
#define SHT_NOBITS		8
#define SHT_REL			9
#define SHT_SHLIB		10
#define SHT_DYNSYM		11
#define SHT_LOOS		0x60000000
#define SHT_HIOS		0x6FFFFFFF
#define SHT_LOPROC		0x70000000
#define SHT_HIPROC		0x7FFFFFFF
#define SHT_LOUSER		0x80000000
#define SHT_HIUSER		0xFFFFFFFF

// sh_flags
#define SHF_WRITE		1
#define SHF_ALLOC		2
#define SHF_EXECSTR		4
#define SHF_MASKOS		0x0FF00000
#define SHF_MASKPROC	0xF0000000

// Relocation Type
#define R_X86_64_NONE		0	// none
#define R_X86_64_64			1	// word64	S + A
#define R_X86_64_PC32		2	// word32	S + A - P
#define R_X86_64_GOT32		3	// word32	G + A
#define R_X86_64_PLT32		4	// word32	L + A - P
#define R_X86_64_COPY		5	// none
#define R_X86_64_GLOB_DAT	6	// word64	S
#define R_X86_64_JMP_SLOT	7	// word64	S
#define R_X86_64_RELATIVE	8	// word64	B + A
#define R_X86_64_GOTPCREL	9	// word32	G + GOT + A - P
#define R_X86_64_32			10	// word32	S + A
#define R_X86_64_32S		11	// word32	S + A
#define R_X86_64_16			12	// word16	S + A
#define R_X86_64_PC16		13	// word16	S + A - P
#define R_X86_64_8			14	// word8	S + A
#define R_X86_64_PC8		15	// word8	S + A - P
#define R_X86_64_DPTMOD64	16	// word64
#define R_X86_64_DPTOFF64	17	// word64
#define R_X86_64_TPOFF64	18	// word64
#define R_X86_64_TLSGD		19	// word32
#define R_X86_64_TLSLD		20	// word32
#define R_X86_64_DTPOFF32	21	// word32
#define R_X86_64_GOTTPOFF	22	// word32
#define R_X86_64_TPOFF32	23	// word32
#define R_X86_64_PC64		24	// word64	S + A - P
#define R_X86_64_GOTOFF64	25	// word64	S + A - GOT
#define R_X86_64_GOTPC32	26	// word32	GOT + A - P
#define R_X86_64_SIZE32		32	// word32	Z + A
#define R_X86_64_SIZE64		33	// word64	Z + A

// 상위 32비트와 하위 32비트 값 추출 매크로
#define RELOCATION_UPPER32(x)	((x) >> 32)
#define RELOCATION_LOWER32(x)	((x) & 0xFFFFFFFF)

#pragma pack(push, 1)

// ELF64 파일 포맷 ELF 헤더 자료구조
typedef struct {
	unsigned char e_id[16];	// ELF 식별자
	ELF64_HALF e_type;			// 오브젝트 파일 형식
	ELF64_HALF e_machine;		// 머신 타입
	ELF64_WORD e_version;		// 오브젝트 파일 버전
	ELF64_ADDR e_entry;			// 엔트리포인트 어드레스
	ELF64_OFF e_phoff;			// 프로그램 헤더 오프셋(파일 내 존재하는 프로그램 헤더 테이블 위치)
	ELF64_OFF e_shoff;			// 섹션 헤더 오프셋(파일 내 존재하는 섹션 헤더 테이블 위치)
	ELF64_WORD e_flag;			// 프로세서 의존적 플래그
	ELF64_HALF e_ehsize;		// ELF 헤더 크기
	ELF64_HALF e_phentsize;		// 프로그램 헤더 엔트리 한 개 크기
	ELF64_HALF e_phnum;			// 프로그램 헤더 엔트리 개수
	ELF64_HALF e_shentsize;		// 섹션 헤더 엔트리 한 개 크기
	ELF64_HALF e_shnum;			// 섹션 헤더 엔트리 개수
	ELF64_HALF e_shstridx;		// 섹션 이름 문자열 저장된 섹션 헤더 인덱스
} ELF64_EHDR;

// ELF64 섹션 헤더 자료구조
typedef struct {
	ELF64_WORD sh_name;			// 섹션 이름이 저장된 오프셋
	ELF64_WORD sh_type;			// 섹션 타입
	ELF64_XWORD sh_flag;		// 섹션 플래그
	ELF64_ADDR sh_addr;			// 메모리에 로딩할 어드레스
	ELF64_OFF sh_offset;		// 파일 내 존재하는 섹션 오프셋
	ELF64_XWORD sh_size;		// 섹션 크기
	ELF64_WORD sh_link;			// 연결된 다른 섹션
	ELF64_WORD sh_info;			// 부가적인 정보
	ELF64_XWORD sh_alignAddr;	// 어드레스 정렬
} ELF64_SHDR;

// ELF64 심볼 테이블 엔트리 자료구조
typedef struct {
	ELF64_WORD st_name;			// 심볼 이름이 저장된 오프셋
	unsigned char st_info;		// 심볼 타입과 바인딩 속성
	unsigned char st_reserved;	// 예약됨
	ELF64_HALF st_shidx;		// 심볼이 정의된 섹션 헤더 인덱스
	ELF64_ADDR st_value;		// 심볼 값
	ELF64_XWORD st_size;		// 심볼 크기
} ELF64_SYM;

// ELF64 재배치 엔트리 자료구조(SHT_REL)
typedef struct {
	ELF64_ADDR r_offset;		// 재배치 수행할 어드레스
	ELF64_XWORD r_info;			// 심볼 인덱스와 재배치 타입
} ELF64_REL;

// ELF64 재배치 엔트리 자료구조(SHT_RELA)
typedef struct {
	ELF64_ADDR r_offset;		// 재배치 수행할 어드레스
	ELF64_XWORD r_info;			// 심볼 인덱스와 재배치 타입
	ELF64_SXWORD r_add;			// 더하는 수(상수)
} ELF64_RELA;

#pragma pack(pop)

QWORD kExecFile(const char *fileName, const char *argv, BYTE affinity);
static BOOL kLoadProgReloc(BYTE *buf, QWORD *appMemAddr, QWORD *appMemSize, QWORD *epAddr);
static BOOL kReloc(BYTE *buf);
static void kAddArgvToTask(TCB *task, const char *argv);
QWORD kCreateThread(QWORD ep, QWORD arg, BYTE affinity, QWORD exit);

#endif	/*__LOADER_H__*/
