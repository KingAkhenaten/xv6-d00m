#include "types.h"
#include "riscv.h"
#include "defs.h"
/*
Syscall support for the virtiogpu framebuffer
*/

uint64 sys_gpucmd(void) {
	int callno = 0; // call number userspace gave us
	argint(0,&callno);
	printf("Hello from kernel\n");
	return 42;
}