ovs-ofctl del-flows br0
ovs-ofctl add-flow br0 "in_port=1,actions=output:3"
ovs-ofctl add-flow br0 "in_port=2,actions=output:3"
ovs-ofctl add-flow br0 "in_port=3,dl_src=00:10:94:00:00:02,actions=output:1"
ovs-ofctl add-flow br0 "in_port=3,dl_src=00:10:94:00:00:03,actions=output:2"
