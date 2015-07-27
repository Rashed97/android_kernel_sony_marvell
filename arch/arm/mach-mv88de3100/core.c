#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/page.h>
#include <asm/memory.h>
#include <asm/setup.h>
#include <asm/mach-types.h>
#include <mach/galois_platform.h>
#include <asm/mach/time.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <mach/apb_timer.h>

#include "common.h"
#include "clock.h"

/*
 * the virtual address mapped should be < PAGE_OFFSET or >= VMALLOC_END.
 * Generally, it should be >= VMALLOC_END.
 */
static struct map_desc mv88de3100_io_desc[] __initdata = {
	{/* map phys space 0xF6000000~0xF9000000(doesn't cover Flash area) to same virtual space*/
		.virtual = 0xF6000000,
		.pfn = __phys_to_pfn(0xF6000000),
		.length = 0x03000000,
		.type = MT_DEVICE
	},
	/* Don't map ROM,vectors space: 0xFF800000~0xFFFFFFFF */
};

/*
 * do very early than any initcalls, it's better in setup_arch()
 */
static void __init soc_initialize(void)
{
	apbt_start_counter(GALOIS_CLOCKSOURCE);
	mv88de3100_clk_init();
}

static void __init mv88de3100_map_io(void)
{
	iotable_init(mv88de3100_io_desc, ARRAY_SIZE(mv88de3100_io_desc));
	soc_initialize(); /* some ugly to put here, but it works:-) */
}

/*
 * mv88de3100 SoC initialization, in arch_initcall()
 */
static void __init mv88de3100_init(void)
{
	mv88de3100_devs_init();
}

#ifdef CONFIG_CACHE_TAUROS3
#include <asm/hardware/cache-tauros3.h>
static int __init mv88de3015_l2x0_init(void)
{
	return tauros3_init(__io(MEMMAP_CPU_SL2C_REG_BASE), 0x0, 0x0);
}
early_initcall(mv88de3015_l2x0_init);
#elif defined(CONFIG_CACHE_L2X0)
#include <asm/hardware/cache-l2x0.h>
static int __init mv88de3015_l2x0_init(void)
{
	l2x0_init(__io(MEMMAP_CPU_SL2C_REG_BASE), 0x30000000, 0xfe0fffff);

	return 0;
}
early_initcall(mv88de3015_l2x0_init);
#endif

static void __init mv88de3100_fixup(struct machine_desc *desc, struct tag *tag,
		char **cmdline, struct meminfo *mi)
{
	struct tag *tmp_tag = tag;
	/*
	 * MV88DE3100 only receives ATAG_CORE, ATAG_MEM, ATAG_CMDLINE, ATAG_NONE
	 * Here change ATAG_MEM to be controlled by Linux itself.
	 */
	if (tmp_tag->hdr.tag == ATAG_CORE) {
		for (; tmp_tag->hdr.size; tmp_tag = tag_next(tmp_tag)) {
			if (tmp_tag->hdr.tag == ATAG_MEM) {
				tmp_tag->u.mem.size = CONFIG_MV88DE3100_CPU0MEM_SIZE;
				tmp_tag->u.mem.start = CONFIG_MV88DE3100_CPU0MEM_START;
				break;
			}
		}
	}
}

#define MV88DE3100_PHYSIO_BASE 0xF0000000
#define ARM_BOOT_PARAMS_ADDR (CONFIG_MV88DE3100_CPU0MEM_START + 0x00000100)

MACHINE_START(MV88DE3100, "MV88DE3100")
	/* Maintainer: Jisheng Zhang<jszhang@marvell.com> */
	.phys_io	= MV88DE3100_PHYSIO_BASE,
	.io_pg_offst	= (((u32)MV88DE3100_PHYSIO_BASE) >> 18) & 0xfffc,
	.boot_params	= ARM_BOOT_PARAMS_ADDR,

	.fixup		= mv88de3100_fixup,
	.map_io		= mv88de3100_map_io,
	.init_irq	= mv88de3100_init_irq,
	.timer		= &apbt_timer,
	.init_machine	= mv88de3100_init,
MACHINE_END
