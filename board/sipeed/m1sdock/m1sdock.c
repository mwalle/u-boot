// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <fdt_support.h>
#include <asm/io.h>

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
