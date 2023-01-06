// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) Michael Walle <michael@walle.cc>
 *
 * Based on the psram initialization from the MCU SDK:
 *   https://github.com/bouffalolab/bl_mcu_sdk/tree/a574195a4b3
 */

#include <common.h>
#include <dm.h>
#include <fdtdec.h>
#include <init.h>
#include <ram.h>
#include <syscon.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <clk.h>
#include <wait_bit.h>
#include <linux/bitops.h>

#define BASIC_INIT_EN			BIT(0)
#define BASIC_AF_EN			BIT(1)
#define BASIC_ADDRMB_MSK(x)		(((x) & 0xff) << 16)
#define BASIC_LINEAR_BND_B(x)		(((x) & 0xf) << 28)

#define MANUAL_PCK_T_DIV(x)		(((x) & 0xff) << 24)

#define AUTO_FRESH_4_BUST_CYCLE(x)	(((x) & 0x7f) << 0)
#define AUTO_FRESH_2_REFI_CYCLE(x)	(((x) & 0xffff) << 0)

#define TIMING_CTRL_TRC_CYCLE(x)	(((x) & 0xff) << 0)
#define TIMING_CTRL_TCPHR_CYCLE(x)	(((x) & 0xff) << 8)
#define TIMING_CTRL_TCPHW_CYCLE(x)	(((x) & 0xff) << 16)
#define TIMING_CTRL_TRFC_CYCLE(x)	(((x) & 0xff) << 24)

#define PHY_CFG_00_CK_SR(x)		(((x) & 0x3) << 8)
#define PHY_CFG_00_CK_DLY_DRV(x)	(((x) & 0xf) << 16)
#define PHY_CFG_00_CEN_SR(x)		(((x) & 0x3) << 20)
#define PHY_CFG_00_CEN_DLY_DRV(x)	(((x) & 0xf) << 28)

#define PHY_CFG_04_DM1_SR(x)		(((x) & 0x3) << 4)
#define PHY_CFG_04_DM1_DLY_DRV(x)	(((x) & 0xf) << 12)
#define PHY_CFG_04_DM0_SR(x)		(((x) & 0x3) << 20)
#define PHY_CFG_04_DM0_DLY_DRV(x)	(((x) & 0xf) << 28)

/* phy_cfg_08 to phy_cfg_24 */
#define PHY_CFG_DQn_SR(x)		(((x) & 0x3) << 0)
#define PHY_CFG_DQn_DLY_RX(x)		(((x) & 0xf) << 8)
#define PHY_CFG_DQn_DLY_DRV(x)		(((x) & 0xf) << 12)
#define PHY_CFG_DQm_SR(x)		(((x) & 0x3) << 20)
#define PHY_CFG_DQm_DLY_RX(x)		(((x) & 0xf) << 24)
#define PHY_CFG_DQm_DLY_DRV(x)		(((x) & 0xf) << 28)

#define PHY_CFG_28_DQS0N_DLY_RX(x)	(((x) & 0xf) << 8)
#define PHY_CFG_28_DQS0_SR(x)		(((x) & 0x3) << 12)
#define PHY_CFG_28_DQS0_SEL(x)		(((x) & 0x3) << 14)
#define PHY_CFG_28_DQS0_DLY_RX(x)	(((x) & 0xf) << 20)
#define PHY_CFG_28_DQS0_DLY_DRV(x)	(((x) & 0xf) << 24)
#define PHY_CFG_28_DQS0_DIFF_DLY_RX(x)	(((x) & 0xf) << 28)

#define PHY_CFG_2C_IPP5UN_LPDDR		BIT(0)
#define PHY_CFG_2C_EN_RX_FE		BIT(1)
#define PHY_CFG_2C_EN_BIAS		BIT(2)
#define PHY_CFG_2C_DQS1N_DLY_RX(x)	(((x) & 0xf) << 8)
#define PHY_CFG_2C_DQS1_SR(x)		(((x) & 0x3) << 12)
#define PHY_CFG_2C_DQS1_SEL(x)		(((x) & 0x3) << 14)
#define PHY_CFG_2C_DQS1_DLY_RX(x)	(((x) & 0xf) << 20)
#define PHY_CFG_2C_DQS1_DLY_DRV(x)	(((x) & 0xf) << 24)
#define PHY_CFG_2C_DQS1_DIFF_DLY_RX(x)	(((x) & 0xf) << 28)

#define PHY_CFG_30_WL_DQ_DIG(x)		(((x) & 0x7) << 0)
#define PHY_CFG_30_WL_DQ_ANA(x)		(((x) & 0x7) << 4)
#define PHY_CFG_30_WL_DIG(x)		(((x) & 0x7) << 8)
#define PHY_CFG_30_WL_ANA(x)		(((x) & 0x7) << 12)
#define PHY_CFG_30_RL_DIG(x)		(((x) & 0xf) << 16)
#define PHY_CFG_30_RL_ANA(x)		(((x) & 0x3) << 20)
#define PHY_CFG_30_OE_TIMER(x)		(((x) & 0x3) << 24)
#define PHY_CFG_30_VREF_MODE		BIT(26)
#define PHY_CFG_30_OE_CTRL_HW		BIT(27)
#define PHY_CFG_30_ODT_SEL(x)		(((x) & 0xf) << 28)

#define PHY_CFG_34_TIMER_DQS_START(x)		(((x) & 0xff) << 0)
#define PHY_CFG_34_TIMER_DQS_ARRAY_STOP(x)	(((x) & 0xff) << 8)
#define PHY_CFG_34_TIMER_ARRAY_WRITE(x)		(((x) & 0xff) << 16)
#define PHY_CFG_34_TIMER_ARRAY_READ(x)		(((x) & 0xff) << 24)

#define PHY_CFG_38_TIMER_AUTO_REFRESH(x)	(((x) & 0xff) << 0)
#define PHY_CFG_38_TIMER_REG_WRITE(x)		(((x) & 0xff) << 8)
#define PHY_CFG_38_TIMER_REG_READ(x)		(((x) & 0xff) << 16)
#define PHY_CFG_38_TIMER_DQS_STOP(x)		(((x) & 0xff) << 24)

#define PHY_CFG_3C_TIMER_SELF_REFRESH1_IN(x)	(((x) & 0xff) << 0)
#define PHY_CFG_3C_TIMER_SELF_REFRESH1_EXIT(x)	(((x) & 0xff) << 8)
#define PHY_CFG_3C_TIMER_GLOBAL_RST(x)		(((x) & 0x3fff) << 16)

#define PHY_CFG_40_DMY0(x)		(((x) & 0xff) << 8)
#define PHY_CFG_40_UNK0			0x00030000
#define PHY_CFG_40_UNK1			0x00300000

#define PHY_CFG_44_TIMER_ARRAY_READ_BUSY(x)	(((x) & 0xff) << 0)
#define PHY_CFG_44_TIMER_ARRAY_WRITE_BUSY(x)	(((x) & 0xff) << 8)
#define PHY_CFG_44_TIMER_REG_READ_BUSY(x)	(((x) & 0xff) << 16)
#define PHY_CFG_44_TIMER_REG_WRITE_BUSY(x)	(((x) & 0xff) << 24)

#define PHY_CFG_48_PSRAM_TYPE(x)	(((x) & 0x3) << 8)

#define PHY_CFG_4C_ODT_SEL_DLY(x)	(((x) & 0xf) << 16)
#define PHY_CFG_4C_ODT_SEL_HW		BIT(20)

#define PHY_CFG_50_DQ_OE_UP_P(x)	(((x) & 0x7) << 0)
#define PHY_CFG_50_DQ_OE_UP_N(x)	(((x) & 0x7) << 4)
#define PHY_CFG_50_DQ_OE_MID_P(x)	(((x) & 0x3) << 8)
#define PHY_CFG_50_DQ_OE_MID_N(x)	(((x) & 0x3) << 12)
#define PHY_CFG_50_DQ_OE_DN_P(x)	(((x) & 0x7) << 16)
#define PHY_CFG_50_DQ_OE_DN_N(x)	(((x) & 0x7) << 20)
#define PHY_CFG_50_WL_CEN_ANA(x)	(((x) & 0x7) << 24)

DECLARE_GLOBAL_DATA_PTR;

struct bl808_psram_regs {
	u32 basic;		/* 0x000*/
	u32 cmd;
	u32 fifo_thre;
	u32 manual;
	u32 auto_fresh_1;
	u32 auto_fresh_2;
	u32 auto_fresh_3;
	u32 auto_fresh_4;
	u32 psram_configure;
	u32 psram_status;
	u32 reserved0[2];
	u32 timing_ctrl;	/* 0x030 */
	u32 rsvd_reg;
	u32 reserved1[34];
	u32 dbg_sel;	/* 0x0c0 */
	u32 reserved2[11];
	u32 dummy_reg;	/* 0x0f0 */
	u32 reserved3[3];
	u32 phy_cfg_00;	/* 0x100 */
	u32 phy_cfg_04;
	u32 phy_cfg_08;
	u32 phy_cfg_0c;
	u32 phy_cfg_10;
	u32 phy_cfg_14;
	u32 phy_cfg_18;
	u32 phy_cfg_1c;
	u32 phy_cfg_20;
	u32 phy_cfg_24;
	u32 phy_cfg_28;
	u32 phy_cfg_2c;
	u32 phy_cfg_30;
	u32 phy_cfg_34;
	u32 phy_cfg_38;
	u32 phy_cfg_3c;
	u32 phy_cfg_40;
	u32 phy_cfg_44;
	u32 phy_cfg_48;
	u32 phy_cfg_4c;
	u32 phy_cfg_50;
};

struct bflb_psram_info {
	struct udevice *dev;
	struct ram_info info;
	struct bl808_psram_regs *regs;
	bool ext_temp_range; /* >85degC */
	int pck_freq;
	int mem_size;
	int page_size;
	bool high_temp;
};

struct uhs_phy_cfg {
	u32 wl_cen_ana:3;
	u32 wl_dq_dig:3;
	u32 wl_dq_ana:3;
	u32 wl_dig:3;
	u32 wl_ana:3;
	u32 rl_dig:4;
	u32 rl_ana:3;
	//u32 oe_timer:2;

	u32 timer_dqs_start:8;
	u32 timer_dqs_array_stop:8;
	u32 timer_array_write:8;
	u32 timer_array_read:8;

	u32 timer_auto_refresh:8;
	u32 timer_reg_read:8;
	u32 timer_reg_write:8;
	u32 timer_dqs_stop:8;

	u32 timer_self_refresh1_in:8;
	u32 timer_self_refresh1_exit:8;
	u32 timer_global_rst:14;

	u32 timer_array_read_busy:8;
	u32 timer_array_write_busy:8;
	u32 timer_reg_read_busy:8;
	u32 timer_reg_write_busy:8;
};

/* cfg_30..44 = 0f0a1323 0b030404 050e0419 0a6a1c1c 0711070e */
static const struct uhs_phy_cfg uhs_phy_2133mhz = {
	.wl_dq_dig = 3,
	.wl_dq_ana = 2,
	.wl_dig = 3,
	.wl_ana = 1,
	.rl_dig = 10,
	.rl_ana = 0,
	//.oe_timer = 15,

	.timer_dqs_start = 4,
	.timer_dqs_array_stop = 4,
	.timer_array_write = 3,
	.timer_array_read = 11,

	.timer_auto_refresh = 25,
	.timer_reg_read = 4,
	.timer_reg_write = 14,
	.timer_dqs_stop = 5,

	.timer_self_refresh1_in = 28,
	.timer_self_refresh1_exit = 28,
	.timer_global_rst = 2666,

	.timer_array_read_busy = 14,
	.timer_array_write_busy = 7,
	.timer_reg_read_busy = 17,
	.timer_reg_write_busy = 7,

	.wl_cen_ana = 1,
};

/* cfg_30..44 = 0f283203 0a020303 040d0416 091e1818 0710070d */
static const struct uhs_phy_cfg uhs_phy_1866mhz = {
	.wl_dq_dig = 3,
	.wl_dq_ana = 0,
	.wl_dig = 2,
	.wl_ana = 3,
	.rl_dig = 8,
	.rl_ana = 2,
	//.oe_timer = 15,

	.timer_dqs_start = 3,
	.timer_dqs_array_stop = 3,
	.timer_array_write = 2,
	.timer_array_read = 10,

	.timer_auto_refresh = 22,
	.timer_reg_read = 4,
	.timer_reg_write = 13,
	.timer_dqs_stop = 4,

	.timer_self_refresh1_in = 24,
	.timer_self_refresh1_exit = 24,
	.timer_global_rst = 2334,

	.timer_array_read_busy = 13,
	.timer_array_write_busy = 7,
	.timer_reg_read_busy = 16,
	.timer_reg_write_busy = 7,

	.wl_cen_ana = 3,
};

/* cfg_30..44 = 0f270212 09020303 040c0313 07d11515 060f060c */
static const struct uhs_phy_cfg uhs_phy_1600mhz = {
	.wl_dq_dig = 2,
	.wl_dq_ana = 1,
	.wl_dig = 2,
	.wl_ana = 0,
	.rl_dig = 7,
	.rl_ana = 2,
	//.oe_timer = 15,

	.timer_dqs_start = 3,
	.timer_dqs_array_stop = 3,
	.timer_array_write = 2,
	.timer_array_read = 9,

	.timer_auto_refresh = 19,
	.timer_reg_read = 3,
	.timer_reg_write = 12,
	.timer_dqs_stop = 4,

	.timer_self_refresh1_in = 21,
	.timer_self_refresh1_exit = 21,
	.timer_global_rst = 2001,

	.timer_array_read_busy = 12,
	.timer_array_write_busy = 6,
	.timer_reg_read_busy = 15,
	.timer_reg_write_busy = 6,

	.wl_cen_ana = 1,
};

/* cfg_30..44 = 0f270212 06010202 0309020d 05360e0e 050c0509 */
static const struct uhs_phy_cfg uhs_phy_1066mhz = {
	.wl_dq_dig = 2,
	.wl_dq_ana = 1,
	.wl_dig = 2,
	.wl_ana = 0,
	.rl_dig = 7,
	.rl_ana = 2,
	//.oe_timer = 15,

	.timer_dqs_start = 2,
	.timer_dqs_array_stop = 2,
	.timer_array_write = 1,
	.timer_array_read = 6,

	.timer_auto_refresh = 13,
	.timer_reg_read = 2,
	.timer_reg_write = 9,
	.timer_dqs_stop = 3,

	.timer_self_refresh1_in = 14,
	.timer_self_refresh1_exit = 14,
	.timer_global_rst = 1334,

	.timer_array_read_busy = 9,
	.timer_array_write_busy = 5,
	.timer_reg_read_busy = 12,
	.timer_reg_write_busy = 5,

	.wl_cen_ana = 1,
};

/* cfg_30..44 = 0f041020 05000101 0208010a 03e90b0b 040b0408 */
static const struct uhs_phy_cfg uhs_phy_800mhz = {
	.wl_dq_dig = 0,
	.wl_dq_ana = 2,
	.wl_dig = 0,
	.wl_ana = 1,
	.rl_dig = 4,
	.rl_ana = 0,
	//.oe_timer = 15,

	.timer_dqs_start = 1,
	.timer_dqs_array_stop = 1,
	.timer_array_write = 0,
	.timer_array_read = 5,

	.timer_auto_refresh = 10,
	.timer_reg_read = 1,
	.timer_reg_write = 8,
	.timer_dqs_stop = 2,

	.timer_self_refresh1_in = 11,
	.timer_self_refresh1_exit = 11,
	.timer_global_rst = 1001,

	.timer_array_read_busy = 8,
	.timer_array_write_busy = 4,
	.timer_reg_read_busy = 11,
	.timer_reg_write_busy = 4,

	.wl_cen_ana = 0,
};

/* cfg_30..44 = 0f130010 05000101 02080108 03420909 040b0408 */
static const struct uhs_phy_cfg uhs_phy_666mhz = {
	.wl_dq_dig = 0,
	.wl_dq_ana = 1,
	.wl_dig = 0,
	.wl_ana = 0,
	.rl_dig = 3,
	.rl_ana = 1,
	//.oe_timer = 15,

	.timer_dqs_start = 1,
	.timer_dqs_array_stop = 1,
	.timer_array_write = 0,
	.timer_array_read = 5,

	.timer_auto_refresh = 8,
	.timer_reg_read = 1,
	.timer_reg_write = 8,
	.timer_dqs_stop = 2,

	.timer_self_refresh1_in = 9,
	.timer_self_refresh1_exit = 9,
	.timer_global_rst = 834,

	.timer_array_read_busy = 8,
	.timer_array_write_busy = 4,
	.timer_reg_read_busy = 11,
	.timer_reg_write_busy = 4,

	.wl_cen_ana = 0,
};

/* cfg_30..44 = 0f020010 04000101 02070106 01f50606 040a0407 */
static const struct uhs_phy_cfg uhs_phy_400mhz = {
	.wl_dq_dig = 0,
	.wl_dq_ana = 1,
	.wl_dig = 0,
	.wl_ana = 0,
	.rl_dig = 2,
	.rl_ana = 1,
	//.oe_timer = 15,

	.timer_dqs_start = 1,
	.timer_dqs_array_stop = 1,
	.timer_array_write = 0,
	.timer_array_read = 4,

	.timer_auto_refresh = 6,
	.timer_reg_read = 1,
	.timer_reg_write = 7,
	.timer_dqs_stop = 2,

	.timer_self_refresh1_in = 6,
	.timer_self_refresh1_exit = 6,
	.timer_global_rst = 500,

	.timer_array_read_busy = 7,
	.timer_array_write_busy = 4,
	.timer_reg_read_busy = 10,
	.timer_reg_write_busy = 4,

	.wl_cen_ana = 0,
};

static void config_uhs_phy(struct bflb_psram_info *priv)
{
	struct bl808_psram_regs *regs = priv->regs;
	const struct uhs_phy_cfg *cfg;

	if (priv->pck_freq > 1866)
		cfg = &uhs_phy_2133mhz;
	else if (priv->pck_freq > 1600)
		cfg = &uhs_phy_1866mhz;
	else if (priv->pck_freq > 1066)
		cfg = &uhs_phy_1600mhz;
	else if (priv->pck_freq > 800)
		cfg = &uhs_phy_1066mhz;
	else if (priv->pck_freq > 666)
		cfg = &uhs_phy_800mhz;
	else if (priv->pck_freq > 400)
		cfg = &uhs_phy_666mhz;
	else
		cfg = &uhs_phy_400mhz;

	writel(PHY_CFG_30_WL_DQ_DIG(cfg->wl_dq_dig) |
	       PHY_CFG_30_WL_DQ_ANA(cfg->wl_dq_ana) |
	       PHY_CFG_30_WL_DIG(cfg->wl_dig) |
	       PHY_CFG_30_WL_ANA(cfg->wl_ana) |
	       PHY_CFG_30_RL_DIG(cfg->rl_dig) |
	       PHY_CFG_30_RL_ANA(cfg->rl_ana) |
	       //PHY_CFG_30_OE_TIMER(cfg->oe_timer),
	       PHY_CFG_30_OE_TIMER(3) |
	       PHY_CFG_30_VREF_MODE |
	       PHY_CFG_30_OE_CTRL_HW,
	       &regs->phy_cfg_30);

	writel(PHY_CFG_34_TIMER_DQS_START(cfg->timer_dqs_start) |
	       PHY_CFG_34_TIMER_DQS_ARRAY_STOP(cfg->timer_dqs_array_stop) |
	       PHY_CFG_34_TIMER_ARRAY_WRITE(cfg->timer_array_write) |
	       PHY_CFG_34_TIMER_ARRAY_READ(cfg->timer_array_read),
	       &regs->phy_cfg_34);

	writel(PHY_CFG_38_TIMER_AUTO_REFRESH(cfg->timer_auto_refresh) |
	       PHY_CFG_38_TIMER_REG_WRITE(cfg->timer_reg_write) |
	       PHY_CFG_38_TIMER_REG_READ(cfg->timer_reg_read) |
	       PHY_CFG_38_TIMER_DQS_STOP(cfg->timer_dqs_stop),
	       &regs->phy_cfg_38);

	writel(PHY_CFG_3C_TIMER_SELF_REFRESH1_IN(cfg->timer_self_refresh1_in) |
	       PHY_CFG_3C_TIMER_SELF_REFRESH1_EXIT(cfg->timer_self_refresh1_exit) |
	       PHY_CFG_3C_TIMER_GLOBAL_RST(cfg->timer_global_rst),
	       &regs->phy_cfg_3c);

	writel(PHY_CFG_44_TIMER_ARRAY_READ_BUSY(cfg->timer_array_read_busy) |
	       PHY_CFG_44_TIMER_ARRAY_WRITE_BUSY(cfg->timer_array_write_busy) |
	       PHY_CFG_44_TIMER_REG_READ_BUSY(cfg->timer_reg_read_busy) |
	       PHY_CFG_44_TIMER_REG_WRITE_BUSY(cfg->timer_reg_write_busy),
	       &regs->phy_cfg_44);

	clrsetbits_le32(&regs->phy_cfg_50,
			PHY_CFG_50_WL_CEN_ANA(7),
			PHY_CFG_50_WL_CEN_ANA(cfg->wl_cen_ana));
}

static void glb_config_uhs_pll(void)
{
	u32 *glb_uhs_pll_cfg0 = (void*)0x200007d0;
#define UHS_PLL_CFG0_SDM_RSTB BIT(0)
#define UHS_PLL_CFG0_FBDV_RSTB BIT(2)
#define UHS_PLL_CFG0_PU_UHSPLL_SFREG BIT(9)
#define UHS_PLL_CFG0_PU_UHSPLL BIT(10)
	u32 *glb_uhs_pll_cfg1 = (void*)0x200007d4;
#define UHS_PLL_CFG1_EVEN_DIV_RATIO(x) (((x) & 0x7f) << 0)
#define UHS_PLL_CFG1_EVEN_DIV_EN BIT(7)
#define UHS_PLL_CFG1_REFDIV_RATIO(x) (((x) & 0xf) << 8)
#define UHS_PLL_CFG1_REFCLK_SEL(x) (((x) & 0x3) << 16)
	u32 *glb_uhs_pll_cfg4 = (void*)0x200007e0;
#define UHS_PLL_CFG4_SEL_SAMPLE_CLK(x) (((x) & 0x3) << 0)
	u32 *glb_uhs_pll_cfg5 = (void*)0x200007e4;
#define UHS_PLL_CFG5_VCO_SPEED(x) (((x) & 0x7) << 0)
	u32 *glb_uhs_pll_cfg6 = (void*)0x200007e8;
#define UHS_PLL_CFG6_SDMIN(x) (((x) & 0x7ffff) << 0)
	int freq = 1400;
	int xtal_type = 4;

	clrsetbits_le32(glb_uhs_pll_cfg1,
			UHS_PLL_CFG1_REFCLK_SEL(3),
			UHS_PLL_CFG1_REFCLK_SEL(0));

	clrsetbits_le32(glb_uhs_pll_cfg1,
			UHS_PLL_CFG1_REFDIV_RATIO(15),
			UHS_PLL_CFG1_REFDIV_RATIO(2));

	clrsetbits_le32(glb_uhs_pll_cfg4,
			UHS_PLL_CFG4_SEL_SAMPLE_CLK(3),
			UHS_PLL_CFG4_SEL_SAMPLE_CLK(2));

	clrsetbits_le32(glb_uhs_pll_cfg5,
			UHS_PLL_CFG5_VCO_SPEED(7),
			UHS_PLL_CFG5_VCO_SPEED(4));

	clrsetbits_le32(glb_uhs_pll_cfg1,
			UHS_PLL_CFG1_EVEN_DIV_EN |
			UHS_PLL_CFG1_EVEN_DIV_RATIO(0x7f),
			UHS_PLL_CFG1_EVEN_DIV_EN |
			UHS_PLL_CFG1_EVEN_DIV_RATIO(28));

	clrsetbits_le32(glb_uhs_pll_cfg6,
			UHS_PLL_CFG6_SDMIN(0x7ffff),
			UHS_PLL_CFG6_SDMIN(143360));

	setbits_le32(glb_uhs_pll_cfg0, UHS_PLL_CFG0_PU_UHSPLL_SFREG);
	udelay(3);

	setbits_le32(glb_uhs_pll_cfg0, UHS_PLL_CFG0_PU_UHSPLL);
	udelay(3);

	setbits_le32(glb_uhs_pll_cfg0, UHS_PLL_CFG0_SDM_RSTB);
	udelay(2);
	clrbits_le32(glb_uhs_pll_cfg0, UHS_PLL_CFG0_SDM_RSTB);
	udelay(2);
	setbits_le32(glb_uhs_pll_cfg0, UHS_PLL_CFG0_SDM_RSTB);

	setbits_le32(glb_uhs_pll_cfg0, UHS_PLL_CFG0_FBDV_RSTB);
	udelay(2);
	clrbits_le32(glb_uhs_pll_cfg0, UHS_PLL_CFG0_FBDV_RSTB);
	udelay(2);
	setbits_le32(glb_uhs_pll_cfg0, UHS_PLL_CFG0_FBDV_RSTB);

	udelay(45);
}

void glb_power_up_ldo12uhs(void)
{
	u32 *glb_ldo12uhs = (void*)0x200006d0;
#define LDO12UHS_PU_LDO12UHS BIT(0)
#define LDO12UHS_VOUT_SEL(x) (((x) & 0xf) << 20)

	setbits_le32(glb_ldo12uhs, LDO12UHS_PU_LDO12UHS);
	udelay(300);
	clrsetbits_le32(glb_ldo12uhs, LDO12UHS_VOUT_SEL(15), LDO12UHS_VOUT_SEL(6));
	udelay(1);

}

static void psram_analog_init(struct bflb_psram_info *priv)
{
	struct bl808_psram_regs *regs = priv->regs;
	int i;

	glb_power_up_ldo12uhs();

	/* disable CEn, CK, CKn */
	clrbits_le32(&regs->phy_cfg_50,
		     PHY_CFG_50_DQ_OE_MID_P(3) | PHY_CFG_50_DQ_OE_MID_N(3));
	udelay(1);
	clrsetbits_le32(&regs->phy_cfg_40,
			PHY_CFG_40_DMY0(255) | PHY_CFG_40_UNK0,
			PHY_CFG_40_DMY0(1));
	udelay(1);

	/* configure pads */
	writel(PHY_CFG_00_CK_SR(2) | PHY_CFG_00_CK_DLY_DRV(11) |
	       PHY_CFG_00_CEN_SR(2) | PHY_CFG_00_CEN_DLY_DRV(8),
	       &regs->phy_cfg_00);
	writel(PHY_CFG_04_DM1_SR(2) | PHY_CFG_04_DM1_DLY_DRV(6) |
	       PHY_CFG_04_DM0_SR(2) | PHY_CFG_04_DM0_DLY_DRV(6),
	       &regs->phy_cfg_04);

	/* DQ[0:15] */
	for (i = 0; i < 8; i++) {
		writel(PHY_CFG_DQn_SR(2) | PHY_CFG_DQn_DLY_RX(0) |
		       PHY_CFG_DQn_DLY_DRV(7) | PHY_CFG_DQm_SR(2) |
		       PHY_CFG_DQm_DLY_RX(0) | PHY_CFG_DQm_DLY_DRV(7),
		       &regs->phy_cfg_08 + i);
	}

	writel(PHY_CFG_28_DQS0N_DLY_RX(0) | PHY_CFG_28_DQS0_SR(0) |
	       PHY_CFG_28_DQS0_SEL(0) | PHY_CFG_28_DQS0_DLY_RX(0) |
	       PHY_CFG_28_DQS0_DLY_DRV(6) | PHY_CFG_28_DQS0_DIFF_DLY_RX(2) ,
	       &regs->phy_cfg_28);
	writel(PHY_CFG_2C_EN_RX_FE | PHY_CFG_2C_EN_BIAS |
	       PHY_CFG_2C_DQS1N_DLY_RX(0) | PHY_CFG_2C_DQS1_SR(0) |
	       PHY_CFG_2C_DQS1_SEL(0) | PHY_CFG_2C_DQS1_DLY_RX(0) |
	       PHY_CFG_2C_DQS1_DLY_DRV(6) | PHY_CFG_2C_DQS1_DIFF_DLY_RX(2) ,
	       &regs->phy_cfg_2c);

	clrsetbits_le32(&regs->phy_cfg_30,
			PHY_CFG_30_OE_TIMER(3) | PHY_CFG_30_VREF_MODE |
			PHY_CFG_30_ODT_SEL(15),
			PHY_CFG_30_OE_TIMER(3) | PHY_CFG_30_VREF_MODE |
			PHY_CFG_30_ODT_SEL(0));
	clrsetbits_le32(&regs->phy_cfg_48,
			PHY_CFG_48_PSRAM_TYPE(3),
			PHY_CFG_48_PSRAM_TYPE(2));
	clrsetbits_le32(&regs->phy_cfg_4c,
			PHY_CFG_4C_ODT_SEL_DLY(15) | PHY_CFG_4C_ODT_SEL_HW,
			PHY_CFG_4C_ODT_SEL_DLY(0));
	clrsetbits_le32(&regs->phy_cfg_50,
			PHY_CFG_50_DQ_OE_UP_P(7) | PHY_CFG_50_DQ_OE_UP_N(7) |
			PHY_CFG_50_DQ_OE_DN_P(7) | PHY_CFG_50_DQ_OE_DN_N(7),
			PHY_CFG_50_DQ_OE_UP_P(3) | PHY_CFG_50_DQ_OE_UP_N(3) |
			PHY_CFG_50_DQ_OE_DN_P(3) | PHY_CFG_50_DQ_OE_DN_N(3));
	udelay(1);

	/* switch to LDO 1V2 */
	clrbits_le32(&regs->phy_cfg_40, PHY_CFG_40_UNK1);
	udelay(1);

	/* reenable CEn, CK, CKn */
	clrsetbits_le32(&regs->phy_cfg_40,
			PHY_CFG_40_DMY0(255) | PHY_CFG_40_UNK0,
			PHY_CFG_40_UNK0);
	udelay(1);
	setbits_le32(&regs->phy_cfg_50,
		     PHY_CFG_50_DQ_OE_MID_P(3) | PHY_CFG_50_DQ_OE_MID_N(3));
	udelay(1);
}

static void psram_uhs_init(struct bflb_psram_info *priv)
{
	struct bl808_psram_regs *regs = priv->regs;
	int pck_freq = priv->pck_freq;
	int pck_t_div;
	u32 timing;

	if (pck_freq > 2300)
		panic("wrong pck_freq");
	else if (pck_freq > 1600)
		timing = TIMING_CTRL_TRC_CYCLE(15) | TIMING_CTRL_TCPHR_CYCLE(0) |
			 TIMING_CTRL_TCPHW_CYCLE(3) | TIMING_CTRL_TRFC_CYCLE(26);
	else
		timing = TIMING_CTRL_TRC_CYCLE(11) | TIMING_CTRL_TCPHR_CYCLE(0) |
			 TIMING_CTRL_TCPHW_CYCLE(2) | TIMING_CTRL_TRFC_CYCLE(18);

	writel(timing, &regs->timing_ctrl);

	psram_analog_init(priv);
	config_uhs_phy(priv);
	udelay(150);

	/* set refresh parameter */
	if (pck_freq >= 2200)
		pck_t_div = 5;
	else if (pck_freq >= 1800)
		pck_t_div = 4;
	else if (pck_freq >= 1500)
		pck_t_div = 3;
	else if (pck_freq >= 1400)
		pck_t_div = 2;
	else if (pck_freq >= 666)
		pck_t_div = 1;
	else
		pck_t_div = 0;
	clrsetbits_le32(&regs->manual,
			MANUAL_PCK_T_DIV(255),
			MANUAL_PCK_T_DIV(pck_t_div));

	/* set refresh windows cycle count */
	if (!priv->ext_temp_range) {
		writel(750000, &regs->auto_fresh_1); /* 32ms */
		clrsetbits_le32(&regs->auto_fresh_2,
				AUTO_FRESH_2_REFI_CYCLE(65535),
				AUTO_FRESH_2_REFI_CYCLE(370));
	} else {
		writel(1500000, &regs->auto_fresh_1); /* 16ms */
		clrsetbits_le32(&regs->auto_fresh_2,
				AUTO_FRESH_2_REFI_CYCLE(65535),
				AUTO_FRESH_2_REFI_CYCLE(190));
	}
	clrsetbits_le32(&regs->auto_fresh_4,
			AUTO_FRESH_4_BUST_CYCLE(127),
			AUTO_FRESH_4_BUST_CYCLE(5));

	clrsetbits_le32(&regs->basic,
			BASIC_ADDRMB_MSK(255) |
			BASIC_LINEAR_BND_B(15) |
			BASIC_AF_EN,
			BASIC_ADDRMB_MSK(priv->mem_size) |
			BASIC_LINEAR_BND_B(priv->page_size) |
			BASIC_AF_EN);

	setbits_le32(&regs->basic, BASIC_INIT_EN);
}

static void psram_dump_regs(void)
{
	void *base = (void*)0x3000f000;
	int i;
	for (i = 0; i <= 0x150; i += 4)
		printf("0x%p,0x%x\n", base + i, readl(base + i));
}

static ulong mem_test_alt(vu_long *buf, ulong start_addr, ulong end_addr,
			  vu_long *dummy)
{
	vu_long *addr;
	ulong errs = 0;
	ulong val, readback;
	int j;
	vu_long offset;
	vu_long test_offset;
	vu_long pattern;
	vu_long temp;
	vu_long anti_pattern;
	vu_long num_words;
	static const ulong bitpattern[] = {
		0x00000001,	/* single bit */
		0x00000003,	/* two adjacent bits */
		0x00000007,	/* three adjacent bits */
		0x0000000F,	/* four adjacent bits */
		0x00000005,	/* two non-adjacent bits */
		0x00000015,	/* three non-adjacent bits */
		0x00000055,	/* four non-adjacent bits */
		0xaaaaaaaa,	/* alternating 1/0 */
	};

	num_words = (end_addr - start_addr) / sizeof(vu_long);

	/*
	 * Data line test: write a pattern to the first
	 * location, write the 1's complement to a 'parking'
	 * address (changes the state of the data bus so a
	 * floating bus doesn't give a false OK), and then
	 * read the value back. Note that we read it back
	 * into a variable because the next time we read it,
	 * it might be right (been there, tough to explain to
	 * the quality guys why it prints a failure when the
	 * "is" and "should be" are obviously the same in the
	 * error message).
	 *
	 * Rather than exhaustively testing, we test some
	 * patterns by shifting '1' bits through a field of
	 * '0's and '0' bits through a field of '1's (i.e.
	 * pattern and ~pattern).
	 */
	addr = buf;
	for (j = 0; j < sizeof(bitpattern) / sizeof(bitpattern[0]); j++) {
		val = bitpattern[j];
		for (; val != 0; val <<= 1) {
			*addr = val;
			*dummy  = ~val; /* clear the test data off the bus */
			readback = *addr;
			if (readback != val) {
				printf("FAILURE (data line): "
					"expected %08lx, actual %08lx\n",
						val, readback);
				errs++;
				if (ctrlc())
					return -1;
			}
			*addr  = ~val;
			*dummy  = val;
			readback = *addr;
			if (readback != ~val) {
				printf("FAILURE (data line): "
					"Is %08lx, should be %08lx\n",
						readback, ~val);
				errs++;
				if (ctrlc())
					return -1;
			}
		}
	}

	/*
	 * Based on code whose Original Author and Copyright
	 * information follows: Copyright (c) 1998 by Michael
	 * Barr. This software is placed into the public
	 * domain and may be used for any purpose. However,
	 * this notice must not be changed or removed and no
	 * warranty is either expressed or implied by its
	 * publication or distribution.
	 */

	/*
	* Address line test

	 * Description: Test the address bus wiring in a
	 *              memory region by performing a walking
	 *              1's test on the relevant bits of the
	 *              address and checking for aliasing.
	 *              This test will find single-bit
	 *              address failures such as stuck-high,
	 *              stuck-low, and shorted pins. The base
	 *              address and size of the region are
	 *              selected by the caller.

	 * Notes:	For best results, the selected base
	 *              address should have enough LSB 0's to
	 *              guarantee single address bit changes.
	 *              For example, to test a 64-Kbyte
	 *              region, select a base address on a
	 *              64-Kbyte boundary. Also, select the
	 *              region size as a power-of-two if at
	 *              all possible.
	 *
	 * Returns:     0 if the test succeeds, 1 if the test fails.
	 */
	pattern = (vu_long) 0xaaaaaaaa;
	anti_pattern = (vu_long) 0x55555555;

	printf("%s:%d: length = 0x%.8lx\n", __func__, __LINE__, num_words);
	/*
	 * Write the default pattern at each of the
	 * power-of-two offsets.
	 */
	for (offset = 1; offset < num_words; offset <<= 1)
		addr[offset] = pattern;

	/*
	 * Check for address bits stuck high.
	 */
	test_offset = 0;
	addr[test_offset] = anti_pattern;

	for (offset = 1; offset < num_words; offset <<= 1) {
		temp = addr[offset];
		if (temp != pattern) {
			printf("\nFAILURE: Address bit stuck high @ 0x%.8lx:"
				" expected 0x%.8lx, actual 0x%.8lx\n",
				start_addr + offset*sizeof(vu_long),
				pattern, temp);
			errs++;
			if (ctrlc())
				return -1;
		}
	}
	addr[test_offset] = pattern;
	schedule();

	/*
	 * Check for addr bits stuck low or shorted.
	 */
	for (test_offset = 1; test_offset < num_words; test_offset <<= 1) {
		addr[test_offset] = anti_pattern;

		for (offset = 1; offset < num_words; offset <<= 1) {
			temp = addr[offset];
			if ((temp != pattern) && (offset != test_offset)) {
				printf("\nFAILURE: Address bit stuck low or"
					" shorted @ 0x%.8lx: expected 0x%.8lx,"
					" actual 0x%.8lx\n",
					start_addr + offset*sizeof(vu_long),
					pattern, temp);
				errs++;
				if (ctrlc())
					return -1;
			}
		}
		addr[test_offset] = pattern;
	}

	/*
	 * Description: Test the integrity of a physical
	 *		memory device by performing an
	 *		increment/decrement test over the
	 *		entire region. In the process every
	 *		storage bit in the device is tested
	 *		as a zero and a one. The base address
	 *		and the size of the region are
	 *		selected by the caller.
	 *
	 * Returns:     0 if the test succeeds, 1 if the test fails.
	 */
	num_words++;

	/*
	 * Fill memory with a known pattern.
	 */
	for (pattern = 1, offset = 0; offset < num_words; pattern++, offset++) {
		schedule();
		addr[offset] = pattern;
	}

	/*
	 * Check each location and invert it for the second pass.
	 */
	for (pattern = 1, offset = 0; offset < num_words; pattern++, offset++) {
		schedule();
		temp = addr[offset];
		if (temp != pattern) {
			printf("\nFAILURE (read/write) @ 0x%.8lx:"
				" expected 0x%.8lx, actual 0x%.8lx)\n",
				start_addr + offset*sizeof(vu_long),
				pattern, temp);
			errs++;
			if (ctrlc())
				return -1;
		}

		anti_pattern = ~pattern;
		addr[offset] = anti_pattern;
	}

	/*
	 * Check each location for the inverted pattern and zero it.
	 */
	for (pattern = 1, offset = 0; offset < num_words; pattern++, offset++) {
		schedule();
		anti_pattern = ~pattern;
		temp = addr[offset];
		if (temp != anti_pattern) {
			printf("\nFAILURE (read/write): @ 0x%.8lx:"
				" expected 0x%.8lx, actual 0x%.8lx)\n",
				start_addr + offset*sizeof(vu_long),
				anti_pattern, temp);
			errs++;
			if (ctrlc())
				return -1;
		}
		addr[offset] = 0;
	}

	return errs;
}

static int bflb_psram_probe(struct udevice *dev)
{
	struct bflb_psram_info *priv = dev_get_priv(dev);
printf("%s\n", __func__);

	/* Read memory base and size from DT */
	fdtdec_setup_mem_size_base();
	priv->info.base = gd->ram_base;
	priv->info.size = gd->ram_size;

	priv->regs = dev_read_addr_ptr(dev);
	if (!priv->regs)
		return -EINVAL;

	priv->pck_freq = 1400;
	priv->mem_size = 0x3f;
	priv->page_size = 0x0b;

	glb_config_uhs_pll();
	psram_uhs_init(priv);

	//psram_dump_regs();

	//vu_long dummy;
	//mem_test_alt((void*)0x50000000, 0x50000000, 0x50400000, &dummy);

	return 0;
}

static int bflb_psram_get_info(struct udevice *dev, struct ram_info *info)
{
	struct bflb_psram_info *priv = dev_get_priv(dev);

	*info = priv->info;

	return 0;
}

static struct ram_ops bflb_psram_ops = {
	.get_info = bflb_psram_get_info,
};

static const struct udevice_id bflb_psram_ids[] = {
	{ .compatible = "bouffalolab,bl808-psram-uhs" },
	{ }
};

U_BOOT_DRIVER(bflb_psram) = {
	.name = "bflb_psram",
	.id = UCLASS_RAM,
	.of_match = bflb_psram_ids,
	.ops = &bflb_psram_ops,
	.probe = bflb_psram_probe,
	.priv_auto = sizeof(struct bflb_psram_info),
};
