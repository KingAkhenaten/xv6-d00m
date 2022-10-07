#include "types.h"
#include "param.h"
#include "riscv.h"
#include "memlayout.h"
#include "defs.h"
#include "spinlock.h"
#include "proc.h"

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
				// we have the framebuffer, now make the PTE
				struct proc * this_proc = myproc();
				// Hope this works!
				printf("FB kernva %p userva %p", &framebuffer, FRAMEBUFFER);
				// The complement of mappages is.... uvmunmap. There is no unmappages, nor is there a uvmmap.
				// The two functions also have different requirements for alignment and use different size units...
				// edit: apparently this oddity is also used in proc.c so there's precedent here. Leaving it as is.
				int success = mappages(this_proc->pagetable,FRAMEBUFFER,64*PGSIZE,(uint64) &framebuffer,PTE_R | PTE_W | PTE_U);
				if (success == -1) { // This returns zero on success!
					printf("Mapping failed\n");
					release_fb();
					return 0;
				}
				return (uint64) FRAMEBUFFER; // This is the *userspace* pointer to the kernelspace framebuffer
			}
		case 2:
			// Call 2 - release exclusive access and unmap framebuffer from memory, returns 0
			{
				struct proc * this_proc = myproc();
				uvmunmap(this_proc->pagetable,FRAMEBUFFER,64,0);
				printf("Mapping unmapped\n");
				release_fb();
				return (uint64) 0;
			}
		case 3:
			// Call 3 - test if current process owns the framebuffer, returns 0 or 1
			return (uint64) holds_fb();
	}
	return ~0ULL;
}