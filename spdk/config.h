#undef SPDK_CONFIG_ASAN
#define SPDK_CONFIG_ENV $(SPDK_ROOT_DIR)/lib/env_dpdk
#undef SPDK_CONFIG_DEBUG
#undef SPDK_CONFIG_RDMA
#define SPDK_CONFIG_VHOST 1
#undef SPDK_CONFIG_FIO_PLUGIN
#undef SPDK_CONFIG_COVERAGE
#define SPDK_CONFIG_PREFIX /usr/local
#define SPDK_CONFIG_DPDK_DIR /home/yangye/secure-vhost/dpdk-eal-master/build
#define SPDK_FIO_SOURCE_DIR /usr/src/fio
#undef SPDK_CONFIG_RBD
#undef SPDK_CONFIG_LTO
#undef SPDK_CONFIG_TSAN
#undef SPDK_CONFIG_UBSAN
#undef SPDK_CONFIG_NVML
#define SPDK_CONFIG_VIRTIO 1
#undef SPDK_CONFIG_WERROR
