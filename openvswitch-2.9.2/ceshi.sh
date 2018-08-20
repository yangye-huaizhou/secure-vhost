ovs-ofctl del-flows br0
ovs-ofctl add-flow br0 "in_port=1,dl_dst=00:00:00:00:00:01,actions=output:3"
ovs-ofctl add-flow br0 "in_port=1,dl_dst=00:00:00:00:00:02,actions=output:4"
ovs-ofctl add-flow br0 "in_port=3,actions=output:2"
ovs-ofctl add-flow br0 "in_port=4,actions=output:2"

