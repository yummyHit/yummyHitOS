/*
 * vbe.c
 *
 *  Created on: 2017. 8. 29.
 *      Author: Yummy
 */

#include <vbe.h>

static VBEMODEINFO *gs_vbeModeInfo = (VBEMODEINFO*)VBE_MODEINFO_BLOCKADDR;

VBEMODEINFO *kGetVBEModeInfo(void) {
	return gs_vbeModeInfo;
}
