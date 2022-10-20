#include "types.h"
#include "param.h"
#include "riscv.h"
#include "memlayout.h"
#include "defs.h"
#include "spinlock.h"
#include "proc.h"
#include "virtio.h"

// from virtiokbd.c
extern struct virtio_input_event ring_buffer_advance(void);

uint64 sys_kbdcmd(void){
	struct virtio_input_event input_event = ring_buffer_advance();
	uint64 return_event = 0;
	return_event = return_event | ((uint64) input_event.type) << 48;
	return_event = return_event | ((uint64) input_event.code) << 32;
	return_event = return_event | (uint64) input_event.value;

	return return_event;
}