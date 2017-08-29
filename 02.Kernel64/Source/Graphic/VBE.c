/*
 * VBE.c
 *
 *  Created on: 2017. 8. 29.
 *      Author: Yummy
 */

#include <VBE.h>

static VBEMODEINFO *gs_vbeModeInfo = (VBEMODEINFO*)VBE_MODEINFO_BLOCKADDR;

inline VBEMODEINFO *getVBEModeInfo(void) {
	return gs_vbeModeInfo;
}
