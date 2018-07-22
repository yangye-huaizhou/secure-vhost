ovs-ofctl del-flows br0
ovs-ofctl add-flow br0 "in_port=1,actions=output:3"
ovs-ofctl add-flow br0 "in_port=3,actions=output:2"

