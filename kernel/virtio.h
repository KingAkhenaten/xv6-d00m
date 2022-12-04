//
// virtio device definitions.
// for both the mmio interface, and virtio descriptors.
// only tested with qemu.
//
// the virtio spec:
// https://docs.oasis-open.org/virtio/virtio/v1.1/virtio-v1.1.pdf
//

// virtio mmio control registers, mapped starting at 0x10001000.
// from qemu virtio_mmio.h
#define VIRTIO_MMIO_MAGIC_VALUE		0x000 // 0x74726976
#define VIRTIO_MMIO_VERSION		0x004 // version; should be 2
#define VIRTIO_MMIO_DEVICE_ID		0x008 // device type; 1 is net, 2 is disk
#define VIRTIO_MMIO_VENDOR_ID		0x00c // 0x554d4551
#define VIRTIO_MMIO_DEVICE_FEATURES	0x010
#define VIRTIO_MMIO_DRIVER_FEATURES	0x020
#define VIRTIO_MMIO_QUEUE_SEL		0x030 // select queue, write-only
#define VIRTIO_MMIO_QUEUE_NUM_MAX	0x034 // max size of current queue, read-only
#define VIRTIO_MMIO_QUEUE_NUM		0x038 // size of current queue, write-only
#define VIRTIO_MMIO_QUEUE_READY		0x044 // ready bit
#define VIRTIO_MMIO_QUEUE_NOTIFY	0x050 // write-only
#define VIRTIO_MMIO_INTERRUPT_STATUS	0x060 // read-only
#define VIRTIO_MMIO_INTERRUPT_ACK	0x064 // write-only
#define VIRTIO_MMIO_STATUS		0x070 // read/write
#define VIRTIO_MMIO_QUEUE_DESC_LOW	0x080 // physical address for descriptor table, write-only
#define VIRTIO_MMIO_QUEUE_DESC_HIGH	0x084
#define VIRTIO_MMIO_DRIVER_DESC_LOW	0x090 // physical address for available ring, write-only
#define VIRTIO_MMIO_DRIVER_DESC_HIGH	0x094
#define VIRTIO_MMIO_DEVICE_DESC_LOW	0x0a0 // physical address for used ring, write-only
#define VIRTIO_MMIO_DEVICE_DESC_HIGH	0x0a4
#define VIRTIO_MMIO_DEVICE_CONFIG_SPACE 0x100 // Device-specific configuration space starts at the 
					      // offset 0x100 and is accessed with byte alignment. 
					      // Its meaning and size depend on the device and the driver.

// status register bits, from qemu virtio_config.h
#define VIRTIO_CONFIG_S_ACKNOWLEDGE	1
#define VIRTIO_CONFIG_S_DRIVER		2
#define VIRTIO_CONFIG_S_DRIVER_OK	4
#define VIRTIO_CONFIG_S_FEATURES_OK	8

// device feature bits
#define VIRTIO_BLK_F_RO              5	/* Disk is read-only */
#define VIRTIO_BLK_F_SCSI            7	/* Supports scsi command passthru */
#define VIRTIO_BLK_F_CONFIG_WCE     11	/* Writeback mode available in config */
#define VIRTIO_BLK_F_MQ             12	/* support more than one vq */
#define VIRTIO_F_ANY_LAYOUT         27
#define VIRTIO_RING_F_INDIRECT_DESC 28
#define VIRTIO_RING_F_EVENT_IDX     29

// this many virtio descriptors.
// must be a power of two.
#define NUM 8
// the keyboard needs more descriptors
#define KBD_NUM 64

// a single descriptor, from the spec.
struct virtq_desc { // 16 bytes per descriptor for max of 256 descs/page
  uint64 addr;
  uint32 len;
  uint16 flags;
  uint16 next;
};
#define VRING_DESC_F_NEXT  1 // chained with another descriptor
#define VRING_DESC_F_WRITE 2 // device writes (vs read)

// the (entire) avail ring, from the spec.
struct virtq_avail { // 22 bytes
  uint16 flags; // always zero
  uint16 idx;   // driver will write ring[idx] next
  uint16 ring[NUM]; // descriptor numbers of chain heads
  uint16 unused;
};
// one for the keyboard since NUM is baked in
struct virtq_avail_kbd {
  uint16 flags; // always zero
  uint16 idx;   // driver will write ring[idx] next
  uint16 ring[KBD_NUM]; // descriptor numbers of chain heads
  uint16 unused;
};

// one entry in the "used" ring, with which the
// device tells the driver about completed requests.
struct virtq_used_elem { // 8 bytes
  uint32 id;   // index of start of completed descriptor chain
  uint32 len;
};

struct virtq_used { // 68 bytes
  uint16 flags; // always zero
  uint16 idx;   // device increments when it adds a ring[] entry
  struct virtq_used_elem ring[NUM];
};

struct virtq_used_kbd {
  uint16 flags; // always zero
  uint16 idx;   // device increments when it adds a ring[] entry
  struct virtq_used_elem ring[KBD_NUM];
};

// these are specific to virtio block devices, e.g. disks,
// described in Section 5.2 of the spec.

#define VIRTIO_BLK_T_IN  0 // read the disk
#define VIRTIO_BLK_T_OUT 1 // write the disk

// the format of the first descriptor in a disk request.
// to be followed by two more descriptors containing
// the block, and a one-byte status.
struct virtio_blk_req {
  uint32 type; // VIRTIO_BLK_T_IN or ..._OUT
  uint32 reserved;
  uint64 sector;
};

// copied from the standard- all GPU commands
enum virtio_gpu_ctrl_type { 
	/* 2d commands */ 
	VIRTIO_GPU_CMD_GET_DISPLAY_INFO = 0x0100, 
	VIRTIO_GPU_CMD_RESOURCE_CREATE_2D, 
	VIRTIO_GPU_CMD_RESOURCE_UNREF, 
	VIRTIO_GPU_CMD_SET_SCANOUT, 
	VIRTIO_GPU_CMD_RESOURCE_FLUSH, 
	VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D, 
	VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING, 
	VIRTIO_GPU_CMD_RESOURCE_DETACH_BACKING, 
	VIRTIO_GPU_CMD_GET_CAPSET_INFO, 
	VIRTIO_GPU_CMD_GET_CAPSET, 
	VIRTIO_GPU_CMD_GET_EDID, 

	/* cursor commands */ 
	VIRTIO_GPU_CMD_UPDATE_CURSOR = 0x0300, 
	VIRTIO_GPU_CMD_MOVE_CURSOR, 

	/* success responses */ 
	VIRTIO_GPU_RESP_OK_NODATA = 0x1100, 
	VIRTIO_GPU_RESP_OK_DISPLAY_INFO, 
	VIRTIO_GPU_RESP_OK_CAPSET_INFO, 
	VIRTIO_GPU_RESP_OK_CAPSET, 
	VIRTIO_GPU_RESP_OK_EDID, 

	/* error responses */ 
	VIRTIO_GPU_RESP_ERR_UNSPEC = 0x1200, 
	VIRTIO_GPU_RESP_ERR_OUT_OF_MEMORY, 
	VIRTIO_GPU_RESP_ERR_INVALID_SCANOUT_ID, 
	VIRTIO_GPU_RESP_ERR_INVALID_RESOURCE_ID, 
	VIRTIO_GPU_RESP_ERR_INVALID_CONTEXT_ID, 
	VIRTIO_GPU_RESP_ERR_INVALID_PARAMETER, 
};

#define VIRTIO_GPU_FLAG_FENCE (1 << 0) 
 
// control header defining types
struct virtio_gpu_ctrl_hdr { 
	uint32 type; 
	uint32 flags; 
	uint64 fence_id; 
	uint32 ctx_id; 
	uint32 padding; 
};

#define VIRTIO_GPU_MAX_SCANOUTS 16 
 
// it's just a rectangle
struct virtio_gpu_rect { 
	uint32 x; 
	uint32 y; 
	uint32 width; 
	uint32 height; 
};

// formats for the framebuffer
enum virtio_gpu_formats { 
	VIRTIO_GPU_FORMAT_B8G8R8A8_UNORM  = 1, 
	VIRTIO_GPU_FORMAT_B8G8R8X8_UNORM  = 2, 
	VIRTIO_GPU_FORMAT_A8R8G8B8_UNORM  = 3, 
	VIRTIO_GPU_FORMAT_X8R8G8B8_UNORM  = 4, 

	VIRTIO_GPU_FORMAT_R8G8B8A8_UNORM  = 67, 
	VIRTIO_GPU_FORMAT_X8B8G8R8_UNORM  = 68, 

	VIRTIO_GPU_FORMAT_A8B8G8R8_UNORM  = 121, 
	VIRTIO_GPU_FORMAT_R8G8B8X8_UNORM  = 134, 
}; 
 
// create device-side framebuffer
struct virtio_gpu_resource_create_2d { 
	struct virtio_gpu_ctrl_hdr hdr; 
	uint32 resource_id; 
	uint32 format; 
	uint32 width; 
	uint32 height; 
};

// attach kernel RAM as framebuffer
struct virtio_gpu_resource_attach_backing { 
	struct virtio_gpu_ctrl_hdr hdr; 
	uint32 resource_id; 
	uint32 nr_entries; 
};

struct virtio_gpu_mem_entry { 
	uint64 addr; 
	uint32 length; 
	uint32 padding; 
};

// Needed for the request: nr_entries = 1
struct virtio_gpu_resource_attach_backing_singular { 
	struct virtio_gpu_resource_attach_backing req;
	struct virtio_gpu_mem_entry entry;
};

// configure display to use fb we attached
struct virtio_gpu_set_scanout { 
	struct virtio_gpu_ctrl_hdr hdr; 
	struct virtio_gpu_rect r; 
	uint32 scanout_id; 
	uint32 resource_id; 
};

// transfer from kernel to device
struct virtio_gpu_transfer_to_host_2d { 
	struct virtio_gpu_ctrl_hdr hdr; 
	struct virtio_gpu_rect r; 
	uint64 offset; 
	uint32 resource_id; 
	uint32 padding; 
};

// flush resource to screen (?)
struct virtio_gpu_resource_flush { 
	struct virtio_gpu_ctrl_hdr hdr; 
	struct virtio_gpu_rect r; 
	uint32 resource_id; 
	uint32 padding; 
};

// virtiokbd
struct virtio_input_event {
	uint16 type;
	uint16 code;
	uint32 value;
};

enum virtio_input_config_select {
  VIRTIO_INPUT_CFG_UNSET      = 0x00,
  VIRTIO_INPUT_CFG_ID_NAME    = 0x01,
  VIRTIO_INPUT_CFG_ID_SERIAL  = 0x02,
  VIRTIO_INPUT_CFG_ID_DEVIDS  = 0x03,
  VIRTIO_INPUT_CFG_PROP_BITS  = 0x10,
  VIRTIO_INPUT_CFG_EV_BITS    = 0x11,
  VIRTIO_INPUT_CFG_ABS_INFO   = 0x12,
};

struct virtio_input_absinfo {
  uint32  min;
  uint32  max;
  uint32  fuzz;
  uint32  flat;
  uint32  res;
};

struct virtio_input_devids {
  uint16  bustype;
  uint16  vendor;
  uint16  product;
  uint16  version;
};

struct virtio_input_config {
  uint8    select;
  uint8    subsel;
  uint8    size;
  uint8    reserved[5];
  union {
    char string[128];
    uint8   bitmap[128];
    struct virtio_input_absinfo abs;
    struct virtio_input_devids ids;
  } u;
};

struct virtio_sound_config {
  uint32   jacks;
  uint32   streams;
  uint32   chmaps;
};

// virt queue data
enum virtio_sound_virtqueue {
  VIRTIO_SND_VQ_CONTROL = 0,
  VIRTIO_SND_VQ_EVENT,
  VIRTIO_SND_VQ_TX,
  VIRTIO_SND_VQ_RX,
  VIRTIO_SND_VQ_MAX
};

enum virtio_sound_dataflow {
  VIRTIO_SND_D_OUTPUT = 0,
  VIRTIO_SND_D_INPUT
};

enum virtio_sound_request {
  VIRTIO_SND_R_JACK_INFO = 1,
  VIRTIO_SND_R_JACK_REMAP,
  VIRTIO_SND_R_PCM_INFO = 0x0100,
  VIRTIO_SND_R_PCM_SET_PARAMS,
  VIRTIO_SND_R_PCM_PREPARE,
  VIRTIO_SND_R_PCM_RELEASE,
  VIRTIO_SND_R_PCM_START,
  VIRTIO_SND_R_PCM_STOP,
  VIRTIO_SND_R_CHMAP_INFO = 0x0200
};

enum virtio_sound_events {
  VIRTIO_SND_EVT_JACK_CONNECTED = 0x1000,
  VIRTIO_SND_EVT_JACK_DISCONNECTED,
  VIRTIO_SND_EVT_PCM_PERIOD_ELAPSED = 0x1100,
  VIRTIO_SND_EVT_PCM_XRUN
};

enum virtio_sound_status_codes {
  VIRTIO_SND_S_OK = 0x8000,
  VIRTIO_SND_S_BAD_MSG,
  VIRTIO_SND_S_NOT_SUPP,
  VIRTIO_SND_S_IO_ERR
};

struct virtio_sound_header {
  uint32 code;
};

struct virtio_sound_event {
  struct virtio_sound_header header;
  uint32 data;
};

struct virtio_sound_query_info {
  struct virtio_sound_header header;
  uint32 start_id;
  uint32 count;
  uint32 size;
};

struct virtio_sound_info {
  // high definition audio function group node ID
  uint32 hda_fn_gnid;
};

/* Jack control */

struct virtio_sound_jack_header {
  struct virtio_sound_header header;
  uint32 jack_id;
};

enum virtio_sound_jack_remapping {
  VIRTIO_SND_JACK_F_REMAP = 0
};

struct virtio_sound_jack_info {
  struct virtio_sound_info header;
  uint32 features;
  uint32 hda_reg_defconf;
  uint32 hda_reg_caps;
  uint8 connected;
  uint8 padding[7];
};

struct virtio_sound_jack_remap {
  struct virtio_sound_jack_header header;
  uint32 association;
  uint32 sequence;
};

/* PCM CONTROL MESSAGES */

struct virtio_sound_pcm_header {
  struct virtio_sound_header header;
  uint32 stream_id;
};

enum {
  VIRTIO_SND_PCM_F_SHMEM_HOST = 0,
  VIRTIO_SND_PCM_F_SHMEM_GUEST,
  VIRTIO_SND_PCM_F_MSG_POLLING,
  VIRTIO_SND_PCM_F_EVT_SHMEM_PERIODS,
  VIRTIO_SND_PCM_F_EVT_XRUNS
};

enum {
  VIRTIO_SND_PCM_FMT_IMA_ADPCM = 0,
  VIRTIO_SND_PCM_FMT_MU_LAW,
  VIRTIO_SND_PCM_FMT_A_LAW,
  VIRTIO_SND_PCM_FMT_S8,
  VIRTIO_SND_PCM_FMT_U8,
  VIRTIO_SND_PCM_FMT_S16,
  VIRTIO_SND_PCM_FMT_U16,
  VIRTIO_SND_PCM_FMT_S18_3,
  VIRTIO_SND_PCM_FMT_U18_3,
  VIRTIO_SND_PCM_FMT_S20_3,
  VIRTIO_SND_PCM_FMT_U20_3,
  VIRTIO_SND_PCM_FMT_S24_3,
  VIRTIO_SND_PCM_FMT_U24_3,
  VIRTIO_SND_PCM_FMT_S20,
  VIRTIO_SND_PCM_FMT_U20,
  VIRTIO_SND_PCM_FMT_S24,
  VIRTIO_SND_PCM_FMT_U24,
  VIRTIO_SND_PCM_FMT_S32,
  VIRTIO_SND_PCM_FMT_U32,
  VIRTIO_SND_PCM_FMT_FLOAT,
  VIRTIO_SND_PCM_FMT_FLOAT64,
  VIRTIO_SND_PCM_FMT_DSD_U8,
  VIRTIO_SND_PCM_FMT_DSD_U32,
  VIRTIO_SND_PCM_FMT_IEC958_SUBFRAME
};

enum {

  VIRTIO_SND_PCM_RATE_5512 = 0,
  VIRTIO_SND_PCM_RATE_8000,
  VIRTIO_SND_PCM_RATE_11025,
  VIRTIO_SND_PCM_RATE_16000,
  VIRTIO_SND_PCM_RATE_22050,
  VIRTIO_SND_PCM_RATE_32000,
  VIRTIO_SND_PCM_RATE_44100,
  VIRTIO_SND_PCM_RATE_48000,
  VIRTIO_SND_PCM_RATE_64000,
  VIRTIO_SND_PCM_RATE_88200,
  VIRTIO_SND_PCM_RATE_96000,
  VIRTIO_SND_PCM_RATE_176400,
  VIRTIO_SND_PCM_RATE_192000,
  VIRTIO_SND_PCM_RATE_384000
};

struct virtio_sound_pcm_info {
  struct virtio_sound_info header;
  uint32 features;
  uint64 formats;
  uint64 rates;
  uint8 direction;
  uint8 channels_min;
  uint8 channels_max;
  uint8 padding[5];
};

struct virtio_sound_pcm_set_params {
  struct virtio_sound_pcm_header header;
  uint32 buffer_bytes;
  uint32 period_bytes;
  uint32 features;
  uint8 channels;
  uint8 format;
  uint8 rate;
  uint8 padding;
};

/* PCM IO MESSAGES */

struct virtio_sound_pcm_xfer {
  uint32 stream_id;
};

struct virtio_sound_pcm_status {
  uint32 status;
  uint32 latency_bytes;
};

/* CHANNEL MAP MESSAGES */

struct virtio_sound_chmap_header {
  struct virtio_sound_header header;
  uint32 chmap_id;
};

enum virtio_sound_positions {
  VIRTIO_SND_CHMAP_NONE = 0,
  VIRTIO_SND_CHMAP_NA,
  VIRTIO_SND_CHMAP_MONO,
  VIRTIO_SND_CHMAP_FL,
  VIRTIO_SND_CHMAP_FR,
  VIRTIO_SND_CHMAP_RL,
  VIRTIO_SND_CHMAP_RR,
  VIRTIO_SND_CHMAP_FC,
  VIRTIO_SND_CHMAP_LFE,
  VIRTIO_SND_CHMAP_SL,
  VIRTIO_SND_CHMAP_SR,
  VIRTIO_SND_CHMAP_RC,
  VIRTIO_SND_CHMAP_FLC,
  VIRTIO_SND_CHMAP_FRC,
  VIRTIO_SND_CHMAP_RLC,
  VIRTIO_SND_CHMAP_RRC,
  VIRTIO_SND_CHMAP_FLW,
  VIRTIO_SND_CHMAP_FRW,
  VIRTIO_SND_CHMAP_FLH,
  VIRTIO_SND_CHMAP_FCH,
  VIRTIO_SND_CHMAP_FRH,
  VIRTIO_SND_CHMAP_TC,
  VIRTIO_SND_CHMAP_TFL,
  VIRTIO_SND_CHMAP_TFR,
  VIRTIO_SND_CHMAP_TFC,
  VIRTIO_SND_CHMAP_TRL,
  VIRTIO_SND_CHMAP_TRR,
  VIRTIO_SND_CHMAP_TRC,
  VIRTIO_SND_CHMAP_TFLC,
  VIRTIO_SND_CHMAP_TFRC,
  VIRTIO_SND_CHMAP_TSL,
  VIRTIO_SND_CHMAP_TSR,
  VIRTIO_SND_CHMAP_LLFE,
  VIRTIO_SND_CHMAP_RLFE,
  VIRTIO_SND_CHMAP_BC,
  VIRTIO_SND_CHMAP_BLC,
  VIRTIO_SND_CHMAP_BRC
};

#define VIRTIO_SND_CHMAP_MAX_SIZE 18

struct virtio_sound_chmap_info {
  struct virtio_sound_info header;
  uint8 direction;
  uint8 channels;
  uint8 positions[VIRTIO_SND_CHMAP_MAX_SIZE];
};
