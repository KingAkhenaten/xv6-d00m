#include "types.h"
#include "riscv.h"
#include "defs.h"

/*
Syscall support for the virtiogpu framebuffer
*/
// from virtiogpu.c
extern void transfer_fb_us(void);
extern void flush_resource_us(void);
extern int acquire_fb(void);
extern void release_fb(void);
extern int holds_fb(void);
extern uint32 * framebuffer;

uint64 sys_gpucmd(void) {
	int callno = 0; // call number userspace gave us
	argint(0,&callno); // read into call number
	switch (callno) {
		case 0:
			// Call 0 - transfer and flush framebuffer
			transfer_fb_us();
			flush_resource_us();
			return 0;
		case 1:
			// Call 1 - acquire exclusive access and map framebuffer into user memory, returns uint32 * or NULL
			{
				int acquire = acquire_fb();
				if (acquire == 0) return 0;
				return (uint64) &framebuffer;
			}
		case 2:
			// Call 2 - release exclusive access and unmap framebuffer from memory, returns 0
			release_fb();
			return (uint64) 0;
		case 3:
			// Call 3 - test if current process owns the framebuffer, returns 0 or 1
			return (uint64) holds_fb();
	}
	return ~0ULL;
}