#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char ** argv) {
	printf("Hello World!\n");
	printf("Acquiring framebuffer...\n");
	uint32 * fb = acquire_fb();
	if (fb == 0) {
		printf("Could not get framebuffer\n");
		return 0;
	}
	printf("Writing every pixel...\n");
	for (uint32 y = 0; y < FB_HEIGHT; y++) {
		for (uint32 x = 0; x < FB_WIDTH; x++) {
			uint32 mx = x & 0xFF;
			uint32 my = y & 0xFF;
			uint32 shade = mx ^ my;
			fb[y * FB_WIDTH + x] = 0xFF | (shade << 8) | (shade << 16) | (shade << 24); // BGRA
		}
	}
	printf("Done writing. Commence transfer.\n");
	transfer_fb();
	printf("Done transferring. Releasing the framebuffer.\n");
	release_fb();
	printf("Exiting. This should now trap.\n");
	fb[0] = 0x00F;
	return 0;
}