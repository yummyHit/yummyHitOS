/*
 * GUITask.h
 *
 *  Created on: 2017. 9. 5.
 *      Author: Yummy
 */

#ifndef __GUITASK_H__
#define __GUITASK_H__

#include <Types.h>

#pragma once

// 태스크가 보내는 유저 이벤트 타입 정의
#define EVENT_USER_TESTMSG		0x80000001

void baseGUITask(void);
void firstGUITask(void);
void exitGUITask(void);

#endif /*__GUITASK_H__*/
