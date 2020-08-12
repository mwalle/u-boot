#include <asm/secure.h>
#include <asm/psci.h>
#include <asm/types.h>
#include <asm/io.h>
#include <asm/system.h>

#define GPIO2_GPDIR	0x2310000
#define GPIO2_GPDAT	0x2310008
#define RSTCR		0x1e60000
#define RESET_REQ	BIT(1)

u32 __secure psci_version(void)
{
	return ARM_PSCI_VER_0_2;
}

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
