/*
 * Port.h
 *
 *  Created on: 2017. 7. 20.
 *      Author: Yummy
 */

#ifndef __PORT_H__
#define __PORT_H__

#include <Types.h>

BYTE inByte(WORD port);
void outByte(WORD port, BYTE data);
void loadGDTR(QWORD GDTRAddr);
void loadTSS(WORD TSSSegmentOffset);
void loadIDTR(QWORD IDTRAddr);
void onInterrupt(void);
void offInterrupt(void);
QWORD readRFLAGS(void);
QWORD readTSC(void);

#endif /*__PORT_H__*/
