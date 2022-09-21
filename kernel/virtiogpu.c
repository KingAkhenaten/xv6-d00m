#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "memlayout.h"
#include "virtio.h"

/*
Self note from the virtio specification:

"Virtual environments without PCI support (a common situation in embedded devices models) might use
simple memory mapped device (“virtio-mmio”) instead of the PCI device.

The memory mapped virtio device behaviour is based on the PCI device specification.
Therefore most operations including device initialization, queues configuration and buffer transfers are nearly identical.
Existing differences are described in the following sections..."

Might help if the MMIO path does not work out and I need to use PCI instead (let's hope that does not happen)
*/

#define VIRTIO_MMIO_MAGIC_VALUE_EXPECTED 0x74726976
#define V0(r) ((volatile uint32 *)(VIRTIO0 + (r)))
#define V1(r) ((volatile uint32 *)(VIRTIO1 + (r)))

// virtio structures
struct virtq_desc *desc;
// available ring: kern -> dev
struct virtq_avail *avail;
// used ring: dev -> kern
struct virtq_used *used;

void probe_mmio(void);

void init_virtiogpu(void) {
	printf("initialising virtiogpu\n");
	// determine where it is plugged in
	probe_mmio();
	// should have been 1
	if (*V1(VIRTIO_MMIO_MAGIC_VALUE) != VIRTIO_MMIO_MAGIC_VALUE_EXPECTED)
		panic("virtio1 not a virt device");
	if (*V1(VIRTIO_MMIO_VERSION) != 2)
		panic("virtio1 got wrong version");
	if (*V1(VIRTIO_MMIO_DEVICE_ID) != 16)
		panic("virtio1 not a GPU");
	// try to init the virtio dance
	uint32 status = 0;
	*V1(VIRTIO_MMIO_STATUS) = 0;
	// set the ack bit
	status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;
	*V1(VIRTIO_MMIO_STATUS) = status;
	// set the driver bit
	status |= VIRTIO_CONFIG_S_DRIVER;
  	*V1(VIRTIO_MMIO_STATUS) = status;
	// feature negotiation
	uint64 features = *V1(VIRTIO_MMIO_DEVICE_FEATURES);
	// gpu does not have any meaningful features
	// we cannot use EDID or virgl
	*V1(VIRTIO_MMIO_DRIVER_FEATURES) = features & 0;
	// end negotiation
	status |= VIRTIO_CONFIG_S_FEATURES_OK;
	*V1(VIRTIO_MMIO_STATUS) = status;
	// did it balk?
	status = *V1(VIRTIO_MMIO_STATUS);
	if(!(status & VIRTIO_CONFIG_S_FEATURES_OK))
		panic("virtiogpu FEATURES_OK balked");

	// set up the queues
	// 5.7.2
	// controlq -> 0: general control commands
	// cursorq -> 1: cursor update "fast track", probably will not be using this
	// from this point on in the init sequence I am cribbing from virtio_disk.c
	// and hoping it works

	// want queue 0
	*V1(VIRTIO_MMIO_QUEUE_SEL) = 0;
	if (*V1(VIRTIO_MMIO_QUEUE_READY))
		panic("virtiogpu should not be ready yet");

	// max queue supported?
	uint32 max = *V1(VIRTIO_MMIO_QUEUE_NUM_MAX);
	if(max == 0)
		panic("virtiogpu has no queue 0");
	if(max < NUM)
		panic("virtiogpu max queue too short (is it really?)");

	// allocate and zero queue memory. ???
	desc = kalloc();
	avail = kalloc();
	used = kalloc();
	if(!avail || !used || !desc)
		panic("virtiogpu kalloc");
	memset(avail, 0, PGSIZE);
	memset(used, 0, PGSIZE);
	memset(desc, 0, PGSIZE);

	// set queue size.
	*V1(VIRTIO_MMIO_QUEUE_NUM) = NUM;

	// write physical addresses.
	*V1(VIRTIO_MMIO_QUEUE_DESC_LOW) = (uint64)desc;
	*V1(VIRTIO_MMIO_QUEUE_DESC_HIGH) = (uint64)desc >> 32;
	*V1(VIRTIO_MMIO_DRIVER_DESC_LOW) = (uint64)avail;
	*V1(VIRTIO_MMIO_DRIVER_DESC_HIGH) = (uint64)avail >> 32;
	*V1(VIRTIO_MMIO_DEVICE_DESC_LOW) = (uint64)used;
	*V1(VIRTIO_MMIO_DEVICE_DESC_HIGH) = (uint64)used >> 32;

	// queue is ready.
	*V1(VIRTIO_MMIO_QUEUE_READY) = 0x1;

	// tell device config done
	status |= VIRTIO_CONFIG_S_DRIVER_OK;
	*V1(VIRTIO_MMIO_STATUS) = status;

	// cross fingers
}

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