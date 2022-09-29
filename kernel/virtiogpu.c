#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
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

#define VIRTIO_MMIO_MAGIC_VALUE_EXPECTED 0x74726976 // 'virt' in ASCII
#define V0(r) ((volatile uint32 *)(VIRTIO0 + (r))) // Access to VIRTIO0 registers starting at 0x10001000 (only used for probe)
#define V1(r) ((volatile uint32 *)(VIRTIO1 + (r))) // Access to VIRTIO1 registers starting at 0x10002000 (we use this)

// virtio structures
// The descriptor set contains descriptors which describe information about the buffers we expose to the device
// i.e. addresses, lengths, read/write status, associations with other buffers for a command
// desc[0] -> I reserve for outgoing data (varies based on command)
// desc[1] -> I reserve for incoming data (just references response field)
struct virtq_desc *desc;
// available ring: kern -> dev
// where we push buffers so the device can read them off
struct virtq_avail *avail;
// used ring: dev -> kern
// where device pushes buffers we are intended to read
struct virtq_used *used;
// last used entry we have read, < or == to last index of buffer inserted by device
// should be == or < the device's tracking
uint32 used_idx = 0;
// lock for managing hart access to code and ISR await
struct spinlock gpulock;
// this is it- the magic framebuffer
// to clarify, this is our local copy that we upload to the host
#define WIDTH 320
#define HEIGHT 200
uint32 framebuffer[WIDTH * HEIGHT];

// structs used for requests
// these three are ceremonial stuff run once for making the framebuffer on the hypervisor, binding it to memory
// here, then setting up the hypervisor's screen to read our framebuffer
struct virtio_gpu_resource_create_2d createreq;
struct virtio_gpu_resource_attach_backing_singular attachreq;
struct virtio_gpu_set_scanout scanoutreq;
// these two used to upload our local copy to the framebuffer, then make it displayable (as far as I know)
struct virtio_gpu_transfer_to_host_2d transreq;
struct virtio_gpu_resource_flush flushreq;
// int used for response (the buffer the device writes back to with status)
uint32 response;
// is request in flight? 1 if so, 0 otherwise
uint32 request_inflight = 0;

// function declarations
void probe_mmio(void);
void create_device_fb(void);
void attach_fb(void);
void config_scanout(void);
void transfer_fb(void);
void flush_resource(void);

// Initialise the virtiogpu device fully, including device handshaking and any
// virtio commands that need to be sent to make it ready for *us* before we leave
// the initialisation phase of xv6
void init_virtiogpu(void) {
	initlock(&gpulock,"gpulock");
	printf("initialising virtiogpu\n");
	// determine where it is plugged in
	probe_mmio();
	// we should have been VIRTIO1
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
	// gpu does not have any meaningful features for us
	// we cannot use EDID or virgl, so clear those bits
	*V1(VIRTIO_MMIO_DRIVER_FEATURES) = features & 0;
	// end negotiation by writing the OK bit
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

	// we exclusively want queue 0
	*V1(VIRTIO_MMIO_QUEUE_SEL) = 0;
	// the queue should not enter the ready state now, if so something is wrong here
	if (*V1(VIRTIO_MMIO_QUEUE_READY))
		panic("virtiogpu should not be ready yet");

	// Probe the maximum queue size supported by the device.
	// This does not *really* matter as we're only firing one request at a time anyway.
	// But since I cribbed this from the disk driver it asks for 8. I'll come back and change this later.
	uint32 max = *V1(VIRTIO_MMIO_QUEUE_NUM_MAX);
	if(max == 0)
		panic("virtiogpu has no queue 0");
	if(max < NUM)
		panic("virtiogpu max queue too short (is it really?)");

	// allocate and zero queue memory for the three queues.
	desc = kalloc();
	avail = kalloc();
	used = kalloc();
	if(!avail || !used || !desc)
		panic("virtiogpu kalloc");
	memset(avail, 0, PGSIZE);
	memset(used, 0, PGSIZE);
	memset(desc, 0, PGSIZE);

	// set queue size we declare to the device to what we expected
	*V1(VIRTIO_MMIO_QUEUE_NUM) = NUM;

	// write physical addresses so the device knows where to find us
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

	printf("virtio gpu status: %d\n",*V1(VIRTIO_MMIO_STATUS));
	// continue initialisation
	create_device_fb();
	attach_fb();
	config_scanout();
	transfer_fb();
	flush_resource();
}

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

// ISR for virtiogpu interrupts. It's expected the ISR will only be called while an operation has yet to return
// The operation should be spinning at this time waiting for the ISR to finish
void virtiogpu_isr(void) {
	printf("virtiogpu interrupt signalled\n");
	acquire(&gpulock);
	printf("virtiogpu interrupt got the lock\n");
	// time to figure out what virtio just did
	// ack the interrupt
	*V1(VIRTIO_MMIO_INTERRUPT_ACK) = *V1(VIRTIO_MMIO_INTERRUPT_STATUS) & 0x3;
	__sync_synchronize();

	// device writes to used ring, modifies used->idx to determine
	// it's own placement
	// used_idx = our local copy determining where in the buffer
	// we have actually read vs. what virtiogpu wrote back
	// note: this loop likely should not execute more than once
	while(used_idx != used->idx){
		__sync_synchronize();
		// descriptor that just finished - should be 0 since that is the only descriptor used
		int id = used->ring[used_idx % NUM].id; // grab the descriptor ID out of the used ring
		if (id != 0)
			panic("virtiogpu isr did not get 0");
		// handle this descriptor response that the virtiogpu driver will have written into 'response'
		// all responses have no payload, only the status code
		// if it is anything other than OK_NODATA something is wrong
		if (response != VIRTIO_GPU_RESP_OK_NODATA) {
			printf("%d response\n",response);
			panic("did not get response OK_NO_DATA");
		}
		// go to next index
		used_idx += 1;
	}
	// unblock spinning threads
	request_inflight = 0;
	__sync_synchronize();
	release(&gpulock);
}

// Create the framebuffer on the hypervisor side
void create_device_fb(void) {
	// hold lock for requesting
	acquire(&gpulock);
	request_inflight = 1;
	// fill framebuffer with red to make debugging easier
	for (uint32 i = 0; i < WIDTH * HEIGHT; i++) {
		uint32 y = i / WIDTH;
		uint32 x = i % WIDTH;
		framebuffer[i] = 0xFF000000 | (y & 0xFF) << 16 | (x & 0xFF) << 8;
	}
	// create the request struct-or at least write it
	struct virtio_gpu_resource_create_2d * req = &createreq;
	req->hdr.type = VIRTIO_GPU_CMD_RESOURCE_CREATE_2D;
	req->format = VIRTIO_GPU_FORMAT_A8R8G8B8_UNORM;
	req->width = 320;
	req->height = 200;
	req->resource_id = 666; // should not matter what is here theoretically as long as it is consistent
	// set up the descriptors
	desc[0].addr = (uint64) req;
	desc[0].len = sizeof(struct virtio_gpu_resource_create_2d);
	desc[0].next = 1; // next is desc[1]
	desc[0].flags = VRING_DESC_F_NEXT; // device reads, has next

	// I want the device to write into this one int the type
	// none of the ops I use should have payloads so this theoretically should work
	response = 42; // magic value
	desc[1].addr = (uint64) &response;
	desc[1].len = 4;
	desc[1].flags = VRING_DESC_F_WRITE; // device writes
	desc[1].next = 0; // no next
	// ring setup
	// tell device we intend to use descriptor 0
	avail->ring[avail->idx % NUM] = 0;
	__sync_synchronize();
	// signal that next entry exists
	avail->idx += 1;
  	__sync_synchronize();
	// finally fire notification
	*V1(VIRTIO_MMIO_QUEUE_NOTIFY) = 0; // value 0 for controlq

	printf("sent create_device_fb request\n");
	release(&gpulock);
	// Turn on interrupts temporarily and spin until ISR finishes
	intr_on();
	while (request_inflight == 1) {
		__sync_synchronize(); // hacky but it works
	}
	// ...and turn them back off
	intr_off();
	printf("create_device_fb ends\n");
}

// Attach our framebuffer memory to the hypervisor's framebuffer
void attach_fb(void) {
	// hold lock for requesting
	acquire(&gpulock);
	request_inflight = 1;
	// create the request struct
	struct virtio_gpu_resource_attach_backing_singular * req = &attachreq;
	req->req.hdr.type = VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING;
	req->req.resource_id = 666; // should not matter what is here theoretically as long as it is consistent
	req->req.nr_entries = 1; // ALWAYS 1. Never anything else.
	req->entry.addr = (uint64) &framebuffer;
	req->entry.length = WIDTH * HEIGHT * 4;
	req->entry.padding = 0;
	// set up the descriptors
	desc[0].addr = (uint64) req;
	desc[0].len = sizeof(struct virtio_gpu_resource_attach_backing_singular);
	desc[0].next = 1; // next is desc[1]
	desc[0].flags = VRING_DESC_F_NEXT; // device reads, has next

	// I want the device to write into this one int the type
	// none of the ops I use should have payloads so this theoretically should work
	response = 42; // magic value
	desc[1].addr = (uint64) &response;
	desc[1].len = 4;
	desc[1].flags = VRING_DESC_F_WRITE; // device writes
	desc[1].next = 0; // no next
	// ring setup
	// tell device we intend to use descriptor 0
	avail->ring[avail->idx % NUM] = 0;
	__sync_synchronize();
	// signal that next entry exists
	avail->idx += 1;
  	__sync_synchronize();
	// finally fire notification
	*V1(VIRTIO_MMIO_QUEUE_NOTIFY) = 0; // value 0 for controlq

	printf("sent attach_fb request\n");
	release(&gpulock);
	// Turn on interrupts temporarily and spin until ISR finishes
	intr_on();
	while (request_inflight == 1) {
		__sync_synchronize(); // hacky but it works
	}
	// ...and turn them back off
	intr_off();
	printf("attach_fb ends\n");
}

// Set up the screen to use our framebuffer
void config_scanout(void) {
	// hold lock for requesting
	acquire(&gpulock);
	request_inflight = 1;
	// create the request struct
	struct virtio_gpu_set_scanout * req = &scanoutreq;
	req->hdr.type = VIRTIO_GPU_CMD_SET_SCANOUT;
	req->scanout_id = 0; // 0 should be the only screen
	req->resource_id = 666; // should not matter what is here theoretically as long as it is consistent
	req->r.x = 0;
	req->r.y = 0;
	req->r.height = HEIGHT;
	req->r.width = WIDTH;
	// set up the descriptors
	desc[0].addr = (uint64) req;
	desc[0].len = sizeof(struct virtio_gpu_set_scanout);
	desc[0].next = 1; // next is desc[1]
	desc[0].flags = VRING_DESC_F_NEXT; // device reads, has next

	// I want the device to write into this one int the type
	// none of the ops I use should have payloads so this theoretically should work
	response = 42; // magic value
	desc[1].addr = (uint64) &response;
	desc[1].len = 4;
	desc[1].flags = VRING_DESC_F_WRITE; // device writes
	desc[1].next = 0; // no next
	// ring setup
	// tell device we intend to use descriptor 0
	avail->ring[avail->idx % NUM] = 0;
	__sync_synchronize();
	// signal that next entry exists
	avail->idx += 1;
  	__sync_synchronize();
	// finally fire notification
	*V1(VIRTIO_MMIO_QUEUE_NOTIFY) = 0; // value 0 for controlq

	printf("sent config_scanout request\n");
	release(&gpulock);
	// Turn on interrupts temporarily and spin until ISR finishes
	intr_on();
	while (request_inflight == 1) {
		__sync_synchronize(); // hacky but it works
	}
	// ...and turn them back off
	intr_off();
	printf("config_scanout ends\n");
}

// Transfer framebuffer to the hypervisor's framebuffer
void transfer_fb(void) {
	// hold lock for requesting
	acquire(&gpulock);
	request_inflight = 1;
	// create the request struct
	struct virtio_gpu_transfer_to_host_2d * req = &transreq;
	req->hdr.type = VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D;
	req->resource_id = 666; // should not matter what is here theoretically as long as it is consistent
	req->r.x = 0;
	req->r.y = 0;
	req->r.height = HEIGHT;
	req->r.width = WIDTH;
	req->offset = 0; // whole fb transfer so no meaningful offset
	req->padding = 0; // just to be safe
	// set up the descriptors
	desc[0].addr = (uint64) req;
	desc[0].len = sizeof(struct virtio_gpu_transfer_to_host_2d);
	desc[0].next = 1; // next is desc[1]
	desc[0].flags = VRING_DESC_F_NEXT; // device reads, has next

	// I want the device to write into this one int the type
	// none of the ops I use should have payloads so this theoretically should work
	response = 42; // magic value
	desc[1].addr = (uint64) &response;
	desc[1].len = 4;
	desc[1].flags = VRING_DESC_F_WRITE; // device writes
	desc[1].next = 0; // no next
	// ring setup
	// tell device we intend to use descriptor 0
	avail->ring[avail->idx % NUM] = 0;
	__sync_synchronize();
	// signal that next entry exists
	avail->idx += 1;
  	__sync_synchronize();
	// finally fire notification
	*V1(VIRTIO_MMIO_QUEUE_NOTIFY) = 0; // value 0 for controlq

	printf("sent transfer_fb request\n");
	release(&gpulock);
	// Turn on interrupts temporarily and spin until ISR finishes
	intr_on();
	while (request_inflight == 1) {
		__sync_synchronize(); // hacky but it works
	}
	// ...and turn them back off
	intr_off();
	printf("transfer_fb ends\n");
}

// Flush the screen so the framebuffer is drawn
// Partial flushing of selected areas is possible, but this is not used
void flush_resource(void) {
	// hold lock for requesting
	acquire(&gpulock);
	request_inflight = 1;
	// create the request struct
	struct virtio_gpu_resource_flush * req = &flushreq;
	req->hdr.type = VIRTIO_GPU_CMD_RESOURCE_FLUSH;
	req->resource_id = 666; // should not matter what is here theoretically as long as it is consistent
	req->r.x = 0;
	req->r.y = 0;
	req->r.height = HEIGHT;
	req->r.width = WIDTH;
	req->padding = 0; // again, to be safe
	// set up the descriptors
	desc[0].addr = (uint64) req;
	desc[0].len = sizeof(struct virtio_gpu_resource_flush);
	desc[0].next = 1; // next is desc[1]
	desc[0].flags = VRING_DESC_F_NEXT; // device reads, has next

	// I want the device to write into this one int the type
	// none of the ops I use should have payloads so this theoretically should work
	response = 42; // magic value
	desc[1].addr = (uint64) &response;
	desc[1].len = 4;
	desc[1].flags = VRING_DESC_F_WRITE; // device writes
	desc[1].next = 0; // no next
	// ring setup
	// tell device we intend to use descriptor 0
	avail->ring[avail->idx % NUM] = 0;
	__sync_synchronize();
	// signal that next entry exists
	avail->idx += 1;
  	__sync_synchronize();
	// finally fire notification
	*V1(VIRTIO_MMIO_QUEUE_NOTIFY) = 0; // value 0 for controlq

	printf("sent resource_flush request\n");
	release(&gpulock);
	// Turn on interrupts temporarily and spin until ISR finishes
	intr_on();
	while (request_inflight == 1) {
		__sync_synchronize(); // hacky but it works
	}
	// ...and turn them back off
	intr_off();
	printf("resource_flush ends\n");
}