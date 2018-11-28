#!/bin/bash
ovs-vsctl del-br br0
ovs-vsctl add-br br0 -- set bridge br0 datapath_type=netdev
ovs-vsctl add-port br0 dpdk-p0 -- set Interface dpdk-p0 type=dpdk options:dpdk-devargs=0000:01:00.0
ovs-vsctl add-port br0 dpdk-p1 -- set Interface dpdk-p1 type=dpdk options:dpdk-devargs=0000:01:00.1
ovs-vsctl add-port br0 vhost-user-1 -- set Interface vhost-user-1 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock0"
#ovs-vsctl add-port br0 vhost-user-2 -- set Interface vhost-user-2 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock1"
ovs-ofctl del-flows br0
#ovs-ofctl add-flow br0 "in_port=1,actions=output:2"
ovs-ofctl add-flow br0 "in_port=1,actions=output:3"
ovs-ofctl add-flow br0 "in_port=3,actions=output:2"
