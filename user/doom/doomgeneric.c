#include "doomgeneric.h"

uint32_t* DG_ScreenBuffer = 0;

// NOTE! called by i_main.c, which contains the main entry
// After dg_Create returns, DoomMain is called and control transfers away from us

void dg_Create()
{
	DG_ScreenBuffer = malloc(DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4);

	DG_Init();
}

