/*******************************************************************************
*                Copyright 2011, MARVELL SEMICONDUCTOR, LTD.                   *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL.                      *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
*                                                                              *
* MARVELL COMPRISES MARVELL TECHNOLOGY GROUP LTD. (MTGL) AND ITS SUBSIDIARIES, *
* MARVELL INTERNATIONAL LTD. (MIL), MARVELL TECHNOLOGY, INC. (MTI), MARVELL    *
* SEMICONDUCTOR, INC. (MSI), MARVELL ASIA PTE LTD. (MAPL), MARVELL JAPAN K.K.  *
* (MJKK), MARVELL ISRAEL LTD. (MSIL).                                          *
*******************************************************************************/


////////////////////////////////////////////////////////////////////////////////
//! \file shm_type.h
//! \brief Shared Memory type definition
//!
//! Platform = Linux, Berlin_G2
//!
//! Purpose:
//!
//!
//! Version, Date and Author :
//!     V 1.00, 5/19/2010,      Fang Bao
//!     V 1.02, 6/10/2010,      Fang Bao,   replace all type (uint, unsigned int) with size_t
//!     V 1.03, 9/26/2010,      Fang Bao,   add shm_round_size, shm_round_alignment, shm_check_alignment to support MV_SHM_Malloc
//!     v 1.04, 10/30/2011,     Qiang Zhang,add struct for shm lowmem killer
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#ifndef __shm_type_h__
#define __shm_type_h__

#include "shm_api.h"
#include <linux/sched.h>
#include <linux/rbtree.h>

/*******************************************************************************
    Macro Defined
*/
//#define __SHM_SEQFILE__          //default use seqfile for proc info

#define SHM_DEVICE_NAME                 "galois_shm"
#define SHM_DEVICE_TAG                  "[Galois][shm_driver] "

#define SHM_DEVICE_PATH                 "/dev/"
#define SHM_DEVICE_NAME_NONCACHE        "shm_cache"
#define SHM_DEVICE_NAME_CACHE           "shm_noncache"
#define SHM_DEVICE_PATH_NONCACHE        (SHM_DEVICE_PATH SHM_DEVICE_NAME_NONCACHE)
#define SHM_DEVICE_PATH_CACHE           (SHM_DEVICE_PATH SHM_DEVICE_NAME_CACHE)

#define SHM_CACHE			1
#define SHM_NONCACHE			2

#ifdef ENABLE_DEBUG
#define shm_debug(...)   printk(KERN_INFO SHM_DEVICE_TAG __VA_ARGS__)
#else
#define shm_debug(...)
#endif

//#define SHM_LOG_ENABLE

extern uint shm_lowmem_debug_level;

#define shm_trace(...)      printk(KERN_ALERT SHM_DEVICE_TAG __VA_ARGS__)
#define shm_error(...)      printk(KERN_ERR SHM_DEVICE_TAG __VA_ARGS__)

// Shared Memory Physical Address Conventer
#define SHM_GET_PHYSADDR_NONCACHE                   (uint)NON_CACHE_ADDR
#define SHM_GET_PHYSADDR_CACHE                      (uint)CACHE_ADDR

// Shared Memory Device IO Command List
#define SHM_DEVICE_CMD_GET_MEMINFO                 (0x1F01)
#define SHM_DEVICE_CMD_GET_DEVINFO                 (0x1F02)
//#define SHM_DEVICE_CMD_GET_NONCACHEMEMINFO              (0x1F03)
//#define SHM_DEVICE_CMD_GET_NONCACHEDEVINFO              (0x1F04)

#define SHM_DEVICE_CMD_ALLOCATE                     (0x1F11)
#define SHM_DEVICE_CMD_FREE                         (0x1F12)
#define SHM_DEVICE_CMD_ALLOCATE_NONCACHE            (0x1F13)
#define SHM_DEVICE_CMD_FREE_NONCACHE                (0x1F14)

#define SHM_DEVICE_CMD_INVALIDATE                   (0x1F21)
#define SHM_DEVICE_CMD_CLEAN                        (0x1F22)
#define SHM_DEVICE_CMD_CLEANANDINVALIDATE           (0x1F23)

#define SHM_DEVICE_THRESHOLD     	                (0x0040)
#define SHM_DEVICE_ALIGNMENT_MIN   	                (0x0020)
#define SHM_DEVICE_ALIGNMENT_MAX   	                (0x400000)

#define shm_round_size(size)                    \
	do { \
		size_t temp; \
		temp = size % SHM_DEVICE_ALIGNMENT_MIN; \
		if ((temp) || (size == 0)) \
			size += SHM_DEVICE_ALIGNMENT_MIN - temp; \
	} while(0)

#define shm_round_alignment(align)               \
	do { \
		if (align < SHM_DEVICE_ALIGNMENT_MIN) \
			align = SHM_DEVICE_ALIGNMENT_MIN; \
	} while(0)

static int inline shm_check_alignment (size_t Alignment)
{
	size_t temp = 2;

	if (Alignment == 0)
		return 0;

	do {
		if (temp == Alignment)
			return 0;
		temp = temp << 1;
	} while ((temp <= SHM_DEVICE_ALIGNMENT_MAX) && (temp <= Alignment));

	return -1;
}


/*******************************************************************************
    Data structure Defined
*/

typedef struct
{
	unsigned long m_param1;
	unsigned long m_param2;
	unsigned long m_param3;
} shm_driver_operation_t;

typedef struct _memory_node_t *memory_node_ptr_t;

// memory managerment engine in linux kernel
typedef struct _memory_node_t
{

	/* Dual-linked list of nodes. */
	memory_node_ptr_t m_next;
	memory_node_ptr_t m_prev;

	/* Dual-linked list of free nodes. */
	memory_node_ptr_t m_next_free;
	memory_node_ptr_t m_prev_free;

	/* Information for this node. */
	size_t m_addr;		// true addr for alignment = m_addr + m_offset
	size_t m_size;		// true size for alignment = m_size - m_offset

	size_t m_alignment;
	size_t m_offset;
	pid_t m_threadid;		//record the caller's thread id for debug usage
	char m_threadname[16];	// thread name
	pid_t m_taskid;		// 9/23 qzhang@marvell.com add for releas
	char m_taskname[16];
	/* record locking task for shm lowmem killer */
	pid_t m_usrtaskid;
	int  b_takeover;
	char m_usrtaskname[16];

	size_t m_phyaddress;
	struct rb_node __rb_node;

} memory_node_t;

// memory managerment engine
typedef struct
{

	size_t m_base;
	size_t m_size;
	size_t m_virt_base;
	size_t m_cache_or_noncache;
	size_t m_threshold;		/* Allocation threshold. */

	size_t m_size_used;
	size_t m_size_free;

	size_t m_peak_usedmem;	// the peak of memory in use history
	size_t m_max_freeblock;	// size of largest unused block
	size_t m_min_freeblock;	// size of smallest unused block
	size_t m_max_usedblock;	// size of largest memory used
	size_t m_min_usedblock;	// size of smallest memory used
	uint m_num_freeblock;		// amount of unused block
	uint m_num_usedblock;		// amount of unused block

	memory_node_t m_root;

	struct semaphore m_mutex;
	struct rb_root m_shm_root;

} memory_engine_t;

typedef struct
{
	int minor;
	char *name;
	struct cdev *cdev;
	struct file_operations *fops;
} shm_driver_cdev_t;

typedef int (*shm_device_shrinker) (int allocsize);

typedef struct
{

	size_t m_base;
	size_t m_size;
	size_t m_threshold;

	size_t m_base_cache;		//what's this used for?? we use different physical address for cache and non-cache??what's the hardware map

	memory_engine_t *m_engine;

	/* share memory shrinker */
	shm_device_shrinker m_shrinker;
} shm_device_t;

#define UNKNOWN_OOM_ADJ -100
#define MAX_SHM_USR_NODE_NUM 100

typedef struct shm_usr_node *shm_usr_ptr_t;

struct shm_usr_node
{
	pid_t m_taskid;
	char m_taskname[16];
	size_t m_size_alloc;
	size_t m_size_use;
	int m_oom_adj;
};

struct shm_stat_info
{
	size_t m_size;
	size_t m_used;

	size_t m_count;
	struct shm_usr_node m_nodes[MAX_SHM_USR_NODE_NUM];
};

struct shm_device_priv_data
{
	pid_t m_taskid;
	shm_device_t *m_device;
};

static int inline shm_find_oomadj_by_pid (pid_t taskid)
{
	struct task_struct *task = NULL;
	struct signal_struct *sig = NULL;

	task = find_task_by_vpid (taskid);
	if (task) {
		sig = task->signal;

		if (sig)
			return sig->oom_adj;
	}
	return UNKNOWN_OOM_ADJ;
}
#endif /* __shm_type_h__ */
