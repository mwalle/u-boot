// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2023 Michael Walle <michael@walle.cc>
 */

#define LOG_CATEGORY UCLASS_CLK

#include <common.h>
#include <clk.h>
#include <clk-uclass.h>
#include <div64.h>
#include <dm.h>
#include <log.h>
#include <dt-bindings/clock/bl808-clock.h>
#include <linux/bitfield.h>

DECLARE_GLOBAL_DATA_PTR;

struct bl808_clk_priv {
	void *glb;
	struct clk xtal;
};

static ulong bl808_clk_get_rate(struct clk *clk)
{
	printf("%s %ld\n", __func__, clk->id);

	switch (clk->id) {
	case BL808_CLK_UART:
		return 80000000;
	default:
		return 0;
	}
}

static int bl808_clk_enable(struct clk *clk)
{
	printf("%s %ld\n", __func__, clk->id);
	return 0;
}

static int bl808_clk_disable(struct clk *clk)
{
	printf("%s %ld\n", __func__, clk->id);
	return 0;
}

static const struct clk_ops bl808_clk_ops = {
	.get_rate = bl808_clk_get_rate,
	.enable = bl808_clk_enable,
	.disable = bl808_clk_disable,
#if 0
	.request = bl808_clk_request,
	.set_rate = bl808_clk_set_rate,
	.set_parent = bl808_clk_set_parent,
#endif
};

static int bl808_clk_probe(struct udevice *dev)
{
	int ret;
	struct bl808_clk_priv *priv = dev_get_priv(dev);
printf("%s\n", __func__);

	priv->glb = dev_read_addr_ptr(dev_get_parent(dev));
	if (!priv->glb)
		return -EINVAL;

	ret = clk_get_by_index(dev, 0, &priv->xtal);
	if (ret)
		return ret;

#if 0
	/*
	 * Force setting defaults, even before relocation. This is so we can
	 * set the clock rate for PLL1 before we relocate into aisram.
	 */
	if (!(gd->flags & GD_FLG_RELOC))
		clk_set_defaults(dev, CLK_DEFAULTS_POST_FORCE);
#endif

	return 0;
}


static const struct udevice_id bl808_clk_ids[] = {
	{ .compatible = "bouffalolab,bl808-clk" },
	{ },
};

U_BOOT_DRIVER(bl808_clk) = {
	.name = "bl808_clk",
	.id = UCLASS_CLK,
	.of_match = bl808_clk_ids,
	.ops = &bl808_clk_ops,
	.probe = bl808_clk_probe,
	.priv_auto = sizeof(struct bl808_clk_priv),
};

#if 0
#if CONFIG_IS_ENABLED(CMD_CLK)
static char show_enabled(struct bl808_clk_priv *priv, int id)
{
	bool enabled;

	if (bl808_clks[id].flags & bl808_CLKF_PLL) {
		const struct bl808_pll_params *pll =
			&bl808_plls[bl808_clks[id].pll];

		enabled = bl808_pll_enabled(readl(priv->base + pll->off));
	} else if (bl808_clks[id].gate == bl808_CLK_GATE_NONE) {
		return '-';
	} else {
		const struct bl808_gate_params *gate =
			&bl808_gates[bl808_clks[id].gate];

		enabled = bl808_clk_readl(priv, gate->off, gate->bit_idx, 1);
	}

	return enabled ? 'y' : 'n';
}

static void show_clks(struct bl808_clk_priv *priv, int id, int depth)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(bl808_clks); i++) {
		if (bl808_clk_get_parent(priv, i) != id)
			continue;

		printf(" %-9lu %-7c %*s%s\n", do_bl808_clk_get_rate(priv, i),
		       show_enabled(priv, i), depth * 4, "",
		       bl808_clks[i].name);

		show_clks(priv, i, depth + 1);
	}
}

int soc_clk_dump(void)
{
	int ret;
	struct udevice *dev;
	struct bl808_clk_priv *priv;

	ret = uclass_get_device_by_driver(UCLASS_CLK, DM_DRIVER_GET(bl808_clk),
					  &dev);
	if (ret)
		return ret;
	priv = dev_get_priv(dev);

	puts(" Rate      Enabled Name\n");
	puts("------------------------\n");
	printf(" %-9lu %-7c %*s%s\n", clk_get_rate(&priv->xtal), 'y', 0, "",
	       priv->xtal.dev->name);
	show_clks(priv, BL808_CLK_IN0, 1);
	return 0;
}
#endif
#endif
