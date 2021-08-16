Secure-vhost is a isolation enhanced vhost-user para-virtualized network I/O. 
It is built for the cloud platform, which runs VMs in high density. The VM memory 
isolation breakage in vhost-user may bring security risks for VMs on cloud. So 
in Secure-vhost, we modify the memory sharing model to copy packets between VM 
and the host more securely.

## What is the VM isolation issue in vhost-user solution?

A secure risk in tranditional vhost-user network I/O solution is that all VMs' 
whole memory is shared to DPDK accelarated vSwitch (e.g. OVS-DPDK) for memory 
copying between VM buffer and host buffer. A malicious tenant may take control 
the userspace vSwitch and **"legally"** access these VMs' memory. And even worse, 
this kind of illegal memory access is not easy to detected because it occurs 
totally in user space.

To make it clear, we use a example to describe this kind of illegal memory access 
as below:

<p align="left">
  <img src="./media/security_risk.png"/>
<p/>

In this figure, the malicious tenant firstly sends a well-designed packet to 
the user space vSwitch, then the vSwitch triggers a vulnerability to run the 
malicious code in the packet payload. By exploiting the feature that vSwitch 
can access all VMs' whole memory without restriction, the VM1's malicious code 
in packet payload can easily access VM2's whole memory. In this way, the key 
data of VM2 will be leaked and the stability is also threatened.

The vulnerability in open vSwitch can be: https://www.cvedetails.com/cve/CVE-2016-2074/. 
Though the vulnerability has been fixed, then what about other unknown vulnerabilities.

The community also has noticed this issue, they proposed a vIOMMU solution: 
http://www.linux-kvm.org/images/c/c5/03x07B-Peter_Xu_and_Wei_Xu-Vhost_with_Guest_vIOMMU.pdf. 
In this enhanced solution, each time when vSwitch needs to access VM's memory 
for packet copying, it should firstly send a socket based message to query QEMU 
process for address translation. The address translation done by the software 
vIOMMU can confirm the legitimacy of VM memory access. However, frequent 
communication will reduce performance to 20%, with severe packet loss. So it 
is rarely used for practical environments.

## What is secure-vhost?

To solve the security issue in vhost-user without dropping performance, we propose 
secure-vhost. In secure-vhost, we removed the insecure memory sharing, so VM's memory 
did not need to be shared to any other process. To implement packet copying, we 
transfered packet copying task is from vSwitch to QEMU, so that vSwitch should 
securely share its host packet buffer to QEMU. As VM's main process, each QEMU 
process is responsible for the packet copying task of a particular VM. As part 
of hypervisor, QEMU is more trusty than vSwitch and it can access the VM inside 
it by default. So this kind of memory sharing and packet copying is more secure.

More security discussion can be seen in paper: 
>Ye Yang, Haiyang Jiang, Yongzheng Liang, Yulei Wu, Yilong Lv, Xing Li and Gaogang 
Xie, "Isolation Guarantee for Efficient Virtualized Network I/O on Cloud Platform" 
(accepted by HPCC-2020).

## How to compile?

The prototype system was implmented based on `DPDK-17.11.2`, `OVS-2.9.2` and `qemu-2.10.0-rc3`.

It can be successfully built on:
OS: Ubuntu 16.04.1 (Kernel 4.15.0-142-generic)
CPU: Intel(R) Xeon(R) CPU E5-4603 v2 @ 2.20GHz

To compile this demo, you should compile DPDK first:

```
cd dpdk-eal-master/
make install T=x86_64-native-linuxapp-gcc DESTDIR=install
```

Then compile SPDK:

```
cd ../spdk/
./configure --with-dpdk=../dpdk-eal-master/x86_64-native-linuxapp-gcc
make
```

*The SPDK here is used to release the CPU from the heavy memory copying task, and 
to complete the memory-memory DMA operation via IOAT DMA engine. But as far as I 
know, the latest version of DPDK already supports this kind of DMA operation, and 
SPDK is no longer needed if you want to compile secure-vhost into higher version 
DPDK.*

Compile and install OVS:

```
cd ../openvswitch-2.9.2/
./boot.sh
./configure --with-dpdk=../dpdk-eal-master/x86_64-native-linuxapp-gcc
make
make install
```

Compile and install QEMU:

```
cd ../qemu-2.10.0-rc3/
./compile.sh
```
**You need to change the path in compile.sh according to your situation.**

To use OVS, there is nothing different with the original version. We give 
examples in `start-vswitchd.sh` and `bridge.sh`. You can use:

```
./start-vswitchd.sh
./bridge.sh -n <n(VM number)>
```

But for QEMU, we added DPDK EAL commands before QEMU commands. The two kinds of 
commands are separated by "--". 
Here is an example:

```
./qemu-system-x86_64 -c 14 -w 0000:00:04.0 -- -machine accel=kvm -cpu host -smp...
```

We also write an script in `setup.sh`.


## What is the difference between vhost-user and secure-vhost?
