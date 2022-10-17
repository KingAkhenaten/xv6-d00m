#include "doomgeneric.h"
#include "xv6.h"

uint32_t* DG_ScreenBuffer = 0;

// NOTE! called by i_main.c, which contains the main entry
// After dg_Create returns, DoomMain is called and control transfers away from us
// Don't edit this
void dg_Create()
{
	DG_ScreenBuffer = malloc(DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4);

	DG_Init();
}

static uint32 * xv6fb;
// BEGIN XV6 IMPL
void DG_Init() {
	// Acquire framebuffer
	xv6fb = acquire_fb();
	if (xv6fb == NULL) {
		printf("cannot acquire xv6 framebuffer\n");
		exit(1);
	}
	// nothing else to do
}

void DG_DrawFrame() {
	memmove(xv6fb,DG_ScreenBuffer, DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4);
	transfer_fb();
}

void DG_SleepMs(uint32_t ms) {
	sleep(ms);
}

uint32_t DG_GetTicksMs() {
	// This is actually the number of clock ticks; each clock tick is 100ms
	return uptime() * 100;
}

int DG_GetKey(int* pressed, unsigned char* key) {
	// Nothing.
	// Return 1 if there is a keyboard event. *pressed = 1 if key is down
	// *key = the key id pressed
	// Return 0 otherwise
	return 0;
}

void DG_SetWindowTitle(const char * title) {} // nop
