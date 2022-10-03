#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char ** argv) {
	printf("Hello World!\n");
	uint64 syscall_result = gpucmd(0);
	if (syscall_result == 42) {
		printf("syscall_result == 42\n");
	}
	printf("syscall done\n");
	return 0;
}