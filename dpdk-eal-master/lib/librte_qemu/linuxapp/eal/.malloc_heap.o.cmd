cmd_malloc_heap.o = gcc -Wp,-MD,./.malloc_heap.o.d.tmp  -m64 -pthread  -march=native -DRTE_MACHINE_CPUFLAG_SSE -DRTE_MACHINE_CPUFLAG_SSE2 -DRTE_MACHINE_CPUFLAG_SSE3 -DRTE_MACHINE_CPUFLAG_SSSE3 -DRTE_MACHINE_CPUFLAG_SSE4_1 -DRTE_MACHINE_CPUFLAG_SSE4_2 -DRTE_MACHINE_CPUFLAG_AES -DRTE_MACHINE_CPUFLAG_PCLMULQDQ -DRTE_MACHINE_CPUFLAG_AVX -DRTE_MACHINE_CPUFLAG_RDRAND -DRTE_MACHINE_CPUFLAG_FSGSBASE -DRTE_MACHINE_CPUFLAG_F16C  -I/home/yangye/dpdk-qemu/lib/librte_qemu/linuxapp/build/include -I/home/yangye/dpdk-qemu/x86_64-native-linuxapp-gcc/include -include /home/yangye/dpdk-qemu/x86_64-native-linuxapp-gcc/include/rte_config.h -I/eal/eal/include -I/home/yangye/dpdk-qemu/lib/librte_qemu/common -I/home/yangye/dpdk-qemu/lib/librte_qemu/common/include -W -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wold-style-definition -Wpointer-arith -Wcast-align -Wnested-externs -Wcast-qual -Wformat-nonliteral -Wformat-security -Wundef -Wwrite-strings -Werror -O3   -ggdb -O0 -o malloc_heap.o -c /home/yangye/dpdk-qemu/lib/librte_qemu/common/malloc_heap.c 
