// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <fdt_support.h>
#include <asm/io.h>

#define GLB_BASE 0x20000000

void board_debug_uart_init(void)
{
	void __iomem *regs;

	regs = map_physmem(GLB_BASE, SZ_4K, MAP_NOCACHE);

	writel(0xff32ffff, regs + 0x154);
	writel(0x0000ffff, regs + 0x158);
	writel(0x0f000003, regs + 0x120);
	writel(0x40400717, regs + 0x904);
	writel(0x40400717, regs + 0x908);
}

phys_size_t get_effective_memsize(void)
{
	return CONFIG_SYS_SDRAM_SIZE;
}

int board_early_init_f(void)
{
	struct udevice *dev;
	int ret;

	ret = uclass_get_device(UCLASS_RAM, 0, &dev);
	if (ret)
		panic("DRAM init failed: %d\n", ret);

	return 0;
}

int board_init(void)
{
	return 0;
}
