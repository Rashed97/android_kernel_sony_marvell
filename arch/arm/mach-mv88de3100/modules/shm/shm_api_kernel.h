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
//! \file shm_api_kernel.h
//! \brief Shared Memory device
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

#ifndef __shm_api_kernel_h__
#define __shm_api_kernel_h__

int MV_SHM_Init(shm_device_t *device_noncache, shm_device_t *device_cache);

int MV_SHM_Exit(void);

#endif /* __shm_device_h__ */
