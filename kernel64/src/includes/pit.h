/*
 * pit.h
 *
 *  Created on: 2017. 7. 29.
 *      Author: Yummy
 */

#ifndef __PIT_H__
#define __PIT_H__

#include <types.h>

#pragma once
// Macro
#define PIT_FREQUENCY			1193182
#define MSTOCNT(x)			(PIT_FREQUENCY * (x) / 1000)
#define USTOCNT(x)			(PIT_FREQUENCY * (x) / 1000000)

// I/O Port
#define PIT_PORT_CTRL		0x43
#define PIT_PORT_COUNTER_ZERO		0x40
#define PIT_PORT_COUNTER_ONE		0x41
#define PIT_PORT_COUNTER_TWO		0x42

// Mode
#define PIT_CTRL_COUNTER_ZERO	0x00
#define PIT_CTRL_COUNTER_ONE		0x40
#define PIT_CTRL_COUNTER_TWO		0x80
#define PIT_CTRL_LSBMSBRW		0x30
#define PIT_CTRL_LATCH		0x00
#define PIT_CTRL_MODE_ZERO		0x00
#define PIT_CTRL_MODE_TWO		0x04

// Binary Or BCD
#define PIT_CTRL_BINARYCOUNTER	0x00
#define PIT_CTRL_BCDCOUNTER		0x01

#define PIT_COUNTER_ZERO_ONCE		(PIT_CTRL_COUNTER_ZERO | PIT_CTRL_LSBMSBRW | PIT_CTRL_MODE_ZERO | PIT_CTRL_BINARYCOUNTER)
#define PIT_COUNTER_ZERO_TERM		(PIT_CTRL_COUNTER_ZERO | PIT_CTRL_LSBMSBRW | PIT_CTRL_MODE_TWO | PIT_CTRL_BINARYCOUNTER)
#define PIT_COUNTER_ZERO_LATCH		(PIT_CTRL_COUNTER_ZERO | PIT_CTRL_LATCH)

void kInitPIT(WORD cnt, BOOL term);
WORD kReadCntZero(void);
void kWaitPIT(WORD cnt);

#endif /*__PIT_H__*/