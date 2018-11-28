#!/bin/bash
ovs-vsctl del-br br0
ovs-vsctl add-br br0 -- set bridge br0 datapath_type=netdev
ovs-vsctl add-port br0 dpdk-p0 -- set Interface dpdk-p0 type=dpdk options:dpdk-devargs=0000:01:00.0
ovs-vsctl add-port br0 dpdk-p1 -- set Interface dpdk-p1 type=dpdk options:dpdk-devargs=0000:01:00.1
ovs-vsctl add-port br0 vhost-user-1 -- set Interface vhost-user-1 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock0"
ovs-vsctl add-port br0 vhost-user-2 -- set Interface vhost-user-2 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock1"
ovs-vsctl add-port br0 vhost-user-3 -- set Interface vhost-user-3 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock2"
ovs-vsctl add-port br0 vhost-user-4 -- set Interface vhost-user-4 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock3"
ovs-vsctl add-port br0 vhost-user-5 -- set Interface vhost-user-5 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock4"
ovs-vsctl add-port br0 vhost-user-6 -- set Interface vhost-user-6 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock5"
ovs-vsctl add-port br0 vhost-user-7 -- set Interface vhost-user-7 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock6"
ovs-vsctl add-port br0 vhost-user-8 -- set Interface vhost-user-8 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock7"
ovs-vsctl add-port br0 vhost-user-9 -- set Interface vhost-user-9 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock8"
ovs-vsctl add-port br0 vhost-user-10 -- set Interface vhost-user-10 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock9"
ovs-vsctl add-port br0 vhost-user-11 -- set Interface vhost-user-11 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock10"
ovs-vsctl add-port br0 vhost-user-12 -- set Interface vhost-user-12 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock11"
ovs-vsctl add-port br0 vhost-user-13 -- set Interface vhost-user-13 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock12"
ovs-vsctl add-port br0 vhost-user-14 -- set Interface vhost-user-14 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock13"
ovs-vsctl add-port br0 vhost-user-15 -- set Interface vhost-user-15 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock14"
ovs-vsctl add-port br0 vhost-user-16 -- set Interface vhost-user-16 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock15"
ovs-vsctl add-port br0 qdhcp tag=100 -- set interface qdhcp type=internal
ifconfig qdhcp up
ifconfig qdhcp 192.168.1.1 netmask 255.255.255.0
/usr/sbin/dnsmasq --strict-order --bind-interfaces --except-interface lo --interface qdhcp --dhcp-range 192.168.1.1,192.168.1.200 --dhcp-leasefile=/var/run/dnsmasq/qdhcp.pid --dhcp-lease-max=253 --dhcp-no-override --log-queries --log-facility=/tmp/dnsmasq.log
ovs-vsctl set port vhost-user-1 tag=100
ovs-vsctl set port vhost-user-2 tag=100
ovs-vsctl set port vhost-user-3 tag=100
ovs-vsctl set port vhost-user-4 tag=100
ovs-vsctl set port vhost-user-5 tag=100
ovs-vsctl set port vhost-user-6 tag=100
ovs-vsctl set port vhost-user-7 tag=100
ovs-vsctl set port vhost-user-8 tag=100
ovs-vsctl set port vhost-user-9 tag=100
ovs-vsctl set port vhost-user-10 tag=100
ovs-vsctl set port vhost-user-11 tag=100
ovs-vsctl set port vhost-user-12 tag=100
ovs-vsctl set port vhost-user-13 tag=100
ovs-vsctl set port vhost-user-14 tag=100
ovs-vsctl set port vhost-user-15 tag=100
ovs-vsctl set port vhost-user-16 tag=100
ovs-ofctl del-flows br0
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:01,actions=output:3"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:02,actions=output:4"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:03,actions=output:5"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:04,actions=output:6"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:05,actions=output:7"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:06,actions=output:8"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:07,actions=output:9"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:08,actions=output:10"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:09,actions=output:11"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:0a,actions=output:12"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:0b,actions=output:13"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:0c,actions=output:14"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:0d,actions=output:15"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:0e,actions=output:16"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:0f,actions=output:17"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:10,actions=output:18"
ovs-ofctl add-flow br0 "dl_dst=00:10:94:00:00:02,actions=output:2"
ovs-ofctl add-flow br0 "dl_dst=00:10:94:00:00:03,actions=output:1"
ovs-ofctl add-flow br0 "tun_src=0.0.0.0,actions=output:19"
