/*
 * AsmUtil.h
 *
 *  Created on: 2017. 7. 20.
 *      Author: Yummy
 */

#ifndef __ASMUTIL_H__
#define __ASMUTIL_H__

#include <Types.h>
#include <CLITask.h>

#pragma once

BYTE kInByte(WORD port);
void kOutByte(WORD port, BYTE data);
WORD kInWord(WORD port);
void kOutWord(WORD port, WORD data);

void kLoadGDTR(QWORD GDTRAddr);
void kLoadTSS(WORD TSSSegmentOffset);
void kLoadIDTR(QWORD IDTRAddr);

void kOnInterrupt(void);
void kOffInterrupt(void);
QWORD kReadRFLAGS(void);
QWORD kReadTSC(void);

void kSwitchContext(CONTEXT *nowContext, CONTEXT *nextContext);
void kHlt(void);
//void _shutdown(void);
BOOL kTestNSet(volatile BYTE *dest, BYTE cmp, BYTE src);
void kPause(void);

void kInitFPU(void);
void kSaveFPU(void *contextFPU);
void kLoadFPU(void *contextFPU);
void kSetTS(void);
void kClearTS(void);

void kOnLocalAPIC(void);
void kReadMSR(QWORD MSRAddr, QWORD *rdx, QWORD *rax);
void kWriteMSR(QWORD MSRAddr, QWORD rdx, QWORD rax);

#endif /*__ASMUTIL_H__*/
