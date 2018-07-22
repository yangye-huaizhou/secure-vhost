#!/bin/bash
#export CFLAGS='-ggdb -O0'
./boot.sh && ./configure --with-dpdk=/home/yangye/vhostioat/dpdk-eal-master/x86_64-native-linuxapp-gcc && make -j 16 && make install

