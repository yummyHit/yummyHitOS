#ifndef __PORT_H__
#define __PORT_H__

#include "Types.h"

BYTE inByte(WORD port);
void outByte(WORD port, BYTE data);
void loadGDTR(QWORD GDTRAddr);
void loadTSS(WORD TSSSegmentOffset);
void loadIDTR(QWORD IDTRAddr);

#endif /*__PORT_H__*/