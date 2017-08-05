/*
 * RTC.h
 *
 *  Created on: 2017. 7. 29.
 *      Author: Yummy
 */

#ifndef __RTC_H__
#define __RTC_H__

#include <Types.h>

#pragma once
// I/O Port
#define RTC_CMOSADDR		0x70
#define RTC_CMOSDATA		0x71

// CMOS Memory Address
#define RTC_ADDR_SEC		0x00
#define RTC_ADDR_MIN		0x02
#define RTC_ADDR_HOUR		0x04
#define RTC_ADDR_WEEK		0x06
#define RTC_ADDR_DAY		0x07
#define RTC_ADDR_MONTH		0x08
#define RTC_ADDR_YEAR		0x09

// BCD 포맷을 Binary로 변환하는 매크로
#define RTC_BCDTOBINARY( x )	(((( x ) >> 4) * 10) + (( x ) & 0x0F))

void readTime(BYTE *hour, BYTE *min, BYTE *sec);
void readDate(WORD *year, BYTE *month, BYTE *day, BYTE *week);
char *convertWeek(BYTE week);

#endif /*__RTC_H__*/
