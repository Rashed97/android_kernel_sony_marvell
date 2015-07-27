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
//! \file shm_device.h
//! \brief Shared Memory device
//!
//!   Platform = Linux, Berlin_G2
//!
//! Purpose:
//!
//!
//! Version, Date and Author :
//!     V 1.00, 5/19/2010,  Fang Bao
//!     V 1.01, 9/02/2010,  Fang Bao,   Fix bug: InvalidateCache(), CleanCache(), CleanAndInvalidateCache() should accept virtual address of cache memory, not physical address of cache memory
//!     v 1.03, 10/18/2011, Qiang Zhang,kernel dependent on this file, move from GS to kernel tree
//!     v 1.0.4 10/23/2011, Qiang Zhang,add mechanism to track shm usage info
//! Note:
////////////////////////////////////////////////////////////////////////////////

#ifndef __shm_device_h__
#define __shm_device_h__

#include "shm_type.h"

int shm_device_create(shm_device_t **device, uint base, uint size, uint threshold);

int shm_device_destroy(shm_device_t **device);

int shm_device_get_meminfo(shm_device_t *device, pMV_SHM_MemInfo_t pInfo);

int shm_device_get_baseinfo(shm_device_t *device, pMV_SHM_BaseInfo_t pInfo);

uint shm_device_allocate(shm_device_t *device, size_t size, size_t align);

int shm_device_free(shm_device_t *device, size_t offset);

int shm_device_cache_invalidate(void * virtaddr_cache, size_t size);

int shm_device_cache_clean(void * virtaddr_cache, size_t size);

int shm_device_cache_clean_and_invalidate(void * virtaddr_cache, size_t size);

int shm_device_cache(shm_device_t *device, uint cmd,
				shm_driver_operation_t op);

int shm_device_get_detail(shm_device_t *device, char *buffer, int count);

int shm_device_show_detail(shm_device_t *device, struct seq_file *file);

int shm_device_release_by_taskid(shm_device_t *device, pid_t taskid);

int shm_device_release_by_usrtaskid(shm_device_t *device, pid_t usrtaskid);

int shm_device_takeover(shm_device_t *device, size_t offset);

int shm_device_giveup(shm_device_t *device, size_t offset);

int shm_device_get_stat(shm_device_t *device, struct shm_stat_info *stat);

int shm_device_dump_stat(shm_device_t *device);

int shm_device_show_stat(shm_device_t *devce, struct seq_file  *file);
#endif /* __shm_device_h__ */
