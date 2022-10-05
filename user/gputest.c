#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char ** argv) {
	printf("Hello World!\n");
	uint64 syscall_result = gpucmd(1);
	printf("syscall_result == %p\n",syscall_result);
	printf("syscall done\n");
	return 0;
}