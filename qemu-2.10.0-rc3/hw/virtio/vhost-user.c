/*
 * vhost-user
 *
 * Copyright (c) 2013 Virtual Open Systems Sarl.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "hw/virtio/vhost.h"
#include "hw/virtio/vhost-backend.h"
#include "hw/virtio/virtio-net.h"
#include "chardev/char-fe.h"
#include "sysemu/kvm.h"
#include "sysemu/dma.h"
//#include "net/eth.h"
#include "qemu/error-report.h"
#include "qemu/sockets.h"

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/eventfd.h>
#include <linux/vhost.h>
#include <linux/virtio_net.h>
#include <sys/syscall.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/prctl.h>

#include <pthread.h>


#define ETHER_ADDR_LEN 6
#define ETHER_TYPE_VLAN 0x8100
#define ETHER_TYPE_IPv4 0x0800 /**< IPv4 Protocol. */
#define ETHER_TYPE_IPv6 0x86DD /**< IPv6 Protocol. */

extern QLIST_HEAD(, vhost_dev) vhost_devices;


struct ipv4_hdr {
	uint8_t  version_ihl;		/**< version and header length */
	uint8_t  type_of_service;	/**< type of service */
	uint16_t total_length;		/**< length of packet */
	uint16_t packet_id;		/**< packet ID */
	uint16_t fragment_offset;	/**< fragmentation offset */
	uint8_t  time_to_live;		/**< time to live */
	uint8_t  next_proto_id;		/**< protocol ID */
	uint16_t hdr_checksum;		/**< header checksum */
	uint32_t src_addr;		/**< source address */
	uint32_t dst_addr;		/**< destination address */
} __attribute__((__packed__));

struct ipv6_hdr {
	uint32_t vtc_flow;     /**< IP version, traffic class & flow label. */
	uint16_t payload_len;  /**< IP packet length - includes sizeof(ip_header). */
	uint8_t  proto;        /**< Protocol, next header. */
	uint8_t  hop_limits;   /**< Hop limits. */
	uint8_t  src_addr[16]; /**< IP address of source host. */
	uint8_t  dst_addr[16]; /**< IP address of destination host(s). */
} __attribute__((__packed__));

struct tcp_hdr {
	uint16_t src_port;  /**< TCP source port. */
	uint16_t dst_port;  /**< TCP destination port. */
	uint32_t sent_seq;  /**< TX data sequence number. */
	uint32_t recv_ack;  /**< RX data acknowledgement sequence number. */
	uint8_t  data_off;  /**< Data offset. */
	uint8_t  tcp_flags; /**< TCP flags */
	uint16_t rx_win;    /**< RX flow control window. */
	uint16_t cksum;     /**< TCP checksum. */
	uint16_t tcp_urp;   /**< TCP urgent pointer, if any. */
} __attribute__((__packed__));

struct udp_hdr {
	uint16_t src_port;    /**< UDP source port. */
	uint16_t dst_port;    /**< UDP destination port. */
	uint16_t dgram_len;   /**< UDP datagram length */
	uint16_t dgram_cksum; /**< UDP datagram checksum */
} __attribute__((__packed__));

struct sctp_hdr {
	uint16_t src_port; /**< Source port. */
	uint16_t dst_port; /**< Destin port. */
	uint32_t tag;      /**< Validation tag. */
	uint32_t cksum;    /**< Checksum. */
} __attribute__((__packed__));


struct vlan_hdr {
	uint16_t vlan_tci; /**< Priority (3) + CFI (1) + Identifier Code (12) */
	uint16_t eth_proto;/**< Ethernet type of encapsulated frame. */
} __attribute__((__packed__));



#define VHOST_MEMORY_MAX_NREGIONS    8
#define VHOST_USER_F_PROTOCOL_FEATURES 30

#define PKT_TX_TCP_CKSUM     (1ULL << 52) /**< TCP cksum of TX pkt. computed by NIC. */
#define PKT_TX_SCTP_CKSUM    (2ULL << 52) /**< SCTP cksum of TX pkt. computed by NIC. */
#define PKT_TX_UDP_CKSUM     (3ULL << 52) /**< UDP cksum of TX pkt. computed by NIC. */


enum VhostUserProtocolFeature {
    VHOST_USER_PROTOCOL_F_MQ = 0,
    VHOST_USER_PROTOCOL_F_LOG_SHMFD = 1,
    VHOST_USER_PROTOCOL_F_RARP = 2,
    VHOST_USER_PROTOCOL_F_REPLY_ACK = 3,
    VHOST_USER_PROTOCOL_F_NET_MTU = 4,
    VHOST_USER_PROTOCOL_F_SLAVE_REQ = 5,
    VHOST_USER_PROTOCOL_F_CROSS_ENDIAN = 6,

    VHOST_USER_PROTOCOL_F_MAX
};

#define VHOST_USER_PROTOCOL_FEATURE_MASK ((1 << VHOST_USER_PROTOCOL_F_MAX) - 1)

typedef enum VhostUserRequest {
    VHOST_USER_NONE = 0,
    VHOST_USER_GET_FEATURES = 1,
    VHOST_USER_SET_FEATURES = 2,
    VHOST_USER_SET_OWNER = 3,
    VHOST_USER_RESET_OWNER = 4,
    VHOST_USER_GET_MEM_TABLE = 5,
    VHOST_USER_SET_LOG_BASE = 6,
    VHOST_USER_SET_LOG_FD = 7,
    VHOST_USER_SET_VRING_NUM = 8,
    VHOST_USER_GET_VRING_ADDR = 9,
    VHOST_USER_SET_VRING_BASE = 10,
    VHOST_USER_GET_VRING_BASE = 11,
    VHOST_USER_SET_VRING_KICK = 12,
    VHOST_USER_SET_VRING_CALL = 13,
    VHOST_USER_SET_VRING_ERR = 14,
    VHOST_USER_GET_PROTOCOL_FEATURES = 15,
    VHOST_USER_SET_PROTOCOL_FEATURES = 16,
    VHOST_USER_GET_QUEUE_NUM = 17,
    VHOST_USER_SET_VRING_ENABLE = 18,
    VHOST_USER_SEND_RARP = 19,
    VHOST_USER_NET_SET_MTU = 20,
    VHOST_USER_SET_SLAVE_REQ_FD = 21,
    VHOST_USER_IOTLB_MSG = 22,
    VHOST_USER_SET_VRING_ENDIAN = 23,
    VHOST_USER_MAX
} VhostUserRequest;

typedef enum VhostUserSlaveRequest {
    VHOST_USER_SLAVE_NONE = 0,
    VHOST_USER_SLAVE_IOTLB_MSG = 1,
    VHOST_USER_SLAVE_MAX
}  VhostUserSlaveRequest;

typedef struct VhostUserMemoryRegion {
    uint64_t guest_phys_addr;
    uint64_t memory_size;
    uint64_t userspace_addr;
    uint64_t mmap_offset;
} VhostUserMemoryRegion;

typedef struct VhostUserMemory {
    uint32_t nregions;
    uint32_t padding;
    VhostUserMemoryRegion regions[VHOST_MEMORY_MAX_NREGIONS];
} VhostUserMemory;

typedef struct VhostUserLog {
    uint64_t mmap_size;
    uint64_t mmap_offset;
} VhostUserLog;

typedef struct VhostUserMsg {
    VhostUserRequest request;

#define VHOST_USER_VERSION_MASK     (0x3)
#define VHOST_USER_REPLY_MASK       (0x1<<2)
#define VHOST_USER_NEED_REPLY_MASK  (0x1 << 3)
    uint32_t flags;
    uint32_t size; /* the following payload size */
    union {
#define VHOST_USER_VRING_IDX_MASK   (0xff)
#define VHOST_USER_VRING_NOFD_MASK  (0x1<<8)
        uint64_t u64;
        struct vhost_vring_state state;
        struct vhost_vring_addr addr;
        VhostUserMemory memory;
        VhostUserLog log;
        struct vhost_iotlb_msg iotlb;
		char  mempool_name[32];
		char vring_name[32];
		//int vring_fd;
    } payload;
} QEMU_PACKED VhostUserMsg;

static VhostUserMsg m __attribute__ ((unused));
#define VHOST_USER_HDR_SIZE (sizeof(m.request) \
                            + sizeof(m.flags) \
                            + sizeof(m.size))

#define VHOST_USER_PAYLOAD_SIZE (sizeof(m) - VHOST_USER_HDR_SIZE)

/* The version of the protocol we support */
#define VHOST_USER_VERSION    (0x1)

struct vhost_user {
    CharBackend *chr;
    int slave_fd;
};


/*static inline void rte_prefetch0(const volatile void *p)
{
	asm volatile ("prefetcht0 %[p]" : : [p] "m" (*(const volatile char *)p));
}*/

static bool ioeventfd_enabled(void)
{
    return kvm_enabled() && kvm_eventfds_enabled();
}

static int vhost_user_read(struct vhost_dev *dev, VhostUserMsg *msg)
{
    struct vhost_user *u = dev->opaque;
    CharBackend *chr = u->chr;
    uint8_t *p = (uint8_t *) msg;
    int r, size = VHOST_USER_HDR_SIZE;

    r = qemu_chr_fe_read_all(chr, p, size);
    if (r != size) {
        error_report("Failed to read msg header. Read %d instead of %d."
                     " Original request %d.", r, size, msg->request);
        goto fail;
    }

    /* validate received flags */
    if (msg->flags != (VHOST_USER_REPLY_MASK | VHOST_USER_VERSION)) {
        error_report("Failed to read msg header."
                " Flags 0x%x instead of 0x%x.", msg->flags,
                VHOST_USER_REPLY_MASK | VHOST_USER_VERSION);
        goto fail;
    }

    /* validate message size is sane */
    if (msg->size > VHOST_USER_PAYLOAD_SIZE) {
        error_report("Failed to read msg header."
                " Size %d exceeds the maximum %zu.", msg->size,
                VHOST_USER_PAYLOAD_SIZE);
        goto fail;
    }

    if (msg->size) {
        p += VHOST_USER_HDR_SIZE;
        size = msg->size;
        r = qemu_chr_fe_read_all(chr, p, size);
        if (r != size) {
            error_report("Failed to read msg payload."
                         " Read %d instead of %d.", r, msg->size);
            goto fail;
        }
    }

    return 0;

fail:
    return -1;
}


static int vhost_user_read_fds(struct vhost_dev *dev, VhostUserMsg *msg,int *fds)
{
    struct vhost_user *u = dev->opaque;
    CharBackend *chr = u->chr;
    uint8_t *p = (uint8_t *) msg;
    int r, size = VHOST_USER_HDR_SIZE;

    r = qemu_chr_fe_read_all(chr, p, size);
    if (r != size) {
        error_report("Failed to read msg header. Read %d instead of %d."
                     " Original request %d.", r, size, msg->request);
        goto fail;
    }

    /* validate received flags */
    if (msg->flags != (VHOST_USER_REPLY_MASK | VHOST_USER_VERSION)) {
        error_report("Failed to read msg header."
                " Flags 0x%x instead of 0x%x.", msg->flags,
                VHOST_USER_REPLY_MASK | VHOST_USER_VERSION);
        goto fail;
    }

    /* validate message size is sane */
    if (msg->size > VHOST_USER_PAYLOAD_SIZE) {
        error_report("Failed to read msg header."
                " Size %d exceeds the maximum %zu.", msg->size,
                VHOST_USER_PAYLOAD_SIZE);
        goto fail;
    }

    if (msg->size) {
        p += VHOST_USER_HDR_SIZE;
        size = msg->size;
        r = qemu_chr_fe_read_all(chr, p, size);
        if (r != size) {
            error_report("Failed to read msg payload."
                         " Read %d instead of %d.", r, msg->size);
            goto fail;
        }
    }
    if(qemu_chr_fe_get_msgfds(chr,fds,1)<0){
    	error_report("Failed to get msg fds.");
        goto fail;
    }

    return 0;

fail:
    return -1;
}


static int process_message_reply(struct vhost_dev *dev,
                                 const VhostUserMsg *msg)
{
    VhostUserMsg msg_reply;

    if ((msg->flags & VHOST_USER_NEED_REPLY_MASK) == 0) {
        return 0;
    }

    if (vhost_user_read(dev, &msg_reply) < 0) {
        return -1;
    }

    if (msg_reply.request != msg->request) {
        error_report("Received unexpected msg type."
                     "Expected %d received %d",
                     msg->request, msg_reply.request);
        return -1;
    }

    return msg_reply.payload.u64 ? -1 : 0;
}

static bool vhost_user_one_time_request(VhostUserRequest request)
{
    switch (request) {
    case VHOST_USER_SET_OWNER:
    case VHOST_USER_RESET_OWNER:
    //case VHOST_USER_SET_MEM_TABLE:
    case VHOST_USER_GET_QUEUE_NUM:
    case VHOST_USER_NET_SET_MTU:
        return true;
    default:
        return false;
    }
}

/* most non-init callers ignore the error */
static int vhost_user_write(struct vhost_dev *dev, VhostUserMsg *msg,
                            int *fds, int fd_num)
{
    struct vhost_user *u = dev->opaque;
    CharBackend *chr = u->chr;
    int ret, size = VHOST_USER_HDR_SIZE + msg->size;

    /*
     * For non-vring specific requests, like VHOST_USER_SET_MEM_TABLE,
     * we just need send it once in the first time. For later such
     * request, we just ignore it.
     */
    if (vhost_user_one_time_request(msg->request) && dev->vq_index != 0) {
        msg->flags &= ~VHOST_USER_NEED_REPLY_MASK;
        return 0;
    }

    if (qemu_chr_fe_set_msgfds(chr, fds, fd_num) < 0) {
        error_report("Failed to set msg fds.");
        return -1;
    }

    ret = qemu_chr_fe_write_all(chr, (const uint8_t *) msg, size);
    if (ret != size) {
        error_report("Failed to write msg."
                     " Wrote %d instead of %d.", ret, size);
        return -1;
    }

    return 0;
}

static int vhost_user_set_log_base(struct vhost_dev *dev, uint64_t base,
                                   struct vhost_log *log)
{
    int fds[VHOST_MEMORY_MAX_NREGIONS];
    size_t fd_num = 0;
    bool shmfd = virtio_has_feature(dev->protocol_features,
                                    VHOST_USER_PROTOCOL_F_LOG_SHMFD);
    VhostUserMsg msg = {
        .request = VHOST_USER_SET_LOG_BASE,
        .flags = VHOST_USER_VERSION,
        .payload.log.mmap_size = log->size * sizeof(*(log->log)),
        .payload.log.mmap_offset = 0,
        .size = sizeof(msg.payload.log),
    };

    if (shmfd && log->fd != -1) {
        fds[fd_num++] = log->fd;
    }

    if (vhost_user_write(dev, &msg, fds, fd_num) < 0) {
        return -1;
    }

    if (shmfd) {
        msg.size = 0;
        if (vhost_user_read(dev, &msg) < 0) {
            return -1;
        }

        if (msg.request != VHOST_USER_SET_LOG_BASE) {
            error_report("Received unexpected msg type. "
                         "Expected %d received %d",
                         VHOST_USER_SET_LOG_BASE, msg.request);
            return -1;
        }
    }

    return 0;
}

static uint64_t
get_blk_size(int fd)
{
	struct stat stat;
	int ret;

	ret = fstat(fd, &stat);
	return ret == -1 ? (uint64_t)-1 : (uint64_t)stat.st_blksize;
}

static int vhost_user_get_mem_table(struct vhost_dev *dev,
                                    struct vhost_memory *mem)
{
    int fds[VHOST_MEMORY_MAX_NREGIONS];
    int i, fd;
    size_t fd_num = 0;
    bool reply_supported = virtio_has_feature(dev->protocol_features,
                                              VHOST_USER_PROTOCOL_F_REPLY_ACK);

    VhostUserMsg msg = {
        .request = VHOST_USER_GET_MEM_TABLE,
        .flags = VHOST_USER_VERSION,
    };

	VhostUserRequest request = VHOST_USER_GET_MEM_TABLE;
	
    if (reply_supported) {
        msg.flags |= VHOST_USER_NEED_REPLY_MASK;
    }

    for (i = 0; i < dev->mem->nregions; ++i) {
        struct vhost_memory_region *reg = dev->mem->regions + i;
        ram_addr_t offset;
        MemoryRegion *mr;

        assert((uintptr_t)reg->userspace_addr == reg->userspace_addr);
        mr = memory_region_from_host((void *)(uintptr_t)reg->userspace_addr,
                                     &offset);
        fd = memory_region_get_fd(mr);
        if (fd > 0) {
            msg.payload.memory.regions[fd_num].userspace_addr = reg->userspace_addr;
            msg.payload.memory.regions[fd_num].memory_size  = reg->memory_size;
            msg.payload.memory.regions[fd_num].guest_phys_addr = reg->guest_phys_addr;
            msg.payload.memory.regions[fd_num].mmap_offset = offset;
            assert(fd_num < VHOST_MEMORY_MAX_NREGIONS);
            fds[fd_num++] = fd;
        }

    }

    uint64_t memory_page_alignment = get_blk_size(fd);
    uint64_t register_size=MAX(msg.payload.memory.regions[fd_num-1].guest_phys_addr+msg.payload.memory.regions[fd_num-1].memory_size,memory_page_alignment);
    int done = spdk_mem_register((void*)(msg.payload.memory.regions[0].userspace_addr),register_size);
    if(done == 0)
    {
        printf("mem[%d] registered!\n",i);
    }

    msg.payload.memory.nregions = fd_num;

    if (!fd_num) {
        error_report("Failed initializing vhost-user memory map, "
                     "consider using -object memory-backend-file share=on");
        return -1;
    }

    msg.size = sizeof(msg.payload.memory.nregions);
    msg.size += sizeof(msg.payload.memory.padding);
    msg.size += fd_num * sizeof(VhostUserMemoryRegion);

    if (vhost_user_write(dev, &msg, fds, fd_num) < 0) {
        return -1;
    }

	if (vhost_user_read(dev, &msg) < 0) {
        return -1;
    }

	if (msg.request != request) {
        error_report("Received unexpected msg type. Expected %d received %d",
                     request, msg.request);
        return -1;
    }

    if (msg.size != sizeof(msg.payload.mempool_name)) {
        error_report("Received bad msg size.");
        return -1;
    }
/*
	if(dev->already_init==0)
	{
		//rte_qemu_init();
	        int argc=6;
                char *argv[argc];
                argv[1]="-c";
                argv[2]="4";
                argv[3]="-n";
                argv[4]="4";
                argv[5]="--proc-type=secondary";
                rte_eal_init(argc,argv);
		dev->already_init=1;
	}
    
*/	
	dev->mempool=rte_mempool_lookup(msg.payload.mempool_name);

	dev->ready=0;
	
    //*u64 = msg.payload.u64;

    //if (reply_supported) {
    //    return process_message_reply(dev, &msg);
    //}

    return 0;
}

static int vhost_user_get_vring_addr(struct vhost_dev *dev,
                                     struct vhost_vring_addr *addr)
{
    VhostUserMsg msg = {
        .request = VHOST_USER_GET_VRING_ADDR,
        .flags = VHOST_USER_VERSION,
        .payload.addr = *addr,
        .size = sizeof(msg.payload.addr),
    };

    VhostUserRequest request = VHOST_USER_GET_VRING_ADDR;

    if (vhost_user_write(dev, &msg, NULL, 0) < 0) {
        return -1;
    }


	if (vhost_user_read(dev, &msg) < 0) {
        return -1;
    }
	
	if (msg.request != request) {
        error_report("Received unexpected msg type. Expected %d received %d",
                     request, msg.request);
        return -1;
    }

    if (msg.size != sizeof(msg.payload.vring_name)) {
        error_report("Received bad msg size.");
        return -1;
    }

    
   // if ((tmp_fd=open(msg.payload.vring_file_name,O_RDWR | O_CREAT,0644))<0)
   // {
   //     perror("open");
	///	return NULL;
	//}
	//printf("tmp_fd=%d\n",tmp_fd);
	//int tmp_fd=msg.payload.vring_fd;
	//void *mmap_addr;
	//int pagesize; default 4096
	//if ((mmap_addr = mmap(NULL, 4096*2, PROT_READ |   
    //            PROT_WRITE, MAP_SHARED | MAP_POPULATE, tmp_fd, 0)) == (void *)-1) {  //映射共享内存，本来mmap返回值是void *，强制转换我们自己要的类型
    //    perror("mmap");
	//	return -1;
    //}
    //printf("0x%16x\n",mmap_addr);
	//char*ring_name=msg.payload.vring_name;
	dev->vhost_vring[addr->index % 2]=rte_ring_lookup(msg.payload.vring_name);			
	//dev->vhost_vq[addr->index]=(struct vhost_vring*)mmap_addr;
	//printf("0x%16x\n",dev->vhost_vq);
	//dev->vhost_vq[1]=(struct vhost_vring*)(mmap_addr+4096);

	

    return 0;
}

static int vhost_user_set_vring_endian(struct vhost_dev *dev,
                                       struct vhost_vring_state *ring)
{
    bool cross_endian = virtio_has_feature(dev->protocol_features,
                                           VHOST_USER_PROTOCOL_F_CROSS_ENDIAN);
    VhostUserMsg msg = {
        .request = VHOST_USER_SET_VRING_ENDIAN,
        .flags = VHOST_USER_VERSION,
        .payload.state = *ring,
        .size = sizeof(msg.payload.state),
    };

    if (!cross_endian) {
        error_report("vhost-user trying to send unhandled ioctl");
        return -1;
    }

    if (vhost_user_write(dev, &msg, NULL, 0) < 0) {
        return -1;
    }

    return 0;
}

static int vhost_set_vring(struct vhost_dev *dev,
                           unsigned long int request,
                           struct vhost_vring_state *ring)
{
    VhostUserMsg msg = {
        .request = request,
        .flags = VHOST_USER_VERSION,
        .payload.state = *ring,
        .size = sizeof(msg.payload.state),
    };

    if (vhost_user_write(dev, &msg, NULL, 0) < 0) {
        return -1;
    }

    return 0;
}

static int vhost_user_set_vring_num(struct vhost_dev *dev,
                                    struct vhost_vring_state *ring)
{
    return vhost_set_vring(dev, VHOST_USER_SET_VRING_NUM, ring);
}

static int vhost_user_set_vring_base(struct vhost_dev *dev,
                                     struct vhost_vring_state *ring)
{
    return vhost_set_vring(dev, VHOST_USER_SET_VRING_BASE, ring);
}

static int vhost_user_set_vring_enable(struct vhost_dev *dev, int enable)
{
    int i;

    if (!virtio_has_feature(dev->features, VHOST_USER_F_PROTOCOL_FEATURES)) {
        return -1;
    }

    for (i = 0; i < dev->nvqs; ++i) {
        struct vhost_vring_state state = {
            .index = dev->vq_index + i,
            .num   = enable,
        };

        vhost_set_vring(dev, VHOST_USER_SET_VRING_ENABLE, &state);
    }

    return 0;
}

static int vhost_user_get_vring_base(struct vhost_dev *dev,
                                     struct vhost_vring_state *ring)
{
    VhostUserMsg msg = {
        .request = VHOST_USER_GET_VRING_BASE,
        .flags = VHOST_USER_VERSION,
        .payload.state = *ring,
        .size = sizeof(msg.payload.state),
    };

    if (vhost_user_write(dev, &msg, NULL, 0) < 0) {
        return -1;
    }

    if (vhost_user_read(dev, &msg) < 0) {
        return -1;
    }

    if (msg.request != VHOST_USER_GET_VRING_BASE) {
        error_report("Received unexpected msg type. Expected %d received %d",
                     VHOST_USER_GET_VRING_BASE, msg.request);
        return -1;
    }

    if (msg.size != sizeof(msg.payload.state)) {
        error_report("Received bad msg size.");
        return -1;
    }

    *ring = msg.payload.state;

    return 0;
}

static int vhost_set_vring_file(struct vhost_dev *dev,
                                VhostUserRequest request,
                                struct vhost_vring_file *file)
{
    int fds[VHOST_MEMORY_MAX_NREGIONS];
    size_t fd_num = 0;
    VhostUserMsg msg = {
        .request = request,
        .flags = VHOST_USER_VERSION,
        .payload.u64 = file->index & VHOST_USER_VRING_IDX_MASK,
        .size = sizeof(msg.payload.u64),
    };

    if (ioeventfd_enabled() && file->fd > 0) {
        fds[fd_num++] = file->fd;
    } else {
        msg.payload.u64 |= VHOST_USER_VRING_NOFD_MASK;
    }   

    if (vhost_user_write(dev, &msg, fds, fd_num) < 0) {
        return -1;
    }

    return 0;
}

static int vhost_user_set_vring_kick(struct vhost_dev *dev,
                                     struct vhost_vring_file *file)
{
    return vhost_set_vring_file(dev, VHOST_USER_SET_VRING_KICK, file);
}

static int vhost_user_set_vring_call(struct vhost_dev *dev,
                                     struct vhost_vring_file *file)
{
    return vhost_set_vring_file(dev, VHOST_USER_SET_VRING_CALL, file);
}

static int vhost_user_set_u64(struct vhost_dev *dev, int request, uint64_t u64)
{
    VhostUserMsg msg = {
        .request = request,
        .flags = VHOST_USER_VERSION,
        .payload.u64 = u64,
        .size = sizeof(msg.payload.u64),
    };

    if (vhost_user_write(dev, &msg, NULL, 0) < 0) {
        return -1;
    }

    return 0;
}

static int vhost_user_set_features(struct vhost_dev *dev,
                                   uint64_t features)
{
	if (features & ((1 << VIRTIO_NET_F_MRG_RXBUF) | (1ULL << VIRTIO_F_VERSION_1))) {
		dev->vhost_hlen = sizeof(struct virtio_net_hdr_mrg_rxbuf);
	} else {
		dev->vhost_hlen = sizeof(struct virtio_net_hdr);
	}
    return vhost_user_set_u64(dev, VHOST_USER_SET_FEATURES, features);
}

static int vhost_user_set_protocol_features(struct vhost_dev *dev,
                                            uint64_t features)
{
    return vhost_user_set_u64(dev, VHOST_USER_SET_PROTOCOL_FEATURES, features);
}

static int vhost_user_get_u64(struct vhost_dev *dev, int request, uint64_t *u64)
{
    VhostUserMsg msg = {
        .request = request,
        .flags = VHOST_USER_VERSION,
    };

    if (vhost_user_one_time_request(request) && dev->vq_index != 0) {
        return 0;
    }

    if (vhost_user_write(dev, &msg, NULL, 0) < 0) {
        return -1;
    }

    if (vhost_user_read(dev, &msg) < 0) {
        return -1;
    }

    if (msg.request != request) {
        error_report("Received unexpected msg type. Expected %d received %d",
                     request, msg.request);
        return -1;
    }

    if (msg.size != sizeof(msg.payload.u64)) {
        error_report("Received bad msg size.");
        return -1;
    }

    *u64 = msg.payload.u64;

    return 0;
}

static int vhost_user_get_features(struct vhost_dev *dev, uint64_t *features)
{
    return vhost_user_get_u64(dev, VHOST_USER_GET_FEATURES, features);
}

static int vhost_user_set_owner(struct vhost_dev *dev)
{
    VhostUserMsg msg = {
        .request = VHOST_USER_SET_OWNER,
        .flags = VHOST_USER_VERSION,
    };

    if (vhost_user_write(dev, &msg, NULL, 0) < 0) {
        return -1;
    }

    return 0;
}

static int vhost_user_reset_device(struct vhost_dev *dev)
{
    VhostUserMsg msg = {
        .request = VHOST_USER_RESET_OWNER,
        .flags = VHOST_USER_VERSION,
    };

    if (vhost_user_write(dev, &msg, NULL, 0) < 0) {
        return -1;
    }

    return 0;
}

static void slave_read(void *opaque)
{
    struct vhost_dev *dev = opaque;
    struct vhost_user *u = dev->opaque;
    VhostUserMsg msg = { 0, };
    int size, ret = 0;

    /* Read header */
    size = read(u->slave_fd, &msg, VHOST_USER_HDR_SIZE);
    if (size != VHOST_USER_HDR_SIZE) {
        error_report("Failed to read from slave.");
        goto err;
    }

    if (msg.size > VHOST_USER_PAYLOAD_SIZE) {
        error_report("Failed to read msg header."
                " Size %d exceeds the maximum %zu.", msg.size,
                VHOST_USER_PAYLOAD_SIZE);
        goto err;
    }

    /* Read payload */
    size = read(u->slave_fd, &msg.payload, msg.size);
    if (size != msg.size) {
        error_report("Failed to read payload from slave.");
        goto err;
    }

    switch (msg.request) {
    case VHOST_USER_SLAVE_IOTLB_MSG:
        ret = vhost_backend_handle_iotlb_msg(dev, &msg.payload.iotlb);
        break;
    default:
        error_report("Received unexpected msg type.");
        ret = -EINVAL;
    }

    /*
     * REPLY_ACK feature handling. Other reply types has to be managed
     * directly in their request handlers.
     */
    if (msg.flags & VHOST_USER_NEED_REPLY_MASK) {
        msg.flags &= ~VHOST_USER_NEED_REPLY_MASK;
        msg.flags |= VHOST_USER_REPLY_MASK;

        msg.payload.u64 = !!ret;
        msg.size = sizeof(msg.payload.u64);

        size = write(u->slave_fd, &msg, VHOST_USER_HDR_SIZE + msg.size);
        if (size != VHOST_USER_HDR_SIZE + msg.size) {
            error_report("Failed to send msg reply to slave.");
            goto err;
        }
    }

    return;

err:
    qemu_set_fd_handler(u->slave_fd, NULL, NULL, NULL);
    close(u->slave_fd);
    u->slave_fd = -1;
    return;
}

static int vhost_setup_slave_channel(struct vhost_dev *dev)
{
    VhostUserMsg msg = {
        .request = VHOST_USER_SET_SLAVE_REQ_FD,
        .flags = VHOST_USER_VERSION,
    };
    struct vhost_user *u = dev->opaque;
    int sv[2], ret = 0;
    bool reply_supported = virtio_has_feature(dev->protocol_features,
                                              VHOST_USER_PROTOCOL_F_REPLY_ACK);

    if (!virtio_has_feature(dev->protocol_features,
                            VHOST_USER_PROTOCOL_F_SLAVE_REQ)) {
        return 0;
    }

    if (socketpair(PF_UNIX, SOCK_STREAM, 0, sv) == -1) {
        error_report("socketpair() failed");
        return -1;
    }

    u->slave_fd = sv[0];
    qemu_set_fd_handler(u->slave_fd, slave_read, NULL, dev);

    if (reply_supported) {
        msg.flags |= VHOST_USER_NEED_REPLY_MASK;
    }

    ret = vhost_user_write(dev, &msg, &sv[1], 1);
    if (ret) {
        goto out;
    }

    if (reply_supported) {
        ret = process_message_reply(dev, &msg);
    }

out:
    close(sv[1]);
    if (ret) {
        qemu_set_fd_handler(u->slave_fd, NULL, NULL, NULL);
        close(u->slave_fd);
        u->slave_fd = -1;
    }

    return ret;
}

static int vhost_user_init(struct vhost_dev *dev, void *opaque)
{
    uint64_t features, protocol_features;
    struct vhost_user *u;
    int err;

    assert(dev->vhost_ops->backend_type == VHOST_BACKEND_TYPE_USER);

    u = g_new0(struct vhost_user, 1);
    u->chr = opaque;
    u->slave_fd = -1;
    dev->opaque = u;

    err = vhost_user_get_features(dev, &features);
    if (err < 0) {
        return err;
    }

    if (virtio_has_feature(features, VHOST_USER_F_PROTOCOL_FEATURES)) {
        dev->backend_features |= 1ULL << VHOST_USER_F_PROTOCOL_FEATURES;

        err = vhost_user_get_u64(dev, VHOST_USER_GET_PROTOCOL_FEATURES,
                                 &protocol_features);
        if (err < 0) {
            return err;
        }

        dev->protocol_features =
            protocol_features & VHOST_USER_PROTOCOL_FEATURE_MASK;
        err = vhost_user_set_protocol_features(dev, dev->protocol_features);
        if (err < 0) {
            return err;
        }

        /* query the max queues we support if backend supports Multiple Queue */
        if (dev->protocol_features & (1ULL << VHOST_USER_PROTOCOL_F_MQ)) {
            err = vhost_user_get_u64(dev, VHOST_USER_GET_QUEUE_NUM,
                                     &dev->max_queues);
            if (err < 0) {
                return err;
            }
        }

        if (virtio_has_feature(features, VIRTIO_F_IOMMU_PLATFORM) &&
                !(virtio_has_feature(dev->protocol_features,
                    VHOST_USER_PROTOCOL_F_SLAVE_REQ) &&
                 virtio_has_feature(dev->protocol_features,
                    VHOST_USER_PROTOCOL_F_REPLY_ACK))) {
            error_report("IOMMU support requires reply-ack and "
                         "slave-req protocol features.");
            return -1;
        }
    }

    if (dev->migration_blocker == NULL &&
        !virtio_has_feature(dev->protocol_features,
                            VHOST_USER_PROTOCOL_F_LOG_SHMFD)) {
        error_setg(&dev->migration_blocker,
                   "Migration disabled: vhost-user backend lacks "
                   "VHOST_USER_PROTOCOL_F_LOG_SHMFD feature.");
    }

    err = vhost_setup_slave_channel(dev);
    if (err < 0) {
        return err;
    }

    return 0;
}

static int vhost_user_cleanup(struct vhost_dev *dev)
{
    struct vhost_user *u;

    assert(dev->vhost_ops->backend_type == VHOST_BACKEND_TYPE_USER);

    u = dev->opaque;
    if (u->slave_fd >= 0) {
        qemu_set_fd_handler(u->slave_fd, NULL, NULL, NULL);
        close(u->slave_fd);
        u->slave_fd = -1;
    }
    g_free(u);
    dev->opaque = 0;

    return 0;
}

static int vhost_user_get_vq_index(struct vhost_dev *dev, int idx)
{
    assert(idx >= dev->vq_index && idx < dev->vq_index + dev->nvqs);

    return idx;
}

static int vhost_user_memslots_limit(struct vhost_dev *dev)
{
    return VHOST_MEMORY_MAX_NREGIONS;
}

static bool vhost_user_requires_shm_log(struct vhost_dev *dev)
{
    assert(dev->vhost_ops->backend_type == VHOST_BACKEND_TYPE_USER);

    return virtio_has_feature(dev->protocol_features,
                              VHOST_USER_PROTOCOL_F_LOG_SHMFD);
}

static int vhost_user_migration_done(struct vhost_dev *dev, char* mac_addr)
{
    VhostUserMsg msg = { 0 };

    assert(dev->vhost_ops->backend_type == VHOST_BACKEND_TYPE_USER);

    /* If guest supports GUEST_ANNOUNCE do nothing */
    if (virtio_has_feature(dev->acked_features, VIRTIO_NET_F_GUEST_ANNOUNCE)) {
        return 0;
    }

    /* if backend supports VHOST_USER_PROTOCOL_F_RARP ask it to send the RARP */
    if (virtio_has_feature(dev->protocol_features,
                           VHOST_USER_PROTOCOL_F_RARP)) {
        msg.request = VHOST_USER_SEND_RARP;
        msg.flags = VHOST_USER_VERSION;
        rte_memcpy((char *)&msg.payload.u64, mac_addr, 6);
        msg.size = sizeof(msg.payload.u64);

        return vhost_user_write(dev, &msg, NULL, 0);
    }
    return -1;
}

static bool vhost_user_can_merge(struct vhost_dev *dev,
                                 uint64_t start1, uint64_t size1,
                                 uint64_t start2, uint64_t size2)
{
    ram_addr_t offset;
    int mfd, rfd;
    MemoryRegion *mr;

    mr = memory_region_from_host((void *)(uintptr_t)start1, &offset);
    mfd = memory_region_get_fd(mr);

    mr = memory_region_from_host((void *)(uintptr_t)start2, &offset);
    rfd = memory_region_get_fd(mr);

    return mfd == rfd;
}

static int vhost_user_net_set_mtu(struct vhost_dev *dev, uint16_t mtu)
{
    VhostUserMsg msg;
    bool reply_supported = virtio_has_feature(dev->protocol_features,
                                              VHOST_USER_PROTOCOL_F_REPLY_ACK);

    if (!(dev->protocol_features & (1ULL << VHOST_USER_PROTOCOL_F_NET_MTU))) {
        return 0;
    }

    msg.request = VHOST_USER_NET_SET_MTU;
    msg.payload.u64 = mtu;
    msg.size = sizeof(msg.payload.u64);
    msg.flags = VHOST_USER_VERSION;
    if (reply_supported) {
        msg.flags |= VHOST_USER_NEED_REPLY_MASK;
    }

    if (vhost_user_write(dev, &msg, NULL, 0) < 0) {
        return -1;
    }

    /* If reply_ack supported, slave has to ack specified MTU is valid */
    if (reply_supported) {
        return process_message_reply(dev, &msg);
    }

    return 0;
}

static int vhost_user_send_device_iotlb_msg(struct vhost_dev *dev,
                                            struct vhost_iotlb_msg *imsg)
{
    VhostUserMsg msg = {
        .request = VHOST_USER_IOTLB_MSG,
        .size = sizeof(msg.payload.iotlb),
        .flags = VHOST_USER_VERSION | VHOST_USER_NEED_REPLY_MASK,
        .payload.iotlb = *imsg,
    };

    if (vhost_user_write(dev, &msg, NULL, 0) < 0) {
        return -EFAULT;
    }

    return process_message_reply(dev, &msg);
}


static void vhost_user_set_iotlb_callback(struct vhost_dev *dev, int enabled)
{
    /* No-op as the receive channel is not dedicated to IOTLB messages. */
}


static void
ioat_done(void *cb_arg)
{
	struct vhost_dev *dev = (struct vhost_dev *)cb_arg;
	dev->ioat_submit--;
}


static inline uint64_t rte_vhost_gpa_to_qva(struct vhost_dev *dev, uint64_t gpa)
{
	struct vhost_memory_region *reg;
	uint32_t i;

	for (i = 0; i < dev->mem->nregions; i++) {
		reg = dev->mem->regions + i;
		if (gpa >= reg->guest_phys_addr &&
		    gpa <  reg->guest_phys_addr + reg->memory_size) {
			return gpa - reg->guest_phys_addr +
			       reg->userspace_addr;
		}
	}

	return 0;
}


//static inline __attribute__((always_inline)) struct vring_desc *
static inline struct vring_desc *
alloc_copy_ind_table(struct vhost_dev *dev, struct virtio_virtqueue *vq,
					 struct vring_desc *desc)
{
	struct vring_desc *idesc;
	uint64_t src, dst;
	uint64_t len, remain = desc->len;
	uint64_t desc_addr = desc->addr;

	idesc = malloc(desc->len);
	if (unlikely(!idesc))
		return 0;

	dst = (uint64_t)(uintptr_t)idesc;
///////////////////////////////////////////////////////////////////////////
	while (remain) {
		len = remain;
		src = rte_vhost_gpa_to_qva(dev,desc_addr);
		if (unlikely(!src || !len)) {
			free(idesc);
			return 0;
		}

		//rte_rte_memcpy((void *)(uintptr_t)dst, (void *)(uintptr_t)src, len);
		rte_memcpy((void *)(uintptr_t)dst, (void *)(uintptr_t)src, len);

		remain -= len;
		dst += len;
		desc_addr += len;
	}

	return idesc;
}


static inline bool
virtio_net_with_host_offload(struct vhost_dev *dev)
{
	if (dev->features &
			((1ULL << VIRTIO_NET_F_CSUM) |
			(1ULL << VIRTIO_NET_F_HOST_ECN) |
			(1ULL << VIRTIO_NET_F_HOST_TSO4) |
			(1ULL << VIRTIO_NET_F_HOST_TSO6) |
			(1ULL << VIRTIO_NET_F_HOST_UFO)))
	   return true;
					 
    return false;
}

struct ether_addr {
	uint8_t addr_bytes[ETHER_ADDR_LEN]; /**< Addr bytes in tx order */
} __attribute__((__packed__));

struct ether_hdr {
	struct ether_addr d_addr; /**< Destination address. */
	struct ether_addr s_addr; /**< Source address. */
	uint16_t ether_type;      /**< Frame type. */
} __attribute__((__packed__));

#define RTE_STATIC_BSWAP16(v) \
	((((uint16_t)(v) & UINT16_C(0x00ff)) << 8) | \
	 (((uint16_t)(v) & UINT16_C(0xff00)) >> 8))

static inline uint16_t
rte_constant_bswap16(uint16_t x)
{
	return RTE_STATIC_BSWAP16(x);
}


#define rte_bswap16(x) ((uint16_t)(__builtin_constant_p(x) ?		\
				   rte_constant_bswap16(x) :		\
				   rte_arch_bswap16(x)))

#define rte_be_to_cpu_16(x) rte_bswap16(x)

static inline uint16_t rte_arch_bswap16(uint16_t _x)
{
	register uint16_t x = _x;
	asm volatile ("xchgb %b[x1],%h[x2]"
		      : [x1] "=Q" (x)
		      : [x2] "0" (x)
		      );
	return x;
}





static inline void
parse_ethernet(struct rte_mbuf *m, uint16_t *l4_proto, void **l4_hdr)
{
	struct ipv4_hdr *ipv4_hdr;
	struct ipv6_hdr *ipv6_hdr;
	void *l3_hdr = NULL;
	struct ether_hdr *eth_hdr;
	uint16_t ethertype;

	eth_hdr = rte_pktmbuf_mtod(m, struct ether_hdr *);

	m->l2_len = sizeof(struct ether_hdr);
	ethertype = rte_be_to_cpu_16(eth_hdr->ether_type);

	if (ethertype == ETHER_TYPE_VLAN) {
		struct vlan_hdr *vlan_hdr = (struct vlan_hdr *)(eth_hdr + 1);

		m->l2_len += sizeof(struct vlan_hdr);
		ethertype = rte_be_to_cpu_16(vlan_hdr->eth_proto);
	}

	l3_hdr = (char *)eth_hdr + m->l2_len;

	switch (ethertype) {
	case ETHER_TYPE_IPv4:
		ipv4_hdr = l3_hdr;
		*l4_proto = ipv4_hdr->next_proto_id;
		m->l3_len = (ipv4_hdr->version_ihl & 0x0f) * 4;
		*l4_hdr = (char *)l3_hdr + m->l3_len;
		m->ol_flags |= PKT_TX_IPV4;
		break;
	case ETHER_TYPE_IPv6:
		ipv6_hdr = l3_hdr;
		*l4_proto = ipv6_hdr->proto;
		m->l3_len = sizeof(struct ipv6_hdr);
		*l4_hdr = (char *)l3_hdr + m->l3_len;
		m->ol_flags |= PKT_TX_IPV6;
		break;
	default:
		m->l3_len = 0;
		*l4_proto = 0;
		*l4_hdr = NULL;
		break;
	}
}


static inline void
vhost_dequeue_offload(struct virtio_net_hdr *hdr, struct rte_mbuf *m)
{
	uint16_t l4_proto = 0;
	void *l4_hdr = NULL;
	struct tcp_hdr *tcp_hdr = NULL;

	if (hdr->flags == 0 && hdr->gso_type == VIRTIO_NET_HDR_GSO_NONE)
		return;

	parse_ethernet(m, &l4_proto, &l4_hdr);
	if (hdr->flags == VIRTIO_NET_HDR_F_NEEDS_CSUM) {
		if (hdr->csum_start == (m->l2_len + m->l3_len)) {
			switch (hdr->csum_offset) {
			case (offsetof(struct tcp_hdr, cksum)):
				if (l4_proto == IPPROTO_TCP)
					m->ol_flags |= PKT_TX_TCP_CKSUM;
				break;
			case (offsetof(struct udp_hdr, dgram_cksum)):
				if (l4_proto == IPPROTO_UDP)
					m->ol_flags |= PKT_TX_UDP_CKSUM;
				break;
			case (offsetof(struct sctp_hdr, cksum)):
				if (l4_proto == IPPROTO_SCTP)
					m->ol_flags |= PKT_TX_SCTP_CKSUM;
				break;
			default:
				break;
			}
		}
	}

	if (l4_hdr && hdr->gso_type != VIRTIO_NET_HDR_GSO_NONE) {
		switch (hdr->gso_type & ~VIRTIO_NET_HDR_GSO_ECN) {
		case VIRTIO_NET_HDR_GSO_TCPV4:
		case VIRTIO_NET_HDR_GSO_TCPV6:
			tcp_hdr = l4_hdr;
			m->ol_flags |= PKT_TX_TCP_SEG;
			m->tso_segsz = hdr->gso_size;
			m->l4_len = (tcp_hdr->data_off & 0xf0) >> 2;
			break;
		default:
		//	RTE_LOG(WARNING, VHOST_DATA,
		//		"unsupported gso type %u.\n", hdr->gso_type);
			break;
		}
	}
}




#define	rte_compiler_barrier() do {		\
	asm volatile ("" : : : "memory");	\
} while(0)


#define rte_smp_wmb() rte_compiler_barrier()
#define rte_smp_rmb() rte_compiler_barrier()

#define MAX_BATCH_LEN 256

static inline void
do_data_copy_dequeue(struct vhost_dev *dev,struct virtio_virtqueue *vq)
{
	struct batch_copy_elem *elem = vq->batch_copy_elems;
	uint16_t count = vq->batch_copy_nb_elems;
	int i;

	for (i = 0; i < count; i++) {
//		if(elem[i].len>=512)
//		{
			//spdk_ioat_submit_copy(dev->dev_ioat, dev, ioat_done, elem[i].dst, elem[i].src, elem[i].len);
			//dev->ioat_submit++;
//		}
//		else
			//rte_memcpy(elem[i].dst, elem[i].src, elem[i].len);
		rte_memcpy(elem[i].dst, elem[i].src, elem[i].len);
		
		//vhost_log_write(dev, elem[i].log_addr, elem[i].len);
		//PRINT_PACKET(dev, (uintptr_t)elem[i].dst, elem[i].len, 0);
	}
	//sched_yield();

//	while(dev->ioat_submit>0)
//	{
//		spdk_ioat_process_events(dev->dev_ioat);
//	}

}


static inline void
update_used_idx(struct vhost_dev *dev, struct virtio_virtqueue *vi_vq, struct rte_mbuf **pkts,
		uint32_t count)
{
	if (unlikely(count == 0))
		return;

	rte_smp_wmb();
	rte_smp_rmb();

	vi_vq->used->idx += count;
	//vhost_log_used_vring(dev, vq, offsetof(struct vring_used, idx),
	//		sizeof(vq->used->idx));
	rte_ring_sp_enqueue_burst(dev->vhost_vring[1],(void **)pkts,count,NULL);

	/* Kick guest if required. */
	if (!(vi_vq->avail->flags & VRING_AVAIL_F_NO_INTERRUPT)
			&& (vi_vq->callfd >= 0))
		eventfd_write(vi_vq->callfd, (eventfd_t)1);
}



static inline int
copy_desc_to_mbuf(struct vhost_dev *dev, struct virtio_virtqueue *vq,
          struct vring_desc *descs, uint16_t max_desc,
          struct rte_mbuf *m, uint16_t desc_idx,
          struct rte_mempool *mbuf_pool)
{
    struct vring_desc *desc;
    uint64_t desc_addr, desc_gaddr;
    uint32_t desc_avail, desc_offset;
    uint32_t mbuf_avail, mbuf_offset;
    uint32_t cpy_len;
    uint64_t desc_chunck_len;
    struct rte_mbuf *cur = m, *prev = m;
    struct virtio_net_hdr tmp_hdr;
    struct virtio_net_hdr *hdr = NULL;
    /* A counter to avoid desc dead loop chain */
    uint32_t nr_desc = 1;
    struct batch_copy_elem *batch_copy = vq->batch_copy_elems;
    uint16_t copy_nb = vq->batch_copy_nb_elems;
    int error = 0;

    desc = &descs[desc_idx];
    if (unlikely((desc->len < dev->vhost_hlen)) ||
            (desc->flags & VRING_DESC_F_INDIRECT)) {
        error = -1;
        goto out;
    }

    desc_chunck_len = desc->len;
    desc_gaddr = desc->addr;
    ///////////////////////////////////////////////////////////
    desc_addr = rte_vhost_gpa_to_qva(dev,desc_gaddr);
    if (unlikely(!desc_addr)) {
        error = -1;
        goto out;
    }

    if (virtio_net_with_host_offload(dev)) {
        if (unlikely(desc_chunck_len < sizeof(struct virtio_net_hdr))) {
            uint64_t len = desc_chunck_len;
            uint64_t remain = sizeof(struct virtio_net_hdr);
            uint64_t src = desc_addr;
            uint64_t dst = (uint64_t)(uintptr_t)&tmp_hdr;
            uint64_t guest_addr = desc_gaddr;

            /*
             * No luck, the virtio-net header doesn't fit
             * in a contiguous virtual area.
             */
            while (remain) {
                len = remain;
                //////////////////////////////////////
                src = rte_vhost_gpa_to_qva(dev,guest_addr);
                if (unlikely(!src || !len)) {
                    error = -1;
                    goto out;
                }

                rte_memcpy((void *)(uintptr_t)dst,
                           (void *)(uintptr_t)src, len);

                guest_addr += len;
                remain -= len;
                dst += len;
            }

            hdr = &tmp_hdr;
        } else {
            hdr = (struct virtio_net_hdr *)((uintptr_t)desc_addr);
            rte_prefetch0(hdr);
        }
    }

    /*
     * A virtio driver normally uses at least 2 desc buffers
     * for Tx: the first for storing the header, and others
     * for storing the data.
     */
    if (likely((desc->len == dev->vhost_hlen) &&
           (desc->flags & VRING_DESC_F_NEXT) != 0)) {
        desc = &descs[desc->next];
        if (unlikely(desc->flags & VRING_DESC_F_INDIRECT)) {
            error = -1;
            goto out;
        }

        desc_chunck_len = desc->len;
        desc_gaddr = desc->addr;
        ////////////////////////////////////////////////////
        desc_addr = rte_vhost_gpa_to_qva(dev,desc_gaddr);
        if (unlikely(!desc_addr)) {
            error = -1;
            goto out;
        }

        desc_offset = 0;
        desc_avail  = desc->len;
        nr_desc    += 1;
    } else {
        desc_avail  = desc->len - dev->vhost_hlen;

        if (unlikely(desc_chunck_len < dev->vhost_hlen)) {
            desc_chunck_len = desc_avail;
            desc_gaddr += dev->vhost_hlen;
            ///////////////////////////////////////////////////////
            desc_addr = rte_vhost_gpa_to_qva(dev,desc_gaddr);
            if (unlikely(!desc_addr)) {
                error = -1;
                goto out;
            }

            desc_offset = 0;
        } else {
            desc_offset = dev->vhost_hlen;
            desc_chunck_len -= dev->vhost_hlen;
        }
    }

    rte_prefetch0((void *)(uintptr_t)(desc_addr + desc_offset));

    //PRINT_PACKET(dev, (uintptr_t)(desc_addr + desc_offset),
    //        desc_chunck_len, 0);

    mbuf_offset = 0;
    mbuf_avail  = m->buf_len - RTE_PKTMBUF_HEADROOM;
    while (1) {
        //uint64_t hpa;

        cpy_len = MIN(desc_chunck_len, mbuf_avail);

        /*
         * A desc buf might across two host physical pages that are
         * not continuous. In such case (gpa_to_hpa returns 0), data
         * will be copied even though zero copy is enabled.
         */
        if (likely(cpy_len > MAX_BATCH_LEN ||
               copy_nb >= vq->size ||
               (hdr && cur == m) ||
               desc->len != desc_chunck_len)) {
            rte_memcpy(rte_pktmbuf_mtod_offset(cur, void *,
                               mbuf_offset),
                   (void *)((uintptr_t)(desc_addr +
                            desc_offset)),
                   cpy_len);
        } else {
            batch_copy[copy_nb].dst =
                rte_pktmbuf_mtod_offset(cur, void *,
                            mbuf_offset);
            batch_copy[copy_nb].src =
                (void *)((uintptr_t)(desc_addr +
                             desc_offset));
            batch_copy[copy_nb].len = cpy_len;
            copy_nb++;
        }
   

        mbuf_avail  -= cpy_len;
        mbuf_offset += cpy_len;
        desc_avail  -= cpy_len;
        desc_chunck_len -= cpy_len;
        desc_offset += cpy_len;

        /* This desc reaches to its end, get the next one */
        if (desc_avail == 0) {
            if ((desc->flags & VRING_DESC_F_NEXT) == 0)
                break;

            if (unlikely(desc->next >= max_desc ||
                     ++nr_desc > max_desc)) {
                error = -1;
                goto out;
            }
            desc = &descs[desc->next];
            if (unlikely(desc->flags & VRING_DESC_F_INDIRECT)) {
                error = -1;
                goto out;
            }

            desc_chunck_len = desc->len;
            desc_gaddr = desc->addr;
            ////////////////////////////////////////////////////
            desc_addr = rte_vhost_gpa_to_qva(dev,desc_gaddr);
            if (unlikely(!desc_addr)) {
                error = -1;
                goto out;
            }

            rte_prefetch0((void *)(uintptr_t)desc_addr);

            desc_offset = 0;
            desc_avail  = desc->len;

           // PRINT_PACKET(dev, (uintptr_t)desc_addr,
           //         desc_chunck_len, 0);
        } else if (unlikely(desc_chunck_len == 0)) {
            desc_chunck_len = desc_avail;
            desc_gaddr += desc_offset;
            /////////////////////////////////////////////////////////
            desc_addr = rte_vhost_gpa_to_qva(dev,desc_gaddr);
            if (unlikely(!desc_addr)) {
                error = -1;
                goto out;
            }
            desc_offset = 0;

            //PRINT_PACKET(dev, (uintptr_t)desc_addr,
            //       desc_chunck_len, 0);
        }

        /*
         * This mbuf reaches to its end, get a new one
         * to hold more data.
         */
        if (mbuf_avail == 0) {
            cur = rte_pktmbuf_alloc(mbuf_pool);
            if (unlikely(cur == NULL)) {
                //RTE_LOG(ERR, VHOST_DATA, "Failed to "
                //    "allocate memory for mbuf.\n");
                error = -1;
                goto out;
            }
            //if (unlikely(dev->dequeue_zero_copy))
            //    rte_mbuf_refcnt_update(cur, 1);

            prev->next = cur;
            prev->data_len = mbuf_offset;
            m->nb_segs += 1;
            m->pkt_len += mbuf_offset;
            prev = cur;

            mbuf_offset = 0;
            mbuf_avail  = cur->buf_len - RTE_PKTMBUF_HEADROOM;
        }
    }

    prev->data_len = mbuf_offset;
    m->pkt_len    += mbuf_offset;

    if (hdr)
        vhost_dequeue_offload(hdr, m);

out:
    vq->batch_copy_nb_elems = copy_nb;

    return error;
}

static inline void
update_used_ring(struct vhost_dev *dev, struct virtio_virtqueue *vq,
		uint32_t used_idx, uint32_t desc_idx)
{
	vq->used->ring[used_idx].id  = desc_idx;
	vq->used->ring[used_idx].len = 0;
}


//虚拟机发包，后端收包，vq[1]
static inline int vdev_dequeue(struct vhost_dev *hdev,uint16_t count)
{
struct virtio_virtqueue *vi_vq=hdev->virtio_vq[1];
	struct rte_mempool *mbuf_pool=hdev->mempool;
	struct rte_mbuf *pkts[MAX_PKT_BURST];
	
	//uint16_t count;

	int i;
	
	vi_vq->batch_copy_nb_elems = 0;

	//int free_vh_entries=rte_ring_free_count(hdev->vhost_vring[1]);

	/*if (unlikely(free_vh_entries==0))
	{
		return 0;
	}*/

	//int free_vi_entries;

	/*free_vi_entries = *((volatile uint16_t *)&vi_vq->avail->idx) -
			vi_vq->last_avail_idx;*

	if(unlikely(free_vi_entries==0))
	{
		return 0;
	}*/
	
	//if(vh_vq->tail<vh_vq->head)
    //{
    //    free_vh_entries = vh_vq->head - vh_vq->tail;
    //}
    //else
    //{
    //    free_vh_entries = vh_vq->head + MAX_VRING_SIZE - vh_vq->tail;
    //}

	//count=MIN(free_vi_entries,free_vh_entries);

	//count=MIN(free_vi_entries,free_vh_entries);
	//count=MIN(count,MAX_PKT_BURST);

	uint32_t desc_indexes[MAX_PKT_BURST];
	
	uint32_t used_idx;

	uint16_t avail_idx;	

	
	avail_idx = vi_vq->last_avail_idx & (vi_vq->size - 1);
	used_idx  = vi_vq->last_used_idx  & (vi_vq->size - 1);
	rte_prefetch0(&vi_vq->avail->ring[avail_idx]);
	rte_prefetch0(&vi_vq->used->ring[used_idx]);
	
	for (i = 0; i < count; i++) {
		avail_idx = (vi_vq->last_avail_idx + i) & (vi_vq->size - 1);
		used_idx  = (vi_vq->last_used_idx  + i) & (vi_vq->size - 1);
		desc_indexes[i] = vi_vq->avail->ring[avail_idx];

		update_used_ring(hdev, vi_vq, used_idx, desc_indexes[i]);
	}
	
	rte_prefetch0(&vi_vq->desc[desc_indexes[0]]);

	for (i = 0; i < count; i++) {
		struct vring_desc *desc, *idesc = NULL;
		uint16_t sz, idx;
		uint64_t dlen;
		int err;

		if (likely(i + 1 < count))
			rte_prefetch0(&vi_vq->desc[desc_indexes[i + 1]]); //预取后一项
//////////////////////////////////////////////////////////////////////////////////////////////////////
		if (vi_vq->desc[desc_indexes[i]].flags & VRING_DESC_F_INDIRECT) {  //如果该项支持indirect desc，按照indirect处理
			dlen = vi_vq->desc[desc_indexes[i]].len;
			desc = (struct vring_desc *)(uintptr_t)     //地址转换成vva
				rte_vhost_gpa_to_qva(hdev,vi_vq->desc[desc_indexes[i]].addr);
			if (unlikely(!desc))
				break;

			if (unlikely(dlen < vi_vq->desc[desc_indexes[i]].len)) {
				/*
				 * The indirect desc table is not contiguous
				 * in process VA space, we have to copy it.
				 */
				idesc = alloc_copy_ind_table(hdev, vi_vq,
						&vi_vq->desc[desc_indexes[i]]);
				if (unlikely(!idesc))
					break;

				desc = idesc;
			}

			rte_prefetch0(desc);   //预取数据包
			sz = vi_vq->desc[desc_indexes[i]].len / sizeof(*desc);
			idx = 0;
		} 
		else {
			desc = vi_vq->desc;    //desc数组
			sz = vi_vq->size;		//size，个数
			idx = desc_indexes[i];	//desc索引项
		}

		pkts[i] = rte_pktmbuf_alloc(mbuf_pool);   //给正在处理的这个数据包分配mbuf
		if (unlikely(pkts[i] == NULL)) {	//分配mbuf失败，跳出包处理
			//RTE_LOG(ERR, VHOST_DATA,
			//	"Failed to allocate memory for mbuf.\n");
			free(idesc);
			break;
		}

		err = copy_desc_to_mbuf(hdev, vi_vq, desc, sz, pkts[i], idx,
					mbuf_pool);
		if (unlikely(err)) {
			rte_pktmbuf_free(pkts[i]);
			free(idesc);
			break;
		}

		
		if (unlikely(!!idesc))
			free(idesc);
	}
	vi_vq->last_avail_idx += i;

	do_data_copy_dequeue(hdev,vi_vq);
	vi_vq->last_used_idx += i;
	update_used_idx(hdev, vi_vq, pkts, i);  //更新used ring的当前索引，并eventfd通知虚拟机接收完成

	//printf("frontend send %d packets.\n",i);

//out:
//	if (dev->features & (1ULL << VIRTIO_F_IOMMU_PLATFORM))
//		vhost_user_iotlb_rd_unlock(vq);

//out_access_unlock:
//	rte_spinlock_unlock(&vq->access_lock);

//	if (unlikely(rarp_mbuf != NULL)) {	//再次检查有arp报文需要发送的话，就加入到pkts数组首位，虚拟交换机的mac学习表就能第一时间更新
		/*
		 * Inject it to the head of "pkts" array, so that switch's mac
		 * learning table will get updated first.
		 */
//		memmove(&pkts[1], pkts, i * sizeof(struct rte_mbuf *));
//		pkts[0] = rarp_mbuf;
//		i += 1;
//	}

	return i;
	
	
	
}

struct buf_vector {
	uint64_t buf_addr;
	uint32_t buf_len;
	uint32_t desc_idx;
};

#define BUF_VECTOR_MAX 256

static inline int
fill_vec_buf(struct vhost_dev *dev, struct virtio_virtqueue *vq,
			 uint32_t avail_idx, uint32_t *vec_idx,
			 struct buf_vector *buf_vec, uint16_t *desc_chain_head,
			 uint16_t *desc_chain_len)
{
	uint16_t idx = vq->avail->ring[avail_idx & (vq->size - 1)];
	uint32_t vec_id = *vec_idx;
	uint32_t len    = 0;
	uint64_t dlen;
	struct vring_desc *descs = vq->desc;
	struct vring_desc *idesc = NULL;

	*desc_chain_head = idx;

	if (vq->desc[idx].flags & VRING_DESC_F_INDIRECT) {
		dlen = vq->desc[idx].len;
		/////////////////////////////////////////////////////////////////
		descs = (struct vring_desc *)(uintptr_t)
			rte_vhost_gpa_to_qva(dev,vq->desc[idx].addr);
		if (unlikely(!descs))
			return -1;

		if (unlikely(dlen < vq->desc[idx].len)) {
			/*
			 * The indirect desc table is not contiguous
			 * in process VA space, we have to copy it.
			 */
			idesc = alloc_copy_ind_table(dev, vq, &vq->desc[idx]);
			if (unlikely(!idesc))
				return -1;

			descs = idesc;
		}

		idx = 0;
	}

	while (1) {
		if (unlikely(vec_id >= BUF_VECTOR_MAX || idx >= vq->size)) {
			free(idesc);
			return -1;
		}

		len += descs[idx].len;
		buf_vec[vec_id].buf_addr = descs[idx].addr;
		buf_vec[vec_id].buf_len  = descs[idx].len;
		buf_vec[vec_id].desc_idx = idx;
		vec_id++;

		if ((descs[idx].flags & VRING_DESC_F_NEXT) == 0)
			break;

		idx = descs[idx].next;
	}

	*desc_chain_len = len;
	*vec_idx = vec_id;

	if (unlikely(!!idesc))
		free(idesc);

	return 0;
}

static inline void
update_shadow_used_ring(struct virtio_virtqueue *vq,
			uint16_t desc_idx, uint16_t len)
{
	uint16_t i = vq->shadow_used_idx++;
			 
	vq->shadow_used_ring[i].id  = desc_idx;
	vq->shadow_used_ring[i].len = len;
}


static inline int
reserve_avail_buf_mergeable(struct vhost_dev *dev, struct virtio_virtqueue *vq,
				uint32_t size, struct buf_vector *buf_vec,
				uint16_t *num_buffers, uint16_t avail_head)
{
	uint16_t cur_idx;
	uint32_t vec_idx = 0;
	uint16_t tries = 0;

	uint16_t head_idx = 0;
	uint16_t len = 0;

	*num_buffers = 0;
	cur_idx  = vq->last_avail_idx;

	while (size > 0) {
		if (unlikely(cur_idx == avail_head))
			return -1;

		if (unlikely(fill_vec_buf(dev, vq, cur_idx, &vec_idx, buf_vec,
						&head_idx, &len) < 0))
			return -1;
		len = MIN(len, size);
		update_shadow_used_ring(vq, head_idx, len);
		size -= len;

		cur_idx++;
		tries++;
		*num_buffers += 1;

		/*
		 * if we tried all available ring items, and still
		 * can't get enough buf, it means something abnormal
		 * happened.
		 */
		if (unlikely(tries >= vq->size))
			return -1;
	}

	return 0;
}

#define ASSIGN_UNLESS_EQUAL(var, val) do {	\
	if ((var) != (val)) 		\
		(var) = (val);			\
} while (0)

static inline uint32_t
__rte_raw_cksum(const void *buf, size_t len, uint32_t sum)
{
	/* workaround gcc strict-aliasing warning */
	uintptr_t ptr = (uintptr_t)buf;
	typedef uint16_t __attribute__((__may_alias__)) u16_p;
	const u16_p *u16 = (const u16_p *)ptr;
	
	while (len >= (sizeof(*u16) * 4)) {
		sum += u16[0];
		sum += u16[1];
		sum += u16[2];
		sum += u16[3];
		len -= sizeof(*u16) * 4;
		u16 += 4;
	}
	while (len >= sizeof(*u16)) {
		sum += *u16;
		len -= sizeof(*u16);
		u16 += 1;
	}
	
	/* if length is in odd bytes */
	if (len == 1)
		sum += *((const uint8_t *)u16);
	
	return sum;
}

static inline uint16_t
__rte_raw_cksum_reduce(uint32_t sum)
{
	sum = ((sum & 0xffff0000) >> 16) + (sum & 0xffff);
	sum = ((sum & 0xffff0000) >> 16) + (sum & 0xffff);
	return (uint16_t)sum;
}


static inline uint16_t
rte_raw_cksum(const void *buf, size_t len)
{
	uint32_t sum;
	
	sum = __rte_raw_cksum(buf, len, 0);
	return __rte_raw_cksum_reduce(sum);
}


static inline uint16_t
rte_ipv4_cksum(const struct ipv4_hdr *ipv4_hdr)
{
	uint16_t cksum;
	cksum = rte_raw_cksum(ipv4_hdr, sizeof(struct ipv4_hdr));
	return (cksum == 0xffff) ? cksum : ~cksum;
}


static inline void
virtio_enqueue_offload(struct rte_mbuf *m_buf, struct virtio_net_hdr *net_hdr)
{
    uint64_t csum_l4 = m_buf->ol_flags & PKT_TX_L4_MASK;

    if (m_buf->ol_flags & PKT_TX_TCP_SEG)
        csum_l4 |= PKT_TX_TCP_CKSUM;

    if (csum_l4) {
        net_hdr->flags = VIRTIO_NET_HDR_F_NEEDS_CSUM;
        net_hdr->csum_start = m_buf->l2_len + m_buf->l3_len;

        switch (csum_l4) {
        case PKT_TX_TCP_CKSUM:
            net_hdr->csum_offset = (offsetof(struct tcp_hdr,
                        cksum));
            break;
        case PKT_TX_UDP_CKSUM:
            net_hdr->csum_offset = (offsetof(struct udp_hdr,
                        dgram_cksum));
            break;
        case PKT_TX_SCTP_CKSUM:
            net_hdr->csum_offset = (offsetof(struct sctp_hdr,
                        cksum));
            break;
        }
    } else {
        ASSIGN_UNLESS_EQUAL(net_hdr->csum_start, 0);
        ASSIGN_UNLESS_EQUAL(net_hdr->csum_offset, 0);
        ASSIGN_UNLESS_EQUAL(net_hdr->flags, 0);
    }

    /* IP cksum verification cannot be bypassed, then calculate here */
    if (m_buf->ol_flags & PKT_TX_IP_CKSUM) {
        struct ipv4_hdr *ipv4_hdr;

        ipv4_hdr = rte_pktmbuf_mtod_offset(m_buf, struct ipv4_hdr *,
                           m_buf->l2_len);
        ipv4_hdr->hdr_checksum = rte_ipv4_cksum(ipv4_hdr);
    }

    if (m_buf->ol_flags & PKT_TX_TCP_SEG) {
        if (m_buf->ol_flags & PKT_TX_IPV4)
            net_hdr->gso_type = VIRTIO_NET_HDR_GSO_TCPV4;
        else
            net_hdr->gso_type = VIRTIO_NET_HDR_GSO_TCPV6;
        net_hdr->gso_size = m_buf->tso_segsz;
        net_hdr->hdr_len = m_buf->l2_len + m_buf->l3_len
                    + m_buf->l4_len;
    } else {
        ASSIGN_UNLESS_EQUAL(net_hdr->gso_type, 0);
        ASSIGN_UNLESS_EQUAL(net_hdr->gso_size, 0);
        ASSIGN_UNLESS_EQUAL(net_hdr->hdr_len, 0);
    }
}


static inline int
copy_mbuf_to_desc_mergeable(struct vhost_dev *dev, struct virtio_virtqueue *vq,
                struct rte_mbuf *m, struct buf_vector *buf_vec,
                uint16_t num_buffers)
{
    uint32_t vec_idx = 0;
    uint64_t desc_addr, desc_gaddr;
    uint32_t mbuf_offset, mbuf_avail;
    uint32_t desc_offset, desc_avail;
    uint32_t cpy_len;
    uint64_t desc_chunck_len;
    uint64_t hdr_addr, hdr_phys_addr;
    struct rte_mbuf *hdr_mbuf;
    struct batch_copy_elem *batch_copy = vq->batch_copy_elems;
    struct virtio_net_hdr_mrg_rxbuf tmp_hdr, *hdr = NULL;
    uint16_t copy_nb = vq->batch_copy_nb_elems;
    int error = 0;

    if (unlikely(m == NULL)) {
        error = -1;
        goto out;
    }

    desc_chunck_len = buf_vec[vec_idx].buf_len;
    desc_gaddr = buf_vec[vec_idx].buf_addr;
    //////////////////////////////////////////////
    desc_addr = rte_vhost_gpa_to_qva(dev,desc_gaddr);
    if (buf_vec[vec_idx].buf_len < dev->vhost_hlen || !desc_addr) {
        error = -1;
        goto out;
    }

    hdr_mbuf = m;
    hdr_addr = desc_addr;
    if (unlikely(desc_chunck_len < dev->vhost_hlen))
        hdr = &tmp_hdr;
    else
        hdr = (struct virtio_net_hdr_mrg_rxbuf *)(uintptr_t)hdr_addr;
    hdr_phys_addr = desc_gaddr;
    rte_prefetch0((void *)(uintptr_t)hdr_addr);

    //LOG_DEBUG(VHOST_DATA, "(%d) RX: num merge buffers %d\n",
    //    dev->vid, num_buffers);

    desc_avail  = buf_vec[vec_idx].buf_len - dev->vhost_hlen;
    if (unlikely(desc_chunck_len < dev->vhost_hlen)) {
        desc_chunck_len = desc_avail;
        desc_gaddr += dev->vhost_hlen;
        //////////////////////////////////////////////////////////////
        desc_addr = rte_vhost_gpa_to_qva(dev,desc_gaddr);
        if (unlikely(!desc_addr)) {
            error = -1;
            goto out;
        }

        desc_offset = 0;
    } else {
        desc_offset = dev->vhost_hlen;
        desc_chunck_len -= dev->vhost_hlen;
    }


    mbuf_avail  = rte_pktmbuf_data_len(m);
    //printf("mbuf_data_len=%d\n",mbuf_avail);
    mbuf_offset = 0;
    while (mbuf_avail != 0 || m->next != NULL) {
        /* done with current desc buf, get the next one */
        if (desc_avail == 0) {
            vec_idx++;
            desc_chunck_len = buf_vec[vec_idx].buf_len;
            desc_gaddr = buf_vec[vec_idx].buf_addr;
            /////////////////////////////////////////////////////////
            desc_addr =
                rte_vhost_gpa_to_qva(dev,desc_gaddr);
            if (unlikely(!desc_addr)) {
                error = -1;
                goto out;
            }

            /* Prefetch buffer address. */
            rte_prefetch0((void *)(uintptr_t)desc_addr);
            desc_offset = 0;
            desc_avail  = buf_vec[vec_idx].buf_len;
        } else if (unlikely(desc_chunck_len == 0)) {
            desc_chunck_len = desc_avail;
            desc_gaddr += desc_offset;
            //////////////////////////////////////////////////////////////
            desc_addr = rte_vhost_gpa_to_qva(dev,desc_gaddr);
            if (unlikely(!desc_addr)) {
                error = -1;
                goto out;
            }
            desc_offset = 0;
        }

        /* done with current mbuf, get the next one */
        if (mbuf_avail == 0) {
            m = m->next;

            mbuf_offset = 0;
            mbuf_avail  = rte_pktmbuf_data_len(m);
        }

        if (hdr_addr) {
            virtio_enqueue_offload(hdr_mbuf, &hdr->hdr);
            ASSIGN_UNLESS_EQUAL(hdr->num_buffers, num_buffers);

            if (unlikely(hdr == &tmp_hdr)) {
                uint64_t len;
                uint64_t remain = dev->vhost_hlen;
                uint64_t src = (uint64_t)(uintptr_t)hdr, dst;
                uint64_t guest_addr = hdr_phys_addr;

                while (remain) {
                    len = remain;
                    /////////////////////////////////////////
                    dst = rte_vhost_gpa_to_qva(dev,guest_addr);
                    if (unlikely(!dst || !len)) {
                        error = -1;
                        goto out;
                    }

                    rte_memcpy((void *)(uintptr_t)dst,
                            (void *)(uintptr_t)src,
                            len);

                 //   PRINT_PACKET(dev, (uintptr_t)dst,
                 //           len, 0);
                 //   vhost_log_write(dev, guest_addr, len);

                    remain -= len;
                    guest_addr += len;
                    dst += len;
                }
            } //else {
                //PRINT_PACKET(dev, (uintptr_t)hdr_addr,
                //        dev->vhost_hlen, 0);
                //vhost_log_write(dev, hdr_phys_addr,
                //        dev->vhost_hlen);
            //}

            hdr_addr = 0;
        }

        cpy_len = MIN(desc_chunck_len, mbuf_avail);

        if (likely(cpy_len > MAX_BATCH_LEN || copy_nb >= vq->size)) {
            rte_memcpy((void *)((uintptr_t)(desc_addr +
                            desc_offset)),
                rte_pktmbuf_mtod_offset(m, void *, mbuf_offset),
                cpy_len);
            //vhost_log_write(dev, desc_gaddr + desc_offset, cpy_len);
            //PRINT_PACKET(dev, (uintptr_t)(desc_addr + desc_offset),
            //    cpy_len, 0);
        } else {
            batch_copy[copy_nb].dst =
                (void *)((uintptr_t)(desc_addr + desc_offset));
            batch_copy[copy_nb].src =
                rte_pktmbuf_mtod_offset(m, void *, mbuf_offset);
            batch_copy[copy_nb].log_addr = desc_gaddr + desc_offset;
            batch_copy[copy_nb].len = cpy_len;
            copy_nb++;
        }

        mbuf_avail  -= cpy_len;
        mbuf_offset += cpy_len;
        desc_avail  -= cpy_len;
        desc_offset += cpy_len;
        desc_chunck_len -= cpy_len;
    }

out:
    vq->batch_copy_nb_elems = copy_nb;

    return error;
}

static inline void
do_data_copy_enqueue(struct vhost_dev *dev, struct virtio_virtqueue *vq)
{
	struct batch_copy_elem *elem = vq->batch_copy_elems;
	uint16_t count = vq->batch_copy_nb_elems;
	int i;
				
	for (i = 0; i < count; i++) {
//		if(elem[i].len>=512)
//		{
//			spdk_ioat_submit_copy(dev->dev_ioat, dev, ioat_done, elem[i].dst, elem[i].src, elem[i].len);
//			dev->ioat_submit++;
//		}
//		else
			rte_memcpy(elem[i].dst, elem[i].src, elem[i].len);
		
		//vhost_log_write(dev, elem[i].log_addr, elem[i].len);
		//PRINT_PACKET(dev, (uintptr_t)elem[i].dst, elem[i].len, 0);
	}
	//sched_yield();
//	while(dev->ioat_submit>0)
//	{
//		spdk_ioat_process_events(dev->dev_ioat);
//	}

}

static inline void
do_flush_shadow_used_ring(struct vhost_dev *dev, struct virtio_virtqueue *vq,
			  uint16_t to, uint16_t from, uint16_t size)
{
	rte_memcpy(&vq->used->ring[to],
			&vq->shadow_used_ring[from],
			size * sizeof(struct vring_used_elem));
	//vhost_log_used_vring(dev, vq,
	//		offsetof(struct vring_used, ring[to]),
	//		size * sizeof(struct vring_used_elem));
}


static inline void
flush_shadow_used_ring(struct vhost_dev *dev, struct virtio_virtqueue *vq)
{
	uint16_t used_idx = vq->last_used_idx & (vq->size - 1);

	if (used_idx + vq->shadow_used_idx <= vq->size) {
		do_flush_shadow_used_ring(dev, vq, used_idx, 0,
					  vq->shadow_used_idx);
	} else {
		uint16_t size;

		/* update used ring interval [used_idx, vq->size] */
		size = vq->size - used_idx;
		do_flush_shadow_used_ring(dev, vq, used_idx, 0, size);

		/* update the left half used ring interval [0, left_size] */
		do_flush_shadow_used_ring(dev, vq, 0, size,
					  vq->shadow_used_idx - size);
	}
	vq->last_used_idx += vq->shadow_used_idx;

	rte_smp_wmb();

	*(volatile uint16_t *)&vq->used->idx += vq->shadow_used_idx;
	//vhost_log_used_vring(dev, vq, offsetof(struct vring_used, idx),
	//	sizeof(vq->used->idx));
}

//unsigned int counts = 0;
//unsigned int dequeue_num[2][257];
//unsigned int enqueue_num[2][257];

static inline int find_enqueuenum(struct vhost_dev *hdev)
{
	struct virtio_virtqueue *vi_vq=hdev->virtio_vq[0];
    uint16_t count = MAX_PKT_BURST;
    uint16_t free_vh_entries=rte_ring_count(hdev->vhost_vring[0]);
    uint16_t free_vi_entries = *((volatile uint16_t *)&vi_vq->avail->idx) -
		vi_vq->last_avail_idx;
    //printf("%u %u\n",free_vh_entries,free_vi_entries);
/*
	if(counts==10000000)
	{
		printf("--------------------------\n");
		for(int i=0;i<257;i++)
		{
			printf("%u %u\n", enqueue_num[0][i],enqueue_num[1][i]);
			enqueue_num[0][i]=0;
			enqueue_num[1][i]=0;
		}
		counts=0;
	}
	enqueue_num[0][free_vh_entries]++;
	enqueue_num[1][free_vi_entries]++;
	counts++;
*/
	count = RTE_MIN(count,free_vh_entries);
	count = RTE_MIN(count,free_vi_entries);
	return count;
}

static inline uint16_t find_dequeuenum(struct vhost_dev *hdev)
{
	struct virtio_virtqueue *vi_vq=hdev->virtio_vq[1];
    uint16_t count = MAX_PKT_BURST;
    uint16_t free_vh_entries=rte_ring_free_count(hdev->vhost_vring[1]);
    uint16_t free_vi_entries = *((volatile uint16_t *)&vi_vq->avail->idx) -
		vi_vq->last_avail_idx;
    //printf("%u %u\n",free_vh_entries,free_vi_entries);
	count = RTE_MIN(count,free_vh_entries);
	count = RTE_MIN(count,free_vi_entries);
	return count;
}

#define	rte_mb() _mm_mfence()

//虚拟机收包，后端发包，vq[0]
static inline int vdev_enqueue(struct vhost_dev *hdev,uint16_t count)
{

	struct virtio_virtqueue *vi_vq=hdev->virtio_vq[0];

	//uint16_t count=MAX_PKT_BURST;

	struct rte_mbuf *pkts[MAX_PKT_BURST];
    rte_ring_sc_dequeue_burst(hdev->vhost_vring[0],(void **)(&pkts),count,NULL);
	//int free_vh_entries=rte_ring_count(hdev->vhost_vring[0]);

	/*if (unlikely(free_vh_entries==0))
	{
        usleep(10);
		return 0;
	}*/

	//int free_vi_entries = *((volatile uint16_t *)&vi_vq->avail->idx) -
	//	vi_vq->last_avail_idx;

	//if(unlikely(free_vi_entries==0))
	//{
	//	return 0;
	//}

	//free_vh_entries=MIN(free_vh_entries,MAX_PKT_BURST);
	//count=MIN(free_vh_entries,free_vi_entries);
	//count=MIN(count,MAX_PKT_BURST);

	//int countreal=rte_ring_sc_dequeue_burst(hdev->vhost_vring[0],(void**)pkts,count,NULL);
	
	//uint32_t desc_indexes[MAX_PKT_BURST];
	
	uint32_t pkt_idx = 0;
	uint16_t num_buffers;
	struct buf_vector buf_vec[BUF_VECTOR_MAX];
	uint16_t avail_head;
        unsigned int county = 1; 


	vi_vq->batch_copy_nb_elems = 0;
	struct rte_ring * ppkt = hdev->vhost_vring[0];
	rte_prefetch0(&vi_vq->avail->ring[vi_vq->last_avail_idx & (vi_vq->size - 1)]);

	vi_vq->shadow_used_idx = 0;
	avail_head = *((volatile uint16_t *)&vi_vq->avail->idx);
	//void **vp = &pkt;
	for (pkt_idx = 0; pkt_idx < count; pkt_idx++) {
		//if (unlikely(rte_ring_count(hdev->vhost_vring[0]) == 0))
        //            break;
		//if(rte_ring_sc_dequeue(hdev->vhost_vring[0],&pkt)<0)
		//	break;
		//DEQUEUE_PTRS(ppkt,&ppkt[1],(ppkt->cons).head,vp,county,void *);
		uint32_t pkt_len = (pkts[pkt_idx])->pkt_len + hdev->vhost_hlen;

		
		//pkts[pkt_idx]=(struct rte_mbuf*)pkt;	
		//uint32_t pkt_len = pkts[pkt_idx]->pkt_len + hdev->vhost_hlen;

		if (unlikely(reserve_avail_buf_mergeable(hdev, vi_vq,
						pkt_len, buf_vec, &num_buffers,
						avail_head) < 0)) {
		//	LOG_DEBUG(VHOST_DATA,
		//		"(%d) failed to get enough desc from vring\n",
		//		dev->vid);
			vi_vq->shadow_used_idx -= num_buffers;
			break;
		}

		if (copy_mbuf_to_desc_mergeable(hdev, vi_vq, pkts[pkt_idx],
						buf_vec, num_buffers) < 0) {
			vi_vq->shadow_used_idx -= num_buffers;
			break;
		}
		
		vi_vq->last_avail_idx += num_buffers;
	}
	if(pkt_idx == 0)
	    usleep(10);


	do_data_copy_enqueue(hdev, vi_vq);

	if (likely(vi_vq->shadow_used_idx)) {
		flush_shadow_used_ring(hdev, vi_vq);
		
		/* flush used->idx update before we read avail->flags. */
		rte_mb();

		/* Kick the guest if necessary. */
		if (!(vi_vq->avail->flags & VRING_AVAIL_F_NO_INTERRUPT)
				&& (vi_vq->callfd >= 0))
			eventfd_write(vi_vq->callfd, (eventfd_t)1);
	}

//out:
//	if (dev->features & (1ULL << VIRTIO_F_IOMMU_PLATFORM))
//		vhost_user_iotlb_rd_unlock(vq);

//out_access_unlock:
//	rte_spinlock_unlock(&vq->access_lock);
	for (int i=0;i<pkt_idx;i++)
	{
		rte_pktmbuf_free(pkts[i]);
	}

	//printf("frontend received %d packets, but qemu send %d packets to guest.\n",count,pkt_idx);
	
	return pkt_idx;
}

#define DRAIN_TIME 1000
#define MAX_STIME 32

#define timesthrshold 5

void *packet_process_burst(void *arg)
{
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
        pthread_detach(pthread_self());
        //prctl(PR_SET_NAME,"packet");
        //struct sched_param param; 
        //int maxpri; 
        //maxpri = 50;
        //param.sched_priority = maxpri; 
        //if(sched_setscheduler(syscall(SYS_gettid), SCHED_FIFO, &param) == -1)
        //{
        //    perror("sched_setscheduler() failed"); 
        //    exit(1); 
        //}
	printf("copy thread %d running\n",syscall(SYS_gettid));
        struct vhost_dev *hdev=arg;
	uint16_t count_eq;
	uint16_t count_dq;
	uint16_t sleep_time;
	static uint16_t last_count_eq = 0;
	static uint16_t last_count_dq = 0;
	uint16_t enqueue_waittime = 0;
	uint16_t dequeue_waittime = 0;
        /*pthread_mutex_t *mptr;
       pthread_mutexattr_t mattr;
       key_t key_id = ftok(".",1);
       int shmid = shmget(key_id,sizeof(pthread_mutex_t) + sizeof(int),IPC_CREAT |  0644);
       mptr = shmat(shmid,NULL,0);*/
       //mptr=mmap(0,sizeof(pthread_mutex_t)+sizeof(int),PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
      /* int *val = (int *)(mptr + 1);
       //printf("%d",*val);
       if(*val == 0)
       {
            //printf("111");
            *val = 1;
            pthread_mutexattr_init(&mattr);
            pthread_mutexattr_setpshared(&mattr,PTHREAD_PROCESS_SHARED);
            pthread_mutex_init(mptr,&mattr);
       }*/
        

        while(1)
        {
		//sched_yield();
                //QLIST_FOREACH(hdev, &vhost_devices, entry)
                //{
                //pthread_mutex_lock(mptr);
                        if(hdev->ready==1)
                        {
/*
                                int i;
                                int count=MAX_PKT_BURST;
                                int free_dq_entries=rte_ring_free_count(hdev->vhost_vring[1]);
                                int free_eq_entries=rte_ring_count(hdev->vhost_vring[0]);
                                void *pkts[MAX_PKT_BURST];
                                count=MIN(count,free_dq_entries);
                                count=MIN(count,free_eq_entries);
                                rte_ring_sc_dequeue_burst(hdev->vhost_vring[0],(void **)pkts,count,NULL);
                                rte_ring_sp_enqueue_burst(hdev->vhost_vring[1],(void **)pkts,count,NULL);

*/

                                count_eq=find_enqueuenum(hdev);
                                count_dq=find_dequeuenum(hdev);
				if(count_eq==0 && count_dq==0)
				{
					goto yield;
					//sched_yield();
					//continue;
				}
				if(count_eq!=0)
				{
					vdev_enqueue(hdev,count_eq);
				}
				if(count_dq!=0)
                                {
                                        vdev_dequeue(hdev,count_dq);
                                }

                        }
                //}
	yield:                             
		//sched_yield();
                pthread_testcancel();
                //pthread_mutex_unlock(mptr);
		sched_yield();
        }
        return NULL;
}

/*
void *packet_process_burst_enqueue(void *arg)
{
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
        pthread_detach(pthread_self());
        printf("copy thread %d running\n",syscall(SYS_gettid));
        struct vhost_dev *hdev=arg;
        uint16_t count_eq;
        while(1)
        {
                //sched_yield();
                //QLIST_FOREACH(hdev, &vhost_devices, entry)
                //{
                        if(likely(hdev->ready==1))
                        {
                                count_eq=find_enqueuenum(hdev);
                                if(count_eq!=0)
                                {
                                        vdev_enqueue(hdev,count_eq);
				}
			}
			pthread_testcancel();
	}
	return NULL;
}

void * packet_process_burst_dequeue(void *arg)
{
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
        pthread_detach(pthread_self());
        printf("copy thread %d running\n",syscall(SYS_gettid));
        struct vhost_dev *hdev=arg;
        uint16_t count_dq;
        while(1)
        {
                //sched_yield();
                //QLIST_FOREACH(hdev, &vhost_devices, entry)
                //{
                        if(likely(hdev->ready==1))
                        {
                                count_dq=find_dequeuenum(hdev);
                                if(count_dq!=0)
                                {
                                        vdev_dequeue(hdev,count_dq);
                                }
                        }
                        pthread_testcancel();
        }
        return NULL;
}
*/
const VhostOps user_ops = {
        .backend_type = VHOST_BACKEND_TYPE_USER,
        .vhost_backend_init = vhost_user_init,
        .vhost_backend_cleanup = vhost_user_cleanup,
        .vhost_backend_memslots_limit = vhost_user_memslots_limit,
        .vhost_set_log_base = vhost_user_set_log_base,
        .vhost_set_mem_table = vhost_user_get_mem_table,
        .vhost_set_vring_addr = vhost_user_get_vring_addr,
        .vhost_set_vring_endian = vhost_user_set_vring_endian,
        .vhost_set_vring_num = vhost_user_set_vring_num,
        .vhost_set_vring_base = vhost_user_set_vring_base,
        .vhost_get_vring_base = vhost_user_get_vring_base,
        .vhost_set_vring_kick = vhost_user_set_vring_kick,
        .vhost_set_vring_call = vhost_user_set_vring_call,
        .vhost_set_features = vhost_user_set_features,
        .vhost_get_features = vhost_user_get_features,
        .vhost_set_owner = vhost_user_set_owner,
        .vhost_reset_device = vhost_user_reset_device,
        .vhost_get_vq_index = vhost_user_get_vq_index,
        .vhost_set_vring_enable = vhost_user_set_vring_enable,
        .vhost_requires_shm_log = vhost_user_requires_shm_log,
        .vhost_migration_done = vhost_user_migration_done,
        .vhost_backend_can_merge = vhost_user_can_merge,
        .vhost_net_set_mtu = vhost_user_net_set_mtu,
        .vhost_set_iotlb_callback = vhost_user_set_iotlb_callback,
        .vhost_send_device_iotlb_msg = vhost_user_send_device_iotlb_msg,
};
