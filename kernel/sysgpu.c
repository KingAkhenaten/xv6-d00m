#include "types.h"
#include "riscv.h"
#include "defs.h"

/*
Syscall support for the virtiogpu framebuffer
*/
// from virtiogpu.c
void transfer_fb_us(void);
void flush_resource_us(void);

uint64 sys_gpucmd(void) {
	int callno = 0; // call number userspace gave us
	argint(0,&callno); // read into call number
	switch (callno) {
		case 0:
			// Call 0 - transfer and flush framebuffer
			transfer_fb_us();
			flush_resource_us();
			return 0;
	}
	return ~0ULL;
}