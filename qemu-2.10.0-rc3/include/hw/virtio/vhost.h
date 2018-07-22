#ifndef VHOST_H
#define VHOST_H

#include "hw/hw.h"
#include "hw/virtio/vhost-backend.h"
#include "hw/virtio/virtio.h"
#include "exec/memory.h"


#include "rte_eal.h"
#include "rte_mempool.h"
#include "rte_mbuf.h"

#include "spdk/stdinc.h"
#include "spdk/ioat.h"
#include "spdk/env.h"
#include "spdk/queue.h"
#include "spdk/string.h"

#define MAX_VRING_SIZE 256
#define MAX_PKT_BURST 32

/* Generic structures common for any vhost based device. */
struct vhost_virtqueue {
    int kick;
    int call;
    void *desc;
    void *avail;
    void *used;
    int num;
    unsigned long long desc_phys;
    unsigned desc_size;
    unsigned long long avail_phys;
    unsigned avail_size;
    unsigned long long used_phys;
    unsigned used_size;
    EventNotifier masked_notifier;
    struct vhost_dev *dev;
};

//vring里每一项内容
struct vring_elem
{
	uint64_t addr;	//mbuf地址
	uint32_t flag;	//标志位
	//uint32_t next;	//indiate 
};

struct vhost_vring{
	uint32_t tail;
	uint32_t head;
	struct vring_elem vring[MAX_VRING_SIZE];
};


typedef unsigned long vhost_log_chunk_t;
#define VHOST_LOG_PAGE 0x1000
#define VHOST_LOG_BITS (8 * sizeof(vhost_log_chunk_t))
#define VHOST_LOG_CHUNK (VHOST_LOG_PAGE * VHOST_LOG_BITS)
#define VHOST_INVALID_FEATURE_BIT   (0xff)

struct vhost_log {
    unsigned long long size;
    int refcnt;
    int fd;
    vhost_log_chunk_t *log;
};

struct vhost_dev;
struct vhost_iommu {
    struct vhost_dev *hdev;
    MemoryRegion *mr;
    hwaddr iommu_offset;
    IOMMUNotifier n;
    QLIST_ENTRY(vhost_iommu) iommu_next;
};

struct vhost_memory;
struct vhost_dev {
    VirtIODevice *vdev;
    MemoryListener memory_listener;
    MemoryListener iommu_listener;
    struct vhost_memory *mem;
    int n_mem_sections;
    MemoryRegionSection *mem_sections;
    struct vhost_virtqueue *vqs;
    int nvqs;
	struct rte_mempool *mempool;
	//struct vring_elem *vring_new[2];
	pthread_t ntid;
	int already_init;
	struct vhost_vring *vhost_vq[2];
	struct virtio_virtqueue *virtio_vq[2];
	struct rte_ring *vhost_vring[2];
	uint16_t vhost_hlen;
    /* the first virtqueue which would be used by this vhost dev */
    int vq_index;
    uint64_t features;
    uint64_t acked_features;
    uint64_t backend_features;
    uint64_t protocol_features;
    uint64_t max_queues;
    bool started;
    bool log_enabled;
    uint64_t log_size;
    Error *migration_blocker;
    bool memory_changed;
    hwaddr mem_changed_start_addr;
    hwaddr mem_changed_end_addr;
    const VhostOps *vhost_ops;
    void *opaque;
    struct vhost_log *log;
	int ready;	
    QLIST_ENTRY(vhost_dev) entry;
    QLIST_HEAD(, vhost_iommu) iommu_list;
    IOMMUNotifier n;
    struct spdk_ioat_chan *dev_ioat;
    int ioat_submit;
};

struct batch_copy_elem {
	void *dst;
	void *src;
	uint32_t len;
	uint64_t log_addr;
};


struct virtio_virtqueue {
	struct vring_desc	*desc;
	struct vring_avail	*avail;
	struct vring_used	*used;
	uint32_t		size;

	uint16_t		last_avail_idx;
	uint16_t		last_used_idx;
	int			enabled;

	uint64_t		log_guest_addr;

		/* Used to notify the guest (trigger interrupt) */
	int			callfd;
	/* Currently unused as polling mode is enabled */
	int			kickfd;
	struct batch_copy_elem *batch_copy_elems;
	uint16_t batch_copy_nb_elems;

	struct vring_used_elem  *shadow_used_ring;
	uint16_t                shadow_used_idx;
};

//struct virtio_virtqueue *vritio_vq[2];

int vhost_dev_init(struct vhost_dev *hdev, void *opaque,
                   VhostBackendType backend_type,
                   uint32_t busyloop_timeout);
void vhost_dev_cleanup(struct vhost_dev *hdev);
int vhost_dev_start(struct vhost_dev *hdev, VirtIODevice *vdev);
void vhost_dev_stop(struct vhost_dev *hdev, VirtIODevice *vdev);
int vhost_dev_enable_notifiers(struct vhost_dev *hdev, VirtIODevice *vdev);
void vhost_dev_disable_notifiers(struct vhost_dev *hdev, VirtIODevice *vdev);

/* Test and clear masked event pending status.
 * Should be called after unmask to avoid losing events.
 */
bool vhost_virtqueue_pending(struct vhost_dev *hdev, int n);

/* Mask/unmask events from this vq.
 */
void vhost_virtqueue_mask(struct vhost_dev *hdev, VirtIODevice *vdev, int n,
                          bool mask);
uint64_t vhost_get_features(struct vhost_dev *hdev, const int *feature_bits,
                            uint64_t features);
void vhost_ack_features(struct vhost_dev *hdev, const int *feature_bits,
                        uint64_t features);
bool vhost_has_free_slot(void);

int vhost_net_set_backend(struct vhost_dev *hdev,
                          struct vhost_vring_file *file);

int vhost_device_iotlb_miss(struct vhost_dev *dev, uint64_t iova, int write);

void *packet_process_burst(void *arg);
//void *packet_process_send_burst(void *arg);

//void *packet_process_receive_burst(void *arg);

//int vdev_dequeue(struct vhost_dev *hdev);

//int vdev_enqueue(struct vhost_dev *hdev);
#endif
