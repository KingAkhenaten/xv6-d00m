#include "kernel/types.h"
#include "user/user.h"

extern int doom_main(int argc, char ** argv);
int main(int argc, char ** argv) {
	return doom_main(argc,argv);
}