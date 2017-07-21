/*
 * InterruptHandler.h
 *
 *  Created on: 2017. 7. 22.
 *      Author: Yummy
 */

#ifndef __INTERRUPTHANDLER_H__
#define __INTERRUPTHANDLER_H__

#include "Types.h"

void exceptionHandler(int vecNum, QWORD errCode);
void interruptHandler(int vecNum);
void keyboardHandler(int vecNum);

#endif /*__INTERRUPTHANDLER_H__*/
