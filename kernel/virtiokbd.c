#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "virtio.h"
#include "input-event-codes.h"

#define VIRTIO_MMIO_MAGIC_VALUE_EXPECTED 0x74726976
#define V0(r) ((volatile uint32 *)(VIRTIO0 + (r)))
#define V1(r) ((volatile uint32 *)(VIRTIO1 + (r)))
#define V2(r) ((volatile uint32 *)(VIRTIO2 + (r)))

// virtio structures

// EVENTQ - 64 entries, uses KBD_NUM
// descriptor set - holds information mapping descriptors to the buffers address/length/flags virtio expects to read
struct virtq_desc *eventq_desc;
// available ring: kern -> dev - holds descriptor ids we have inserted
struct virtq_avail_kbd *eventq_avail;
// used ring: dev -> kern - holds descriptors kbd has given back
struct virtq_used_kbd *eventq_used;

// STATUSQ - 8 entries, uses NUM like everything else
struct virtq_desc *statusq_desc;
struct virtq_avail_kbd *statusq_avail;
struct virtq_used_kbd *statusq_used;

// last used entry that we have read
// when we take the interrupt we want to keep reading until we catch up with the device
// should be == or < the device's tracking
// seperate counter for every virtqueue
uint32 eventq_used_idx = 0;
uint32 statusq_used_idx = 0;
// lock for managing hart access to code and ISR await from multiple harts
struct spinlock kbdlock;

// holds the array of buffers holding input event data we are waiting to get
// each buffer is mapped to the virtio descriptor with the same index i.e. iea[43] is used for descriptor 43
struct virtio_input_event input_event_array[KBD_NUM];
// marks buffers (and their descriptor bound to the same id) whether they are in use or not: 1 = in use, 0 = not
char used_buffers_eventq[KBD_NUM] = { [0 ... (KBD_NUM-1)] = 0};

// Function Delcarations
//void probe_mmio(void);
void kbd_bind_desc_and_fire_eventq(int);

// Search the input event array for a buffer that isn't in use yet, -1 if all buffers used
int find_available_buffer_eventq(void){
	for (int i = 0; i < KBD_NUM; i++){
		if (used_buffers_eventq[i] == 0)
			return i;
	}
	return -1;
}

void init_virtiokbd(void) {
	initlock(&kbdlock, "kbdlock");
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
	if (eventq_max < KBD_NUM)
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
	*V2(VIRTIO_MMIO_QUEUE_NUM) = KBD_NUM;
	
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

	// query the device for supported event types and codes.
	struct virtio_input_config* kbd_input_conf = (struct virtio_input_config*) V2(VIRTIO_MMIO_DEVICE_CONFIG_SPACE);

	// ID_NAME
	kbd_input_conf->select = VIRTIO_INPUT_CFG_ID_NAME;
	kbd_input_conf->subsel = 0;
	// force cpu to writethrough to virtio mmio address space
	__sync_synchronize();
	printf("VIRTIO_INPUT_CFG_ID_NAME: %s\n", kbd_input_conf->u.string);
	
	// ID_SERIAL
	kbd_input_conf->select = VIRTIO_INPUT_CFG_ID_SERIAL;
        kbd_input_conf->subsel = 0;
	__sync_synchronize();
	printf("VIRTIO_INPUT_CFG_ID_SERIAL: %s\n", kbd_input_conf->u.string);
	
	// ID_DEVIDS
	kbd_input_conf->select = VIRTIO_INPUT_CFG_ID_DEVIDS;
        kbd_input_conf->subsel = 0;
        __sync_synchronize();
        printf("VIRTIO_INPUT_CFG_ID_DEVIDS (bustype): %d\n", kbd_input_conf->u.ids.bustype);
        printf("VIRTIO_INPUT_CFG_ID_DEVIDS (vendor): %d\n", kbd_input_conf->u.ids.vendor);
        printf("VIRTIO_INPUT_CFG_ID_DEVIDS (product): %d\n", kbd_input_conf->u.ids.product);
        printf("VIRTIO_INPUT_CFG_ID_DEVIDS (version): %d\n", kbd_input_conf->u.ids.version);

	// PROP_BITS
	kbd_input_conf->select = VIRTIO_INPUT_CFG_PROP_BITS;
        kbd_input_conf->subsel = 0;
        __sync_synchronize();
        printf("VIRTIO_INPUT_CFG_PROP_BITS: %d\n", kbd_input_conf->u.bitmap[0]);

	// EV_BITS
	kbd_input_conf->select = VIRTIO_INPUT_CFG_EV_BITS;
        kbd_input_conf->subsel = EV_KEY;
        __sync_synchronize();
	if (kbd_input_conf->size > 0) {
		printf("EV_KEY supported\n");
        	printf("VIRTIO_INPUT_CFG_EV_BITS: %s\n", kbd_input_conf->u.bitmap);	
	}
	/*
	// ABS_INFO
	kbd_input_conf->select = VIRTIO_INPUT_CFG_ABS_INFO;
        kbd_input_conf->subsel = 0;
        __sync_synchronize();
        printf("VIRTIO_INPUT_CFG_ABS_INFO: %s\n", kbd_input_conf->u.abs);	
	*/

	// populate eventq with receive buffers
	acquire(&kbdlock);
	for (int desc_idx = 0; desc_idx < KBD_NUM; desc_idx++){
		// look for unused descriptor; this should not possibly fail
		int buffer_idx = find_available_buffer_eventq();
		if (buffer_idx == -1)
			panic("virtiokbd failed to find buffer in init; this should not happen");
		kbd_bind_desc_and_fire_eventq(buffer_idx);
	}
	release(&kbdlock); // stop the interrupt from getting too far ahead of us until we stick all the buffers in
}

void virtiokbd_isr(void) {
	printf("virtiokbd interrupt signalled\n");
	acquire(&kbdlock);
        printf("virtiokbd interrupt got the lock\n");
	// time to figure out what virtio just did
        // ack the interrupt
        *V2(VIRTIO_MMIO_INTERRUPT_ACK) = *V2(VIRTIO_MMIO_INTERRUPT_STATUS) & 0x3;
        __sync_synchronize();

        // device writes to used ring, modifies used->idx to determine
        // it's own placement
        // used_idx = our local copy determining where in the buffer
        // we have actually read vs. what virtiokbd wrote back
        while(eventq_used_idx != eventq_used->idx){
                __sync_synchronize();
                // get descriptor that just finished at eventq_used_idx
                int id = eventq_used->ring[eventq_used_idx % KBD_NUM].id; // grab the descriptor ID out of the used ring
                // handle this descriptor response that the virtiokbd driver will have written into our input buffer at iea[id]
		printf("id=%d type=%d\tcode=%d\tvalue=%d\n", id
						     , input_event_array[id].type
						     , input_event_array[id].code
						     , input_event_array[id].value);
                // go to next index
                eventq_used_idx += 1;
        	// mark the buffer as free
		used_buffers_eventq[id] = 0;
		__sync_synchronize();
		// push more buffers to keep the events rolling since this buffer just became unused
		kbd_bind_desc_and_fire_eventq(id);
	}
	release(&kbdlock);
	printf("virtiokbd released the lock\n");
}

// Set up the descriptor desc_idx, prepare it's associated buffer and fire the request into the eventq
void kbd_bind_desc_and_fire_eventq(int desc_idx) {
	// Zero initialise data that the device will overwrite when it returns in the interrupt
       	input_event_array[desc_idx].type = 0;
       	input_event_array[desc_idx].code = 0;
	input_event_array[desc_idx].value = 0;
	used_buffers_eventq[desc_idx] = 1;
	// set up the descriptor info to bind the buffer
	eventq_desc[desc_idx].addr = (uint64) &input_event_array[desc_idx];
        eventq_desc[desc_idx].len = sizeof(struct virtio_input_event);
        eventq_desc[desc_idx].flags = VRING_DESC_F_WRITE; // device writes
        eventq_desc[desc_idx].next = 0; // no next; no descriptor chaining here

	// ring setup - place descriptor in the eventq virtqueue/ring
        eventq_avail->ring[eventq_avail->idx % KBD_NUM] = desc_idx;
        __sync_synchronize();
        // signal that next entry exists
        eventq_avail->idx += 1;
        __sync_synchronize();
        // finally fire notification
        *V2(VIRTIO_MMIO_QUEUE_NOTIFY) = 0; // value 0 for eventq
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
