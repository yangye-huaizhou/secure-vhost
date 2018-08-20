#!/bin/bash

DEVICE="0000:01:00.0 0000:01:00.1"
DRIVER="igb_uio"

while getopts ":hd:r:" optname
do
  case "$optname" in
    "h")
      echo "   `basename ${0}`:usage:[-d device_name] [-r driver_name]"
      echo "   where device_name can be one in: {eth2,eth5},driver_name can be one in: {vfio-pci}"
      exit 1
      ;;
    "d")
      DEVICE=$OPTARG
      ;;
    "r")
      DRIVER=$OPTARG
      ;;
    *)
    # Should not occur
      echo "Unknown error while processing options"
      ;;
  esac
done
echo "mounting hugepages..."
mount -t hugetlbfs -o pagesize=1G none /dev/hugepages
mount -t hugetlbfs nodev /mnt/huge
#mount -t hugetlbfs -o pagesize=2M none /dev/hugepages
echo "modprobing igb_uio..."
modprobe uio
insmod x86_64-native-linuxapp-gcc/kmod/igb_uio.ko
#echo "modifying directory permissions..."
#chmod a+x /dev/vfio
#chmod 0666 /dev/vfio/*
echo "binding device..."
./usertools/dpdk-devbind.py --bind=$DRIVER $DEVICE
echo "all Done."

