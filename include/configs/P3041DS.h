/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2010-2011 Freescale Semiconductor, Inc.
 */

/*
 * P3041 DS board configuration file
 *
 */
#define CONFIG_SYS_DPAA_RMAN

#define CONFIG_SYS_SRIO
#define CONFIG_SRIO1			/* SRIO port 1 */
#define CONFIG_SRIO2			/* SRIO port 2 */
#define CONFIG_SRIO_PCIE_BOOT_MASTER
#define CONFIG_ICS307_REFCLK_HZ		25000000  /* ICS307 ref clk freq */

#include "corenet_ds.h"
