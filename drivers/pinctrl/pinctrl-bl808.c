// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2023 Michael Walle <michael@walle.cc>
 */

#include <asm/io.h>
#include <common.h>
#include <dm.h>
#include <dm/pinctrl.h>
#include <dt-bindings/pinctrl/bl808-pinctrl.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>

#define BL808_PIN_COUNT 46

#define GLB_UART_CFG1			0x154
#define GLB_UART_CFG2			0x158
#define GLB_GPIO_CFG(n)			(0x8c4 + 4 * (n))

#define GPIO_CFG_IE			BIT(0)
#define GPIO_CFG_SMT			BIT(1)
#define GPIO_CFG_DRV			GENMASK(3, 2)
#define GPIO_CFG_DRV_8_MA		0
#define GPIO_CFG_DRV_9_6_MA		1
#define GPIO_CFG_DRV_11_2_MA		2
#define GPIO_CFG_DRV_12_8_MA		3
#define GPIO_CFG_PU			BIT(4)
#define GPIO_CFG_PD			BIT(5)
#define GPIO_CFG_OE			BIT(6)
#define GPIO_CFG_FUNC_SEL		GENMASK(12, 8)
#define GPIO_CFG_INT_MODE		GENMASK(19, 16)
#define GPIO_CFG_INT_MODE_FALLING	0
#define GPIO_CFG_INT_MODE_RISING	1
#define GPIO_CFG_INT_MODE_LOW		2
#define GPIO_CFG_INT_MODE_HIGH		3
#define GPIO_CFG_INT_MODE_BOTH		4
#define GPIO_CFG_INT_CLR		BIT(20)
#define GPIO_CFG_INT_STAT		BIT(21)
#define GPIO_CFG_INT_MASK		BIT(22)
#define GPIO_CFG_O			BIT(24)
#define GPIO_CFG_SET			BIT(25)
#define GPIO_CFG_CLR			BIT(26)
#define GPIO_CFG_I			BIT(28)
#define GPIO_CFG_MODE			GENMASK(31, 30)


struct bl808_pinctrl_priv {
	void *glb;
	int pin_count;
};

static const struct pinconf_param bl808_pinctrl_pinconf_params[] = {
	{ "bias-disable", PIN_CONFIG_BIAS_DISABLE, 0 },
	{ "bias-pull-down", PIN_CONFIG_BIAS_PULL_DOWN, 1 },
	{ "bias-pull-up", PIN_CONFIG_BIAS_PULL_UP, 1 },
	{ "drive-strength-microamp", PIN_CONFIG_DRIVE_STRENGTH_UA, U32_MAX },
	{ "output-enable", PIN_CONFIG_OUTPUT_ENABLE, 1 },
	{ "output-disable", PIN_CONFIG_OUTPUT_ENABLE, 0 },
	{ "input-enable", PIN_CONFIG_INPUT_ENABLE, 1 },
	{ "input-disable", PIN_CONFIG_INPUT_ENABLE, 0 },
	{ "input-schmitt-enable", PIN_CONFIG_INPUT_SCHMITT_ENABLE, 1 },
	{ "input-schmitt-disable", PIN_CONFIG_INPUT_SCHMITT_ENABLE, 0 },
};

static int bl808_pinctrl_get_pins_count(struct udevice *dev)
{
	struct bl808_pinctrl_priv *priv = dev_get_priv(dev);

	return priv->pin_count;
}

static const char *bl808_pinctrl_get_pin_name(struct udevice *dev,
					      unsigned selector)
{
	static char name[PINNAME_SIZE];

	snprintf(name, PINNAME_SIZE, "gpio%d", selector);
	return name;
}

static const char *func_names[32] = {
	[BL808_FUNC_SDH] = "SDH",
	[BL808_FUNC_SPI0] = "SPI0",
	[BL808_FUNC_SFLASH] = "SFLASH",
	[BL808_FUNC_I2S] = "I2S",
	[BL808_FUNC_PDM] = "PDM",
	[BL808_FUNC_I2C0] = "I2C0",
	[BL808_FUNC_I2C1] = "I2C1",
	[BL808_FUNC_UART] = "UART",
	[BL808_FUNC_RMII] = "RMII",
	[BL808_FUNC_CAM] = "CAM",
	[BL808_FUNC_ADC] = "ADC",
	[BL808_FUNC_GPIO] = "GPIO",
	[BL808_FUNC_PWM0] = "PWM0",
	[BL808_FUNC_PWM1] = "PWM1",
	[BL808_FUNC_SPI1] = "SPI1",
	[BL808_FUNC_I2C2] = "I2C2",
	[BL808_FUNC_I2C3] = "I2C3",
	[BL808_FUNC_UART3] = "UART3",
	[BL808_FUNC_DBI_B] = "DBI_B",
	[BL808_FUNC_DBI_C] = "DBI_C",
	[BL808_FUNC_DPI] = "DPI",
	[BL808_FUNC_JTAG_LP] = "JTAG_LP",
	[BL808_FUNC_JTAG_M0] = "JTAG_M0",
	[BL808_FUNC_JTAG_D0] = "JTAG_D0",
	[BL808_FUNC_CLK_OUT] = "CLK_OUT",
};

static const char *drv_strength_str[] = {
	[GPIO_CFG_DRV_8_MA] = "8mA",
	[GPIO_CFG_DRV_9_6_MA] = "9.6mA",
	[GPIO_CFG_DRV_11_2_MA] = "11.2mA",
	[GPIO_CFG_DRV_12_8_MA] = "12.8mA",
};

static const char *uart_subfunc[16] = {
	[BL808_FUNC_UART0_RTS >> 8] = "UART0_RTS",
	[BL808_FUNC_UART0_CTS >> 8] = "UART0_CTS",
	[BL808_FUNC_UART0_TX  >> 8] = "UART0_TX",
	[BL808_FUNC_UART0_RX  >> 8] = "UART0_RX",
	[BL808_FUNC_UART1_RTS >> 8] = "UART1_RTS",
	[BL808_FUNC_UART1_CTS >> 8] = "UART1_CTS",
	[BL808_FUNC_UART1_TX  >> 8] = "UART1_TX",
	[BL808_FUNC_UART1_RX  >> 8] = "UART1_RX",
	[BL808_FUNC_UART2_RTS >> 8] = "UART2_RTS",
	[BL808_FUNC_UART2_CTS >> 8] = "UART2_CTS",
	[BL808_FUNC_UART2_TX  >> 8] = "UART2_TX",
	[BL808_FUNC_UART2_RX  >> 8] = "UART2_RX",
};

static int bl808_pinctrl_get_pin_muxing(struct udevice *dev, unsigned selector,
					char *buf, int size)
{
	struct bl808_pinctrl_priv *priv = dev_get_priv(dev);
	const char *func_str = NULL;
	int func;
        u32 val;

	if (selector > priv->pin_count)
		return -EINVAL;

	val = readl(priv->glb + GLB_GPIO_CFG(selector));
	func = u32_get_bits(val, GPIO_CFG_FUNC_SEL);

	if (func == BL808_FUNC_UART) {
		int subsel = selector % 12;
		int subfunc;

		if (subsel < 8)
			val = readl(priv->glb + GLB_UART_CFG1);
		else
			val = readl(priv->glb + GLB_UART_CFG2);
		subfunc = (val >> (4 * (subsel % 8))) & 0xf;
		func_str = uart_subfunc[subfunc];
	}
	if (!func_str)
		func_str = func_names[func];
	if (!func_str)
		func_str = simple_itoa(func);

	snprintf(buf, size, "%s [%s%s%s%s%s%s]",
		 func_str,
		 (val & GPIO_CFG_PU) ? "pull-up " : "",
		 (val & GPIO_CFG_PD) ? "pull-down " : "",
		 (val & GPIO_CFG_IE) ? "input-enable " : "",
		 (val & GPIO_CFG_OE) ? "output-enable " : "",
		 (val & GPIO_CFG_SMT) ? "schmitt-trigger " : "",
		 drv_strength_str[u32_get_bits(val, GPIO_CFG_DRV)]);

	return 0;
}

static int bl808_pinctrl_pinconf_set(struct udevice *dev, unsigned pin,
				     unsigned param, unsigned argument)
{
	struct bl808_pinctrl_priv *priv = dev_get_priv(dev);
	u32 val, tmp;

	if (pin > priv->pin_count)
		return -EINVAL;

        val = readl(priv->glb + GLB_GPIO_CFG(pin));

	switch (param) {
	case PIN_CONFIG_BIAS_DISABLE:
		val &= ~(GPIO_CFG_PU | GPIO_CFG_PD);
		break;

	case PIN_CONFIG_BIAS_PULL_DOWN:
		val |= GPIO_CFG_PD;
		val &= ~GPIO_CFG_PU;
		break;

	case PIN_CONFIG_BIAS_PULL_UP:
		val |= GPIO_CFG_PU;
		val &= ~GPIO_CFG_PD;
		break;

	case PIN_CONFIG_DRIVE_STRENGTH_UA:
		switch (argument) {
		case 8000:
			tmp = GPIO_CFG_DRV_8_MA;
			break;
		case 9600:
			tmp = GPIO_CFG_DRV_9_6_MA;
			break;
		case 11200:
			tmp = GPIO_CFG_DRV_11_2_MA;
			break;
		case 12800:
			tmp = GPIO_CFG_DRV_12_8_MA;
			break;
		default:
			return -EINVAL;
		}
		u32p_replace_bits(&val, tmp, GPIO_CFG_DRV);
		break;

	case PIN_CONFIG_OUTPUT_ENABLE:
		if (argument)
			val |= GPIO_CFG_OE;
		else
			val &= ~GPIO_CFG_OE;
		break;

	case PIN_CONFIG_INPUT_ENABLE:
		if (argument)
			val |= GPIO_CFG_IE;
		else
			val &= ~GPIO_CFG_IE;
		break;

	case PIN_CONFIG_INPUT_SCHMITT_ENABLE:
		if (argument)
			val |= GPIO_CFG_SMT;
		else
			val &= ~GPIO_CFG_SMT;
		break;

	default:
		return -EINVAL;
	}

	writel(val, priv->glb + GLB_GPIO_CFG(pin));

	return 0;
}

static int bl808_pinctrl_pinmux_property_set(struct udevice *dev,
					     u32 pinmux_group)
{
	struct bl808_pinctrl_priv *priv = dev_get_priv(dev);
	int func = pinmux_group & 0x1f;
	int subfunc = (pinmux_group >> 8) & 0xf;
	int pin = (pinmux_group >> 16) & 0xff;
	u32 val;

	if (pin > priv->pin_count)
		return -EINVAL;

	val = readl(priv->glb + GLB_GPIO_CFG(pin));
	u32p_replace_bits(&val, func, GPIO_CFG_FUNC_SEL);
	/* XXX: unknown */
	u32p_replace_bits(&val, 1, GPIO_CFG_MODE);
	writel(val, priv->glb + GLB_GPIO_CFG(pin));

	if (func == BL808_FUNC_UART) {
		int subsel = pin % 12;
		int shift = 4 * (subsel % 8);
		int mask = 0xf << shift;

		if (subsel < 8)
			val = readl(priv->glb + GLB_UART_CFG1);
		else
			val = readl(priv->glb + GLB_UART_CFG2);

		val &= ~mask;
		val |= subfunc << shift;

		if (subsel < 8)
			writel(val, priv->glb + GLB_UART_CFG1);
		else
			writel(val, priv->glb + GLB_UART_CFG2);
	}

	return pin;
}

static const struct pinctrl_ops bl808_pinctrl_ops = {
	.set_state = pinctrl_generic_set_state,
	.get_pins_count = bl808_pinctrl_get_pins_count,
	.get_pin_name = bl808_pinctrl_get_pin_name,
	.get_pin_muxing = bl808_pinctrl_get_pin_muxing,
	.pinmux_property_set = bl808_pinctrl_pinmux_property_set,
	.pinconf_num_params = ARRAY_SIZE(bl808_pinctrl_pinconf_params),
	.pinconf_params = bl808_pinctrl_pinconf_params,
	.pinconf_set = bl808_pinctrl_pinconf_set,
};

#define BL808_PINCTRL_DEFAULT 0x00400b02
static int bl808_init_gpio_mode(struct bl808_pinctrl_priv *priv)
{
	int pin;

	for (pin = 0; pin < priv->pin_count; pin++)
		writel(BL808_PINCTRL_DEFAULT, priv->glb + GLB_GPIO_CFG(pin));
	writel(~0, priv->glb + GLB_UART_CFG1);
	writel(~0, priv->glb + GLB_UART_CFG2);

	return 0;
}

static int bl808_pinctrl_probe(struct udevice *dev)
{
	struct bl808_pinctrl_priv *priv = dev_get_priv(dev);

	priv->glb = dev_read_addr_ptr(dev_get_parent(dev));
	if (!priv->glb)
		return -EINVAL;

	priv->pin_count = BL808_PIN_COUNT;

	return bl808_init_gpio_mode(priv);
}

static const struct udevice_id bl808_pinctrl_ids[] = {
	{ .compatible = "bouffalolab,bl808-pinctrl" },
	{ }
};

U_BOOT_DRIVER(pinctrl_bl808) = {
	.name = "pinctrl_bl808",
	.id = UCLASS_PINCTRL,
	.of_match = bl808_pinctrl_ids,
	.probe = bl808_pinctrl_probe,
	.priv_auto = sizeof(struct bl808_pinctrl_priv),
	.ops = &bl808_pinctrl_ops,
};
