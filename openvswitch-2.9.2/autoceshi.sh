ovs-ofctl del-flows br0
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:01,actions=output:3"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:02,actions=output:4"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:03,actions=output:5"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:04,actions=output:6"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:05,actions=output:7"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:06,actions=output:8"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:07,actions=output:9"
ovs-ofctl add-flow br0 "dl_dst=00:00:00:00:00:08,actions=output:10"
ovs-ofctl add-flow br0 "dl_dst=00:10:94:00:00:02,actions=output:2"
ovs-ofctl add-flow br0 "dl_dst=00:10:94:00:00:03,actions=output:1"
ovs-ofctl add-flow br0 "tun_src=0.0.0.0,actions=output:11"
