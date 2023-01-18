// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2023 Michael Walle <michael@walle.cc>
 *
 */

#include <common.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <common.h>
#include <dm.h>
#include <dm/pinctrl.h>

#define BL808_GPIO_COUNT 46

struct bl808_gpio_priv {
	void *glb;
	struct udevice *pinctrl;
};

#define GLB_GPIO_CFG(n)		(0x8c4 + 4 * (n))
#define GPIO_CFG_IE		BIT(0)
#define GPIO_CFG_OE		BIT(6)
#define GLB_GPIO_IN(gpio)	((gpio < 32) ? 0xac4 : 0xac8)
#define GLB_GPIO_SET(gpio)	((gpio < 32) ? 0xaec : 0xaf0)
#define GLB_GPIO_CLR(gpio)	((gpio < 32) ? 0xaf4 : 0xaf8)

static int bl808_gpio_direction_input(struct udevice *dev, unsigned gpio)
{
	struct bl808_gpio_priv *priv = dev_get_priv(dev);
	u32 val;

        val = readl(priv->glb + GLB_GPIO_CFG(gpio));
	val &= ~GPIO_CFG_OE;
	val |= GPIO_CFG_IE;
	writel(val, priv->glb + GLB_GPIO_CFG(gpio));

	return 0;
}

static int bl808_gpio_direction_output(struct udevice *dev, unsigned int gpio,
				       int value)
{
	struct bl808_gpio_priv *priv = dev_get_priv(dev);
	u32 val;

        val = readl(priv->glb + GLB_GPIO_CFG(gpio));
	val &= ~GPIO_CFG_IE;
	val |= GPIO_CFG_OE;
	writel(val, priv->glb + GLB_GPIO_CFG(gpio));

	return 0;
}

static int bl808_gpio_get_value(struct udevice *dev, unsigned gpio)
{
	struct bl808_gpio_priv *priv = dev_get_priv(dev);
	u32 val;

	val = readl(priv->glb + GLB_GPIO_IN(gpio));

	return !!(val & BIT(gpio & 0x1f));
}

static int bl808_gpio_set_value(struct udevice *dev, unsigned gpio, int value)
{
	struct bl808_gpio_priv *priv = dev_get_priv(dev);

	if (value)
		writel(BIT(gpio & 0x1f), priv->glb + GLB_GPIO_SET(gpio));
	else
		writel(BIT(gpio & 0x1f), priv->glb + GLB_GPIO_CLR(gpio));

	return 0;
}

static int bl808_gpio_get_function(struct udevice *dev, unsigned offset)
{
	struct bl808_gpio_priv *priv = dev_get_priv(dev);

	return pinctrl_get_gpio_mux(priv->pinctrl, 0, offset);
}

static const struct dm_gpio_ops bl808_gpio_ops = {
	.direction_input	= bl808_gpio_direction_input,
	.direction_output	= bl808_gpio_direction_output,
	.get_value		= bl808_gpio_get_value,
	.set_value		= bl808_gpio_set_value,
	.get_function		= bl808_gpio_get_function,
};

static int bl808_gpio_probe(struct udevice *dev)
{
	struct bl808_gpio_priv *priv = dev_get_plat(dev);
	struct gpio_dev_priv *uc_priv = dev_get_uclass_priv(dev);

	priv->glb = dev_read_addr_ptr(dev_get_parent(dev));
	if (!priv->glb)
		return -EINVAL;

	priv->pinctrl = dev_get_parent(dev);

	uc_priv->bank_name = dev->name;
	uc_priv->gpio_count = BL808_GPIO_COUNT;

	return 0;
}

U_BOOT_DRIVER(bl808_gpio) = {
	.name	= "bl808_gpio",
	.id	= UCLASS_GPIO,
	.probe	= bl808_gpio_probe,
	.priv_auto = sizeof(struct bl808_gpio_priv),
	.ops	= &bl808_gpio_ops,
};
