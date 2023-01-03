/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2023 Michael Walle <michael@walle.cc>
 */

#ifndef CONFIGS_SIPEED_M1SDOCK_H
#define CONFIGS_SIPEED_M1SDOCK_H

#include <linux/sizes.h>

#define CONFIG_SYS_SDRAM_BASE 0x50000000
#define CONFIG_SYS_SDRAM_SIZE SZ_64M

#ifndef CONFIG_EXTRA_ENV_SETTINGS
#define CONFIG_EXTRA_ENV_SETTINGS \
	"loadaddr=0x50060000\0" \
	"fdt_addr_r=0x50400000\0" \
	"scriptaddr=0x50020000\0" \
	"kernel_addr_r=0x50060000\0" \
	"fdtfile=sipeed/" CONFIG_DEFAULT_DEVICE_TREE ".dtb\0"
#endif

#endif /* CONFIGS_SIPEED_M1SDOCK_H */
