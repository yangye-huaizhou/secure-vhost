Secure-vhost is a isolation enhanced vhost-user para-virtualized network I/O. 
It is built for the cloud platform, which runs VMs in high density. 

A secure risk in tranditional vhost-user architecture is that all VMs' whole memory is shared to DPDK accelarated vSwitch for memory copying between VM buffer and host buffer. A malicious tenant may take control the userspace vSwitch and "legally" access these VMs' memory.
So in secure-vhost, the memory copying task is transfered from vSwitch to QEMU. The host buffer is shared to these QEMU processes for packet copying. Each QEMU process is responsible for the packet copying task of a particular VM. That eliminates illegal memory access.

More design details can be seen in paper: 
Ye Yang, Haiyang Jiang, Yongzheng Liang, Yulei Wu, Yilong Lv, Xing Li and Gaogang Xie, "Isolation Guarantee for Efficient Virtualized Network I/O on Cloud Platform" (accepted by HPCC-2020).

The SPDK here is used to release the CPU from the heavy memory copying task, and to complete the memory-memory DMA operation via IOAT DMA engine. But as far as I know, the latest version of DPDK already supports this kind of DMA operation, and SPDK is no longer needed.

It should be noted that as a demo for verifying feasibility, we did not really remove the VM memory in the vSwitch address space, but only do not use them. This kind of issue for deployment can easily be implemented by modifying the processing function on socket signal `VHOST_USER_SET_MEM_TABLE`.
