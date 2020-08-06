// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <asm/io.h>
#include <asm/spl.h>
#include <linux/sizes.h>
#include <spl.h>
#include <atf_common.h>

#define DCFG_RCWSR25 0x160
#define GPINFO_HW_VARIANT_MASK 0xff

int sl28_variant(void)
{
	return in_le32(DCFG_BASE + DCFG_RCWSR25) & GPINFO_HW_VARIANT_MASK;
}

int board_fit_config_name_match(const char *name)
{
	int variant = sl28_variant();

	switch (variant) {
	case 3:
		return strcmp(name, "fsl-ls1028a-kontron-sl28-var3");
	case 4:
		return strcmp(name, "fsl-ls1028a-kontron-sl28-var4");
	default:
		return strcmp(name, "fsl-ls1028a-kontron-sl28");
	}
}

void board_boot_order(u32 *spl_boot_list)
{
	spl_boot_list[0] = BOOT_DEVICE_SPI;
}

#define NUM_DRAM_REGIONS 3
struct region_info {
	uint64_t addr;
	uint64_t size;
};

struct dram_regions_info {
	uint64_t num_dram_regions;
	uint64_t total_dram_size;
	struct region_info region[NUM_DRAM_REGIONS];
};

struct bl_params *bl2_plat_get_bl31_params_v2(uintptr_t bl32_entry,
					      uintptr_t bl33_entry,
					      uintptr_t fdt_addr)
{
	static struct dram_regions_info dram_regions_info;
	struct bl_params *bl_params;
	struct bl_params_node *node;
	void *dcfg_ccsr = (void*)DCFG_BASE;

	dram_regions_info.total_dram_size = SZ_4G - 66 * SZ_1M;
	dram_regions_info.num_dram_regions = NUM_DRAM_REGIONS;
	dram_regions_info.region[0].addr = 0x80000000;
	dram_regions_info.region[0].size = SZ_2G - 66 * SZ_1M;

	dram_regions_info.region[1].addr = 0x2080000000;
	dram_regions_info.region[1].size = SZ_2G;
	//dram_regions_info.region[1].size = 126 * SZ_1G;

	dram_regions_info.region[2].addr = 0x6000000000;
	dram_regions_info.region[2].size = 0;
	//dram_regions_info.region[2].size = 128 * SZ_1G;

	bl_params = bl2_plat_get_bl31_params_v2_default(bl32_entry, bl33_entry,
							fdt_addr);

	for_each_bl_params_node(bl_params, node) {
		if (node->image_id == ATF_BL31_IMAGE_ID) {
			node->ep_info->args.arg3 = (uintptr_t)&dram_regions_info;
			node->ep_info->args.arg4 = in_le32(dcfg_ccsr + DCFG_PORSR1);
		}
	}

	return bl_params;
}
