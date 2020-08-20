#include <asm/secure.h>
#include <asm/psci.h>
#include <asm/types.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/gic.h>

#include <asm/arch-fsl-layerscape/soc.h>

#define GPIO2_GPDIR	0x2310000
#define GPIO2_GPDAT	0x2310008
#define RSTCR		0x1e60000
#define RESET_REQ	BIT(1)

void __secure psci_system_reset(void)
{
	writel(RESET_REQ, RSTCR);

	while (1)
		wfi();
}

void __secure psci_system_off(void)
{
	int i;

	/*
	 * This is the CPLD_INTERRUPT# line, which is actually driven by the
	 * CPLD, but is used bidirectionally in case of the reset. If this line
	 * is low during reset, the board will actually power off. The hardware
	 * ensures there will be no short if we pull it low.
	 */
	writel(0x02000000, GPIO2_GPDIR);
	writel(0, GPIO2_GPDAT);

	/*
	 * Poor mans sleep, because we don't want to have external
	 * dependencies.
	 *
	 * We just need to sleep for (at least) one cycle of the CPLD clock
	 * which is ~4MHz.
	 */
	for (i=0; i < (1<<11); i++)
		asm("nop");

	writel(RESET_REQ, RSTCR);

	while (1)
		wfi();
}

void __secure psci_arch_release_cpu_64(u64 mpidr)
{
	struct ccsr_gur __iomem *gur = (void *)(CONFIG_SYS_FSL_GUTS_ADDR);
	struct ccsr_reset __iomem *rst = (void *)(CONFIG_SYS_FSL_RST_ADDR);
	uintptr_t entry = (uintptr_t)psci_cpu_entry_64;
	int cpu = __psci_get_cpu_id(mpidr);

	gur_out32(&gur->bootlocptrh, entry >> 32);
	gur_out32(&gur->bootlocptrl, entry);

	/*
	 * mpidr_el1 register value of core which needs to be released
	 * is written to scratchrw[6] register
	 */
	gur_out32(&gur->scratchrw[6], mpidr);
	dsb();
	rst->brrl |= 1 << cpu;
	dsb();
	/*
	 * scratchrw[6] register value is polled
	 * when the value becomes zero, this means that this core is up
	 * and running, next core can be released now
	 */
	while (gur_in32(&gur->scratchrw[6]))
		;
}
