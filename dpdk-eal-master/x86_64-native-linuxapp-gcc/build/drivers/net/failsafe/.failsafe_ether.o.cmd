cmd_failsafe_ether.o = gcc -Wp,-MD,./.failsafe_ether.o.d.tmp  -m64 -pthread  -march=native -DRTE_MACHINE_CPUFLAG_SSE -DRTE_MACHINE_CPUFLAG_SSE2 -DRTE_MACHINE_CPUFLAG_SSE3 -DRTE_MACHINE_CPUFLAG_SSSE3 -DRTE_MACHINE_CPUFLAG_SSE4_1 -DRTE_MACHINE_CPUFLAG_SSE4_2 -DRTE_MACHINE_CPUFLAG_AES -DRTE_MACHINE_CPUFLAG_PCLMULQDQ -DRTE_MACHINE_CPUFLAG_AVX -DRTE_MACHINE_CPUFLAG_RDRAND -DRTE_MACHINE_CPUFLAG_FSGSBASE -DRTE_MACHINE_CPUFLAG_F16C  -I/home/lyz/multi/dpdk-eal-master/x86_64-native-linuxapp-gcc/include -include /home/lyz/multi/dpdk-eal-master/x86_64-native-linuxapp-gcc/include/rte_config.h -std=gnu99 -Wextra -O3 -I. -D_DEFAULT_SOURCE -D_XOPEN_SOURCE=700  -Wno-strict-prototypes -pedantic   -ggdb -O0 -o failsafe_ether.o -c /home/lyz/multi/dpdk-eal-master/drivers/net/failsafe/failsafe_ether.c 
