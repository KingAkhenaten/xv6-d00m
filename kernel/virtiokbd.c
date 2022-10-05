#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "virtio.h"

#define VIRTIO_MMIO_MAGIC_VALUE_EXPECTED 0x74726976
#define V0(r) ((volatile uint32 *)(VIRTIO0 + (r)))
#define V1(r) ((volatile uint32 *)(VIRTIO0 + (r)))
#define V2(r) ((volatile uint32 *)(VIRTIO2 + (r)))

// virtio structures
// descriptor set
// desc[0] -> outgoing
// desc[1] -> incoming
// This is temporary while I figure out how this is supposed to work
struct virtq_desc *eventq_desc;
// available ring: kern -> dev
struct virtq_avail *eventq_avail;
// used ring: dev -> kern
struct virtq_used *eventq_used;


struct virtq_desc *statusq_desc;
struct virtq_avail *statusq_avail;
struct virtq_used *statusq_used;

// last used entry read??
// should be == or < the device's tracking
uint32 eventq_used_idx = 0;
uint32 statusq_used_idx = 0;
// lock for managing hart access to code and ISR await
// struct spinlock gpulock;

struct virtio_input_event input_event;

// Function Delcarations
//void probe_mmio(void);

void init_virtiokbd(void) {
	//initlock(&kbdlock, "kbdlock");
	// determine where kbd is
	//probe_mmio();

	if (*V2(VIRTIO_MMIO_MAGIC_VALUE) != VIRTIO_MMIO_MAGIC_VALUE_EXPECTED)
                panic("virtio2 not a virt device");
        if (*V2(VIRTIO_MMIO_VERSION) != 2)
                panic("virtio2 got wrong version");
        if (*V2(VIRTIO_MMIO_DEVICE_ID) != 18)
                panic("virtio2 not a kbd");
	if (*V2(VIRTIO_MMIO_DEVICE_ID) == 0)
		panic("Abort Initialization");

	// Reset the device
	uint32 status = 0;
	*V2(VIRTIO_MMIO_STATUS) = 0;
	// Set the ACKNOWLEDGE status bit: the guest OS has to notice the device
	status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;
	*V2(VIRTIO_MMIO_STATUS) = status;
	// Set the DRIVER status bit: the guest OS 
	status |= VIRTIO_CONFIG_S_DRIVER;
	*V2(VIRTIO_MMIO_STATUS) = status;
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
		panic("virtiokbd device features error");
	// Perform device-specific setup, including discovery of virtqueues for the device, optional
	// 	per-bus setup, reading and possibly writing the device's virtio configuration space,
	// 	and population of virtqueues.
	
	// need 0 eventq and 1 statusq
	*V2(VIRTIO_MMIO_QUEUE_SEL) = 0;
	if (*V2(VIRTIO_MMIO_QUEUE_READY))
		panic("virtiokbd should not be ready yet");
	
	// find max queue supported
	uint32 eventq_max = *V2(VIRTIO_MMIO_QUEUE_NUM_MAX);
	if (eventq_max == 0)
		panic("virtiokbd has no queue 0");
	if (eventq_max < NUM)
		panic("virtiokbd max queue too short");

	// allocate and zero queue memory
	eventq_desc = kalloc();
	eventq_avail = kalloc();
	eventq_used = kalloc();
	if(!eventq_avail || !eventq_used || !eventq_desc)
		panic("virtiokbd kalloc");
	memset(eventq_avail, 0, PGSIZE);
	memset(eventq_used, 0, PGSIZE);
	memset(eventq_desc, 0, PGSIZE);
	
	// Notify the device about the queue size
	*V2(VIRTIO_MMIO_QUEUE_NUM) = NUM;
	
	// Write physical addresses to pointers
	*V2(VIRTIO_MMIO_QUEUE_DESC_LOW) = (uint64)eventq_desc;
        *V2(VIRTIO_MMIO_QUEUE_DESC_HIGH) = (uint64)eventq_desc >> 32;
        *V2(VIRTIO_MMIO_DRIVER_DESC_LOW) = (uint64)eventq_avail;
        *V2(VIRTIO_MMIO_DRIVER_DESC_HIGH) = (uint64)eventq_avail >> 32;
        *V2(VIRTIO_MMIO_DEVICE_DESC_LOW) = (uint64)eventq_used;
        *V2(VIRTIO_MMIO_DEVICE_DESC_HIGH) = (uint64)eventq_used >> 32;	
	
	*V2(VIRTIO_MMIO_QUEUE_READY) = 0x1;	
	
	// Now do the same for statusq
	*V2(VIRTIO_MMIO_QUEUE_SEL) = 1;
	
	if (*V2(VIRTIO_MMIO_QUEUE_READY))
		panic("virtiokbd should not be ready yet");
	
	uint32 statusq_max = *V2(VIRTIO_MMIO_QUEUE_NUM_MAX);
	if (statusq_max == 0)
		panic("virtiokbd has no queue 0");
	if (statusq_max < NUM)
		panic("virtiokbd max queue too short");

	// allocate and zero queue memory
	statusq_desc = kalloc();
	statusq_avail = kalloc();
	statusq_used = kalloc();

	if(!statusq_avail || !statusq_used || !statusq_desc)
		panic("virtiokbd kalloc");
	memset(statusq_avail, 0, PGSIZE);
	memset(statusq_used, 0, PGSIZE);
	memset(statusq_desc, 0, PGSIZE);
	
	// Notify the device about the queue size
	*V2(VIRTIO_MMIO_QUEUE_NUM) = NUM;
	
	// Write physical addresses to pointers
	*V2(VIRTIO_MMIO_QUEUE_DESC_LOW) = (uint64)statusq_desc;
        *V2(VIRTIO_MMIO_QUEUE_DESC_HIGH) = (uint64)statusq_desc >> 32;
        *V2(VIRTIO_MMIO_DRIVER_DESC_LOW) = (uint64)statusq_avail;
        *V2(VIRTIO_MMIO_DRIVER_DESC_HIGH) = (uint64)statusq_avail >> 32;
        *V2(VIRTIO_MMIO_DEVICE_DESC_LOW) = (uint64)statusq_used;
        *V2(VIRTIO_MMIO_DEVICE_DESC_HIGH) = (uint64)statusq_used >> 32;	

	*V2(VIRTIO_MMIO_QUEUE_READY) = 0x1;	
	// Set the DRIVER_OK status bit. At this point the device is live
	//
	// If any steps fail. set FAILED status bit.
	// The driver MUST NOT notify the device before setting DRIVER_OK
	status |= VIRTIO_CONFIG_S_DRIVER_OK;
	*V2(VIRTIO_MMIO_STATUS) = status;

	printf("virtio kbd status: %d\n", *V2(VIRTIO_MMIO_STATUS));
}
/*
// Probe the MMIO ports we expect and print what is there
void probe_mmio(void) {
        printf("probing virtio0: ");
        if (*V0(VIRTIO_MMIO_MAGIC_VALUE) == VIRTIO_MMIO_MAGIC_VALUE_EXPECTED) {
                printf("virtio ");
                uint32 deviceId = *V0(VIRTIO_MMIO_DEVICE_ID);
                if (deviceId == 0) {
                        printf("<not present>");
                } else if (deviceId == 16) {
                        printf("GPU");
                } else if (deviceId == 2) {
                        printf("blockdev");
                } else {
                        printf("deviceid %d",deviceId);
                }
                printf("\n");
        }

        printf("probing virtio1: ");
        if (*V1(VIRTIO_MMIO_MAGIC_VALUE) == VIRTIO_MMIO_MAGIC_VALUE_EXPECTED) {
                printf("virtio ");
                uint32 deviceId = *V1(VIRTIO_MMIO_DEVICE_ID);
                if (deviceId == 0) {
                        printf("<not present>");
                } else if (deviceId == 16) {
                        printf("GPU");
                } else if (deviceId == 2) {
                        printf("blockdev");
                } else {
                        printf("deviceid %d",deviceId);
                }
                printf("\n");
        }
}
*/
