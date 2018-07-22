/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2016 Intel Corporation. All rights reserved.
 *   Copyright(c) 2012-2014 6WIND S.A.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <pthread.h>
#include <syslog.h>
#include <getopt.h>
#include <sys/file.h>
#include <fcntl.h>
#include <stddef.h>
#include <errno.h>
#include <limits.h>
#include <sys/mman.h>
#include <sys/queue.h>
#include <sys/stat.h>
#if defined(RTE_ARCH_X86)
#include <sys/io.h>
#endif

#include <rte_common.h>
#include <rte_debug.h>
#include <rte_memory.h>
#include <rte_launch.h>
#include <rte_eal.h>
#include <rte_eal_memconfig.h>
#include <rte_errno.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_service_component.h>
#include <rte_log.h>
#include <rte_random.h>
#include <rte_cycles.h>
#include <rte_string_fns.h>
#include <rte_cpuflags.h>
#include <rte_interrupts.h>
#include <rte_bus.h>
#include <rte_dev.h>
#include <rte_devargs.h>
#include <rte_version.h>
#include <rte_atomic.h>
#include <malloc_heap.h>
#include <rte_vfio.h>
#include <rte_qemu.h>

#include "eal_private.h"
#include "eal_thread.h"
#include "eal_internal_cfg.h"
#include "eal_filesystem.h"
#include "eal_hugepages.h"
#include "eal_options.h"
#include "eal_vfio.h"

#define MEMSIZE_IF_NO_HUGE_PAGE (64ULL * 1024ULL * 1024ULL)

#define SOCKET_MEM_STRLEN (RTE_MAX_NUMA_NODES * 10)

/* Allow the application to print its usage message too if set */
static rte_usage_hook_t	rte_application_usage_hook = NULL;

/* early configuration structure, when memory config is not mmapped */
static struct rte_mem_config early_mem_config;

/* define fd variable here, because file needs to be kept open for the
 * duration of the program, as we hold a write lock on it in the primary proc */
static int mem_cfg_fd = -1;

static struct flock wr_lock = {
	.l_type = F_WRLCK,
	.l_whence = SEEK_SET,
	.l_start = offsetof(struct rte_mem_config, memseg),
	.l_len = sizeof(early_mem_config.memseg),
};

/* Address of global and public configuration */
static struct rte_config rte_config = {
	.mem_config = &early_mem_config,
};

/* internal configuration (per-core) */
struct lcore_config lcore_config[RTE_MAX_LCORE];

/* internal configuration */
struct internal_config internal_config;

/* used by rte_rdtsc() */
int rte_cycles_vmware_tsc_map;

/* Return mbuf pool ops name */
	const char *
rte_eal_mbuf_default_mempool_ops(void)
{
	return internal_config.mbuf_pool_ops_name;
}

/* Return a pointer to the configuration structure */
	struct rte_config *
rte_eal_get_configuration(void)
{
	return &rte_config;
}

	enum rte_iova_mode
rte_eal_iova_mode(void)
{
	return rte_eal_get_configuration()->iova_mode;
}

/* parse a sysfs (or other) file containing one integer value */
	int
eal_parse_sysfs_value(const char *filename, unsigned long *val)
{
	FILE *f;
	char buf[BUFSIZ];
	char *end = NULL;

	if ((f = fopen(filename, "r")) == NULL) {
		RTE_LOG(ERR, EAL, "%s(): cannot open sysfs value %s\n",
				__func__, filename);
		return -1;
	}

	if (fgets(buf, sizeof(buf), f) == NULL) {
		RTE_LOG(ERR, EAL, "%s(): cannot read sysfs value %s\n",
				__func__, filename);
		fclose(f);
		return -1;
	}
	*val = strtoul(buf, &end, 0);
	if ((buf[0] == '\0') || (end == NULL) || (*end != '\n')) {
		RTE_LOG(ERR, EAL, "%s(): cannot parse sysfs value %s\n",
				__func__, filename);
		fclose(f);
		return -1;
	}
	fclose(f);
	return 0;
}

/* attach to an existing shared memory config */
	static void
rte_eal_config_attach(void)
{
	struct rte_mem_config *mem_config;

	const char *pathname = eal_runtime_config_path();

	if (internal_config.no_shconf)
		return;

	if (mem_cfg_fd < 0){
		mem_cfg_fd = open(pathname, O_RDWR);
		if (mem_cfg_fd < 0)
			rte_panic("Cannot open '%s' for rte_mem_config\n", pathname);
	}

	/* map it as read-only first */
	mem_config = (struct rte_mem_config *) mmap(NULL, sizeof(*mem_config),
			PROT_READ, MAP_SHARED, mem_cfg_fd, 0);
	if (mem_config == MAP_FAILED)
		rte_panic("Cannot mmap memory for rte_config! error %i (%s)\n",
				errno, strerror(errno));

	rte_config.mem_config = mem_config;
}

/* reattach the shared config at exact memory location primary process has it */
	static void
rte_eal_config_reattach(void)
{
	struct rte_mem_config *mem_config;
	void *rte_mem_cfg_addr;

	if (internal_config.no_shconf)
		return;

	/* save the address primary process has mapped shared config to */
	rte_mem_cfg_addr = (void *) (uintptr_t) rte_config.mem_config->mem_cfg_addr;

	/* unmap original config */
	munmap(rte_config.mem_config, sizeof(struct rte_mem_config));

	/* remap the config at proper address */
	mem_config = (struct rte_mem_config *) mmap(rte_mem_cfg_addr,
			sizeof(*mem_config), PROT_READ | PROT_WRITE, MAP_SHARED,
			mem_cfg_fd, 0);
	if (mem_config == MAP_FAILED || mem_config != rte_mem_cfg_addr) {
		if (mem_config != MAP_FAILED)
			/* errno is stale, don't use */
			rte_panic("Cannot mmap memory for rte_config at [%p], got [%p]"
					" - please use '--base-virtaddr' option\n",
					rte_mem_cfg_addr, mem_config);
		else
			rte_panic("Cannot mmap memory for rte_config! error %i (%s)\n",
					errno, strerror(errno));
	}
	close(mem_cfg_fd);

	rte_config.mem_config = mem_config;
}

/* Detect if we are a primary or a secondary process */
	enum rte_proc_type_t
eal_proc_type_detect(void)
{
	enum rte_proc_type_t ptype = RTE_PROC_PRIMARY;
	const char *pathname = eal_runtime_config_path();

	/* if we can open the file but not get a write-lock we are a secondary
	 * process. NOTE: if we get a file handle back, we keep that open
	 * and don't close it to prevent a race condition between multiple opens */
	if (((mem_cfg_fd = open(pathname, O_RDWR)) >= 0) &&
			(fcntl(mem_cfg_fd, F_SETLK, &wr_lock) < 0))
		ptype = RTE_PROC_SECONDARY;

	RTE_LOG(INFO, EAL, "Auto-detected process type: %s\n",
			ptype == RTE_PROC_PRIMARY ? "PRIMARY" : "SECONDARY");

	return ptype;
}

/* Sets up rte_config structure with the pointer to shared memory config.*/
	static void
rte_config_init(void)
{
	rte_eal_config_attach();
	rte_eal_mcfg_wait_complete(rte_config.mem_config);
	rte_eal_config_reattach();
}

/* Unlocks hugepage directories that were locked by eal_hugepage_info_init */
	static void
eal_hugedirs_unlock(void)
{
	int i;

	for (i = 0; i < MAX_HUGEPAGE_SIZES; i++)
	{
		/* skip uninitialized */
		if (internal_config.hugepage_info[i].lock_descriptor < 0)
			continue;
		/* unlock hugepage file */
		flock(internal_config.hugepage_info[i].lock_descriptor, LOCK_UN);
		close(internal_config.hugepage_info[i].lock_descriptor);
		/* reset the field */
		internal_config.hugepage_info[i].lock_descriptor = -1;
	}
}

/* Set a per-application usage message */
rte_usage_hook_t
rte_set_application_usage_hook( rte_usage_hook_t usage_func )
{
	rte_usage_hook_t	old_func;

	/* Will be NULL on the first call to denote the last usage routine. */
	old_func					= rte_application_usage_hook;
	rte_application_usage_hook	= usage_func;

	return old_func;
}

static void
eal_check_mem_on_local_socket(void)
{
	const struct rte_memseg *ms;
	int i, socket_id;

	socket_id = rte_lcore_to_socket_id(rte_config.master_lcore);

	ms = rte_eal_get_physmem_layout();

	for (i = 0; i < RTE_MAX_MEMSEG; i++)
		if (ms[i].socket_id == socket_id &&
				ms[i].len > 0)
			return;

	RTE_LOG(WARNING, EAL, "WARNING: Master core has no "
			"memory on local socket!\n");
}

inline static void
rte_eal_mcfg_complete(void)
{
	/* ALL shared mem_config related INIT DONE */
	if (rte_config.process_type == RTE_PROC_PRIMARY)
		rte_config.mem_config->magic = RTE_MAGIC;
}

/*
 * Request iopl privilege for all RPL, returns 0 on success
 * iopl() call is mostly for the i386 architecture. For other architectures,
 * return -1 to indicate IO privilege can't be changed in this way.
 */
int
rte_eal_iopl_init(void)
{
#if defined(RTE_ARCH_X86)
	if (iopl(3) != 0)
		return -1;
#endif
	return 0;
}

static void rte_eal_init_alert(const char *msg)
{
	fprintf(stderr, "EAL: FATAL: %s\n", msg);
	RTE_LOG(ERR, EAL, "%s\n", msg);
}

/* Launch threads, called at application init(). */
	int
rte_qemu_init(void)
{
	//int i, fctret, ret;
	//pthread_t thread_id;
	static rte_atomic32_t run_once = RTE_ATOMIC32_INIT(0);
	//const char *logid;
	//char cpuset[RTE_CPU_AFFINITY_STR_LEN];
	//char thread_name[RTE_MAX_THREAD_NAME_LEN];

	/* checks if the machine is adequate */
	if (!rte_cpu_is_supported()) {
		rte_eal_init_alert("unsupported cpu type.");
		rte_errno = ENOTSUP;
		return -1;
	}

	if (!rte_atomic32_test_and_set(&run_once)) {
		rte_eal_init_alert("already called initialization.");
		rte_errno = EALREADY;
		return -1;
	}


	eal_reset_internal_config(&internal_config);

	/* set log level as early as possible */
	//eal_log_level_parse(argc, argv);

	if (rte_eal_cpu_init() < 0) {
		rte_eal_init_alert("Cannot detect lcores.");
		rte_errno = ENOTSUP;
		return -1;
	}

	//fctret = eal_parse_args(argc, argv);
	//if (fctret < 0) {
	//	rte_eal_init_alert("Invalid 'command line' arguments.");
	//	rte_errno = EINVAL;
	//	rte_atomic32_clear(&run_once);
	//	return -1;
	//}

	//if (eal_plugins_init() < 0) {
	//	rte_eal_init_alert("Cannot init plugins\n");
	//	rte_errno = EINVAL;
	//	rte_atomic32_clear(&run_once);
	//	return -1;
	//}

	//if (eal_option_device_parse()) {
	//	rte_errno = ENODEV;
	//	rte_atomic32_clear(&run_once);
	//	return -1;
	//}

	//if (rte_bus_scan()) {
	//	rte_eal_init_alert("Cannot scan the buses for devices\n");
	//	rte_errno = ENODEV;
	//	rte_atomic32_clear(&run_once);
	//	return -1;
	//}

	//if (internal_config.no_hugetlbfs == 0 &&
	//		internal_config.process_type != RTE_PROC_SECONDARY &&
	//		eal_hugepage_info_init() < 0) {
	//	rte_eal_init_alert("Cannot get hugepage information.");
	//	rte_errno = EACCES;
	//	rte_atomic32_clear(&run_once);
	//	return -1;
	//}

	if (internal_config.memory == 0 && internal_config.force_sockets == 0) {
		if (internal_config.no_hugetlbfs)
			internal_config.memory = MEMSIZE_IF_NO_HUGE_PAGE;
	}

	rte_srand(rte_rdtsc());

	rte_config_init();

	if (rte_eal_log_init("qemu", internal_config.syslog_facility) < 0) {
		rte_eal_init_alert("Cannot init logging.");
		rte_errno = ENOMEM;
		rte_atomic32_clear(&run_once);
		return -1;
	}

	if (rte_eal_memory_init() < 0) {
		rte_eal_init_alert("Cannot init memory\n");
		rte_errno = ENOMEM;
		return -1;
	}

	/* the directories are locked during eal_hugepage_info_init */
	eal_hugedirs_unlock();

	if (rte_eal_memzone_init() < 0) {
		rte_eal_init_alert("Cannot init memzone\n");
		rte_errno = ENODEV;
		return -1;
	}

	if (rte_eal_tailqs_init() < 0) {
		rte_eal_init_alert("Cannot init tail queues for objects\n");
		rte_errno = EFAULT;
		return -1;
	}

	//if (rte_eal_alarm_init() < 0) {
	//	rte_eal_init_alert("Cannot init interrupt-handling thread\n");
	//	/* rte_eal_alarm_init sets rte_errno on failure. */
	//	return -1;
	//}

	//if (rte_eal_timer_init() < 0) {
	//	rte_eal_init_alert("Cannot init HPET or TSC timers\n");
	//	rte_errno = ENOTSUP;
	//	return -1;
	//}

	eal_check_mem_on_local_socket();

	//eal_thread_init_master(rte_config.master_lcore);

	//ret = eal_thread_dump_affinity(cpuset, RTE_CPU_AFFINITY_STR_LEN);

	//RTE_LOG(DEBUG, EAL, "Master lcore %u is ready (tid=%x;cpuset=[%s%s])\n",
	//	rte_config.master_lcore, (int)thread_id, cpuset,
	//	ret == 0 ? "" : "...");

	//if (rte_eal_intr_init() < 0) {
	//	rte_eal_init_alert("Cannot init interrupt-handling thread\n");
	//	return -1;
	//}

	rte_eal_mcfg_complete();

	return 0;
}

/* get core role */
	enum rte_lcore_role_t
rte_eal_lcore_role(unsigned lcore_id)
{
	return rte_config.lcore_role[lcore_id];
}

	enum rte_proc_type_t
rte_eal_process_type(void)
{
	return rte_config.process_type;
}

int rte_eal_has_hugepages(void)
{
	return ! internal_config.no_hugetlbfs;
}

int rte_eal_has_pci(void)
{
	return !internal_config.no_pci;
}

int rte_eal_create_uio_dev(void)
{
	return internal_config.create_uio_dev;
}

	enum rte_intr_mode
rte_eal_vfio_intr_mode(void)
{
	return internal_config.vfio_intr_mode;
}

	int
rte_eal_check_module(const char *module_name)
{
	char sysfs_mod_name[PATH_MAX];
	struct stat st;
	int n;

	if (NULL == module_name)
		return -1;

	/* Check if there is sysfs mounted */
	if (stat("/sys/module", &st) != 0) {
		RTE_LOG(DEBUG, EAL, "sysfs is not mounted! error %i (%s)\n",
				errno, strerror(errno));
		return -1;
	}

	/* A module might be built-in, therefore try sysfs */
	n = snprintf(sysfs_mod_name, PATH_MAX, "/sys/module/%s", module_name);
	if (n < 0 || n > PATH_MAX) {
		RTE_LOG(DEBUG, EAL, "Could not format module path\n");
		return -1;
	}

	if (stat(sysfs_mod_name, &st) != 0) {
		RTE_LOG(DEBUG, EAL, "Module %s not found! error %i (%s)\n",
				sysfs_mod_name, errno, strerror(errno));
		return 0;
	}

	/* Module has been found */
	return 1;
}
