#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/serial_8250.h>
#include <linux/serial_reg.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/ahci_platform.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <asm/irq.h>
#include <asm/pmu.h>
#include <mach/galois_platform.h>
#include <mach/irqs.h>
#include <mach/mmc_priv.h>
#include <plat/pxa3xx_nand_debu.h>

static u64 mv88de3100_dma_mask = 0xffffffffULL;

/*
 * PMU
 */
static struct resource pmu_resource[] = {
	[0] = {
		.start	= IRQ_PMU_CPU0,
		.end	= IRQ_PMU_CPU0,
		.flags	= IORESOURCE_IRQ,
	},
	[1] = {
		.start	= IRQ_PMU_CPU1,
		.end	= IRQ_PMU_CPU1,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device pmu_dev = {
	.name	= "arm-pmu",
	.id	= ARM_PMU_DEVICE_CPU,
	.num_resources = ARRAY_SIZE(pmu_resource),
	.resource = pmu_resource,
};

/*
 * UART device
 */
#define APB_UART_USR	0x7c
static struct plat_serial8250_port platform_serial_ports[] = {
	{
		.membase = (void *)(APB_UART_INST0_BASE),
		.mapbase = (unsigned long)(APB_UART_INST0_BASE),
		.irq = IRQ_SM_UART0,
		.uartclk = GALOIS_UART_TCLK,
		.regshift = 2,
		.iotype = UPIO_DWAPB,
		.flags = UPF_BOOT_AUTOCONF | UPF_SKIP_TEST,
		/* read USR to clear busy int */
		.private_data = (void *)(APB_UART_INST0_BASE + APB_UART_USR),
	},
#ifdef CONFIG_MV88DE3100_UART1
	{
		.membase = (void *)(APB_UART_INST1_BASE),
		.mapbase = (unsigned long)(APB_UART_INST1_BASE),
		.irq = IRQ_SM_UART1,
		.uartclk = GALOIS_UART_TCLK,
		.regshift = 2,
		.iotype = UPIO_DWAPB,
		.flags = UPF_BOOT_AUTOCONF | UPF_SKIP_TEST,
		/* read USR to clear busy int */
		.private_data = (void *)(APB_UART_INST1_BASE + APB_UART_USR),
	},
#endif
	{}
};

static struct platform_device mv88de3100_serial_dev = {
	.name = "serial8250",
	.id = PLAT8250_DEV_PLATFORM,
	.dev = {
		.platform_data = &platform_serial_ports,
	},
};

#ifdef CONFIG_MV88DE3100_ETHERNET
/*
 * Ethernet device
 */
static struct resource mv88de3100_eth_resource[] = {
	[0] = {
		.start	= MEMMAP_ETHERNET1_REG_BASE,
		.end	= MEMMAP_ETHERNET1_REG_BASE + MEMMAP_ETHERNET1_REG_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_ETH1IRQ,
		.end	= IRQ_ETH1IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device mv88de3100_eth_dev = {
	.name = "mv88de3100_eth1",
	.id	= -1,
	.num_resources = ARRAY_SIZE(mv88de3100_eth_resource),
	.resource = mv88de3100_eth_resource,
	.dev = {
		.dma_mask = &mv88de3100_dma_mask,
		.coherent_dma_mask = 0xffffffff,
	},
};
#endif

#if defined(CONFIG_MV88DE3100_PXA3XX_NFC) || defined(CONFIG_MV88DE3100_PXA3XX_NFC_MODULE)
static struct resource mv_nand_resources[] = {
	[0] = {
		.start	= MEMMAP_NAND_FLASH_REG_BASE,
		.end	= MEMMAP_NAND_FLASH_REG_BASE + MEMMAP_NAND_FLASH_REG_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_INTRPB,
		.end	= IRQ_INTRPB,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct pxa3xx_nand_platform_data mv88de3100_nand_platform_data = {
	.controller_attrs = PXA3XX_ARBI_EN | PXA3XX_NAKED_CMD_EN
		| PXA3XX_DMA_EN,
	.cs_num		= 1,
	.parts[0]	= NULL,
	.nr_parts[0]	= 0
};

static struct platform_device pxa3xx_nand_dev = {
	.name		= "mv_nand",
	.id		= -1,
	.dev	= {
		.platform_data = &mv88de3100_nand_platform_data,
		.dma_mask = &mv88de3100_dma_mask,
		.coherent_dma_mask = 0xffffffff,
	},
	.num_resources	= ARRAY_SIZE(mv_nand_resources),
	.resource	= mv_nand_resources,
};
#endif

#ifdef CONFIG_MV88DE3100_USB_EHCI_HCD
/*
 * USB Host Controller device
 */
static struct resource mv88de3100_usb0_resource[] = {
	[0] = {
		.start	= MEMMAP_USB0_REG_BASE,
		.end	= MEMMAP_USB0_REG_BASE + MEMMAP_USB0_REG_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_USB0INTERRUPT,
		.end	= IRQ_USB0INTERRUPT,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device mv88de3100_usb0_dev = {
	.name = "mv88de3100_ehci",
	.id	= 0,
	.num_resources = ARRAY_SIZE(mv88de3100_usb0_resource),
	.resource = mv88de3100_usb0_resource,
	.dev = {
		.dma_mask = &mv88de3100_dma_mask,
		.coherent_dma_mask = 0xffffffff,
	},
};

static struct resource mv88de3100_usb1_resource[] = {
	[0] = {
		.start	= MEMMAP_USB1_REG_BASE,
		.end	= MEMMAP_USB1_REG_BASE + MEMMAP_USB1_REG_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_USB1INTERRUPT,
		.end	= IRQ_USB1INTERRUPT,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device mv88de3100_usb1_dev = {
	.name = "mv88de3100_ehci",
	.id	= 1,
	.num_resources = ARRAY_SIZE(mv88de3100_usb1_resource),
	.resource = mv88de3100_usb1_resource,
	.dev = {
		.dma_mask = &mv88de3100_dma_mask,
		.coherent_dma_mask = 0xffffffff,
	},
};
#endif /* CONFIG_MV88DE3100_USB_EHCI_HCD */

/*
 * SATA device
 */
#define PORT_VSR_ADDR		0x78	/* port Vendor Specific Register Address */
#define PORT_VSR_DATA		0x7c	/* port Vendor Specific Register Data */
#define PORT_SCR_CTL		0x2c	/* SATA phy register: SControl */
#define MAX_PORTS		2
#define MAX_MAP			((1 << MAX_PORTS) - 1)

static inline void disable_sata_port(int port)
{
	void *base = __io(MEMMAP_SATA_REG_BASE + 0x100 + (port * 0x80));
	__raw_writel(0x80 + 0x26, base + PORT_VSR_ADDR);
	__raw_writel(0x8122, base + PORT_VSR_DATA);
	__raw_writel(0x80 + 0x1, base + PORT_VSR_ADDR);
	__raw_writel(0x8901, base + PORT_VSR_DATA);
}

#ifdef CONFIG_SATA_AHCI_PLATFORM
static struct resource mv88de3100_sata_resource[] = {
	[0] = {
		.start	= MEMMAP_SATA_REG_BASE,
		.end	= MEMMAP_SATA_REG_BASE + MEMMAP_SATA_REG_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_SATAINTR,
		.end	= IRQ_SATAINTR,
		.flags	= IORESOURCE_IRQ,
	},
};

#ifdef CONFIG_MV88DE3100_BG2_CT
#define HOST_VSA_ADDR	0xA0
#define HOST_VSA_DATA	0xA4
#define RA_GBL_PERIF	0x0480
#define RA_PERIF_COMPHY_CTRL0	0x0000
#define GBL_PERIF_REG	(RA_GBL_PERIF+MEMMAP_CHIP_CTRL_REG_BASE)

static void port_init(struct device *dev, void __iomem *base, void __iomem *addr)
{
	u32 tmp;

	__raw_writel(0x4, base + HOST_VSA_ADDR);
	__raw_writel(0x240034, base + HOST_VSA_DATA);

	tmp = __raw_readl(GBL_PERIF_REG + RA_PERIF_COMPHY_CTRL0);
	tmp |= 0x1;
	__raw_writel(tmp, GBL_PERIF_REG + RA_PERIF_COMPHY_CTRL0);

	__raw_writel(0x0, base + HOST_VSA_ADDR);
	tmp = __raw_readl(base + HOST_VSA_DATA);
	tmp |= 0x40;
	__raw_writel(tmp, base + HOST_VSA_DATA);

	__raw_writel(0x200 + 0x1, addr + PORT_VSR_ADDR);
	tmp = __raw_readl(addr + PORT_VSR_DATA);
	tmp &= 0xFF00;
	tmp |= 0x01;
	__raw_writel(tmp, addr + PORT_VSR_DATA);

	__raw_writel(0x200 + 0x25, addr + PORT_VSR_ADDR);
	tmp = __raw_readl(addr + PORT_VSR_DATA);
	tmp &= 0xF3FF;
	__raw_writel(tmp, addr + PORT_VSR_DATA);

	__raw_writel(0x200 + 0x23, addr + PORT_VSR_ADDR);
	tmp = __raw_readl(addr + PORT_VSR_DATA);
	tmp &= 0xF3FF;
	tmp |= 0x800;
	__raw_writel(tmp, addr + PORT_VSR_DATA);


	__raw_writel(0x0, base + HOST_VSA_ADDR);
	tmp = __raw_readl(base + HOST_VSA_DATA);
	tmp &= 0xFFFFFFBF;
	__raw_writel(tmp, base + HOST_VSA_DATA);

	__raw_writel(0x0 + 0x42, addr + PORT_VSR_ADDR);
	__raw_writel(0xBC06, addr + PORT_VSR_DATA);

	writel(0x00000030, addr + PORT_SCR_CTL);
	mdelay(10);
	writel(0x00000031, addr + PORT_SCR_CTL);
	mdelay(10);
	writel(0x00000030, addr + PORT_SCR_CTL);
	mdelay(10);
}
#else
static void port_init(struct device *dev, void __iomem *base, void __iomem *addr)
{
	u32 tmp;

	/* improve TX hold time for Gen2 */
	writel(0x80 + 0x5C, addr + PORT_VSR_ADDR);
	writel(0x0D97, addr + PORT_VSR_DATA);
	writel(0x80 + 0x4E, addr + PORT_VSR_ADDR);
	writel(0x5155, addr + PORT_VSR_DATA);
	/* Change clock to 25Mhz (It's 40Mhz by default) */
	writel(0x80 + 0x1, addr + PORT_VSR_ADDR);
	tmp = readl(addr + PORT_VSR_DATA);
	tmp &= 0xFFE0;
	tmp |= 0x01;
	writel(tmp, addr + PORT_VSR_DATA);

	/* After a clock change, we should calibrate */
	writel(0x80 + 0x2, addr + PORT_VSR_ADDR);
	tmp = readl(addr + PORT_VSR_DATA);
	tmp |= 0x8000;
	writel(tmp, addr + PORT_VSR_DATA);

	/* Wait for calibration to Complete */
	while (1) {
		writel(0x80 + 0x2, addr + PORT_VSR_ADDR);
		tmp = readl(addr + PORT_VSR_DATA);
		if (tmp & 0x4000)
			break;
	}

	/* Setup SATA PHY */

	/* Set SATA PHY to be 40-bit data bus mode */
	writel(0x80 + 0x23, addr + PORT_VSR_ADDR);
	tmp = readl(addr + PORT_VSR_DATA);
	tmp &= ~0xC00;
	tmp |= 0x800;
	writel(tmp, addr + PORT_VSR_DATA);

	/* Set SATA PHY PLL to use maxiumum PLL rate to TX and RX */
	writel(0x80 + 0x2, addr + PORT_VSR_ADDR);
	tmp = readl(addr + PORT_VSR_DATA);
	tmp |= 0x100;
	writel(tmp, addr + PORT_VSR_DATA);

	/* Set SATA PHY to support GEN1/GEN2/GEN3 */
	writel(0x00000030, addr + PORT_SCR_CTL);
	mdelay(10);
	writel(0x00000031, addr + PORT_SCR_CTL);
	mdelay(10);
	writel(0x00000030, addr + PORT_SCR_CTL);
	mdelay(10);

	/* improve SATA signal integrity */
	writel(0x80 + 0x0D, addr + PORT_VSR_ADDR);
	writel(0xC926, addr + PORT_VSR_DATA);
	writel(0x80 + 0x0F, addr + PORT_VSR_ADDR);
	writel(0x9A4E, addr + PORT_VSR_DATA);
}
#endif

static int mv_ahci_init(struct device *dev, void __iomem *base)
{
	int i;
	unsigned int port_map;
	struct ahci_platform_data *pdata = dev->platform_data;

	port_map = pdata->force_port_map ? pdata->force_port_map : MAX_MAP;
	for (i = 0; i < MAX_PORTS; i++)
		if (port_map & (1 << i))
			port_init(dev, base, base + 0x100 + (i * 0x80));

	return 0;
}

static struct ahci_platform_data mv_ahci_pdata = {
	.init		= mv_ahci_init,
	.force_port_map	= 1,
};

static struct platform_device mv_ahci_dev = {
	.name		= "ahci",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(mv88de3100_sata_resource),
	.resource	= mv88de3100_sata_resource,
	.dev		= {
		.platform_data		= &mv_ahci_pdata,
		.dma_mask		= &mv88de3100_dma_mask,
		.coherent_dma_mask	= 0xffffffff,
	},
};
#endif

#ifdef CONFIG_MV88DE3100_LINUX_I2C
static struct platform_device mv_i2c0_dev = {
	.name		= "mv88de3100-i2c",
	.id		= 0,
};

static struct platform_device mv_i2c1_dev = {
	.name		= "mv88de3100-i2c",
	.id		= 1,
};

static struct platform_device mv_i2c2_dev = {
	.name		= "mv88de3100-i2c",
	.id		= 2,
};

static struct platform_device mv_i2c3_dev = {
	.name		= "mv88de3100-i2c",
	.id		= 3,
};
#endif

static struct platform_device *mv88de3100_platform_devs[] __initdata = {
	&pmu_dev,
	&mv88de3100_serial_dev,
#ifdef CONFIG_SATA_AHCI_PLATFORM
	&mv_ahci_dev,
#endif
#ifdef CONFIG_MV88DE3100_ETHERNET
	&mv88de3100_eth_dev,
#endif
#ifdef CONFIG_MV88DE3100_SDHC
#ifdef CONFIG_MV88DE3100_MMC_INTERFACE
	&mv88de3100_mmc_dev,
#endif
#ifdef CONFIG_MV88DE3100_SDHC_INTERFACE0
	&mv88de3100_sdhc_dev0,
#endif
#ifdef CONFIG_MV88DE3100_SDHC_INTERFACE1
	&mv88de3100_sdhc_dev1,
#endif
#endif /* end of CONFIG_MV88DE3100_SDHC */
#if defined(CONFIG_MV88DE3100_PXA3XX_NFC) || defined(CONFIG_MV88DE3100_PXA3XX_NFC_MODULE)
	&pxa3xx_nand_dev,
#endif
#ifdef CONFIG_MV88DE3100_USB_EHCI_HCD
	&mv88de3100_usb0_dev,
#ifdef CONFIG_MV88DE3100_EHCI_USB1
	&mv88de3100_usb1_dev,
#endif
#endif
#ifdef CONFIG_MV88DE3100_LINUX_I2C
	&mv_i2c0_dev,
	&mv_i2c1_dev,
	&mv_i2c2_dev,
	&mv_i2c3_dev,
#endif
};

void __init mv88de3100_devs_init(void)
{
	platform_add_devices(mv88de3100_platform_devs, ARRAY_SIZE(mv88de3100_platform_devs));
}

#ifdef CONFIG_MV88DE3100_BG2_A0
int mv88de3100_close_phys_cores(void)
{
	u32 val;

	/* we never use sata 2nd port*/
	disable_sata_port(1);

	/* we never use eth0 */
	val = __raw_readl(0xF7EA0150);
	val &= ~(1<<7);
#ifndef CONFIG_SATA_AHCI_PLATFORM
	disable_sata_port(0);
	/* if we don't use 1st sata port we can turn off sata clock */
	val &= ~(1<<9);
#endif
#ifndef CONFIG_MV88DE3100_SDHC
	/* turn off clk */
	val &= ~(1<<14);
#endif
#ifndef CONFIG_MV88DE3100_EHCI_USB1
	/* turn off PHY layer */
	__raw_writel(0x1680, 0xF7B78034);
	/* turn off clk */
	val &= ~(1<<12);
#endif
	__raw_writel(val, 0xF7EA0150);
	return 0;
}

late_initcall(mv88de3100_close_phys_cores);
#endif
