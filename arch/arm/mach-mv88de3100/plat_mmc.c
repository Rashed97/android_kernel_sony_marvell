/*
 * plat related code for Marvell 88DE3100 SoC MMC controller
 * Copyright (C) 2010-2012 Marvell
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/mmc/host.h>
#include <mach/gpio.h>
#include <mach/mmc_priv.h>
#include <mach/galois_platform.h>
#include "platform_setting.h"

#define SDIO_CONTROLLER_VERSION_REG_OFFSET	0x264
#define SDIO_CONTROLLER_VERSION_30		3
#define SDIO_CONTROLLER_VERSION_20		0

#define SDIO_DLL_MST_STATUS_OFFSET		0x25C
#define DELAY_CTRL4_MASK			0x1FF
#define DELAY_CTRL4_OFFSET			18

void set_sdio_controller_version(void)
{
#if defined(CONFIG_MV88DE3100_SDHC_300) && defined(CONFIG_MV88DE3100_BG2_A0)
	writel(SDIO_CONTROLLER_VERSION_30,
		MEMMAP_CHIP_CTRL_REG_BASE + SDIO_CONTROLLER_VERSION_REG_OFFSET);
#endif
}

/* set the bus power of sd card */
void set_sdio0_voltage(int voltage)
{
#if (defined(SDIO0_SET_VOLTAGE_1V8) && defined(SDIO0_SET_VOLTAGE_3V3))
	if (MMC_SIGNAL_VOLTAGE_180 == voltage)
		SDIO0_SET_VOLTAGE_1V8();
	else
		SDIO0_SET_VOLTAGE_3V3();
#endif
}

/* set the bus power of sd card */
void set_sdio1_voltage(int voltage)
{
#if (defined(SDIO1_SET_VOLTAGE_1V8) && defined(SDIO1_SET_VOLTAGE_3V3))
	if (MMC_SIGNAL_VOLTAGE_180 == voltage)
		SDIO1_SET_VOLTAGE_1V8();
	else
		SDIO1_SET_VOLTAGE_3V3();
#endif
}

/* close the power of card */
void sdio0_close_card_power(void)
{
#ifdef SDIO0_CLOSE_CARD_POWER
	SDIO0_CLOSE_CARD_POWER();
	msleep(100);
#endif
}

/* close the power of card */
void sdio1_close_card_power(void)
{
#ifdef SDIO1_CLOSE_CARD_POWER
	SDIO1_CLOSE_CARD_POWER();
	msleep(100);
#endif
}

#ifdef CONFIG_MV88DE3100_BG2_A0
unsigned int get_sdio0_tx_hold_delay(unsigned int timing)
{
	unsigned int value;
	switch (timing) {
		case MMC_TIMING_UHS_SDR104:
			value = 0x0;
			break;
		case MMC_TIMING_LEGACY:
			value = 0x160;
			break;
		default:
			value = 0x290070;
			break;
	}
	return value;
}

unsigned int get_sdio1_tx_hold_delay(unsigned int timing)
{
	unsigned int value;
	switch (timing) {
		case MMC_TIMING_UHS_SDR104:
			value = 0x1A0000;
			break;
		case MMC_TIMING_LEGACY:
			value = 0x160;
			break;
		default:
			value = 0x290070;
			break;
	}
	return value;
}

/*
 * Read the status using the sdioDllMstStatus register.
 * This is the delay value that needs to be programmed into
 * the delay element of MMC controller
 */
unsigned int get_mmc_tx_hold_delay(unsigned int timing)
{
	unsigned int value;
	value = readl(MEMMAP_CHIP_CTRL_REG_BASE + SDIO_DLL_MST_STATUS_OFFSET);
	return (value >> DELAY_CTRL4_OFFSET) & DELAY_CTRL4_MASK;
}
#endif

#ifdef CONFIG_MV88DE3100_SDHC
#ifdef CONFIG_MV88DE3100_SDHC_INTERFACE0
static struct sdhci_mv_mmc_platdata sdhc_plat_data0 = {
#ifdef SDHC_INTERFACE0_FLAG
	.flags = SDHC_INTERFACE0_FLAG,
#endif
	.set_controller_version = set_sdio_controller_version,
	.set_sdio_voltage = set_sdio0_voltage,
#ifdef CONFIG_MV88DE3100_BG2_A0
	.get_sdio_tx_hold_delay = get_sdio0_tx_hold_delay,
#endif
	.close_card_power = sdio0_close_card_power,
};
#ifdef CONFIG_MV88DE3100_SDHC_200
static struct resource mv88de3100_sdhc_resource0[] = {
	[0] = {
		.start  = MEMMAP_SDIO_REG_BASE,
		.end    = MEMMAP_SDIO_REG_BASE + MEMMAP_SDIO_REG_SIZE - 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_SDIO_INTERRUPT,
		.end    = IRQ_SDIO_INTERRUPT,
		.flags  = IORESOURCE_IRQ,
	},
 };
#elif defined(CONFIG_MV88DE3100_SDHC_300)
static struct resource mv88de3100_sdhc_resource0[] = {
	[0] = {
		.start  = MEMMAP_SDIO3_REG_BASE,
		.end    = MEMMAP_SDIO3_REG_BASE + 0x200 - 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_SDIO_INTERRUPT,
		.end    = IRQ_SDIO_INTERRUPT,
		.flags  = IORESOURCE_IRQ,
	},
 };
#else
#error
#endif

struct platform_device mv88de3100_sdhc_dev0 = {
	.name = "mv_sdhci",
	.id = 0,
	.dev = {
		.platform_data = &sdhc_plat_data0,
	},
	.resource = mv88de3100_sdhc_resource0,
	.num_resources = ARRAY_SIZE(mv88de3100_sdhc_resource0),
};
#endif

#ifdef CONFIG_MV88DE3100_SDHC_INTERFACE1
static struct sdhci_mv_mmc_platdata sdhc_plat_data1 = {
#ifdef SDHC_INTERFACE1_FLAG
	.flags = SDHC_INTERFACE1_FLAG,
#endif
	.set_controller_version = set_sdio_controller_version,
	.set_sdio_voltage = set_sdio1_voltage,
#ifdef CONFIG_MV88DE3100_BG2_A0
	.get_sdio_tx_hold_delay = get_sdio1_tx_hold_delay,
#endif
	.close_card_power = sdio1_close_card_power,
};
#ifdef CONFIG_MV88DE3100_SDHC_200
static struct resource mv88de3100_sdhc_resource1[] = {
	[0] = {
		.start  = MEMMAP_SDIO1_REG_BASE,
		.end    = MEMMAP_SDIO1_REG_BASE + MEMMAP_SDIO1_REG_SIZE - 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_SDIO1_INTERRUPT,
		.end    = IRQ_SDIO1_INTERRUPT,
		.flags  = IORESOURCE_IRQ,
	},
 };
#elif defined(CONFIG_MV88DE3100_SDHC_300)
static struct resource mv88de3100_sdhc_resource1[] = {
	[0] = {
		.start  = MEMMAP_SDIO3_REG_BASE + 0x800,
		.end    = MEMMAP_SDIO3_REG_BASE + 0x800 + 0x200 - 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_SDIO1_INTERRUPT,
		.end    = IRQ_SDIO1_INTERRUPT,
		.flags  = IORESOURCE_IRQ,
	},
 };
#else
#error
#endif

struct platform_device mv88de3100_sdhc_dev1 = {
	.name = "mv_sdhci",
	.id = 1,
	.dev = {
		.platform_data = &sdhc_plat_data1,
	},
	.resource = mv88de3100_sdhc_resource1,
	.num_resources = ARRAY_SIZE(mv88de3100_sdhc_resource1),
};
#endif

#ifdef CONFIG_MV88DE3100_MMC_INTERFACE
static struct sdhci_mv_mmc_platdata mmc_plat_data = {
	.clk_delay_cycles = 0x1f,
	.flags = MV_FLAG_CARD_PERMANENT |
		 MV_FLAG_SD_8_BIT_CAPABLE_SLOT,
	.set_controller_version = set_sdio_controller_version,
#ifdef CONFIG_MV88DE3100_BG2_A0
	.get_sdio_tx_hold_delay = get_mmc_tx_hold_delay,
#endif
};

static struct resource mv88de3100_mmc_resource0[] = {
	[0] = {
		.start  = MEMMAP_SDIO3_REG_BASE + 0x1000,
		.end    = MEMMAP_SDIO3_REG_BASE + 0x1000 + MEMMAP_SDIO3_REG_SIZE - 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_MMC_INTERRUPT,
		.end    = IRQ_MMC_INTERRUPT,
		.flags  = IORESOURCE_IRQ,
	},
 };

struct platform_device mv88de3100_mmc_dev = {
	.name = "mv_sdhci",
	.id = -1,
	.dev = {
		.platform_data = &mmc_plat_data,
	},
	.resource = mv88de3100_mmc_resource0,
	.num_resources = ARRAY_SIZE(mv88de3100_mmc_resource0),
};
#endif
#endif
