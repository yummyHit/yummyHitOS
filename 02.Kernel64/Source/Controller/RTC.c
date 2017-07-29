/*
 * RTC.c
 *
 *  Created on: 2017. 7. 29.
 *      Author: Yummy
 */

#include <RTC.h>

// CMOS 메모리에서 RTC 컨트롤러가 저장한 현재 시간 읽음
void readTime(BYTE *hour, BYTE *min, BYTE *sec) {
	BYTE data;

	// CMOS 메모리 어드레스 레지스터(포트 0x70)에 시간을 저장하는 레지스터 지정
	outByte(RTC_CMOSADDR, RTC_ADDR_HOUR);
	// CMOS 데이터 레지스터(포트 0x71)에서 시간 읽음
	data = inByte(RTC_CMOSDATA);
	*hour = RTC_BCDTOBINARY(data);

	// CMOS 메모리 어드레스 레지스터(포트 0x70)에 분을 저장하는 레지스터 지정
	outByte(RTC_CMOSADDR, RTC_ADDR_MIN);
	// CMOS 데이터 레지스터(포트 0x71)에서 분을 읽음
	data = inByte(RTC_CMOSDATA);
	*min = RTC_BCDTOBINARY(data);

	// CMOS 메모리 어드레스 레지스터(포트 0x70)에 초를 저장하는 레지스터 지정
	outByte(RTC_CMOSADDR, RTC_ADDR_SEC);
	// CMOS 데이터 레지스터(포트 0x71)에서 초를 읽음
	data = inByte(RTC_CMOSDATA);
	*sec = RTC_BCDTOBINARY(data);
}

// CMOS 메모리에서 RTC 컨트롤러가 저장한 현재 일자 읽음
void readDate(WORD *year, BYTE *month, BYTE *day, BYTE *week) {
	BYTE data;

	// CMOS 메모리 어드레스 레지스터(포트 0x70)에 연도를 저장하는 레지스터 지정
	outByte(RTC_CMOSADDR, RTC_ADDR_YEAR);
	// CMOS 데이터 레지스터(포트 0x71)에서 연도 읽음
	data = inByte(RTC_CMOSDATA);
	*year = RTC_BCDTOBINARY(data) + 2000;

	// CMOS 메모리 어드레스 레지스터(포트 0x70)에 월을 저장하는 레지스터 지정
	outByte(RTC_CMOSADDR, RTC_ADDR_MONTH);
	// CMOS 데이터 레지스터(포트 0x71)에서 월을 읽음
	data = inByte(RTC_CMOSDATA);
	*month = RTC_BCDTOBINARY(data);

	// CMOS 메모리 어드레스 레지스터(포트 0x70)에 일을 저장하는 레지스터 지정
	outByte(RTC_CMOSADDR, RTC_ADDR_DAY);
	// CMOS 데이터 레지스터(포트 0x71)에서 일을 읽음
	data = inByte(RTC_CMOSDATA);
	*day = RTC_BCDTOBINARY(data);

	// CMOS 메모리 어드레스 레지스터(포트 0x70)에 요일을 저장하는 레지스터 지정
	outByte(RTC_CMOSADDR, RTC_ADDR_WEEK);
	// CMOS 데이터 레지스터(포트 0x71)에서 요일 읽음
	data = inByte(RTC_CMOSDATA);
	*week = RTC_BCDTOBINARY(data);
}

// 요일 값을 이용해 해당 요일 문자열 반환
char *convertWeek(BYTE week) {
	static char *weekStr[8] = { "Error", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

	// 요일 범위가 넘어가면 에러
	if(week >= 8) return weekStr[0];
	return weekStr[week];
}
