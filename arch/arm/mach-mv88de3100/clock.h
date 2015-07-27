#include <asm/clkdev.h>

#define RA_GBL_SYSPLLCTL	0x0014
#define RA_GBL_SYSPLLCTL1	0x0018
#define RA_GBL_CPUPLLCTL	0x003C
#define RA_GBL_CPUPLLCTL1	0x0040
#define RA_GBL_PLLSTATUS	0x014C
#define RA_GBL_CLKSELECT	0x0154
#define RA_GBL_CLKSELECT1	0x0158
#define RA_GBL_CLKSELECT2	0x015C
#define RA_GBL_CLKSWITCH	0x0164
#define RA_GBL_CLKSWITCH1	0x0168
#define RA_GBL_RESETTRIGGER	0x0178
#define RA_GBL_SDIOXINCLKCTRL	0x023C
#define RA_GBL_SDIO1XINCLKCTRL	0x0240


struct clkops {
	unsigned long		(*getrate)(struct clk *);
};

struct clk {
	const struct clkops	*ops;
	int	ctl;
	int	ctl1;
};

#ifndef CONFIG_MV88DE3100_ASIC
#define CLK(_name, _ctl, _ctl1)				\
static struct clkops _name##_clk_ops = {		\
	.getrate	= _name##_get_rate,		\
};							\
static struct clk _name##_clk = {			\
	.ctl	= _ctl,					\
	.ctl1	= _ctl1,				\
	.ops	= &_name##_clk_ops,			\
}
#else
#define CLK(_name, _ctl, _ctl1)				\
static unsigned long _name##_get_rate(struct clk *clk)	\
{							\
	unsigned long pll = get_pll(clk);		\
	u32 divider = _name##_get_divider();		\
	return 1000000*pll/divider;			\
}							\
static struct clkops _name##_clk_ops = {		\
	.getrate	= _name##_get_rate,		\
};							\
static struct clk _name##_clk = {			\
	.ctl	= _ctl,					\
	.ctl1	= _ctl1,				\
	.ops	= &_name##_clk_ops,			\
}
#endif

#define CLK_LOOKUP(_name)				\
	{						\
		.clk		= &_name##_clk,		\
		.con_id		= #_name,		\
	}

extern void __init mv88de3100_clk_init(void);
