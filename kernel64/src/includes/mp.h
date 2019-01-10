/*
 * mp.h
 *
 *  Created on: 2017. 8. 19.
 *      Author: Yummy
 */

#ifndef __MP_H__
#define __MP_H__

#include <types.h>

#pragma once

// 매크로, MultiProcessor 관련
#define BSP_FLAGADDR	0x7C09
// 지원 가능한 최대 프로세서 또는 코어 개수
#define MAXPROCESSORCNT	16

BOOL kStartUpAP(void);
BYTE kGetAPICID(void);
static BOOL kAwakeAP(void);

#endif /*__MP_H__*/
