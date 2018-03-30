/*
 * WinManager.h
 *
 *  Created on: 2017. 9. 3.
 *      Author: Yummy
 */

#ifndef __WINMANAGER_H__
#define __WINMANAGER_H__

#include <Types.h>

#pragma once

// 윈도우 매니저 태스크가 처리할 데이터나 이벤트 통합 최대 개수
#define WINDOWMANAGER_DATACNT	20

void startWinManager(void);
BOOL procMouseData(void);
BOOL procKeyData(void);
BOOL procEventData(void);

#endif /*__WINMANAGER_H__*/
