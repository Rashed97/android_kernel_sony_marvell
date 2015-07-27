/*
 * Debugging macro include header
 *
 *  Copyright (C) 1994-1999 Russell King
 *  Moved from linux/arch/arm/kernel/debug.S by Ben Dooks
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/
#include <mach/galois_platform.h>

#define _SM_APB_REG_BASE		(MEMMAP_SM_REG_BASE + 0x40000)
#define _SM_APB_UART0_BASE		(_SM_APB_REG_BASE + 0x9000)

	.macro	addruart, rp, rv
	ldr	\rp, =_SM_APB_UART0_BASE
	ldr	\rv, =_SM_APB_UART0_BASE	@ virtual = physical
	.endm

#define UART_SHIFT	2
#include <asm/hardware/debug-8250.S>
