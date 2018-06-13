/*
 * JPEG decoding engine for DCT-baseline
 *
 *      JPEGLS - Copyright(c) 2004 by Hajime Uchimura(nikutama@gmail.com)
 * 
 * history::
 * 2003/04/28 | added OSASK-GUI ( by H.Kawai ) 
 * 2003/05/12 | optimized DCT ( 20-bits fixed point, etc...) -> line 407-464 ( by I.Tak. )
 * 2009/11/21 | optimized to RGB565 ( by kkamagui )
 * 2018/04/09 | studied MINT 64 OS book and modify ( by yummyHit )
 */

 #ifndef __JPEG_H__
 #define __JPEG_H__

 #include <Types.h>
 #include <BaseGraph.h>

 #pragma once

 // 허프만 테이블
 typedef struct {
 	int elem;	// 요소 갯수
	unsigned short code[256];
	unsigned char size[256];
	unsigned char value[256];
} HUFF;

// JPEG decoding 자료구조
typedef struct {
	// SOF
	int width, height;

	// MCU
	int mcu_width, mcu_height;
	int max_h, max_v;
	int compo_cnt;
	int compo_id[3], compo_sample[3];
	int compo_h[3], compo_v[3], compo_qt[3];

	// SOS
	int scan_cnt;
	int scan_id[3], scan_ac[3], scan_dc[3];
	int scan_h[3], scan_v[3], scan_qt[3];	// h, v : sampling component count / qt : quantization table index

	// DRI
	int interval;
	int mcu_buf[32 * 32 * 4];	// buffer
	int *mcu_yuv[4];
	int mcu_preDC[3];

	// DQT
	int dqt[3][64];
	int n_dqt;

	// DHT
	HUFF huff[2][3];

	// i/o
	unsigned char *data;
	int data_idx, data_size;
	unsigned long bit_buf;
	int bit_remain;
} JPEG;

// 함수
BOOL kJpgInit(JPEG *jpg, BYTE *fileBuf, DWORD fileSize);
BOOL kJpgDecode(JPEG *jpg, COLOR *outBuf);

#endif /*__JPEG_H__*/
