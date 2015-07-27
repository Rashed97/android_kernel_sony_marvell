/*
 * MV88DE3xxx Power Management Routines
 *
 * Copyright (c) 2010-2011 Marvell
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License.
 */
#include <linux/init.h>
#include <linux/suspend.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <asm/cacheflush.h>
#include <asm/io.h>

#include <mach/irqs.h>
#include <mach/galois_generic.h>
#include <mach/galois_platform.h>
#include <mach/io.h>
#include <mach/timex.h>
#include <mach/sm.h>
#include "common.h"


extern void tauros3_shutdown(void);
extern void tauros3_restart(void);
extern void galois_irq_suspend(void);
extern void galois_irq_resume(void);
extern void galois_pwm_suspend(void);
extern void galois_pwm_resume(void);
extern void galois_spi_resume(void);
extern void galois_twsi_resume(void);
extern void galois_gpio_suspend(void);
extern void galois_gpio_resume(void);
extern void berlin_sm_suspend(void);
extern void berlin_sm_resume(void);
extern void mv88de3100_close_phys_cores(void);

extern void mv88de3100_cpu_suspend(long);
extern void mv88de3100_cpu_resume(void);
extern void resume_prepare_cpus(void);

static void mv88de3100_restore_system(void)
{
	int msg = MV_SM_IR_Linuxready;

	galois_irq_resume();

#ifndef CONFIG_MV88DE3100_PM_DISABLE_SPI_TWSI
#ifdef CONFIG_MV88DE3100_SPI
	galois_spi_resume();
#endif
#ifdef CONFIG_MV88DE3100_I2C
	galois_twsi_resume();
#endif
#endif
#ifdef CONFIG_MV88DE3100_PWM
	galois_pwm_resume();
#endif
#ifdef CONFIG_MV88DE3100_GPIO
	galois_gpio_resume();
#endif
	/* It's ugly put here. But IR is registered as a miscdevice there's no
	 * resume hook to use. remove this code once there's a better way
	 */
	bsm_msg_send(MV_SM_ID_IR, &msg, sizeof(msg));
	/* The linux ready mark should be set in the power module in new SM,
	 * so I send it to the ID_POWER, but consider that old SM is still in
	 * use, so I can't remove the msg sent to IR before we abandon old SM.
	 */
	bsm_msg_send(MV_SM_ID_POWER, &msg, sizeof(msg));

	mv88de3100_close_phys_cores();
}

static void mv88de3100_suspend_system(void)
{
#ifdef CONFIG_MV88DE3100_GPIO
	galois_gpio_suspend();
#endif
#ifdef CONFIG_MV88DE3100_PWM
	galois_pwm_suspend();
#endif
	galois_irq_suspend();
}

void EnterStandby_2(void)
{
	int msg[2];

	msg[0] = MV_SM_POWER_WARMDOWN_REQUEST_2;
	msg[1] = virt_to_phys(mv88de3100_cpu_resume);

	bsm_msg_send(MV_SM_ID_POWER,msg,2*sizeof(int));
}

/*
 * task freezed, deviced stopped
 */
static int mv88de3100_pm_enter(suspend_state_t state)
{
	if (state != PM_SUSPEND_MEM)
		return -EINVAL;

	mv88de3100_suspend_system();

	flush_cache_all();
#ifdef CONFIG_CACHE_TAUROS3
	tauros3_shutdown();
#endif
	mv88de3100_cpu_suspend(PHYS_OFFSET - PAGE_OFFSET);

	/* come back */
	cpu_init();

#ifdef CONFIG_CACHE_TAUROS3
	tauros3_restart();
#endif
#ifdef CONFIG_SMP
	resume_prepare_cpus();
#endif
	mv88de3100_restore_system();
	printk("Welcome back\n");
	return 0;
}

static struct platform_suspend_ops mv88de3100_pm_ops = {
	.valid = suspend_valid_only_mem,
	.enter = mv88de3100_pm_enter,
};

static int __init mv88de3100_pm_init(void)
{
	suspend_set_ops(&mv88de3100_pm_ops);
	return 0;
}

/* callback from assembly code */
void mv88de3100_pm_cb_flushcache(void)
{
	flush_cache_all();
}

late_initcall(mv88de3100_pm_init);
