/*
 * apb_timer.h: Driver for Langwell APB timer based on Synopsis DesignWare
 *
 * Some definition are copied from arch/x86/include/asm/apb_timer.h
 *
 * (C) Copyright 2009 Intel Corporation
 * Author: Jacob Pan (jacob.jun.pan@intel.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 *
 * Note:
 */

#ifndef GALOIS_APBT_H
#define GALOIS_APBT_H

/* Langwell DW APB timer registers */
#define APBTMR_N_LOAD_COUNT    0x00
#define APBTMR_N_CURRENT_VALUE 0x04
#define APBTMR_N_CONTROL       0x08
#define APBTMR_N_EOI           0x0c
#define APBTMR_N_INT_STATUS    0x10

#define APBTMRS_INT_STATUS     0xa0
#define APBTMRS_EOI            0xa4
#define APBTMRS_RAW_INT_STATUS 0xa8
#define APBTMRS_COMP_VERSION   0xac
#define APBTMRS_REG_SIZE       0x14

/* register bits */
#define APBTMR_CONTROL_ENABLE  (1<<0)
#define APBTMR_CONTROL_MODE_PERIODIC   (1<<1) /*1: periodic 0:free running */
#define APBTMR_CONTROL_INT     (1<<2)

extern void apbt_start_counter(int n);

/* timer: LINUX_TIMER=0, CLOCKSOURCE= 7 */
#define LINUX_TIMER		0
#define GALOIS_DEBUG_TIMER	7
#define GALOIS_CLOCKSOURCE  (GALOIS_DEBUG_TIMER)

#endif
