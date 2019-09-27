#!/bin/bash
#export CFLAGS='-ggdb -O0'
./boot.sh && ./configure --with-dpdk=../dpdk-eal-master/build && make -j 16 && make install

