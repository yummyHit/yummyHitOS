/*
 * Util.h
 *
 *  Created on: 2017. 7. 22.
 *      Author: Yummy
 */

#ifndef __UTIL_H__
#define __UTIL_H__

#include "Types.h"

void memSet(void *dest, BYTE data, int size);
int memCpy(void *dest, const void *src, int size);
int memCmp(const void *dest, const void *src, int size);
BOOL setInterruptFlag(BOOL _onInterrupt);

#endif /*__UTIL_H__*/
