#undef SPDK_CONFIG_RDMA
#undef SPDK_CONFIG_DEBUG
#define SPDK_CONFIG_PREFIX /usr/local
#define SPDK_CONFIG_VIRTIO 1
#undef SPDK_CONFIG_UBSAN
#undef SPDK_CONFIG_ASAN
#undef SPDK_CONFIG_COVERAGE
#define SPDK_FIO_SOURCE_DIR /usr/src/fio
#undef SPDK_CONFIG_FIO_PLUGIN
#undef SPDK_CONFIG_WERROR
#undef SPDK_CONFIG_RBD
#define SPDK_CONFIG_ENV $(SPDK_ROOT_DIR)/lib/env_dpdk
#undef SPDK_CONFIG_NVML
#undef SPDK_CONFIG_LTO
#undef SPDK_CONFIG_TSAN
#define SPDK_CONFIG_VHOST 1
#define SPDK_CONFIG_DPDK_DIR /home/yangye/secure-vhost/dpdk-eal-master/x86_64-native-linuxapp-gcc
