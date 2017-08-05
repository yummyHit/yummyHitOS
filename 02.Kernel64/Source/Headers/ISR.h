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
void ISRDivErr(void);
void ISRDebug(void);
void ISRNMI(void);
void ISRBP(void);
void ISROF(void);
void ISRExceed(void);
void ISROPErr(void);
void ISRDevErr(void);
void ISRDoubleErr(void);
void ISRSegmentOverrun(void);
void ISRTSSErr(void);
void ISRSegmentErr(void);
void ISRStackErr(void);
void ISRProtectErr(void);
void ISRPageErr(void);
void ISR15(void);
void ISRFPUErr(void);
void ISRAlignChk(void);
void ISRMachChk(void);
void ISRSIMDErr(void);
void ISRETCErr(void);

// 인터럽트 처리용 ISR
void ISRTimer(void);
void ISRKeyboard(void);
void ISRSlavePIC(void);
void ISRSerial2(void);
void ISRSerial1(void);
void ISRParallel2(void);
void ISRFloppy(void);
void ISRParallel1(void);
void ISRRTC(void);
void ISRReserved(void);
void ISRNotUsed1(void);
void ISRNotUsed2(void);
void ISRMouse(void);
void ISRCoprocessor(void);
void ISRHDD1(void);
void ISRHDD2(void);
void ISRETC(void);

#endif /*__ISR_H__*/
