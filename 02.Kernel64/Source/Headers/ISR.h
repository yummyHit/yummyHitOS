/*
 * ISR.h
 *
 *  Created on: 2017. 7. 22.
 *      Author: Yummy
 */
#ifndef __ISR_H__
#define __ISR_H__

#pragma once
// 예외처리용 ISR
void kISRDivErr(void);
void kISRDebug(void);
void kISRNMI(void);
void kISRBP(void);
void kISROF(void);
void kISRExceed(void);
void kISROPErr(void);
void kISRDevErr(void);
void kISRDoubleErr(void);
void kISRSegmentOverrun(void);
void kISRTSSErr(void);
void kISRSegmentErr(void);
void kISRStackErr(void);
void kISRProtectErr(void);
void kISRPageErr(void);
void kISR15(void);
void kISRFPUErr(void);
void kISRAlignChk(void);
void kISRMachChk(void);
void kISRSIMDErr(void);
void kISRETCErr(void);

// 인터럽트 처리용 ISR
void kISRTimer(void);
void kISRKeyboard(void);
void kISRSlavePIC(void);
void kISRSerial2(void);
void kISRSerial1(void);
void kISRParallel2(void);
void kISRFloppy(void);
void kISRParallel1(void);
void kISRRTC(void);
void kISRReserved(void);
void kISRNotUsed1(void);
void kISRNotUsed2(void);
void kISRMouse(void);
void kISRCoprocessor(void);
void kISRHDD1(void);
void kISRHDD2(void);
void kISRETC(void);

#endif /*__ISR_H__*/
