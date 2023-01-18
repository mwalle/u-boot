// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2022
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

#define UART_TXFIFO_CNT_MASK 0x0000003f
#define UART_RXFIFO_CNT_MASK 0x00003f00

#define UART_BIT_PRD_RX_DIV(x)	(((x) & 0xffff) << 16)
#define UART_BIT_PRD_TX_DIV(x)	((x) & 0xffff)

struct uart_bflb {
	u32 utx_config;
	u32 urx_config;
	u32 uart_bit_prd;
	u32 data_config;
	u32 utx_ir_position;
	u32 urx_ir_position;
	u32 urx_rto_timer;
	u32 uart_sw_mode;
	u32 uart_int_sts;
	u32 uart_int_mask;
	u32 uart_int_clear;
	u32 uart_int_en;
	u32 uart_status;
	u32 sts_urx_abr_prd;
	u32 urx_abr_prd_b01;
	u32 urx_abr_prd_b23;
	u32 urx_abr_prd_b45;
	u32 urx_abr_prd_b67;
	u32 urx_abr_pw_tol;
	u32 rsvd0[1];
	u32 urx_bcr_int_cfg;
	u32 utx_rs485_cfg;
	u32 rsvd1[10];
	u32 uart_fifo_config_0;
	u32 uart_fifo_config_1;
	u32 uart_fifo_wdata;
	u32 uart_fifo_rdata;
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
	int div = clock / baud;

	writel(UART_BIT_PRD_RX_DIV(div) |
	       UART_BIT_PRD_TX_DIV(div), &regs->uart_bit_prd);
}

static void _bflb_serial_init(struct uart_bflb *regs)
{
	writel(0xf05, &regs->utx_config);
	writel(0x701, &regs->urx_config);
	//writel(UART_TXCTRL_TXEN, &regs->txctrl);
	//writel(UART_RXCTRL_RXEN, &regs->rxctrl);
	//writel(0, &regs->ie);
}

static int _bflb_serial_putc(struct uart_bflb *regs, const char c)
{
	if ((readl(&regs->uart_fifo_config_1) & UART_TXFIFO_CNT_MASK) == 0)
		return -EAGAIN;

	writel(c, &regs->uart_fifo_wdata);

	return 0;
}

static int _bflb_serial_getc(struct uart_bflb *regs)
{
	if ((readl(&regs->uart_fifo_config_1) & UART_RXFIFO_CNT_MASK) == 0)
		return -EAGAIN;

	return readl(&regs->uart_fifo_rdata);
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

#if 0
	/* No need to reinitialize the UART after relocation */
	if (gd->flags & GD_FLG_RELOC)
		return 0;
#endif
	ret = clk_get_by_index(dev, 0, &priv->clk);
	if (ret < 0)
		return ret;

	ret = clk_enable(&priv->clk);
	if (ret < 0)
		return ret;

       writel(0x0f000003, 0x20000120);

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

	stat = readl(&regs->uart_fifo_config_1);

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
	{ }
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
#if 0
	while ((readl(&regs->uart_fifo_config_1) & UART_TXFIFO_CNT_MASK) == 0)
		schedule();

	writel(ch, &regs->uart_fifo_wdata);
#endif
}

DEBUG_UART_FUNCS

#endif
