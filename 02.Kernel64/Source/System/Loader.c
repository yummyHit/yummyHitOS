/*
 * Loader.c
 *
 *  Created on: 2018. 7. 7.
 *      Author: Yummy
 */

#include <Loader.h>
#include <FileSystem.h>
#include <DynMem.h>

QWORD kExecProg(const char *fileName, const char *argv, BYTE affinity) {
	DIR *dir;
	struct dirent *entry;
	DWORD fileSize, readSize;
	BYTE *tmpBuf;
	FILE *fp;
	QWORD epAddr, appMem, memSize;
	TCB *task;

	// 루트 디렉터리를 열어 파일 검색
	dir = dopen("/");
	fileSize = 0;

	// 디렉터리 내 파일 검색
	while(1) {
		// 디렉터리에서 엔트리 하나 읽음
		entry = dread(dir);
		// 더 이상 파일이 없으면 끝
		if(entry == NULL) break;

		// 파일 이름의 기일와 내용이 같은 것 검색
		if((kStrLen(entry->d_name) == kStrLen(fileName)) && (kMemCmp(entry->d_name, fileName, kStrLen(fileName)) == 0)) {
			fileSize = entry->size;
			break;
		}
	}
	// 디렉터리 핸들 반환, 핸들 반환을 안하면 메모리가 해제되지 않음
	dclose(dir);

	if(fileSize == 0) {
		kPrintf("%s file not found or size is zero!\n", fileName);
		return TASK_INVALID_ID;
	}

	// 파일 전체 저장가능한 임시 버퍼를 할당 받아 파일 내용 모두 저장
	tmpBuf = (BYTE*)kAllocMem(fileSize);
	if(tmpBuf == NULL) {
		kPrintf("Memory %dbytes allocate failed..\n", fileSize);
		return TASK_INVALID_ID;
	}

	// 파일 열어서 모두 읽어 메모리에 저장
	fp = fopen(fileName, "r");
	if((fp != NULL) && (fread(tmpBuf, 1, fileSize, fp) == fileSize)) {
		fclose(fp);
		kPrintf("%s file read success!\n", fileName);
	} else {
		kPrintf("%s file read failed..\n", fileName);
		kFreeMem(tmpBuf);
		fclose(fp);
		return TASK_INVALID_ID;
	}

	// 파일 내용 분석해 섹션 로딩 및 재배치 수행
	if(kLoadProgReloc(tmpBuf, &appMem, &memSize, &epAddr) == FALSE) {
		kPrintf("%s file isn't binary file or loading failed..\n", fileName);
		kFreeMem(tmpBuf);
		return TASK_INVALID_ID;
	}

	kFreeMem(tmpBuf);

	// 태스크 생성 및 스택에 아규먼트 저장
	task = kCreateTask(TASK_FLAGS_USERLV | TASK_FLAGS_PROC, (void*)appMem, memSize, epAddr, affinity);
	if(task == NULL) {
		kFreeMem((void*)appMem);
		return TASK_INVALID_ID;
	}

	// 아규먼트 저장
	kAddArgvToTask(task, argv);

	return task->link.id;
}

// 응용프로그램 섹션 로딩 및 재배치 수행
static BOOL kLoadProgReloc(BYTE *buf, QWORD *appMemAddr, QWORD *appMemSize, QWORD *epAddr) {
	ELF64_EHDR *elf_h;
	ELF64_SHDR *section_h, *sectionNameTbl_h;
	ELF64_XWORD lastSectionSize = 0;
	ELF64_ADDR lastSectionAddr = 0;
	int i;
	QWORD memSize, stackAddr;
	BYTE *loadAddr;

	// ELF 헤더 정보 출력 및 분석에 필요한 정보 저장
	elf_h = (ELF64_EHDR*)buf;
	section_h = (ELF64_SHDR*)(buf + elf_h->e_shoff);
	sectionNameTbl_h = section_h + elf_h->e_shstridx;

	kPrintf("======================== ELF Header Info ========================\n");
	kPrintf("Magic Number [%c%c%c] Section Header Count [%d]\n", elf_h->e_id[1], elf_h->e_id[2], elf_h->e_id[3], elf_h->e_shnum);
	kPrintf("File Type [%d]\n", elf_h->e_type);
	kPrintf("Section Header Offset [0x%X] Size [0x%X]\n", elf_h->e_shoff, elf_h->e_shentsize);
	kPrintf("Program Header Offset [0x%X] Size [0x%X]\n", elf_h->e_phoff, elf_h->e_phentsize);
	kPrintf("Section Name String Table Section Index [%d]\n", elf_h->e_shstridx);

	// ELF -- id, class, encoding, type 확인해 올바른 응용프로그램인지 확인
	if((elf_h->e_id[EI_MAG0] != ELFMAG0) || (elf_h->e_id[EI_MAG1] != ELFMAG1) || (elf_h->e_id[EI_MAG1] != ELFMAG1) || (elf_h->e_id[EI_MAG2] != ELFMAG2) || (elf_h->e_id[EI_MAG3] != ELFMAG3) || (elf_h->e_id[EI_CLASS] != ELFCLASS64) || (elf_h->e_id[EI_DATA] != ELFDATA2LSB) || (elf_h->e_type != ET_REL)) return FALSE;

	// 모든 섹션 헤더 로딩 메모리 어드레스를 확인해 가장 마지막 섹션 찾고 섹션 정보 표시
	for(i = 0; i < elf_h->e_shnum; i++) {
		// 가장 마지막 섹션인지 확인, 이 값으로 프로그램이 사용할 전체 메모리 크기 확인
		if((section_h[i].sh_flag & SHF_ALLOC) && (section_h[i].sh_addr >= lastSectionAddr)) {
			lastSectionAddr = section_h[i].sh_addr;
			lastSectionSize = section_h[i].sh_size;
		}
	}

	kPrintf("\n======================== Load & Relocation ========================\n");
	kPrintf("Last Section Address [0x%q] Size [0x%q]\n", lastSectionAddr, lastSectionSize);

	// 마지막 섹션 위치로 최대 메모리 계산, 4KB 단위로 정렬
	memSize = (lastSectionAddr + lastSectionSize + 0x1000 - 1) & 0xfffffffffffff000;
	kPrintf("Aligned Memory Size [0x%q]\n", memSize);

	// 응용프로그램에서 사용할 메모리 할당
	loadAddr = (char*)kAllocMem(memSize);
	if(loadAddr == NULL) {
		kPrintf("Memory allocate failed..\n");
		return FALSE;
	} else {
		kPrintf("Loaded Address [0x%q]\n", loadAddr);
	}

	// 파일 내용 메모리에 복사(로딩)
	for(i = 1; i < elf_h->e_shnum; i++) {
		// 메모리에 올릴 필요가 없는 섹션이거나 Size 가 0인 Section이면 복사 안함
		if(!(section_h[i].sh_flag & SHF_ALLOC) || (section_h[i].sh_size == 0)) continue;

		// 섹션 헤더에 로딩할 어드레스 적용
		section_h[i].sh_addr += (ELF64_ADDR) loadAddr;

		// .bss 와 같이 SHT_NOBITS가 설정된 섹션은 파일에 데이터가 없으므로 0으로 초기화
		if(section_h[i].sh_type == SHT_NOBITS) kMemSet(section_h[i].sh_addr, 0, section_h[i].sh_size);	// 응용프로그램에게 할당된 메모리를 0으로 설정
		else kMemCpy(section_h[i].sh_addr, buf + section_h[i].sh_offset, section_h[i].sh_size);	// 파일 버퍼의 내용을 응용프로그램에게 할당된 메모리로 복사
		kPrintf("Section [%x] Virtual Address [%q] File Address [%q] Size [%q]\n", i, section_h[i].sh_addr, buf + section_h[i].sh_offset, section_h[i].sh_size);
	}
	kPrintf("Program load success!\n");

	// 재배치 수행
	if(kReloc(buf) == FALSE) {
		kPrintf("Relocation failed..\n");
		return FALSE;
	} else {
		kPrintf("Relocation success!\n");
	}

	// 응용프로그램의 어드레스와 엔트리 포인트 어드레스 반환
	*appMemAddr = (QWORD)loadAddr;
	*appMemSize = memSize;
	*epAddr = elf_h->e_entry + (QWORD)loadAddr;

	return TRUE;
}

// 재배치 수행. 섹션 헤더에는 메모리 어드레스가 할당되어 있어야 함
static BOOL kReloc(BYTE *buf) {
	ELF64_EHDR *elf_h;
	ELF64_SHDR *section_h;
	int i, j, symbolTblIdx, sectionIdxSymbol, sectionIdxReloc, numOfByte;
	ELF64_ADDR offset;
	ELF64_XWORD info;
	ELF64_SXWORD add, res;
	ELF64_REL *rel;
	ELF64_RELA *rela;
	ELF64_SYM *symbolTbl;

	// ELF 헤더와 섹션 헤더 테이블의 첫 번째 헤더를 찾음
	elf_h = (ELF64_EHDR*)buf;
	section_h = (ELF64_SHDR*)(buf + elf_h->e_shoff);

	// 모든 섹션 헤더를 검색해 SHT_REL 또는 SHT_RELA 타입을 가지는 섹션을 찾아 재배치를 수행
	for(i = 1; i < elf_h->e_shnum; i++) {
		if((section_h[i].sh_type != SHT_RELA) && (section_h[i].sh_type != SHT_REL)) continue;

		// sh_info 필드에 재배치를 수행해야 할 섹션 헤더의 인덱스가 저장되어 있음
		sectionIdxReloc = section_h[i].sh_info;

		// sh_link 에 참고하는 심볼 테이블 섹션 헤더의 인덱스가 저장되어 있음
		symbolTblIdx = section_h[i].sh_link;

		// 심볼 테이블 섹션의 첫 번째 엔트리 저장
		symbolTbl = (ELF64_SYM*)(buf + section_h[symbolTblIdx].sh_offset);

		// 재배치 섹션의 엔트리 모두 찾아 재배치 수행
		for(j = 0; j < section_h[i].sh_size;) {
			// SHT_REL 타입
			if(section_h[i].sh_type == SHT_REL) {
				// SHT_REL 타입은 더해야 하는 값이 없으므로 0으로 설정
				rel = (ELF64_REL*)(buf + section_h[i].sh_offset + j);
				offset = rel->r_offset;
				info = rel->r_info;
				add = 0;

				// SHT_REL 자료구조 크기만큼 이동
				j += sizeof(ELF64_REL);
			} else {
				//SHT_RELA 타입
				rela = (ELF64_RELA*)(buf + section_h[i].sh_offset + j);
				offset = rela->r_offset;
				info = rela->r_info;
				add = rela->r_add;

				// SHT_RELA 자료구조 크기만큼 이동
				j += sizeof(ELF64_RELA);
			}

			// 절대 어드레스 타입은 재배치가 필요 없음
			if(symbolTbl[RELOCATION_UPPER32(info)].st_shidx == SHI_ABS) continue;
			// 공통 타입 심볼은 지원하지 않으니 오류 표시 후 종료
			else if(symbolTbl[RELOCATION_UPPER32(info)].st_shidx == SHI_COMMON) {
				kPrintf("Common Symbol is not supported..\n");
				return FALSE;
			}

			// 재배치 타입을 구해 재배치 수행 값 계산
			switch(RELOCATION_LOWER32(info)) {
				// S(st_value) + A(r_add)로 계산
				case R_X86_64_64:
				case R_X86_64_32:
				case R_X86_64_32S:
				case R_X86_64_16:
				case R_X86_64_8:
					// 심볼이 존재하는 섹션 헤더 인덱스
					sectionIdxSymbol = symbolTbl[RELOCATION_UPPER32(info)].st_shidx;
					res = (symbolTbl[RELOCATION_UPPER32(info)].st_value + section_h[sectionIdxSymbol].sh_addr) + add;
					break;

				// S(st_value) + A(r_add) - P(r_offset)로 계산하는 타입
				case R_X86_64_PC32:
				case R_X86_64_PC16:
				case R_X86_64_PC8:
				case R_X86_64_PC64:
					// 심볼이 존재하는 섹션 헤더 인덱스
					sectionIdxSymbol = symbolTbl[RELOCATION_UPPER32(info)].st_shidx;
					res = (symbolTbl[RELOCATION_UPPER32(info)].st_value + section_h[sectionIdxSymbol].sh_addr) + add - (offset + section_h[sectionIdxReloc].sh_addr);
					break;

				// B(sh_addr) + A(r_add)로 계산하는 타입
				case R_X86_64_RELATIVE:
					res = section_h[i].sh_addr + add;
					break;

				// Z(st_size) + A(r_add)로 계산하는 타입
				case R_X86_64_SIZE32:
				case R_X86_64_SIZE64:
					res = symbolTbl[RELOCATION_UPPER32(info)].st_size + add;
					break;

				// 그 외 지원하지 않으니 오류 표시 후 종료
				default:
					kPrintf("Un supported relocation type [%X]\n", RELOCATION_LOWER32(info));
					return FALSE;
					break;
			}

			// 재배치 타입으로 적용할 범위 계싼
			switch(RELOCATION_LOWER32(info)) {
				// 64비트 크기
				case R_X86_64_64:
				case R_X86_64_PC64:
				case R_X86_64_SIZE64:
					numOfByte = 8;
					break;

				// 32비트 크기
				case R_X86_64_PC32:
				case R_X86_64_32:
				case R_X86_64_32S:
				case R_X86_64_SIZE32:
					numOfByte = 4;
					break;

				// 16비트 크기
				case R_X86_64_16:
				case R_X86_64_PC16:
					numOfByte = 2;
					break;

				// 8비트 크기
				case R_X86_64_8:
				case R_X86_64_PC8:
					numOfByte = 1;
					break;

				// 기타 타입은 오류 표시 후 종료
				default:
					kPrintf("Unsupported relocation type [%X]\n", RELOCATION_LOWER32(info));
					return FALSE;
					break;
			}

			// 계산 결과와 적용 범위가 나왔으니 해당 섹션 적용
			switch(numOfByte) {
				case 8:
					*((ELF64_SXWORD*)(section_h[sectionIdxReloc].sh_addr + offset)) += res;
					break;

				case 4:
					*((int*)(section_h[sectionIdxReloc].sh_addr + offset)) += (int)res;
					break;

				case 2:
					*((short*)(section_h[sectionIdxReloc].sh_addr + offset)) += (short)res;
					break;

				case 1:
					*((char*)(section_h[sectionIdxReloc].sh_addr + offset)) += (char)res;
					break;

				// 그 외 크기는 지원하지 않으니 오류 표시 후 종료
				default:
					kPrintf("Relocation error. Relocation byte size is [%d]byte\n", numOfByte);
					return FALSE;
					break;
			}
		}
	}
	return TRUE;
}

// 태스크에 인자 문자열 저장
static void kAddArgvToTask(TCB *task, const char *argv) {
	int len, alignLen;
	QWORD rspAddr;

	// 인자 문자열 길이 계산
	if(argv == NULL) len = 0;
	else {
		// 인자 문자열은 최대 1KB
		len = kStrLen(argv);

		if(len > 1023) len = 1023;
	}

	// 인자 문자열 길이를 8byte로 정렬
	alignLen = (len + 7) & 0xFFFFFFF8;

	// 새로운 rsp 레지스터 값 계산 후 스택에 인자 리스트 복사
	rspAddr = task->context.reg[TASK_RSPOFFSET] - (QWORD)alignLen;
	kMemCpy((void*)rspAddr, argv, len);
	*((BYTE*)rspAddr + len) = '\0';

	// RSP 레지스터와 RBP 레지스터 값을 새로운 스택 어드레스로 갱신
	task->context.reg[TASK_RSPOFFSET] = rspAddr;
	task->context.reg[TASK_RBPOFFSET] = rspAddr;

	// 첫 번째 파라미터로 사용되는 RDI 레지스터를 파라미터가 저장된 스택 어드레스로 지정
	task->context.reg[TASK_RDIOFFSET] = rspAddr;
}

// 응용프로그램에서 동작하는 쓰레드 생성
QWORD kCreateThread(QWORD ep, QWORD arg, BYTE affinity, QWORD exit) {
	TCB *task;

	// 유저 레벨 응용프로그램 태스크 생성
	task = kCreateTask(TASK_FLAGS_USERLV | TASK_FLAGS_THREAD, NULL, 0, ep, affinity);
	if(task == NULL) return TASK_INVALID_ID;

	// 종료될 때 호출되는 kTaskFin() 함수를 전달받은 함수로 대체. 현재 RSP 레지스터가 가리킴
	*((QWORD*)task->context.reg[TASK_RSPOFFSET]) = exit;

	// 첫 번째 파라미터로 사용되는 RDI 레지스터에 인자 삽입
	task->context.reg[TASK_RDIOFFSET] = arg;

	return task->link.id;
}
