cmd_eal_interrupts.o = gcc -Wp,-MD,./.eal_interrupts.o.d.tmp  -m64 -pthread  -march=native -DRTE_MACHINE_CPUFLAG_SSE -DRTE_MACHINE_CPUFLAG_SSE2 -DRTE_MACHINE_CPUFLAG_SSE3 -DRTE_MACHINE_CPUFLAG_SSSE3 -DRTE_MACHINE_CPUFLAG_SSE4_1 -DRTE_MACHINE_CPUFLAG_SSE4_2 -DRTE_MACHINE_CPUFLAG_AES -DRTE_MACHINE_CPUFLAG_PCLMULQDQ -DRTE_MACHINE_CPUFLAG_AVX -DRTE_MACHINE_CPUFLAG_RDRAND -DRTE_MACHINE_CPUFLAG_FSGSBASE -DRTE_MACHINE_CPUFLAG_F16C  -I/home/lyz/multi/dpdk-eal-master/x86_64-native-linuxapp-gcc/include -include /home/lyz/multi/dpdk-eal-master/x86_64-native-linuxapp-gcc/include/rte_config.h -I/home/lyz/multi/dpdk-eal-master/lib/librte_eal/linuxapp/eal/include -I/home/lyz/multi/dpdk-eal-master/lib/librte_eal/common -I/home/lyz/multi/dpdk-eal-master/lib/librte_eal/common/include  -O3 -D_GNU_SOURCE  -ggdb -O0 -o eal_interrupts.o -c /home/lyz/multi/dpdk-eal-master/lib/librte_eal/linuxapp/eal/eal_interrupts.c 
