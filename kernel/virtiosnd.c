#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "virtio.h"

#define VIRTIO_MMIO_MAGIC_VALUE_EXPECTED 0x74726976
#define V0(r) ((volatile uint32 *)(VIRTIO0 + (r)))
#define V1(r) ((volatile uint32 *)(VIRTIO1 + (r)))
#define V2(r) ((volatile uint32 *)(VIRTIO2 + (r)))
#define V3(r) ((volatile uint32 *)(VIRTIO3 + (r)))

void init_virtiosnd(void) {
    printf("initializing virtio sound\n");
}

void virtiosnd_isr(void) {
    printf("virtiosnd interrupt signalled\n");
}
