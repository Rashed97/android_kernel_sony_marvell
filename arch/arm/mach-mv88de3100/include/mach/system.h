#ifndef __ASM_MACH_SYSTEM_H
#define __ASM_MACH_SYSTEM_H

#ifdef CONFIG_MV88DE3100_SM
extern void galois_arch_reset(char mode, const char *cmd);
#else
#define galois_arch_reset(a, b)
#endif
static inline void arch_idle(void)
{
	cpu_do_idle();
}

static inline void arch_reset(char mode, const char *cmd)
{
	galois_arch_reset(mode, cmd);
}
#endif /* __ASM_MACH_SYSTEM_H */
