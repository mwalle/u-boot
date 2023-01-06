// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2023 Michael Walle <michael@walle.cc>
 */

#define DEBUG
#define LOG_CATEGORY UCLASS_SPI

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <log.h>
#include <reset.h>
#include <spi.h>
#include <spi-mem.h>
#include <watchdog.h>
#include <dm/device_compat.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/iopoll.h>
#include <linux/ioport.h>
#include <linux/sizes.h>

#define CTRL0_CLK_SF_RX_INV_SEL	BIT(2)
#define CTRL0_CLK_OUT_GATE_EN	BIT(3)
#define CTRL0_CLK_OUT_INV_SEL	BIT(4)
#define CTRL0_READ_DLY(x)	(((x) & 0x7) << 8)
#define CTRL0_READ_DLY_EN	BIT(11)
#define CTRL0_INT		BIT(16)
#define CTRL0_INT_CLR		BIT(17)
#define CTRL0_INT_SET		BIT(18)
#define CTRL0_32B_ADR_EN	BIT(19)
#define CTRL0_AES_DOUT_ENDIAN	BIT(20)
#define CTRL0_AES_DIN_ENDIAN	BIT(21)
#define CTRL0_AES_KEY_ENDIAN	BIT(22)
#define CTRL0_AES_IV_ENDIAN	BIT(23)
#define CTRL0_ID(x)		(((x) & 0xff) << 24)

#define CTRL1_SR_PAT_MASK(x)	(((x) & 0xff) << 0)
#define CTRL1_SR_PAT(x)		(((x) & 0xff) << 8)
#define CTRL1_SR_INT		BIT(16)
#define CTRL1_SR_INT_EN		BIT(17)
#define CTRL1_SR_INT_SET	BIT(18)
#define CTRL1_IF0_ACK_LAT(x)	(((x) & 0x7) << 20)
#define CTRL1_AHB2SIF_DISWRAP	BIT(23)
#define CTRL1_IF_REG_HOLD	BIT(24)
#define CTRL1_IF_REG_WP		BIT(25)
#define CTRL1_AHB2SIF_STOPPED	BIT(26)
#define CTRL1_AHB2SIF_STOP	BIT(27)
#define CTRL1_IF_FN_IAHB	BIT(28)
#define CTRL1_IF_EN		BIT(29)
#define CTRL1_AHB2SIF_EN	BIT(30)
#define CTRL1_AHB2SRAM_EN	BIT(31)

#define SAHB0_IF_BUSY		BIT(0)
#define SAHB0_IF_TRIG		BIT(1)
#define SAHB0_IF_DAT_BYTE(x)	(((x) & 0x3ff) << 2)
#define SAHB0_IF_DMY_BYTE(x)	(((x) & 0x1f) << 12)
#define SAHB0_IF_ADR_BYTE(x)	(((x) & 0x7) << 17)
#define SAHB0_IF_CMD_BYTE(x)	(((x) & 0x7) << 20)
#define SAHB0_IF_DAT_RW		BIT(23)
#define SAHB0_IF_DAT_EN		BIT(24)
#define SAHB0_IF_DMY_EN		BIT(25)
#define SAHB0_IF_ADR_EN		BIT(26)
#define SAHB0_IF_CMD_EN		BIT(27)
#define SAHB0_IF_MODE_SINGLE	(0 << 28)
#define SAHB0_IF_MODE_DUAL	(1 << 28)
#define SAHB0_IF_MODE_QUAD	(2 << 28)
#define SAHB0_IF_MODE_DUAL_IO	(3 << 28)
#define SAHB0_IF_MODE_QUAD_IO	(4 << 28)
#define SAHB0_IF_QPI_EN		BIT(31) /* 4 bit command mode */

#define IO_DLY0_CS_DLY_SEL(x)		(((x) & 0x3) << 0)
#define IO_DLY0_CS2_DLY_SEL(x)		(((x) & 0x3) << 2)
#define IO_DLY0_CLK_OUT_DLY_SEL(x)	(((x) & 0x3) << 8)
#define IO_DLY0_DQS_OE_DLY_SEL(x)	(((x) & 0x3) << 26)
#define IO_DLY0_DQS_DI_DLY_SEL(x)	(((x) & 0x3) << 28)
#define IO_DLY0_DQS_DO_DLY_SEL(x)	(((x) & 0x3) << 30)

#define IO_DLY1_IO0_OE_DLY_SEL(x)	(((x) & 0x3) << 0)
#define IO_DLY1_IO0_DI_DLY_SEL(x)	(((x) & 0x3) << 8)
#define IO_DLY1_IO0_DO_DLY_SEL(x)	(((x) & 0x3) << 16)

#define IO_DLY2_IO1_OE_DLY_SEL(x)	(((x) & 0x3) << 0)
#define IO_DLY2_IO1_DI_DLY_SEL(x)	(((x) & 0x3) << 8)
#define IO_DLY2_IO1_DO_DLY_SEL(x)	(((x) & 0x3) << 16)

#define IO_DLY3_IO2_OE_DLY_SEL(x)	(((x) & 0x3) << 0)
#define IO_DLY3_IO2_DI_DLY_SEL(x)	(((x) & 0x3) << 8)
#define IO_DLY3_IO2_DO_DLY_SEL(x)	(((x) & 0x3) << 16)

#define IO_DLY4_IO3_OE_DLY_SEL(x)	(((x) & 0x3) << 0)
#define IO_DLY4_IO3_DI_DLY_SEL(x)	(((x) & 0x3) << 8)
#define IO_DLY4_IO3_DO_DLY_SEL(x)	(((x) & 0x3) << 16)

#define BFLB_SFLASH_BUF_SIZE 0x800

struct bflb_sflash_regs {
	u32 ctrl0;
	u32 ctrl1;
	u32 if_sahb0;
	u32 if_sahb1;
	u32 if_sahb2;
	u32 if_iahb0;
	u32 if_iahb1;
	u32 if_iahb2;
	u32 if_status0;
	u32 if_status1;
	u32 aes;
	u32 ahb2sif_status;
	u32 if_io_dly0[5];
	u32 __reserved0;
	u32 if_io_dly1[5];
	u32 if_io_dly2[5];
	u32 ctrl2;
	u32 ctrl3;
	u32 if_iahb3;
	u32 if_iahb4;
	u32 if_iahb5;
	u32 if_iahb6;
	u32 if_iahb7;
	u32 if_iahb8;
	u32 if_iahb9;
	u32 if_iahb10;
	u32 if_iahb11;
	u32 if_iahb12;
	u32 id0_offset;
	u32 id1_offset;
	u32 bk2_id0_offset;
	u32 bk2_id1_offset;
	u32 dbg;
	u32 __reserved1[3];
	u32 if2_ctrl0;
	u32 if2_ctrl1;
	u32 if2_sahb0;
	u32 if2_sahb1;
	u32 if2_sahb2;
	u32 __reserved2[11];
	u32 ctrl_prot_en_rd;
	u32 ctrl_prot_en;
	u32 __reserved3[62];
	u32 aes_key_r0[8];
	u32 aes_iv_r0[4];
	u32 aes_r0_start;
	u32 aes_r0_end;
	u32 __reserved4[18];
	u32 aes_key_r1[8];
	u32 aes_iv_r1[4];
	u32 aes_r1_start;
	u32 aes_r1_end;
	u32 __reserved5[18];
	u32 aes_key_r2[8];
	u32 aes_iv_r2[4];
	u32 aes_r2_start;
	u32 aes_r2_end;
	u32 __reserved6[178];
	u32 buffer[];
};

static_assert(offsetof(struct bflb_sflash_regs, dbg) == 0xb0);
static_assert(offsetof(struct bflb_sflash_regs, dbg) == 0xb0);
static_assert(offsetof(struct bflb_sflash_regs, if2_sahb2) == 0xd0);
static_assert(offsetof(struct bflb_sflash_regs, ctrl_prot_en) == 0x104);
static_assert(offsetof(struct bflb_sflash_regs, aes_key_r0) == 0x200);
static_assert(offsetof(struct bflb_sflash_regs, aes_r0_end) == 0x234);
static_assert(offsetof(struct bflb_sflash_regs, aes_key_r1) == 0x280);
static_assert(offsetof(struct bflb_sflash_regs, aes_r1_end) == 0x2b4);
static_assert(offsetof(struct bflb_sflash_regs, aes_key_r2) == 0x300);
static_assert(offsetof(struct bflb_sflash_regs, aes_r2_end) == 0x334);
static_assert(offsetof(struct bflb_sflash_regs, buffer) == 0x600);

struct bflb_sflash_priv {
	struct udevice *dev;
	struct bflb_sflash_regs *regs;
};

static int bflb_sflash_adjust_op_size(struct spi_slave *slave,
				   struct spi_mem_op *op)
{
	if (op->data.nbytes > BFLB_SFLASH_BUF_SIZE)
		op->data.nbytes = BFLB_SFLASH_BUF_SIZE;

	return 0;
}

static bool bflb_sflash_is_op(const struct spi_mem_op *op,
			      int cmd, int adr_dmy, int dat)
{
	if (op->cmd.buswidth != cmd)
		return false;
	if (op->addr.nbytes && op->addr.buswidth != adr_dmy)
		return false;
	if (op->dummy.nbytes && op->dummy.buswidth != adr_dmy)
		return false;
	if (op->data.nbytes && op->data.buswidth != dat)
		return false;
	return true;
}

static bool bflb_sflash_supports_op(struct spi_slave *slave,
				  const struct spi_mem_op *op)
{
	return bflb_sflash_is_op(op, 1, 1, 1) ||
	       bflb_sflash_is_op(op, 1, 1, 2) ||
	       bflb_sflash_is_op(op, 1, 1, 4) ||
	       bflb_sflash_is_op(op, 1, 2, 2) ||
	       bflb_sflash_is_op(op, 1, 4, 4) ||
	       bflb_sflash_is_op(op, 4, 4, 4);
}

static int bflb_sflash_exec_op(struct spi_slave *slave,
			     const struct spi_mem_op *op)
{
	struct udevice *dev = dev_get_parent(slave->dev);
	struct bflb_sflash_priv *priv = dev_get_priv(dev);
	struct bflb_sflash_regs *regs = priv->regs;
	u32 val;
	int ret;

	debug("%s cmd:%#x mode:%d.%d.%d.%d addr:%#llx len:%#x\n",
	      __func__,
	      op->cmd.opcode, op->cmd.buswidth, op->addr.buswidth,
	      op->dummy.buswidth, op->data.buswidth, op->addr.val,
	      op->data.nbytes);

	writel(op->cmd.opcode << 24, &regs->if_sahb1);
	if (op->cmd.buswidth == 4)
		val = SAHB0_IF_MODE_QUAD_IO | SAHB0_IF_QPI_EN;
	else if (op->data.buswidth == 1)
		val = SAHB0_IF_MODE_SINGLE;
	else if (op->data.buswidth == 2)
		if (op->addr.buswidth == 1)
			val = SAHB0_IF_MODE_DUAL;
		else
			val = SAHB0_IF_MODE_DUAL_IO;
	else
		if (op->addr.buswidth == 1)
			val = SAHB0_IF_MODE_QUAD;
		else
			val = SAHB0_IF_MODE_QUAD_IO;

	if (op->cmd.nbytes)
		val |= SAHB0_IF_CMD_EN | SAHB0_IF_CMD_BYTE(op->cmd.nbytes - 1);
	if (op->addr.nbytes)
		val |= SAHB0_IF_ADR_EN | SAHB0_IF_ADR_BYTE(op->addr.nbytes - 1);
	if (op->dummy.nbytes)
		val |= SAHB0_IF_DMY_EN | SAHB0_IF_DMY_BYTE(op->dummy.nbytes - 1);
	if (op->data.nbytes)
		val |= SAHB0_IF_DAT_EN | SAHB0_IF_DAT_BYTE(op->data.nbytes - 1);
	writel(val, &regs->if_sahb0);

	if (op->data.dir == SPI_MEM_DATA_OUT)
		memcpy(&regs->buffer, op->data.buf.out, op->data.nbytes);

	/*
	 * This needs a rising edge. We cannot combine the tigger bit in the
	 * former write.
	 */
	setbits_le32(&regs->if_sahb0, SAHB0_IF_TRIG);
	ret = readl_poll_timeout(&regs->if_sahb0, val, !(val & SAHB0_IF_BUSY), 10000);
	if (ret)
		return ret;

	if (op->data.dir == SPI_MEM_DATA_IN)
		memcpy(op->data.buf.in, &regs->buffer, op->data.nbytes);

	return 0;
}

static int bflb_sflash_set_mode(struct udevice *bus, uint mode)
{
	return 0;
}

static int bflb_sflash_set_speed(struct udevice *bus, uint speed)
{
	debug("%s %u\n", __func__, speed);

	return 0;
}

static const struct spi_controller_mem_ops bflb_sflash_mem_ops = {
	.adjust_op_size = bflb_sflash_adjust_op_size,
	.supports_op = bflb_sflash_supports_op,
	.exec_op = bflb_sflash_exec_op
};

static const struct dm_spi_ops bflb_sflash_ops = {
	.mem_ops = &bflb_sflash_mem_ops,
	.set_speed = bflb_sflash_set_speed,
	.set_mode = bflb_sflash_set_mode,
};

static void pinmux_init(void)
{
	writel(0x40400256, (void*)0x2000094c);
	writel(0x40400256, (void*)0x20000950);
	writel(0x40400217, (void*)0x20000954);
	writel(0x40400217, (void*)0x20000958);
	writel(0x40400217, (void*)0x2000095c);
	writel(0x40400217, (void*)0x20000960);

	writel(0x900, (void*)0x20000510);
	writel(0x001, (void*)0x2000b070);
}

static int bflb_sflash_probe(struct udevice *dev)
{
	struct bflb_sflash_priv *priv = dev_get_priv(dev);

	priv->regs = dev_read_addr_ptr(dev);
	if (!priv->regs)
		return -EINVAL;

	pinmux_init();

	writel(CTRL0_CLK_OUT_GATE_EN |
	       CTRL0_CLK_OUT_INV_SEL |
	       CTRL0_READ_DLY(0) |
	       CTRL0_READ_DLY_EN |
	       CTRL0_ID(26), &priv->regs->ctrl0);

	writel(CTRL1_IF_EN |
	       CTRL1_IF0_ACK_LAT(6) |
	       CTRL1_AHB2SIF_EN	|
	       CTRL1_AHB2SRAM_EN, &priv->regs->ctrl1);

	return 0;
}

static const struct udevice_id bflb_sflash_ids[] = {
	{ .compatible = "bouffalolab,bl808-sflash" },
	{}
};

U_BOOT_DRIVER(bflb_sflash) = {
	.name = "bflb_sflash",
	.id = UCLASS_SPI,
	.of_match = bflb_sflash_ids,
	.ops = &bflb_sflash_ops,
	.priv_auto = sizeof(struct bflb_sflash_priv),
	.probe = bflb_sflash_probe,
};
