#include <linux/io.h>
#include <mach/galois_platform.h>

#define APB_UART_LCR	0x0C
#define APB_UART_LSR	0x14
#define APB_UART_THR	0x00

static inline void putc(int c)
{
	unsigned int value;
	void __iomem *uart_base = (void __iomem *)APB_UART_INST0_BASE;

	/*
	 * LCR[7]=1, known as DLAB bit, is used to enable rw DLL/DLH registers.
	 * it must be cleared(LCR[7]=0) to allow accessing other registers.
	 */
	value = __raw_readl(uart_base + APB_UART_LCR);
	value &= ~0x00000080;
	__raw_writel(value, uart_base + APB_UART_LCR);

	while (!(__raw_readl(uart_base + APB_UART_LSR) & 0x00000020))
		barrier();

	__raw_writel(c, uart_base + APB_UART_THR);
}

static inline void flush(void)
{
}

/*
 * nothing to do
 */
#define arch_decomp_setup()
#define arch_decomp_wdog()
