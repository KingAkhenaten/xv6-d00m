#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "virtio.h"

#define GPU(r) ((volatile uint32 *)(VIRTIO0 + (r)))

void init_virtiogpu(void) {
	printf("initialising virtiogpu\n");
}