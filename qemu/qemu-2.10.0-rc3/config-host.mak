# Automatically generated by configure - do not modify

all:
prefix=/usr/local
bindir=${prefix}/bin
libdir=${prefix}/lib
libexecdir=${prefix}/libexec
includedir=${prefix}/include
mandir=${prefix}/share/man
sysconfdir=${prefix}/etc
qemu_confdir=${prefix}/etc/qemu
qemu_datadir=${prefix}/share/qemu
qemu_docdir=${prefix}/share/doc/qemu
qemu_moddir=${prefix}/lib/qemu
qemu_localstatedir=${prefix}/var
qemu_helperdir=${prefix}/libexec
extra_cflags=-m64 -mcx16 -I/home/yangye/vhostioat/dpdk-eal-master/x86_64-native-linuxapp-gcc/include/ -I/home/yangye/vhostioat/spdk/include/ -L/home/yangye/vhostioat/dpdk-eal-master/x86_64-native-linuxapp-gcc/lib/ -L/home/yangye/vhostioat/spdk/build/lib/ -Wl,--whole-archive -lspdk_util -lspdk_ioat -lspdk_env_dpdk -lspdk_log -lspdk_copy -lspdk_conf -lspdk_copy_ioat -lrte_mempool_octeontx -lrte_pci -lrte_kvargs -lrte_ethdev -lrte_bus_pci -lrte_bus_vdev -lrte_eal -lrte_mempool -lrte_mempool_ring -lrte_ring -lrte_mbuf -Wl,--no-whole-archive -lpthread -lnuma -ldl -mavx
extra_cxxflags=
extra_ldflags=
qemu_localedir=${prefix}/share/locale
libs_softmmu=-lpixman-1 -lutil -lnuma  -lpng12 -ljpeg -lSDL2 -lX11  -lrdmacm -libverbs -L$(BUILD_DIR)/dtc/libfdt -lfdt
ARCH=x86_64
STRIP=strip
CONFIG_POSIX=y
CONFIG_LINUX=y
CONFIG_SLIRP=y
CONFIG_SMBD_COMMAND="/usr/sbin/smbd"
CONFIG_L2TPV3=y
CONFIG_AUDIO_DRIVERS=oss
CONFIG_OSS=y
CONFIG_BDRV_RW_WHITELIST=
CONFIG_BDRV_RO_WHITELIST=
CONFIG_VNC=y
CONFIG_VNC_JPEG=y
CONFIG_VNC_PNG=y
CONFIG_FNMATCH=y
VERSION=2.9.93
PKGVERSION=
SRC_PATH=/home/yangye/vhostioat/qemu-2.10.0-rc3
TARGET_DIRS=aarch64-softmmu alpha-softmmu arm-softmmu cris-softmmu i386-softmmu lm32-softmmu m68k-softmmu microblaze-softmmu microblazeel-softmmu mips-softmmu mips64-softmmu mips64el-softmmu mipsel-softmmu moxie-softmmu nios2-softmmu or1k-softmmu ppc-softmmu ppc64-softmmu ppcemb-softmmu s390x-softmmu sh4-softmmu sh4eb-softmmu sparc-softmmu sparc64-softmmu tricore-softmmu unicore32-softmmu x86_64-softmmu xtensa-softmmu xtensaeb-softmmu aarch64-linux-user alpha-linux-user arm-linux-user armeb-linux-user cris-linux-user hppa-linux-user i386-linux-user m68k-linux-user microblaze-linux-user microblazeel-linux-user mips-linux-user mips64-linux-user mips64el-linux-user mipsel-linux-user mipsn32-linux-user mipsn32el-linux-user nios2-linux-user or1k-linux-user ppc-linux-user ppc64-linux-user ppc64abi32-linux-user ppc64le-linux-user s390x-linux-user sh4-linux-user sh4eb-linux-user sparc-linux-user sparc32plus-linux-user sparc64-linux-user tilegx-linux-user x86_64-linux-user
CONFIG_SDL=y
CONFIG_SDLABI=2.0
SDL_CFLAGS=-D_REENTRANT -I/usr/include/SDL2 
CONFIG_PIPE2=y
CONFIG_ACCEPT4=y
CONFIG_SPLICE=y
CONFIG_EVENTFD=y
CONFIG_FALLOCATE=y
CONFIG_FALLOCATE_PUNCH_HOLE=y
CONFIG_FALLOCATE_ZERO_RANGE=y
CONFIG_POSIX_FALLOCATE=y
CONFIG_SYNC_FILE_RANGE=y
CONFIG_FIEMAP=y
CONFIG_DUP3=y
CONFIG_PPOLL=y
CONFIG_PRCTL_PR_SET_TIMERSLACK=y
CONFIG_EPOLL=y
CONFIG_EPOLL_CREATE1=y
CONFIG_SENDFILE=y
CONFIG_TIMERFD=y
CONFIG_SETNS=y
CONFIG_CLOCK_ADJTIME=y
CONFIG_SYNCFS=y
CONFIG_INOTIFY=y
CONFIG_INOTIFY1=y
CONFIG_BYTESWAP_H=y
CONFIG_HAS_GLIB_SUBPROCESS_TESTS=y
CONFIG_TLS_PRIORITY="NORMAL"
HAVE_IFADDRS_H=y
CONFIG_LINUX_AIO=y
CONFIG_ATTR=y
CONFIG_VHOST_SCSI=y
CONFIG_VHOST_NET_USED=y
CONFIG_VHOST_VSOCK=y
CONFIG_VHOST_USER=y
INSTALL_BLOBS=yes
CONFIG_IOVEC=y
CONFIG_PREADV=y
CONFIG_FDT=y
CONFIG_SIGNALFD=y
CONFIG_TCG=y
CONFIG_FDATASYNC=y
CONFIG_MADVISE=y
CONFIG_POSIX_MADVISE=y
CONFIG_AVX2_OPT=y
CONFIG_QOM_CAST_DEBUG=y
CONFIG_COROUTINE_BACKEND=ucontext
CONFIG_COROUTINE_POOL=1
CONFIG_OPEN_BY_HANDLE=y
CONFIG_LINUX_MAGIC_H=y
CONFIG_PRAGMA_DIAGNOSTIC_AVAILABLE=y
CONFIG_HAS_ENVIRON=y
CONFIG_CPUID_H=y
CONFIG_INT128=y
CONFIG_ATOMIC128=y
CONFIG_ATOMIC64=y
CONFIG_GETAUXVAL=y
CONFIG_LIVE_BLOCK_MIGRATION=y
HOST_USB=stub
CONFIG_TPM=$(CONFIG_SOFTMMU)
CONFIG_TPM_PASSTHROUGH=y
TRACE_BACKENDS=log
CONFIG_TRACE_LOG=y
CONFIG_TRACE_FILE=trace
CONFIG_RDMA=y
CONFIG_RTNETLINK=y
CONFIG_REPLICATION=y
CONFIG_AF_VSOCK=y
CONFIG_SYSMACROS=y
CONFIG_STATIC_ASSERT=y
HAVE_UTMPX=y
CONFIG_IVSHMEM=y
CONFIG_THREAD_SETNAME_BYTHREAD=y
CONFIG_PTHREAD_SETNAME_NP=y
TOOLS=qemu-ga ivshmem-client$(EXESUF) ivshmem-server$(EXESUF) qemu-nbd$(EXESUF) qemu-img$(EXESUF) qemu-io$(EXESUF) 
ROMS=optionrom
MAKE=make
INSTALL=install
INSTALL_DIR=install -d -m 0755
INSTALL_DATA=install -c -m 0644
INSTALL_PROG=install -c -m 0755
INSTALL_LIB=install -c -m 0644
PYTHON=python -B
CC=cc
CC_I386=$(CC) -m32
HOST_CC=cc
CXX=c++
OBJCC=clang
AR=ar
ARFLAGS=rv
AS=as
CCAS=cc
CPP=cc -E
OBJCOPY=objcopy
LD=ld
NM=nm
WINDRES=windres
CFLAGS=-O2 -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -g 
CFLAGS_NOPIE=
QEMU_CFLAGS=-I/usr/include/pixman-1 -I$(SRC_PATH)/dtc/libfdt -pthread -I/usr/local/include/glib-2.0 -I/usr/local/lib/glib-2.0/include -m64 -mcx16 -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -Wstrict-prototypes -Wredundant-decls -Wall -Wundef -Wwrite-strings -Wmissing-prototypes -fno-strict-aliasing -fno-common -fwrapv  -I/home/yangye/vhostioat/dpdk-eal-master/x86_64-native-linuxapp-gcc/include/ -I/home/yangye/vhostioat/spdk/include/ -L/home/yangye/vhostioat/dpdk-eal-master/x86_64-native-linuxapp-gcc/lib/ -L/home/yangye/vhostioat/spdk/build/lib/ -Wl,--whole-archive -lspdk_util -lspdk_ioat -lspdk_env_dpdk -lspdk_log -lspdk_copy -lspdk_conf -lspdk_copy_ioat -lrte_mempool_octeontx -lrte_pci -lrte_kvargs -lrte_ethdev -lrte_bus_pci -lrte_bus_vdev -lrte_eal -lrte_mempool -lrte_mempool_ring -lrte_ring -lrte_mbuf -Wl,--no-whole-archive -lpthread -lnuma -ldl -mavx -Wendif-labels -Wno-missing-include-dirs -Wempty-body -Wnested-externs -Wformat-security -Wformat-y2k -Winit-self -Wignored-qualifiers -Wold-style-declaration -Wold-style-definition -Wtype-limits -fstack-protector-strong  -I/usr/include/libpng12
QEMU_CXXFLAGS= -D__STDC_LIMIT_MACROS -pthread -I/usr/local/include/glib-2.0 -I/usr/local/lib/glib-2.0/include -m64 -mcx16 -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -Wall -Wundef -Wwrite-strings -fno-strict-aliasing -fno-common -fwrapv -I/home/yangye/vhostioat/dpdk-eal-master/x86_64-native-linuxapp-gcc/include/ -I/home/yangye/vhostioat/spdk/include/ -L/home/yangye/vhostioat/dpdk-eal-master/x86_64-native-linuxapp-gcc/lib/ -L/home/yangye/vhostioat/spdk/build/lib/ -Wl,--whole-archive -lspdk_util -lspdk_ioat -lspdk_env_dpdk -lspdk_log -lspdk_copy -lspdk_conf -lspdk_copy_ioat -lrte_mempool_octeontx -lrte_pci -lrte_kvargs -lrte_ethdev -lrte_bus_pci -lrte_bus_vdev -lrte_eal -lrte_mempool -lrte_mempool_ring -lrte_ring -lrte_mbuf -Wl,--no-whole-archive -lpthread -lnuma -ldl -mavx -Wendif-labels -Wno-missing-include-dirs -Wempty-body -Wformat-security -Wformat-y2k -Winit-self -Wignored-qualifiers -Wtype-limits -fstack-protector-strong -I/usr/include/libpng12
QEMU_INCLUDES=-I$(SRC_PATH)/tcg -I$(SRC_PATH)/tcg/i386 -I$(SRC_PATH)/linux-headers -I/home/yangye/vhostioat/qemu-2.10.0-rc3/linux-headers -I. -I$(SRC_PATH) -I$(SRC_PATH)/accel/tcg -I$(SRC_PATH)/include
AUTOCONF_HOST := 
LDFLAGS=-Wl,--warn-common -m64 -g 
LDFLAGS_NOPIE=
LD_REL_FLAGS=-Wl,-r -Wl,--no-relax
LD_I386_EMULATION=elf_i386
LIBS+=-lm -L/usr/local/lib -lgthread-2.0 -pthread -lglib-2.0  -lz -lrt
LIBS_TOOLS+=
PTHREAD_LIB=
EXESUF=
DSOSUF=.so
LDFLAGS_SHARED=-shared
LIBS_QGA+=-lm -L/usr/local/lib -lgthread-2.0 -pthread -lglib-2.0  -lrt
TASN1_LIBS=
TASN1_CFLAGS=
POD2MAN=pod2man --utf8
TRANSLATE_OPT_CFLAGS=
CONFIG_VHOST_USER_NET_TEST_i386=y
CONFIG_VHOST_USER_NET_TEST_x86_64=y
config-host.h: subdir-dtc
CONFIG_NUMA=y