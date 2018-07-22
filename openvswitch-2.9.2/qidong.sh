#!/bin/bash
export RTE_SDK=/home/yangye/formalvhost/dpdk-eal-master
export LD_LIBRARY_PATH=/home/yangye/formalvhost/dpdk-eal-master/x86_64-native-linuxapp-gcc/lib/
ovsdb-server --remote=punix:/usr/local/var/run/openvswitch/db.sock --remote=db:Open_vSwitch,Open_vSwitch,manager_options --pidfile --detach --log-file
ovs-vsctl --no-wait init
export DB_SOCK=/usr/local/var/run/openvswitch/db.sock
ovs-vsctl --no-wait set Open_vSwitch . other_config:dpdk-init=true
ovs-vsctl --no-wait set Open_vSwitch . other_config:dpdk-extra="--base-virtaddr 7ffe00000000 --file-prefix=spdk1"
#ovs-vsctl --no-wait set Open_vSwitch . other_config:dpdk-extra="--proc-type=secondary"
ovs-vsctl --no-wait set Open_vSwitch . other_config:dpdk-socket-mem="2048,0"
#ovs-vsctl --no-wait set Open_vSwitch . other_config:dpdk-extra=""
#ovs-vsctl --no-wait set Open_vSwitch . other_config:dpdk-socket-mem=""
ovs-vswitchd --pidfile --detach --log-file

