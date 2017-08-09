/*
 * InterruptHandler.h
 *
 *  Created on: 2017. 7. 22.
 *      Author: Yummy
 */

#ifndef __INTERRUPTHANDLER_H__
#define __INTERRUPTHANDLER_H__

#include <Types.h>

#pragma once

void exceptionHandler(int vecNum, QWORD errCode);
void interruptHandler(int vecNum);
void keyboardHandler(int vecNum);
void timerHandler(int verNum);
void devFPUHandler(int vecNum);
void hardDiskHandler(int vecNum);

#endif /*__INTERRUPTHANDLER_H__*/
