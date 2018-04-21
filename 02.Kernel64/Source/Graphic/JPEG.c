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
#include <JPEG.h>

int zigzag_tbl[] = {
	0, 1, 8, 16, 9, 2, 3, 10,
	17, 24, 32, 25, 18, 11, 4, 5,
	12, 19, 26, 33, 40, 48, 41, 34,
	27, 20, 13, 6, 7, 14, 21, 28,
	35, 42, 49, 56, 57, 50, 43, 36,
	29, 22, 15, 23, 30, 37, 44, 51,
	58, 59, 52, 45, 38, 31, 39, 46,
	53, 60, 61, 54, 47, 55, 62, 63,
	0
};

// 버퍼에서 데이터를 반환하는 함수
unsigned char get_byte(JPEG *jpg) {
	unsigned char c;

	c = jpg->data[jpg->data_idx];
	jpg->data_idx++;

	return c;
}

int get_word(JPEG *jpg) {
	unsigned char h, l;

	h = get_byte(jpg);
	l = get_byte(jpg);

	return (h << 8) | l;
}

unsigned short get_bits(JPEG *jpg, int bit) {
	unsigned char c;
	unsigned short ret;
	unsigned long buf;
	int remain;

	buf = jpg->bit_buf;
	remain = jpg->bit_remain;

	while(remain <= 16) {
		c = get_byte(jpg);
		if(c == 0xFF) get_byte(jpg);
		buf = (buf << 8) | c;
		remain += 8;
	}

	ret = (unsigned short)(buf >> (remain - bit)) & ((1 << bit) - 1);
	remain -= bit;

	jpg->bit_remain = remain;
	jpg->bit_buf = buf;

	return ret;
}

// 지원하지 않는 것은 버퍼에서 제외
void jpg_skip(JPEG *jpg) {
	unsigned w;
	w = get_word(jpg) - 2;
	jpg->data_idx += w;
}

// start of frame
int jpg_sof(JPEG *jpg) {
	unsigned char c, n;
	int i, h, v;

	c = get_word(jpg);

	c = get_byte(jpg);	// bpp
	jpg->height = get_word(jpg);
	jpg->width = get_word(jpg);

	n = get_byte(jpg);		// Num of compo
	jpg->compo_cnt = n;		// nf

	for(i = 0; i < n; i++) {
		jpg->compo_id[i] = get_byte(jpg);

		c = get_byte(jpg);
		jpg->compo_sample[i] = c;
		h = (c >> 4) & 0x0F;
		v = c & 0x0F;

		if(jpg->max_h < h) jpg->max_h = h;
		if(jpg->max_v < v) jpg->max_v = v;

		jpg->compo_h[i] = (c >> 4) & 0x0F;
		jpg->compo_v[i] = c & 0x0F;

		jpg->compo_qt[i] = get_byte(jpg);
	}

	return 0;
}

// data restart interval
void jpg_dri(JPEG *jpg) {
	get_word(jpg);
	jpg->interval = get_word(jpg);
}

// define quantize table
int jpg_dqt(JPEG *jpg) {
	unsigned char c;
	int i, j, v, size;

	size = get_word(jpg) - 2;

	while(size > 0) {
		c = get_byte(jpg);
		size--;
		j = c & 7;
		if(j > jpg->n_dqt) jpg->n_dqt = j;

		if(c >> 3) {
			// 16 bit DQT
			for(i = 0; i < 64; i++) {
				v = get_word(jpg);
				size -= 2;
				jpg->dqt[j][i] = v >> 8;
			}
		} else {
			// 8 bit DQT
			for(i = 0; i < 64; i++) {
				v = get_byte(jpg);
				size--;
				jpg->dqt[j][i] = v;
			}
		}
	}

	return 0;
}

// define huffman table
int jpg_dht(JPEG *jpg) {
	unsigned tc, th;
	unsigned code = 0;
	unsigned char val;
	int i, j, k, num, len, Li[17];
	HUFF *tbl;

	len = get_word(jpg) - 2;

	while(len > 0) {
		val = get_byte(jpg);

		tc = (val >> 4) & 0x0F;		// Table class(DC/AC)
		th = val & 0x0F;		// Table Header(How index plain it is)

		tbl = (HUFF*)&(jpg->huff[tc][th]);
		num = 0;

		for(i = 1; i <= 16; i++) {
			Li[i] = get_byte(jpg);
			num += Li[i];
		}
		tbl->elem = num;

		// Generate Code
		for(i = 1, k = 0; i <= 16; i++) for(j = 0; j < Li[i]; j++) tbl->size[k++] = i;

		k = 0;
		code = 0;
		i = tbl->size[0];
		while(k < num) {
			while(tbl->size[k] == i) tbl->code[k++] = code++;

			if(k >= num) break;

			do {
				code = code << 1;
				i++;
			} while(tbl->size[k] != i);
		}

		for(k = 0; k < num; k++) tbl->value[k] = get_byte(jpg);

		len = len - 18 - num;
	}

	return 0;
}

// start of scan
void jpg_sos(JPEG *jpg) {
	int i;
	unsigned char c;

	get_word(jpg);

	jpg->scan_cnt = get_byte(jpg);

	for(i = 0; i < jpg->scan_cnt; i++) {
		c = get_byte(jpg);
		// printF(" id :%d\n", c);
		jpg->scan_id[i] = c;

		c = get_byte(jpg);
		jpg->scan_dc[i] = c >> 4;		// DC Huffman Table
		jpg->scan_ac[i] = c & 0x0F;		// AC Huffman Table
	}
	// 3 bytes skip
	get_byte(jpg);
	get_byte(jpg);
	get_byte(jpg);
}

void jpg_idct_init(void);

/*
 *  JPEG 이미지 파일의 전체가 담긴 파일 버퍼와 크기를 이용해서 JPEG 자료구조를 초기화
 *  파일 버퍼의 내용을 분석하여 이미지 전체의 크기와 기타 정보를 JPEG 자료구조에 삽입 
 */
BOOL jpgInit(JPEG *jpg, BYTE *fileBuf, DWORD fileSize) {
	int i;
	unsigned char c;

	jpg_idct_init();

	for(i = 0; i < 3; i++) jpg->mcu_preDC[i] = 0;

	jpg->n_dqt = 0;
	jpg->max_h = 0;
	jpg->max_v = 0;
	jpg->bit_remain = 0;
	jpg->bit_buf = 0;

	// DRI
	jpg->interval = 0;

	// Data 정보 설정
	jpg->data_idx = 0;
	jpg->data_size = fileSize;
	jpg->data = fileBuf;

//	return 0;
//}
//
//int jpg_header(JPEG *jpg) {
//	unsigned char c;

	while(1) {
		if(jpg->data_idx > jpg->data_size) return FALSE;
		c = get_byte(jpg);

		if(jpg->data_idx > jpg->data_size) return FALSE;
		c = get_byte(jpg);

		switch(c) {
			case 0xD8:
				// printF("SOI\n");
				break;
			case 0xD9:
				// printF("EOI\n");
				return FALSE;
				break;
			case 0xC0:
				jpg_sof(jpg);
				break;
			case 0xC4:
				jpg_dht(jpg);
				break;
			case 0xDB:
				jpg_dqt(jpg);
				break;
			case 0xDD:
				jpg_dri(jpg);
				break;
			case 0xDA:
				jpg_sos(jpg);
				return TRUE;
			default:
				jpg_skip(jpg);
				break;
		}
	}

	return FALSE;
}

// MCU Decode
int jpg_dec_init(JPEG *jpg) {
	int i, j;

	for(i = 0; i < jpg->scan_cnt; i++) {
		// i is scan
		for(j = 0; j < jpg->compo_cnt; j++) {
			// j is frame
			if(jpg->scan_id[i] == jpg->compo_id[j]) {
				// printF("scan %d is frame %d\n", i, j);
				jpg->scan_h[i] = jpg->compo_h[j];
				jpg->scan_v[i] = jpg->compo_v[j];
				jpg->scan_qt[i] = jpg->compo_qt[j];
				break;
			}
		}

		if(j >= jpg->compo_cnt) return 1;
	}

	jpg->mcu_width = jpg->max_h * 8;
	jpg->mcu_height = jpg->max_v * 8;

	for(i = 0; i < 32 * 32 * 4; i++) jpg->mcu_buf[i] = 0x80;

	for(i = 0; i < jpg->scan_cnt; i++) jpg->mcu_yuv[i] = jpg->mcu_buf + i * 32 * 32;

	return 0;
}

// Huffman code decoding
int jpg_huff_dec(JPEG *jpg, int tc, int th) {
	HUFF *h = &(jpg->huff[tc][th]);
	int code = 0, size = 0, k = 0, v = 0;

	while(size < 16) {
		size++;
		v = get_bits(jpg, 1);
		if(v < 0) return v;
		code = (code << 1) | v;

		while(h->size[k] == size) {
			if(h->code[k] == code) return h->value[k];
			k++;
		}
	}

	return -1;
}

// reverse DCT
int base_img[64][64];

void jpg_idct_init(void) {
	int u, v, m, n;
	int tmpm[8], tmpn[8], cost[32];

	cost[0] = 32768;
	cost[1] = 32138;
	cost[2] = 30274;
	cost[3] = 27246;
	cost[4] = 23170;
	cost[5] = 18205;
	cost[6] = 12540;
	cost[7] = 6393;
	cost[8] = 0;
	cost[9] = -6393;
	cost[10] = -12540;
	cost[11] = -18205;
	cost[12] = -23170;
	cost[13] = -27246;
	cost[14] = -30274;
	cost[15] = -32138;

	for(u = 0; u < 8; u++) cost[u + 16] = -cost[u];

	for(u = 0; u < 8; u++) {
		int i = u, d = u * 2;
		if(d == 0) i = 4;
		for(m = 0; m < 8; m++) {
			tmpm[m] = cost[i];	// Column cosign(COS) value
			i = (i + d) & 31;
		}
		
		for(v = 0; v < 8; v++) {
			int i = v, d = v * 2;
			if(d == 0) i = 4;
			for(n = 0; n < 8; n++) {
				tmpn[n] = cost[i];
				i = (i + d) & 31;
			}

			for(m = 0; m < 8; m++) for(n = 0; n < 8; n++) base_img[u * 8 + v][m * 8 + n] = (tmpm[m] * tmpn[n]) >> 15;
		}
	}

	return;
}

void jpg_idct(int *block, int *dst) {
	int i, j, k;

	for(i = 0; i < 64; i++) dst[i] = 0;

	for(i = 0; i < 64; i++) {
		k = block[i];
		// k 가 0 인 경우 제외
		if(k) for(j = 0; j < 64; j++) dst[j] += k * base_img[i][j];
	}
	// 고정 소수점을 정수로 변환
	for(i = 0; i < 64; i++) dst[i] >>= 17;

	return;
}

// 디코딩 된 숫자 반환
int jpg_get_value(JPEG *jpg, int size) {
	int val = 0;

	if(size == 0) val = 0;
	else {
		val = get_bits(jpg, size);
		if(!(val & (1 << (size - 1)))) val = val - (1 << size) + 1;
	}

	return val;
}

// 허프만 디코딩 + 역 양자화 + 역 지그재그
int jpg_dec_huff(JPEG *jpg, int scan, int *block) {
	int size, val, run, idx;
	int *pQt = (int *)(jpg->dqt[jpg->scan_qt[scan]]);

	// DC decode
	size = jpg_huff_dec(jpg, 0, jpg->scan_dc[scan]);
	if(size < 0) return 0;
	val = jpg_get_value(jpg, size);
	jpg->mcu_preDC[scan] += val;
	block[0] = jpg->mcu_preDC[scan] * pQt[0];

	// AC decode
	idx = 1;
	while(idx < 64) {
		size = jpg_huff_dec(jpg, 1, jpg->scan_ac[scan]);
		if(size < 0) break;

		// EOB
		if(size == 0) break;

		// RLE
		run = (size >> 4) & 0xF;
		size = size & 0x0F;

		val = jpg_get_value(jpg, size);
		if(val >= 0x10000) return val;	// 마커 발견

		// ZRL
		while(run-- > 0) block[zigzag_tbl[idx++]] = 0;
		block[zigzag_tbl[idx]] = val * pQt[idx];
		idx++;
	}

	while(idx < 64) block[zigzag_tbl[idx++]] = 0;

	return 0;
}

// resampling(복원)
void jpg_mcu_bitblt(int *src, int *dst, int width, int x0, int y0, int x1, int y1) {
	int w, h, x, y, x2, y2;
	w = x1 - x0;
	h = y1 - y0;

	for(y = y0; y < y1; y++) {
		y2 = (y - y0) * 8 / h;

		for(x = x0; x < x1; x++) {
			x2 = (x - x0) * 8 / w;
			dst[(y * width) + x] = src[(y2 * 8) + x2];
		}
	}
}

// one of MCU convert
int jpg_dec_mcu(JPEG *jpg) {
	int scan, val, h, v, *p, hh, vv, block[64], dst[64];

	// mcu_width * mcu_height 크기 블록 변환
	for(scan = 0; scan < jpg->scan_cnt; scan++) {
		hh = jpg->scan_h[scan];
		vv = jpg->scan_v[scan];
		for(v = 0; v < vv; v++) {
			for(h = 0; h < hh; h++) {
				// block (8*8) decode
				val = jpg_dec_huff(jpg, scan, block);

				// reverse DCT
				jpg_idct(block, dst);

				// resampling, writable buffer
				p = jpg->mcu_buf + (scan * 32 * 32);

				// expandable send
				jpg_mcu_bitblt(dst, p, 
								jpg->mcu_width,
								jpg->mcu_width * h / hh,
								jpg->mcu_height * v / vv,
								jpg->mcu_width * (h + 1) / hh,
								jpg->mcu_height * (v + 1) / vv);
			}
		}
	}
	
	return 0;
}

// YCrCb -> RGB convert
int jpg_dec_yuv(JPEG *jpg, int h, int v, COLOR *rgb) {
	int x, y, x0, y0, x1, y1, Y, U, V, k, R, G, B, mw, mh, w;
	int *py, *pu, *pv;

	mw = jpg->mcu_width;
	mh = jpg->mcu_height;
	w = jpg->width;

	x0 = h * jpg->max_h * 8;
	y0 = v * jpg->max_v * 8;

	x1 = jpg->width - x0;
	if(x1 > mw) x1 = mw;
	y1 = jpg->height - y0;
	if(y1 > mh) y1 = mh;

	py = jpg->mcu_buf;
	pu = jpg->mcu_buf + 1024;
	pv = jpg->mcu_buf + 2048;

	for(y = 0; y < y1; y++) {
		for(x = 0; x < x1; x++) {
			k = y * mw + x;

			Y = py[k];
			U = pu[k];
			V = pv[k];

			R = 128 + ((Y * 0x1000 + V * 0x166E) / 4096) * 1300 / 1000;
			R = (R & 0xFFFFFF00) ? (R >> 24) ^ 0xFF : R;

			G = 128 + ((Y * 0x1000 - V * 0x0B6C) / 4096) * 1300 / 1000;
			G = (G & 0xFFFFFF00) ? (G >> 24) ^ 0xFF : G;

			B = 128 + ((Y * 0x1000 - V * 4 + U * 0x1C59) / 4096) * 1300 / 1000;
			B = (B & 0xFFFFFF00) ? (B >> 24) ^ 0xFF : B;

			// RGB888 -> RGB565 convert
			rgb[(y0 + y) * w + (x0 + x)] = RGB(R, G, B);
		}
	}
	
	return 0;
}

// JPEG 자료구조에 저장된 정보를 이용해 디코딩 한 결과 출력 버퍼에 저장
BOOL jpgDecode(JPEG *jpg, COLOR *outBuf) {
	int h_unit, v_unit, mcu_cnt, h, v;

	// MCU 크기 계산
	if(jpg_dec_init(jpg)) return FALSE;		// Error

	h_unit = jpg->width / jpg->mcu_width;
	v_unit = jpg->height / jpg->mcu_height;
	if((jpg->width % jpg->mcu_width) > 0) h_unit++;
	if((jpg->height % jpg->mcu_height) > 0) v_unit++;

	// 1 block convert
	mcu_cnt = 0;
	for(v = 0; v < v_unit; v++) {
		for(h = 0; h < h_unit; h++) {
			mcu_cnt++;
			jpg_dec_mcu(jpg);
			jpg_dec_yuv(jpg, h, v, outBuf);

			if(jpg->interval > 0 && mcu_cnt >= jpg->interval) {
				// RST 마커 제외(FF hoge), hoge 뒤 FF 또한 제외
				jpg->bit_remain -= (jpg->bit_remain & 7);
				jpg->bit_remain -= 8;

				jpg->mcu_preDC[0] = 0;
				jpg->mcu_preDC[1] = 0;
				jpg->mcu_preDC[2] = 0;
				mcu_cnt = 0;
			}
		}
	}

	return TRUE;
}
