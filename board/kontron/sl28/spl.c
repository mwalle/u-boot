// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <asm/spl.h>

void board_boot_order(u32 *spl_boot_list)
{
	spl_boot_list[0] = BOOT_DEVICE_SPI;
}
