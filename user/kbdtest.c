#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "kernel/stat.h"
#include "user/user.h"


int main(int argc, char ** argv) {
	struct input_event kbd_input;
	
	while(1){
		kbd_input = poll_kbd();
		printf("%d\t%d\t%d\n", kbd_input.type, kbd_input.code, kbd_input.value);
	}
}
