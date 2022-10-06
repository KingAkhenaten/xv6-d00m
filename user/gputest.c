#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char ** argv) {
	printf("Hello World!\n");
	uint32 * syscall_result = acquire_fb();
	printf("syscall_result == %p\n",syscall_result);
	printf("syscall done\n");
	return 0;
}