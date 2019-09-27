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
 
for((i=0;i<$VMNUM;i++))
do
res=`expr $i % 2`
temp=`expr $i + 1`
vncnum=`expr $i + 50` 
temphex=`printf "%02x" $temp`

#if [ $i -gt 7 ]
#then
#overleft=`expr $i - 8`
#temphex1=`printf "%01x" $overleft`
#echo $temphex1
#nohup qemu-system-x86_64 -c 19 -w 0000:00:04.$temphex1 -- -machine accel=kvm -cpu host -smp sockets=1,cores=1,threads=1 -m 1024M -object memory-backend-file,id=mem,size=1024M,mem-path=/dev/hugepages,share=on -drive file=/var/otheriso/virtual$temp.qcow2 -mem-prealloc -numa node,memdev=mem -vnc 0.0.0.0:$vncnum --enable-kvm -chardev socket,id=char$temp,path=/tmp/sock$i,server -netdev type=vhost-user,id=mynet$temp,chardev=char$temp,vhostforce -device virtio-net-pci,netdev=mynet$temp,id=net$temp,mac=00:00:00:00:00:$temphex &
#else
#temphex1=`printf "%01x" $i`
#echo $temphex1
#nohup qemu-system-x86_64 -c 19 -w 0000:00:04.$temphex1 -- -machine accel=kvm -cpu host -smp sockets=1,cores=1,threads=1 -m 1024M -object memory-backend-file,id=mem,size=1024M,mem-path=/dev/hugepages,share=on -drive file=/var/otheriso/virtual$temp.qcow2 -mem-prealloc -numa node,memdev=mem -vnc 0.0.0.0:$vncnum --enable-kvm -chardev socket,id=char$temp,path=/tmp/sock$i,server -netdev type=vhost-user,id=mynet$temp,chardev=char$temp,vhostforce -device virtio-net-pci,netdev=mynet$temp,id=net$temp,mac=00:00:00:00:00:$temphex &
#fi
if [ "$res" = "0" ]
then
nohup qemu-system-x86_64 -c 6 -w 0000:00:04.0 -- -machine accel=kvm -cpu host -smp sockets=1,cores=1,threads=1 -m 2048M -object memory-backend-file,id=mem,size=2048M,mem-path=/dev/hugepages,share=on -drive file=/home/iso/virtual$temp.qcow2 -mem-prealloc -numa node,memdev=mem -vnc 0.0.0.0:$vncnum --enable-kvm -chardev socket,id=char$temp,path=/tmp/sock$i,server -netdev type=vhost-user,id=mynet$temp,chardev=char$temp,vhostforce -device virtio-net-pci,netdev=mynet$temp,id=net$temp,mac=00:00:00:00:00:$temphex &
else
nohup qemu-system-x86_64 -c 7 -w 0000:00:04.0 -- -machine accel=kvm -cpu host -smp sockets=1,cores=1,threads=1 -m 2048M -object memory-backend-file,id=mem,size=2048M,mem-path=/dev/hugepages,share=on -drive file=/home/iso/virtual$temp.qcow2 -mem-prealloc -numa node,memdev=mem -vnc 0.0.0.0:$vncnum --enable-kvm -chardev socket,id=char$temp,path=/tmp/sock$i,server -netdev type=vhost-user,id=mynet$temp,chardev=char$temp,vhostforce -device virtio-net-pci,netdev=mynet$temp,id=net$temp,mac=00:00:00:00:00:$temphex &
fi
#sleep 5
done

