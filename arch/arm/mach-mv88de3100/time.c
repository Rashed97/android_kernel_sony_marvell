/*
 * SoC's APB timer handling.
 *
 * Author:	Jisheng Zhang <jszhang@marvell.com>
 * Copyright:	Marvell International Ltd.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 *
 * Timer GALOIS_CLOCKSOURCE(7) is used as free-running clocksource,
 * while timer LINUX_TIMER(0) is used as clock_event_device.
 * Within clockevents mode, the original timer tick mechanism will
 * no longer be useed.
 */

#include <linux/kernel.h>
#include <linux/clockchips.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/clocksource.h>
#include <linux/cnt32_to_63.h>
#include <asm/mach/time.h>
#include <asm/smp_twd.h>
#include <mach/galois_platform.h>
#include <mach/apb_timer.h>

#include "common.h"

static void apbt_set_mode(enum clock_event_mode mode,
			  struct clock_event_device *evt);

static inline unsigned long apbt_readl(int n, unsigned long a)
{
	return readl(APB_TIMER_INST_BASE + a + n * APBTMRS_REG_SIZE);
}

static inline void apbt_writel(int n, unsigned long d, unsigned long a)
{
	writel(d, APB_TIMER_INST_BASE + a + n * APBTMRS_REG_SIZE);
}

static void apbt_enable_int(int n)
{
	unsigned long ctrl = apbt_readl(n, APBTMR_N_CONTROL);
	/* clear pending intr */
	apbt_readl(n, APBTMR_N_EOI);
	ctrl &= ~APBTMR_CONTROL_INT;
	apbt_writel(n, ctrl, APBTMR_N_CONTROL);
}

static void apbt_disable_int(int n)
{
	unsigned long ctrl = apbt_readl(n, APBTMR_N_CONTROL);

	ctrl |= APBTMR_CONTROL_INT;
	apbt_writel(n, ctrl, APBTMR_N_CONTROL);
}

/*
 * used to convert timer cycles to ns
 */
static unsigned long cyc2ns_scale;
#define CYC2NS_SCALE_FACTOR 10

static void __init set_cyc2ns_scale(unsigned long timer_hz)
{
	unsigned long timer_khz = timer_hz/1000;
	cyc2ns_scale = (1000000 << CYC2NS_SCALE_FACTOR)/timer_khz;

	/* To clear the top bit of returned by cnt32_to_63().
	 * If it's even, then *cyc2ns_scale means also << 1 too.
	 * So if the LSB is 1, round it up
	 */
	if(cyc2ns_scale & 1)
		cyc2ns_scale++;
}

unsigned long long sched_clock(void)
{
	unsigned long long v = cnt32_to_63(0xFFFFFFFF - apbt_readl(GALOIS_CLOCKSOURCE, APBTMR_N_CURRENT_VALUE));
	return (v * cyc2ns_scale) >> CYC2NS_SCALE_FACTOR;
}

static cycle_t apbt_clksrc_read(struct clocksource *cs)
{
	return 0xffffffff - apbt_readl(GALOIS_CLOCKSOURCE, APBTMR_N_CURRENT_VALUE);
}

/*
 * start count down from 0xffff_ffff. this is done by toggling the enable bit
 * then load initial load count to ~0.
 */
void apbt_start_counter(int n)
{
	unsigned long ctrl = apbt_readl(n, APBTMR_N_CONTROL);

	ctrl &= ~APBTMR_CONTROL_ENABLE;
	apbt_writel(n, ctrl, APBTMR_N_CONTROL);
	apbt_writel(n, ~0, APBTMR_N_LOAD_COUNT);
	/* enable, mask interrupt */
	ctrl &= ~APBTMR_CONTROL_MODE_PERIODIC;
	ctrl |= (APBTMR_CONTROL_ENABLE | APBTMR_CONTROL_INT);
	apbt_writel(n, ctrl, APBTMR_N_CONTROL);
}

static void apbt_restart_clocksource(struct clocksource *cs)
{
	apbt_start_counter(GALOIS_CLOCKSOURCE);
}

static struct clocksource apbt_clksrc = {
	.name		= "apbt",
	.shift		= 22,
	.rating		= 300,
	.read		= apbt_clksrc_read,
	.mask		= CLOCKSOURCE_MASK(32),
	.flags		= CLOCK_SOURCE_IS_CONTINUOUS,
	.resume		= apbt_restart_clocksource,
};

#define CLOCK_MIN 	10000 /*Minimum ns value, for clockevents*/
static int apbt_set_next_event(unsigned long delta,
			       struct clock_event_device *dev)
{
	unsigned long ctrl;
	int timer_num;

	timer_num = LINUX_TIMER;
	/* Disable timer */
	ctrl = apbt_readl(timer_num, APBTMR_N_CONTROL);
	ctrl &= ~APBTMR_CONTROL_ENABLE;
	apbt_writel(timer_num, ctrl, APBTMR_N_CONTROL);
	/* write new count */
	apbt_writel(timer_num, delta, APBTMR_N_LOAD_COUNT);
	ctrl |= APBTMR_CONTROL_ENABLE;
	apbt_writel(timer_num, ctrl, APBTMR_N_CONTROL);

	return 0;
}

static struct clock_event_device apbt_clkevt = {
	.name		= "apbt",
	.features	= CLOCK_EVT_FEAT_ONESHOT | CLOCK_EVT_FEAT_PERIODIC,
	.shift		= 32,
	.rating		= 300,
	.set_next_event	= apbt_set_next_event,
	.set_mode	= apbt_set_mode,
};

static void apbt_set_mode(enum clock_event_mode mode,
			  struct clock_event_device *dev)
{
	unsigned long ctrl;
	uint64_t delta;
	int timer_num = LINUX_TIMER;

	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		delta = ((uint64_t)(NSEC_PER_SEC/HZ)) * apbt_clkevt.mult;
		delta >>= apbt_clkevt.shift;
		ctrl = apbt_readl(timer_num, APBTMR_N_CONTROL);
		ctrl |= APBTMR_CONTROL_MODE_PERIODIC;
		apbt_writel(timer_num, ctrl, APBTMR_N_CONTROL);
		/*
		 * DW APB p. 46, have to disable timer before load counter,
		 * may cause sync problem.
		 */
		ctrl &= ~APBTMR_CONTROL_ENABLE;
		apbt_writel(timer_num, ctrl, APBTMR_N_CONTROL);
		pr_debug("Setting clock period %d for HZ %d\n", (int)delta, HZ);
		apbt_writel(timer_num, delta, APBTMR_N_LOAD_COUNT);
		ctrl &= ~APBTMR_CONTROL_INT;
		ctrl |= APBTMR_CONTROL_ENABLE;
		apbt_writel(timer_num, ctrl, APBTMR_N_CONTROL);
		break;
	case CLOCK_EVT_MODE_ONESHOT:
		/* APB timer does not have one-shot mode, use free running mode */
		ctrl = apbt_readl(timer_num, APBTMR_N_CONTROL);
		/*
		 * set free running mode, this mode will let timer reload max
		 * timeout which will give time (3min on 25MHz clock) to rearm
		 * the next event, therefore emulate the one-shot mode.
		 */
		ctrl &= ~APBTMR_CONTROL_ENABLE;
		ctrl &= ~APBTMR_CONTROL_MODE_PERIODIC;

		apbt_writel(timer_num, ctrl, APBTMR_N_CONTROL);
		/* write again to set free running mode */
		apbt_writel(timer_num, ctrl, APBTMR_N_CONTROL);

		/*
		 * DW APB p. 46, load counter with all 1s before starting free
		 * running mode.
		 */
		apbt_writel(timer_num, ~0, APBTMR_N_LOAD_COUNT);
		ctrl &= ~APBTMR_CONTROL_INT;
		ctrl |= APBTMR_CONTROL_ENABLE;
		apbt_writel(timer_num, ctrl, APBTMR_N_CONTROL);
		break;
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
		apbt_disable_int(timer_num);
		ctrl = apbt_readl(timer_num, APBTMR_N_CONTROL);
		ctrl &= ~APBTMR_CONTROL_ENABLE;
		apbt_writel(timer_num, ctrl, APBTMR_N_CONTROL);
		break;
	case CLOCK_EVT_MODE_RESUME:
		apbt_enable_int(timer_num);
		break;
	}
}

static irqreturn_t apbt_interrupt_handler(int irq, void *dev_id)
{
	/*
	 * clear the interrupt before event_handler(), or if a new short
	 * event is issued in event_handler(), it may be missed, and
	 * system will hang.
	 */
	apbt_readl(LINUX_TIMER, APBTMR_N_EOI);
	apbt_clkevt.event_handler(&apbt_clkevt);
	return IRQ_HANDLED;
}

static struct irqaction apbt_irq = {
	.name		= "apbt",
	.flags		= IRQF_DISABLED | IRQF_TIMER,
	.handler	= apbt_interrupt_handler
};

void read_persistent_clock(struct timespec *ts)
{
	ts->tv_sec = 1234567890; /* 23:31:30 13 Feb 2009 */
	ts->tv_nsec = 0;
}

static void __init apbt_init(void)
{
	u32 tclk;
	struct clk *cfg_clk;

	cfg_clk = clk_get(NULL, "cfg");
	BUG_ON(IS_ERR(cfg_clk));
	tclk = clk_get_rate(cfg_clk);
	set_cyc2ns_scale(tclk); /* set timer cyc2ns scaler */
	apbt_clksrc.mult = clocksource_hz2mult(tclk, apbt_clksrc.shift);
	apbt_clkevt.mult = div_sc(tclk, NSEC_PER_SEC, apbt_clkevt.shift);
#ifdef CONFIG_LOCAL_TIMERS
	twd_base = __io(MEMMAP_CPU_PMR_REG_BASE + 0x600);
#endif

	apbt_clkevt.max_delta_ns = clockevent_delta2ns(0xfffffffe, &apbt_clkevt);
	apbt_clkevt.min_delta_ns = clockevent_delta2ns(CLOCK_MIN, &apbt_clkevt);
	apbt_clkevt.cpumask = cpumask_of(0);
	clocksource_register(&apbt_clksrc);
	clockevents_register_device(&apbt_clkevt);

	/* register s/w timer irq to linux */
	setup_irq(IRQ_APB_TIMERINST1_0, &apbt_irq);
}

struct sys_timer apbt_timer = {
	.init = apbt_init,
};
