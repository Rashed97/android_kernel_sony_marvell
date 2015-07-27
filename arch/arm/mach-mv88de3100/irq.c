/*
 *  Generic IRQ handling, APB Peripheral IRQ demultiplexing, etc.
 *
 *  Author:	Jisheng Zhang <jszhang@marvell.com>
 *  Copyright:	Marvell International Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/irq.h>

#include <asm/io.h>
#include <mach/galois_platform.h>
#include <asm/hardware/gic.h>

#define APB_ICTL_IRQ_INTEN_L		0x00
#define APB_ICTL_IRQ_INTMASK_L		0x08
#define APB_ICTL_IRQ_FINALSTATUS_L	0x30

#define APB_ICTL_INTEN		APB_ICTL_INST0_BASE + APB_ICTL_IRQ_INTEN_L
#define APB_ICTL_INTMASK	APB_ICTL_INST0_BASE + APB_ICTL_IRQ_INTMASK_L
#define APB_ICTL_FINALSTATUS	APB_ICTL_INST0_BASE + APB_ICTL_IRQ_FINALSTATUS_L
#define SM_ICTL_INTEN		SM_APB_ICTL1_BASE + APB_ICTL_IRQ_INTEN_L
#define SM_ICTL_INTMASK		SM_APB_ICTL1_BASE + APB_ICTL_IRQ_INTMASK_L
#define SM_ICTL_FINALSTATUS	SM_APB_ICTL1_BASE + APB_ICTL_IRQ_FINALSTATUS_L

static void apb_enable_irq(unsigned int irq)
{
	u32 u;
	int pin = irq - GALOIS_APB_IRQ_START;

	u = __raw_readl(APB_ICTL_INTMASK);
	u &= ~(1 << (pin & 31));
	__raw_writel(u, APB_ICTL_INTMASK);

	u = __raw_readl(APB_ICTL_INTEN);
	u |= (1 << (pin & 31));
	__raw_writel(u, APB_ICTL_INTEN);
}

static void apb_disable_irq(unsigned int irq)
{
	u32 u;
	int pin = irq - GALOIS_APB_IRQ_START;

	u = __raw_readl(APB_ICTL_INTMASK);
	u |= (1 << (pin & 31));
	__raw_writel(u, APB_ICTL_INTMASK);

	u = __raw_readl(APB_ICTL_INTEN);
	u &= ~(1 << (pin & 31));
	__raw_writel(u, APB_ICTL_INTEN);
}

static void apb_mask_irq(unsigned int irq)
{
	u32 u;
	int pin = irq - GALOIS_APB_IRQ_START;

	u = __raw_readl(APB_ICTL_INTMASK);
	u |= (1 << (pin & 31));
	__raw_writel(u, APB_ICTL_INTMASK);
}

static void apb_unmask_irq(unsigned int irq)
{
	u32 u;
	int pin = irq - GALOIS_APB_IRQ_START;

	u = __raw_readl(APB_ICTL_INTMASK);
	u &= ~(1 << (pin & 31));
	__raw_writel(u, APB_ICTL_INTMASK);
}

static void sm_enable_irq(unsigned int irq)
{
	u32 u;
	int pin = irq - GALOIS_SM_IRQ_START;

	u = __raw_readl(SM_ICTL_INTMASK);
	u &= ~(1 << (pin & 31));
	__raw_writel(u, SM_ICTL_INTMASK);

	u = __raw_readl(SM_ICTL_INTEN);
	u |= (1 << (pin & 31));
	__raw_writel(u, SM_ICTL_INTEN);
}

static void sm_disable_irq(unsigned int irq)
{
	u32 u;
	int pin = irq - GALOIS_SM_IRQ_START;

	u = __raw_readl(SM_ICTL_INTMASK);
	u |= (1 << (pin & 31));
	__raw_writel(u, SM_ICTL_INTMASK);

	u = __raw_readl(SM_ICTL_INTEN);
	u &= ~(1 << (pin & 31));
	__raw_writel(u, SM_ICTL_INTEN);
}

static void sm_mask_irq(unsigned int irq)
{
	u32 u;
	int pin = irq - GALOIS_SM_IRQ_START;

	u = __raw_readl(SM_ICTL_INTMASK);
	u |= (1 << (pin & 31));
	__raw_writel(u, SM_ICTL_INTMASK);
}

static void sm_unmask_irq(unsigned int irq)
{
	u32 u;
	int pin = irq - GALOIS_SM_IRQ_START;

	u = __raw_readl(SM_ICTL_INTMASK);
	u &= ~(1 << (pin & 31));
	__raw_writel(u, SM_ICTL_INTMASK);
}

static struct irq_chip apb_irq_chip = {
	.name		= "apb_ictl",
	.enable		= apb_enable_irq,
	.disable	= apb_disable_irq,
	.mask		= apb_mask_irq,
	.unmask		= apb_unmask_irq,
};

static struct irq_chip sm_irq_chip = {
	.name		= "sm_ictl",
	.enable		= sm_enable_irq,
	.disable	= sm_disable_irq,
	.mask		= sm_mask_irq,
	.unmask		= sm_unmask_irq,
};

static void apb_irq_demux(unsigned int irq, struct irq_desc *desc)
{
	int res;
	unsigned long cause;

	desc->chip->ack(irq);
	cause = __raw_readl(APB_ICTL_FINALSTATUS);
	while (cause) {
		res = ffs(cause) - 1;
		cause &= ~(1 << res);
		generic_handle_irq(res + GALOIS_APB_IRQ_START);
	}
	desc->chip->unmask(irq);

}
static void sm_irq_demux(unsigned int irq, struct irq_desc *desc)
{
	int res;
	unsigned long cause;

	desc->chip->ack(irq);
	cause = __raw_readl(SM_ICTL_FINALSTATUS);
	while (cause) {
		res = ffs(cause) - 1;
		cause &= ~(1 << res);
		generic_handle_irq(res + GALOIS_SM_IRQ_START);
	}
	desc->chip->unmask(irq);
}

void __init mv88de3100_init_irq(void)
{
	int i;

	gic_dist_init(0, __io(MEMMAP_GIC_DIST_BASE), 29);
	gic_cpu_init(0, __io(MEMMAP_GIC_CPU_BASE));

	for (i = GALOIS_APB_IRQ_START; i < GALOIS_SM_IRQ_START; i++)
		set_irq_chip(i, &apb_irq_chip);
	for (i = GALOIS_SM_IRQ_START; i < NR_IRQS; i++)
		set_irq_chip(i, &sm_irq_chip);

	set_irq_chained_handler(IRQ_ICTLINST0CPUIRQ, apb_irq_demux);
	set_irq_chained_handler(IRQ_SM2SOCHWINT0, sm_irq_demux);
}

#ifdef CONFIG_PM
static u32 apb_ictl_intmask;
static u32 apb_ictl_inten;
static u32 sm_ictl_intmask;
static u32 sm_ictl_inten;

void galois_irq_suspend(void)
{
	unsigned long flags;

	local_irq_save(flags);
	apb_ictl_intmask = __raw_readl(APB_ICTL_INTMASK);
	apb_ictl_inten = __raw_readl(APB_ICTL_INTEN);
	sm_ictl_intmask = __raw_readl(SM_ICTL_INTMASK);
	sm_ictl_inten = __raw_readl(SM_ICTL_INTEN);
	gic_dist_save(0);
	local_irq_restore(flags);
}

void galois_irq_resume(void)
{
	unsigned long flags;

	local_irq_save(flags);
	gic_dist_restore(0);
	__raw_writel(apb_ictl_intmask, APB_ICTL_INTMASK);
	__raw_writel(apb_ictl_inten, APB_ICTL_INTEN);
	__raw_writel(sm_ictl_intmask, SM_ICTL_INTMASK);
	__raw_writel(sm_ictl_inten, SM_ICTL_INTEN);
	local_irq_restore(flags);
}
#endif
