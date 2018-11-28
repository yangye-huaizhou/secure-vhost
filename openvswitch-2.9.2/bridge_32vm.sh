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
ovs-vsctl add-port br0 vhost-user-17 -- set Interface vhost-user-17 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock16"
ovs-vsctl add-port br0 vhost-user-18 -- set Interface vhost-user-18 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock17"
ovs-vsctl add-port br0 vhost-user-19 -- set Interface vhost-user-19 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock18"
ovs-vsctl add-port br0 vhost-user-20 -- set Interface vhost-user-20 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock19"
ovs-vsctl add-port br0 vhost-user-21 -- set Interface vhost-user-21 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock20"
ovs-vsctl add-port br0 vhost-user-22 -- set Interface vhost-user-22 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock21"
ovs-vsctl add-port br0 vhost-user-23 -- set Interface vhost-user-23 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock22"
ovs-vsctl add-port br0 vhost-user-24 -- set Interface vhost-user-24 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock23"
ovs-vsctl add-port br0 vhost-user-25 -- set Interface vhost-user-25 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock24"
ovs-vsctl add-port br0 vhost-user-26 -- set Interface vhost-user-26 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock25"
ovs-vsctl add-port br0 vhost-user-27 -- set Interface vhost-user-27 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock26"
ovs-vsctl add-port br0 vhost-user-28 -- set Interface vhost-user-28 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock27"
ovs-vsctl add-port br0 vhost-user-29 -- set Interface vhost-user-29 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock28"
ovs-vsctl add-port br0 vhost-user-30 -- set Interface vhost-user-30 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock29"
ovs-vsctl add-port br0 vhost-user-31 -- set Interface vhost-user-31 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock30"
ovs-vsctl add-port br0 vhost-user-32 -- set Interface vhost-user-32 type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock31"
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
ovs-vsctl set port vhost-user-17 tag=100
ovs-vsctl set port vhost-user-18 tag=100
ovs-vsctl set port vhost-user-19 tag=100
ovs-vsctl set port vhost-user-20 tag=100
ovs-vsctl set port vhost-user-21 tag=100
ovs-vsctl set port vhost-user-22 tag=100
ovs-vsctl set port vhost-user-23 tag=100
ovs-vsctl set port vhost-user-24 tag=100
ovs-vsctl set port vhost-user-25 tag=100
ovs-vsctl set port vhost-user-26 tag=100
ovs-vsctl set port vhost-user-27 tag=100
ovs-vsctl set port vhost-user-28 tag=100
ovs-vsctl set port vhost-user-29 tag=100
ovs-vsctl set port vhost-user-30 tag=100
ovs-vsctl set port vhost-user-31 tag=100
ovs-vsctl set port vhost-user-32 tag=100
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
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:11,actions=output:19"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:12,actions=output:20"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:13,actions=output:21"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:14,actions=output:22"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:15,actions=output:23"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:16,actions=output:24"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:17,actions=output:25"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:18,actions=output:26"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:19,actions=output:27"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:1a,actions=output:28"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:1b,actions=output:29"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:1c,actions=output:30"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:1d,actions=output:31"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:1e,actions=output:32"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:1f,actions=output:33"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:20,actions=output:34"
ovs-ofctl add-flow br0 "dl_dst=00:10:94:00:00:02,actions=output:2"
ovs-ofctl add-flow br0 "dl_dst=00:10:94:00:00:03,actions=output:1"
ovs-ofctl add-flow br0 "tun_src=0.0.0.0,actions=output:35"
