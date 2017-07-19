/*
 * ModeSwitch.h
 *
 *  Created on: 2017. 7. 18.
 *      Author: Yummy
 */

#ifndef __MODESWITCH_H__
#define __MODESWITCH_H__

#include "Types.h"

void kReadCPUID(DWORD dwEAX, DWORD *pdwEAX, DWORD *pdwEBX, DWORD *pdwECX, DWORD *pdwEDX);
void kSwitchAndExecute64bitKernel(void);

#endif /* __MODESWITCH_H__ */
