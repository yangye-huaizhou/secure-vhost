cmd_ef10_filter.o = gcc -Wp,-MD,./.ef10_filter.o.d.tmp  -m64 -pthread  -march=native -DRTE_MACHINE_CPUFLAG_SSE -DRTE_MACHINE_CPUFLAG_SSE2 -DRTE_MACHINE_CPUFLAG_SSE3 -DRTE_MACHINE_CPUFLAG_SSSE3 -DRTE_MACHINE_CPUFLAG_SSE4_1 -DRTE_MACHINE_CPUFLAG_SSE4_2 -DRTE_MACHINE_CPUFLAG_AES -DRTE_MACHINE_CPUFLAG_PCLMULQDQ -DRTE_MACHINE_CPUFLAG_AVX -DRTE_MACHINE_CPUFLAG_RDRAND -DRTE_MACHINE_CPUFLAG_FSGSBASE -DRTE_MACHINE_CPUFLAG_F16C  -I/home/lyz/multi/dpdk-eal-master/x86_64-native-linuxapp-gcc/include -include /home/lyz/multi/dpdk-eal-master/x86_64-native-linuxapp-gcc/include/rte_config.h -I/home/lyz/multi/dpdk-eal-master/drivers/net/sfc/base/ -I/home/lyz/multi/dpdk-eal-master/drivers/net/sfc -O3  -Wno-strict-aliasing -Wextra -Wdisabled-optimization -Waggregate-return -Wnested-externs -Wno-sign-compare -Wno-unused-parameter -Wno-unused-variable -Wno-empty-body -Wno-unused-but-set-variable  -ggdb -O0 -o ef10_filter.o -c /home/lyz/multi/dpdk-eal-master/drivers/net/sfc/base/ef10_filter.c 
