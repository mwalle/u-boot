// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2023 Michael Walle <michael@walle.cc>
 */

#include <common.h>
#include <clk.h>
#include <debug_uart.h>
#include <dm.h>
#include <errno.h>
#include <fdtdec.h>
#include <log.h>
#include <watchdog.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <linux/compiler.h>
#include <serial.h>
#include <linux/err.h>

DECLARE_GLOBAL_DATA_PTR;

#define TX_EN			BIT(0)
#define TX_CTS_EN		BIT(1)
#define TX_FRM_EN		BIT(2)
#define TX_BIT_CNT_D(x)		(((x) & 7) << 8)
#define TX_BIT_CNT_P(x)		(((x) & 3) << 11)

#define RX_EN			BIT(0)
#define RX_BIT_CNT_D(x)		(((x) & 7) << 8)

#define UART_BIT_PRD_RX_DIV(x)	(((x) & 0xffff) << 16)
#define UART_BIT_PRD_TX_DIV(x)	((x) & 0xffff)

#define FIFO_CONFIG_0_TX_FIFO_CLR BIT(2)
#define FIFO_CONFIG_0_RX_FIFO_CLR BIT(3)

#define UART_TXFIFO_CNT_MASK 0x0000003f
#define UART_RXFIFO_CNT_MASK 0x00003f00


struct uart_bflb {
	u32 tx_config;
	u32 rx_config;
	u32 uart_bit_prd;
	u32 data_config;
	u32 tx_ir_position;
	u32 rx_ir_position;
	u32 rx_rto_timer;
	u32 sw_mode;
	u32 int_sts;
	u32 int_mask;
	u32 int_clear;
	u32 int_en;
	u32 status;
	u32 sts_rx_abr_prd;
	u32 rx_abr_prd_b01;
	u32 rx_abr_prd_b23;
	u32 rx_abr_prd_b45;
	u32 rx_abr_prd_b67;
	u32 rx_abr_pw_tol;
	u32 rsvd0[1];
	u32 rx_bcr_int_cfg;
	u32 tx_rs485_cfg;
	u32 rsvd1[10];
	u32 fifo_config_0;
	u32 fifo_config_1;
	u32 fifo_wdata;
	u32 fifo_rdata;
};

struct bflb_uart_plat {
	struct uart_bflb *regs;
};

struct bflb_uart_priv {
	struct clk clk;
};

static void _bflb_serial_setbrg(struct uart_bflb *regs,
				unsigned long clock, unsigned long baud)
{
	unsigned int div = clock / baud - 1;

	/* drain the buffer */
	while (!(readl(&regs->fifo_config_1) & UART_TXFIFO_CNT_MASK));

	writel(UART_BIT_PRD_RX_DIV(div) |
	       UART_BIT_PRD_TX_DIV(div), &regs->uart_bit_prd);
}

static void _bflb_serial_init(struct uart_bflb *regs)
{
	writel(TX_BIT_CNT_P(2) | TX_BIT_CNT_D(7) | TX_FRM_EN | TX_EN,
	       &regs->tx_config);
	writel(RX_BIT_CNT_D(7) | RX_EN, &regs->rx_config);
}

static int _bflb_serial_putc(struct uart_bflb *regs, const char c)
{
	if ((readl(&regs->fifo_config_1) & UART_TXFIFO_CNT_MASK) == 0)
		return -EAGAIN;

	writel(c, &regs->fifo_wdata);

	return 0;
}

static int _bflb_serial_getc(struct uart_bflb *regs)
{
	if ((readl(&regs->fifo_config_1) & UART_RXFIFO_CNT_MASK) == 0)
		return -EAGAIN;

	return readl(&regs->fifo_rdata);
}

static int bflb_serial_setbrg(struct udevice *dev, int baudrate)
{
	struct bflb_uart_plat *plat = dev_get_plat(dev);
	struct bflb_uart_priv *priv = dev_get_priv(dev);

	_bflb_serial_setbrg(plat->regs, clk_get_rate(&priv->clk), baudrate);

	return 0;
}

static int bflb_serial_probe(struct udevice *dev)
{
	struct bflb_uart_plat *plat = dev_get_plat(dev);
	struct bflb_uart_priv *priv = dev_get_priv(dev);
	int ret;

	ret = clk_get_by_index(dev, 0, &priv->clk);
	if (ret < 0)
		return ret;

	ret = clk_enable(&priv->clk);
	if (ret < 0)
		return ret;

	_bflb_serial_init(plat->regs);

	return 0;
}

static int bflb_serial_getc(struct udevice *dev)
{
	int c;
	struct bflb_uart_plat *plat = dev_get_plat(dev);
	struct uart_bflb *regs = plat->regs;

	while ((c = _bflb_serial_getc(regs)) == -EAGAIN) ;

	return c;
}

static int bflb_serial_putc(struct udevice *dev, const char ch)
{
	int rc;
	struct bflb_uart_plat *plat = dev_get_plat(dev);

	while ((rc = _bflb_serial_putc(plat->regs, ch)) == -EAGAIN) ;

	return rc;
}

static int bflb_serial_pending(struct udevice *dev, bool input)
{
	struct bflb_uart_plat *plat = dev_get_plat(dev);
	struct uart_bflb *regs = plat->regs;
	u32 stat;

	stat = readl(&regs->fifo_config_1);

	if (input)
		return !!(stat & UART_RXFIFO_CNT_MASK);
	else
		return !!(stat & UART_TXFIFO_CNT_MASK);
}

static int bflb_serial_of_to_plat(struct udevice *dev)
{
	struct bflb_uart_plat *plat = dev_get_plat(dev);

	plat->regs = (struct uart_bflb *)(uintptr_t)dev_read_addr(dev);
	if (IS_ERR(plat->regs))
		return PTR_ERR(plat->regs);

	return 0;
}

static const struct dm_serial_ops bflb_serial_ops = {
	.putc = bflb_serial_putc,
	.getc = bflb_serial_getc,
	.pending = bflb_serial_pending,
	.setbrg = bflb_serial_setbrg,
};

static const struct udevice_id bflb_serial_ids[] = {
	{ .compatible = "bouffalolab,bl808-uart" },
	{}
};

U_BOOT_DRIVER(serial_bflb) = {
	.name = "serial_bflb",
	.id = UCLASS_SERIAL,
	.of_match = bflb_serial_ids,
	.of_to_plat = bflb_serial_of_to_plat,
	.plat_auto = sizeof(struct bflb_uart_plat),
	.priv_auto = sizeof(struct bflb_uart_priv),
	.probe = bflb_serial_probe,
	.ops = &bflb_serial_ops,
};

#ifdef CONFIG_DEBUG_UART_BFLB
static inline void _debug_uart_init(void)
{
	struct uart_bflb *regs =
			(struct uart_bflb *)CONFIG_VAL(DEBUG_UART_BASE);

	_bflb_serial_setbrg(regs, CONFIG_DEBUG_UART_CLOCK,
			      CONFIG_BAUDRATE);
	_bflb_serial_init(regs);
}

static inline void _debug_uart_putc(int ch)
{
	struct uart_bflb *regs = (struct uart_bflb *)CONFIG_VAL(DEBUG_UART_BASE);

	while (_bflb_serial_putc(regs, ch) == -EAGAIN)
		schedule();
}

DEBUG_UART_FUNCS

#endif
