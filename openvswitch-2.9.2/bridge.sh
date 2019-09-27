#!/bin/bash

VMNUM="0";
while getopts ":hn:" optname
do
  case "$optname" in
    "h")
      echo "   `basename ${0}`:usage:[-n vm_num]"
      echo "   where vm_num can be one in: [0..31], and the max number of VM is 32."
      exit 1
      ;;
    "n")
      VMNUM=$OPTARG
      ;;
    *)
    # Should not occur
      echo "Unknown error while processing options"
      ;;
  esac
done

ovs-vsctl del-br br0
ovs-vsctl add-br br0 -- set bridge br0 datapath_type=netdev
ovs-vsctl add-port br0 dpdk-p0 -- set Interface dpdk-p0 type=dpdk options:dpdk-devargs=0000:05:00.0
ovs-vsctl add-port br0 dpdk-p1 -- set Interface dpdk-p1 type=dpdk options:dpdk-devargs=0000:05:00.1
for((i=0;i<$VMNUM;i++))
do
temp=`expr $i + 1`
ovs-vsctl add-port br0 vhost-user-$temp -- set Interface vhost-user-$temp type=dpdkvhostuserclient options:vhost-server-path="/tmp/sock$i"
ovs-vsctl set port vhost-user-$temp tag=100
done
ovs-vsctl add-port br0 qdhcp tag=100 -- set interface qdhcp type=internal
ifconfig qdhcp up
ifconfig qdhcp 192.168.1.1 netmask 255.255.255.0
pkill -f /usr/sbin/dnsmasq
#sleep 1
/usr/sbin/dnsmasq --strict-order --bind-interfaces --except-interface lo --interface qdhcp --dhcp-range 192.168.1.1,192.168.1.200 --dhcp-leasefile=/var/run/dnsmasq/qdhcp.pid --dhcp-lease-max=253 --dhcp-no-override --log-queries --log-facility=/tmp/dnsmasq.log

ovs-ofctl del-flows br0
for((i=0;i<$VMNUM;i++))
do
temp=`expr $i + 1`
tempout=`expr $i + 3`
temphex=`printf "%02x" $temp`
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:$temphex,actions=output:$tempout"
done
ovs-ofctl add-flow br0 "dl_dst=00:10:94:00:00:02,actions=output:2"
ovs-ofctl add-flow br0 "dl_dst=00:10:94:00:00:03,actions=output:1"
tempqhcp=`expr $VMNUM + 3`
ovs-ofctl add-flow br0 "tun_src=0.0.0.0,actions=output:$tempqhcp"
