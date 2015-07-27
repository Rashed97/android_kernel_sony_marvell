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
//! \file memory_engine.h
//! \brief shared memory managerment engine
//!
//!   Platform = Linux, Berlin_G2
//!
//! Purpose:
//!
//!
//! Version, Date and Author :
//!     V 1.00, 5/19/2010,  Fang Bao
//!     v 1.03, 10/18/2011, Qiang Zhang,kernel dependent on this file, move from GS to kernel tree
//!     v 1.04, 10/23/2011, Qiang Zhang,add mechanism to track shm usage info
//! Note:
////////////////////////////////////////////////////////////////////////////////

#ifndef __memory_engine_h__
#define __memory_engine_h__

#include "shm_type.h"

#define MEMNODE_ALIGN_ADDR(x)    ((x)->m_addr + (x)->m_offset)
#define MEMNODE_ALIGN_SIZE(x)    ((x)->m_size - (x)->m_offset)

int memory_engine_create(memory_engine_t **engine, uint base,
							uint size, uint threshold);

int memory_engine_destroy(memory_engine_t **engine);

int memory_engine_allocate(memory_engine_t *engine, uint size,
							uint alignment, memory_node_t **node);

int memory_engine_free(memory_engine_t *engine, int alignaddr);

int memory_engine_cache(memory_engine_t *engine, uint cmd,
				shm_driver_operation_t op);

int memory_engine_gothrough(memory_engine_t *engine, char *buffer,
								int count);

int memory_engine_show(memory_engine_t *engine, struct seq_file *file);

/* release shm resource once task was killed*/
int memory_engine_release_by_taskid(memory_engine_t *engine, pid_t taskid);

int memory_engine_release_by_usrtaskid(memory_engine_t *engine, pid_t usrtaskid);

int memory_engine_takeover(memory_engine_t *engine, int alignaddr);

int memory_engine_giveup(memory_engine_t *engine, int alignaddr);

int memory_engine_get_stat(memory_engine_t *engine, struct shm_stat_info *stat);

int memory_engine_dump_stat(memory_engine_t *engine);

int memory_engine_show_stat(memory_engine_t *engine, struct seq_file *file);

#endif /* __memory_engine_h__ */
