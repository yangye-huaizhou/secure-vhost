#!/bin/bash
ovs-vsctl del-br br0
ovs-vsctl add-br br0 -- set bridge br0 datapath_type=netdev
ovs-vsctl add-port br0 dpdk-p0 -- set Interface dpdk-p0 type=dpdk options:dpdk-devargs=0000:01:00.0
ovs-vsctl add-port br0 dpdk-p1 -- set Interface dpdk-p1 type=dpdk options:dpdk-devargs=0000:01:00.1
ovs-vsctl add-port br0 vhost-user-1 -- set Interface vhost-user-1 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock0"
ovs-vsctl add-port br0 vhost-user-2 -- set Interface vhost-user-2 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock1"
ovs-vsctl add-port br0 vhost-user-3 -- set Interface vhost-user-3 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock2"
ovs-vsctl add-port br0 vhost-user-4 -- set Interface vhost-user-4 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock3"
ovs-vsctl add-port br0 qdhcp tag=100 -- set interface qdhcp type=internal
ifconfig qdhcp up
ifconfig qdhcp 192.168.1.1 netmask 255.255.255.0
/usr/sbin/dnsmasq --strict-order --bind-interfaces --except-interface lo --interface qdhcp --dhcp-range 192.168.1.1,192.168.1.200 --dhcp-leasefile=/var/run/dnsmasq/qdhcp.pid --dhcp-lease-max=253 --dhcp-no-override --log-queries --log-facility=/tmp/dnsmasq.log
ovs-vsctl set port vhost-user-1 tag=100
ovs-vsctl set port vhost-user-2 tag=100
ovs-vsctl set port vhost-user-3 tag=100
ovs-vsctl set port vhost-user-4 tag=100
ovs-ofctl del-flows br0
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:01,actions=output:3"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:02,actions=output:4"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:03,actions=output:5"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:04,actions=output:6"
ovs-ofctl add-flow br0 "dl_dst=00:10:94:00:00:02,actions=output:2"
ovs-ofctl add-flow br0 "dl_dst=00:10:94:00:00:03,actions=output:1"
ovs-ofctl add-flow br0 "tun_src=0.0.0.0,actions=output:7"
