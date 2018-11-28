ovs-vsctl set Open_vSwitch . other_config:pmd-cpu-mask=6
ovs-vsctl set Interface dpdk-p0 options:n_rxq=2
ovs-vsctl set Interface vhost-user-1 options:n_rxq=2
ovs-appctl dpif-netdev/pmd-rxq-show
