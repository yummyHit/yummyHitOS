/*
 * PIC.h
 *
 *  Created on: 2017. 7. 22.
 *      Author: Yummy
 */

#ifndef __PIC_H__
#define __PIC_H__

#include <Types.h>

// 매크로. IO 포트 정의
#define PIC_MASTER_PORT_A	0x20
#define PIC_MASTER_PORT_B	0x21
#define PIC_SLAVE_PORT_A	0xA0
#define PIC_SLAVE_PORT_B	0xA1

// IDT 테이블에서 인터럽트 벡터가 시작되는 위치
#define PIC_IRQSTARTVECTOR	0x20

void initPIC(void);
void maskPIC(WORD mask);
void sendEOI(int num);

#endif /*__PIC_H__*/
