/*
 * vbe.h
 *
 *  Created on: 2017. 8. 29.
 *      Author: Yummy
 */

#ifndef __VBE_H__
#define __VBE_H__

#include <types.h>

#pragma once

// 매크로, 모드 정보 블록이 저장된 어드레스
#define VBE_MODEINFO_BLOCKADDR		0x7E00
// 그래픽 모드로 시작하는 플래그가 저장된 어드레스
#define VBE_GUIMODE_STARTADDR	0x7C0A

// 구조체
#pragma pack(push, 1)

// VBE에서 정의한 모드 정보 블록 자료구조, 256바이트
typedef struct kVbeModeInformation {
	// 모든 VBE 버전에 공통 부분
	WORD modeAttr;			// 모드 속성
	BYTE winOneAttr;		// 윈도우 1의 속성
	BYTE winTwoAttr;		// 윈도우 2의 속성
	WORD winWeighting;		// 윈도우 가중치
	WORD winSize;			// 윈도우 크기
	WORD winOneSegAddr;		// 윈도우 1이 시작하는 세그먼트 어드레스
	WORD winTwoSegAddr;		// 윈도우 2가 시작하는 세그먼트 어드레스
	DWORD winFuncPtr;		// 윈도우 관련 함수 포인터(리얼모드)
	WORD perByteNum;		// 화면 스캔 라인당 바이트 수

	// VBE 버전 1.2 이상 공통 부분
	WORD xPixel;			// X축 픽셀 수 또는 문자 수
	WORD yPixel;			// Y축 픽셀 수 또는 문자 수
	BYTE xNum;			// 한 문자의 X축 픽셀 수
	BYTE yNum;			// 한 문자의 Y축 픽셀 수
	BYTE memPlaneNum;		// 메모리 플레인 수
	BYTE perPixelBit;		// 한 픽셀을 구성하는 비트 수
	BYTE bankNum;			// 뱅크 수
	BYTE memModel;			// 비디오 메모리 모델
	BYTE bankSize;			// 뱅크의 크기(KB)
	BYTE imgPageNum;		// 이미지 페이지 개수
	BYTE _reserved;			// 페이지 기능을 위해 예약된 영역

	// 다이렉트 컬러에 관련된 필드
	BYTE redMaskSize;		// 빨간색 필드가 차지하는 크기
	BYTE redPosition;		// 빨간색 필드 위치
	BYTE greenMaskSize;		// 녹색 필드가 차지하는 크기
	BYTE greenPosition;		// 녹색 필드 위치
	BYTE blueMaskSize;		// 파란색 필드가 차지하는 크기
	BYTE bluePosition;		// 파란색 필드 위치
	BYTE reserved_maskSize;		// 예약된 필드 크기
	BYTE reserved_position;		// 예약된 필드 위치
	BYTE directModeInfo;		// 다이렉트 컬러 모드 정보

	// VBE 버전 2.0 이상 공통 부분
	DWORD linearBaseAddr;		// 선형 프레임 버퍼 메모리 시작 어드레스
	DWORD reserved_A;
	DWORD reserved_B;

	// VBE 버전 3.0 이상 공통 부분
	WORD linearPerByteNum;		// 선형 프레임 버퍼 모드의 화면 스캔 라인당 바이트 수
	BYTE imgPageBankNum;		// 뱅크 모드일 때 이미지 페이지 수
	BYTE imgPageLinearNum;	// 선형 프레임 버퍼 모드일 때 이미지 페이지 수

	// 선형 프레임 버퍼 모드일 때 다이렉트 컬러에 관련된 필드
	BYTE linearRedMaskSize;		// 빨간색 필드가 차지하는 크기
	BYTE linearRedPosition;		// 빨간색 필드 위치
	BYTE linearGreenMaskSize;	// 녹색 필드가 차지하는 크기
	BYTE linearGreenPosition;	// 녹색 필드 위치
	BYTE linearBlueMaskSize;	// 파란색 필드가 차지하는 크기
	BYTE linearBluePosition;	// 파란색 필드 위치
	BYTE linearReserved_maskSize;	// 예약된 필드 크기
	BYTE linearReserved_position;	// 예약된 필드 위치
	DWORD maxPixelClock;		// 픽셀 클록 최대값(Hz)

	BYTE reserved[189];		// 나머지 영역
} VBEMODEINFO;

#pragma pack(pop)

inline VBEMODEINFO *kGetVBEModeInfo(void);

#endif /* __VBE_H__ */
