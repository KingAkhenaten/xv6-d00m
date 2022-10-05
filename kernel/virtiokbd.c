#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "virtio.h"

#define VIRTIO_MMIO_MAGIC_VALUE_EXPECTED 0x74726976
#define V0(r) ((volatile uint32 *)(VIRTIO0 + (r)))
#define V2(r) ((volatile uint32 *)(VIRTIO2 + (r)))

// virtio structures
// descriptor set
// desc[0] -> outgoing
// desc[1] -> incoming
// This is temporary while I figure out how this is supposed to work
struct virtq_desc *desc;
// available ring: kern -> dev
struct virtq_avail *avail;
// used ring: dev -> kern
struct virtq_used *used;
// last used entry read??
// should be == or < the device's tracking
uint32 used_idx = 0;
// lock for managing hart access to code and ISR await
// struct spinlock gpulock;

struct virtio_input_event input_event;

void init_virtiokbd(void) {
	//initlock(&kbdlock, "kbdlock");
	// determine where kbd is
	probe_mmio();

	if (*V2(VIRTIO_MMIO_MAGIC_VALUE) != VIRTIO_MMIO_MAGIC_VALUE_EXPECTED)
                panic("virtio2 not a virt device");
        if (*V2(VIRTIO_MMIO_VERSION) != 2)
                panic("virtio2 got wrong version");
        if (*V2(VIRTIO_MMIO_DEVICE_ID) != 18)
                panic("virtio2 not a kbd");
	if (*V2(VIRTIO_MMIO_DEVICE_ID == 0))
		panic("Abort Initialization");
	uint32 status = 0;
	*V2(VIRTIO_MMIO_STATUS) = 0;

	// Reset the device
	uint32 status = 0;
	*V2(VIRTIO_MMIO_STATUS) = 0;
	// Set the ACKNOWLEDGE status bit: the guest OS has to notice the device
	status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;
	*V2(VIRTIO_MMIO_STATUS) = status;
	// Set the DRIVER status bit: the guest OS 
	status |= VIRTIO_CONFIG_S_DRIVER;
	*V2(VIRTIO_MMIO_STATUS) = status
	// Read device feature bits, and write the subset of feature bits understood by the OS and driver to
	// 	the device. During this step the driver MAY read (but MUST NOT write) the device-specific
	// 	configuration fields to check that it can support the device before accepting it.
	// 	(No Features)
	uint64 features = *V2(VIRTIO_MMIO_DEVICE_FEATURES);
	*V2(VIRTIO_MMIO_DRIVER_FEATURES) = features & 0;
	// Set the FEATURES_OK status bit. The driver MUST NOT accept new feature bits after this step.
	status |= VIRTIO_CONFIG_S_FEATURES_OK;
	*V2(VIRTIO_MMIO_STATUS) = status;
	// Re-read device status to ensure the FEATURES_OK bit is still set: otherwise, the device does not
	// 	support our subset of features and the device is unusable.
	status = *V2(VIRTIO_MMIO_STATUS);
	if(!(status & VIRTIO_CONFIG_S_FEATURES_OK))
		panic("virtiokbd device features error";)
	// Perform device-specific setup, including discovery of virtqueues for the device, optional
	// 	per-bus setup, reading and possibly writing the device's virtio configuration space,
	// 	and population of virtqueues.
	
	// need 0 eventq and 1 statusq
	*V2(VIRTIO_MMIO_QUEUE_SEL) = 0;
	if (*V2(VIRTIO_MMIO_QUEUE_READY))
		panic("virtiokbd should not be ready yet");
	
	// find max queue supported
	uint32 max = *V1(VIRTIO_MMIO_QUEUE_NUM_MAX);
	if (max == 0)
		panic("virtiokbd has no queue 0")
	if (max < NUM)
		panic("virtiokbd max queue too short");

	// allocate and zero queue memory
	desc = kalloc();
	avail = kalloc();
	used = kalloc();
	if(!avail || !used || !desc)
		panic("virtiokbd kalloc");
	memset(avail, 0, PGSIZE);
	memset(used, 0, PGSIZE);
	memset(desc, 0, PGSIZE);
	
	
	// Set the DRIVER_OK status bit. At this point the device is live
	//
	// If any steps fail. set FAILED status bit.
	// The driver MUST NOT notify the device before setting DRIVER_OK

