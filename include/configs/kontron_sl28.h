/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SL28_H
#define __SL28_H

#include <asm/arch/stream_id_lsch3.h>
#include <asm/arch/config.h>
#include <asm/arch/soc.h>

/* We don't use hwconfig but this has to be defined.. */
#define HWCONFIG_BUFFER_SIZE 256

/* DDR */
#define CONFIG_DDR_ECC
#define CONFIG_ECC_INIT_VIA_DDRCONTROLLER
#define CONFIG_MEM_INIT_VALUE		0xdeadbeef

#define CONFIG_VERY_BIG_RAM
#define CONFIG_CHIP_SELECTS_PER_CTRL	4
#define CONFIG_DIMM_SLOTS_PER_CTLR	1
#define CONFIG_SYS_DDR_SDRAM_BASE	0x80000000UL
#define CONFIG_SYS_FSL_DDR_SDRAM_BASE_PHY	0
#define CONFIG_SYS_SDRAM_BASE		CONFIG_SYS_DDR_SDRAM_BASE
#define CONFIG_SYS_DDR_BLOCK2_BASE	0x2080000000ULL
#define CONFIG_SYS_FSL_DDR_MAIN_NUM_CTRLS	1

/* early stack pointer */
#define CONFIG_SYS_INIT_SP_ADDR		(CONFIG_SYS_FSL_OCRAM_BASE + 0xeff0)

/* memtest command */
#define CONFIG_SYS_MEMTEST_START        0x80000000
#define CONFIG_SYS_MEMTEST_END          0x9fffffff

/* SMP */
#define CPU_RELEASE_ADDR		secondary_boot_addr

/* generic timer */
#define COUNTER_FREQUENCY		25000000

/* size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + 2048 * 1024)

/* early heap for SPL DM */
#define CONFIG_MALLOC_F_ADDR		CONFIG_SYS_FSL_OCRAM_BASE

/* serial port */
#define CONFIG_SYS_NS16550_SERIAL
#define CONFIG_SYS_NS16550_REG_SIZE 1
#define CONFIG_SYS_NS16550_CLK          (get_bus_freq(0) / 2)
#define CONFIG_SYS_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }

#define CONFIG_SYS_CLK_FREQ		100000000
#define CONFIG_DDR_CLK_FREQ		100000000
#define COUNTER_FREQUENCY_REAL		(CONFIG_SYS_CLK_FREQ / 4)

/* MMC */
#ifdef CONFIG_MMC
#define CONFIG_SYS_FSL_MMC_HAS_CAPBLT_VS33
#endif

/* ethernet */
#define CONFIG_SYS_RX_ETH_BUFFER	8

#ifdef CONFIG_SPL
#define CONFIG_SPL_TARGET		"u-boot-with-spl.bin"
#define CONFIG_SPL_BSS_START_ADDR	0x80100000
#define CONFIG_SPL_BSS_MAX_SIZE		0x00100000
#define CONFIG_SPL_MAX_SIZE		0x20000
#define CONFIG_SPL_STACK		(CONFIG_SYS_FSL_OCRAM_BASE + 0x9ff0)

#define CONFIG_SYS_SPL_MALLOC_SIZE	0x00100000
#define CONFIG_SYS_SPL_MALLOC_START	0x80200000
#define CONFIG_SYS_MONITOR_LEN		(1024 * 1024)
#define CONFIG_SYS_SPI_U_BOOT_SIZE	(0x100000 - (CONFIG_SYS_SPI_U_BOOT_OFFS)
#endif

#ifdef CONFIG_SPL_BUILD
#define CONFIG_SYS_MONITOR_BASE		CONFIG_SPL_TEXT_BASE
#else
#define CONFIG_SYS_MONITOR_BASE		CONFIG_SYS_TEXT_BASE
#endif

#define CONFIG_SYS_LOAD_ADDR		(CONFIG_SYS_DDR_SDRAM_BASE + 0x10000000)

/* environment */
/* allow to overwrite serial and ethaddr */
#define CONFIG_ENV_OVERWRITE

#define CONFIG_LOADADDR 0x81000000
#define ENV_MEM_LAYOUT_SETTINGS \
	"scriptaddr=0x90000000\0" \
	"pxefile_addr_r=0x90100000\0" \
	"kernel_addr_r=" __stringify(CONFIG_LOADADDR) "\0" \
	"fdt_addr_r=0x83000000\0" \
	"ramdisk_addr_r=0x83100000\0" \
	"fdtfile=freescale/fsl-ls1028a-kontron-sl28.dtb\0"

#ifndef CONFIG_SPL_BUILD
#define BOOT_TARGET_DEVICES(func) \
	func(MMC, mmc, 1) \
	func(MMC, mmc, 0) \
	func(NVME, nvme, 0) \
	func(USB, usb, 0) \
	func(DHCP, dhcp, 0) \
	func(PXE, pxe, 0)
#include <config_distro_bootcmd.h>
#else
#define BOOTENV
#endif

#define CONFIG_EXTRA_ENV_SETTINGS \
	"fdt_high=0xffffffffffffffff\0" \
	"initrd_high=0xffffffffffffffff\0" \
	"env_addr=0x203e0004\0" \
	"envload=env import -d -b ${env_addr}\0" \
	"install_rcw=source 20200000\0" \
	ENV_MEM_LAYOUT_SETTINGS \
	BOOTENV

#endif /* __SL28_H */
