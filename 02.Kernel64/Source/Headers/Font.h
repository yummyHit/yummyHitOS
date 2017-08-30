/*
 * Font.h
 *
 *  Created on: 2017. 8. 31.
 *      Author: Yummy
 */

#ifndef __FONT_H__
#define __FONT_H__

#pragma once

// 영문 폰트 너비 및 길이
#define FONT_ENG_WIDTH	8
#define FONT_ENG_HEIGHT	16

// 한글 폰트 너비 및 길이
#define FONT_KOR_WIDTH	16
#define FONT_KOR_HEIGHT	16

// 비트맵 폰트 데이터
extern unsigned char g_engFont[];
extern unsigned char g_korFont[];

#endif /*__FONT_H__*/
