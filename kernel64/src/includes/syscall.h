/*
 * syscall.h
 *
 *  Created on: 2018. 6. 16.
 *      Author: Yummy
 */

#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#pragma once

// 파라미터로 전달할 수 있는 최대 개수
#define SYSCALL_MAXPARAMCNT			20

#pragma pack(push, 1)
// 시스템 콜을 호출할 때 전달하는 파라미터를 관리하는 자료구조
typedef struct kSystemCallParameterTable {
	QWORD val[SYSCALL_MAXPARAMCNT];
} PARAMTBL;
#pragma pack(pop)

#define PARAM(x)	(param->val[(x)])

void kInitSysCall(void);
void kSysCallEntryPoint(QWORD num, PARAMTBL *param);
QWORD kSysCallProc(QWORD num, PARAMTBL *param);
void kSysCallTaskTest(void);

#endif	/*__SYSCALL_H__*/
