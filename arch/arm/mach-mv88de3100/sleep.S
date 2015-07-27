#include <linux/linkage.h>
#include <asm/assembler.h>
#include <asm/system.h>

	.text

ENTRY(mv88de3100_cpu_suspend)
	stmfd	sp!, {r4 - r12, lr}	@ save registers on stack

	mov	r1, r0
	ldr	r3, =resume_with_mmu	@ resume function
	bl	cpu_suspend
	bl	EnterStandby_2
20:
	nop
	b	20b

	.align 5
resume_with_mmu:
	ldmfd	sp!, {r4 - r12, pc}		@ return to caller

ENTRY(cpu_suspend)
	mov	r9, lr
	mov	r2, sp			@ current virtual SP
	ldr	r0, =64
	sub	sp, sp, r0		@ allocate CPU state on stack
	mov	r0, sp			@ save pointer
	stmfd	sp!, {r1, r2, r3}	@ save v:p, virt SP, return fn
	ldr	r3, =sleep_save_sp
	add	r2, sp, r1		@ convert SP to phys
	str	r2, [r3]		@ save phys SP
	bl	cpu_do_suspend

	mov	lr, r9
	b	mv88de3100_pm_cb_flushcache
ENDPROC(cpu_suspend)
	.ltorg

ENTRY(cpu_do_suspend)
	stmfd	sp!, {r4 - r11, lr}
	mrc     p15, 0, r4, c2, c0, 2
	mrc     p15, 1, r5, c15, c1, 0
	mrc     p15, 1, r6, c15, c1, 1
	mrc     p15, 1, r7, c15, c2, 0
	mrc	p15, 0, r8, c9, c14, 0
	mrc     p15, 0, r9, c13, c0, 3  @ TLS
	stmia	r0!, {r4 - r9}

	mrc     p15, 0, r4, c13, c0, 0  @ FCSE/PID
	mrc     p15, 0, r5, c13, c0, 1  @ Context ID
	mrc     p15, 0, r6, c3, c0, 0   @ Domain ID
	mrc     p15, 0, r7, c2, c0, 0   @ TTB 0
	mrc     p15, 0, r8, c2, c0, 1   @ TTB 1
	mrc     p15, 0, r9, c1, c0, 0   @ Control register
	mrc     p15, 0, r10, c1, c0, 1  @ Auxiliary control register
	mrc     p15, 0, r11, c1, c0, 2  @ Co-processor access control
	stmia	r0, {r4 - r11}
	ldmfd	sp!, {r4 - r11, pc}
ENDPROC(cpu_do_suspend)


/*
 * r0 = control register value
 * r1 = v:p offset (preserved by cpu_do_resume)
 * r2 = phys page table base
 * r3 = L1 section flags
 */
ENTRY(cpu_resume_mmu)
	adr	r4, cpu_resume_turn_mmu_on
	mov	r4, r4, lsr #20
	orr	r3, r3, r4, lsl #20
	ldr	r5, [r2, r4, lsl #2]	@ save old mapping
	str	r3, [r2, r4, lsl #2]	@ setup 1:1 mapping for mmu code
	sub	r2, r2, r1
	ldr	r3, =cpu_resume_after_mmu
	bic	r1, r0, #CR_C		@ ensure D-cache is disabled
	b	cpu_resume_turn_mmu_on
ENDPROC(cpu_resume_mmu)
	.ltorg
	.align	5
cpu_resume_turn_mmu_on:
	mcr	p15, 0, r1, c1, c0, 0	@ turn on MMU, I-cache, etc
	mrc	p15, 0, r1, c0, c0, 0	@ read id reg
	mov	r1, r1
	mov	r1, r1
	mov	pc, r3			@ jump to virtual address
ENDPROC(cpu_resume_turn_mmu_on)
cpu_resume_after_mmu:
	str	r5, [r2, r4, lsl #2]	@ restore old mapping
	mcr	p15, 0, r0, c1, c0, 0	@ turn on D-cache
	mov	pc, lr
ENDPROC(cpu_resume_after_mmu)

	.data
	.align
ENTRY(mv88de3100_cpu_resume)
	ldr	r0, sleep_save_sp	@ stack phys addr
	setmode	PSR_I_BIT | PSR_F_BIT | SVC_MODE, r1  @ set SVC, irqs off
	ldmia	r0!, {r1, sp, lr}

	mov	ip, #0
	mcr	p15, 0, ip, c8, c7, 0	@ invalidate TLBs
	mcr	p15, 0, ip, c7, c5, 0	@ invalidate I cache
	ldmia	r0!, {r4 - r9}
	mcr     p15, 0, r4, c2, c0, 2
	mcr     p15, 1, r5, c15, c1, 0
	mcr     p15, 1, r6, c15, c1, 1
	mcr     p15, 1, r7, c15, c2, 0
	mcr	p15, 0, r8, c9, c14, 0
	mcr	p15, 0, r9, c13, c0, 3	@ TLS

	ldmia	r0, {r4 - r11}
	mcr	p15, 0, r4, c13, c0, 0	@ FCSE/PID
	mcr	p15, 0, r5, c13, c0, 1	@ Context ID
	mcr	p15, 0, r6, c3, c0, 0	@ Domain ID
	mcr	p15, 0, r7, c2, c0, 0	@ TTB 0
	mcr	p15, 0, r8, c2, c0, 1	@ TTB 1
	mcr	p15, 0, ip, c2, c0, 2	@ TTB control register
	mcr	p15, 0, r10, c1, c0, 1	@ Auxiliary control register
	mcr	p15, 0, r11, c1, c0, 2	@ Co-processor access control
	ldr	r4, =0xff0a81a8		@ PRRR
	ldr	r5, =0x40e040e0		@ NMRR
	mcr	p15, 0, r4, c10, c2, 0	@ write PRRR
	mcr	p15, 0, r5, c10, c2, 1	@ write NMRR
	isb
	mov	r0, r9			@ control register
	mov	r2, r7, lsr #14		@ get TTB0 base
	mov	r2, r2, lsl #14
	ldr	r3, =0x40e
	b	cpu_resume_mmu
ENDPROC(mv88de3100_cpu_resume)

sleep_save_sp:
	.long	0				@ preserve stack phys ptr here
