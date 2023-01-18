/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 Michael Walle <michael@walle.cc>
 */

#ifndef __DT_BINDINGS_BL808_CLOCK_H
#define __DT_BINDINGS_BL808_CLOCK_H

#define BL808_CLK_BCLK		0

#define BL808_CLK_CPU		1 /* GLB_MCU_SYS_CLK_Type */
#define BL808_CLK_HCLK		2
#define BL808_CLK_PBCLK		3

#define BL808_CLK_PSRAMB	4 /* GLB_PSRAM_PLL_Type */
#define BL808_CLK_EMI		5 /* GLB_EMI_CLK_Type */
#define BL808_CLK_MM_DP		6 /* GLB_DSP_DP_CLK_Type */
#define BL808_CLK_MM_CPU	7 /* GLB_DSP_SYS_CLK_Type */
#define BL808_CLK_MM_BCLK2X	8 /* also set by GLB_DSP_SYS_CLK_Type */
#define BL808_CLK_MM_BCLK1X	9 /* GLB_DSP_SYS_PBCLK_Type */
#define BL808_CLK_MM_H264	10 /* GLB_DSP_H264_CLK_Type */
#define BL808_CLK_MM_CNN	11 /* GLB_DSP_CNN_CLK_Type */

#define BL808_CLK_ETH_REF_CLK	12 /* GLB_ETH_REF_CLK_OUT_Type */
#define BL808_CLK_SFLASH	13 /* GLB_SFLASH_CLK_Type */
#define BL808_CLK_IR		14 /* GLB_IR_CLK_SRC_Type */
#define BL808_CLK_SDIO		15 /* GLB_SDH_CLK_Type */
#define BL808_CLK_UART		16 /* HBN_UART_CLK_Type */
#define BL808_CLK_I2C		17 /* GLB_I2C_CLK_Type */
#define BL808_CLK_SPI		18 /* GLB_SPI_CLK_Type */
#define BL808_CLK_I2S		19 /* GLB_I2S_OUT_REF_CLK_Type */
#define BL808_CLK_AUDIO_ADC	20 /* GLB_Set_Audio_ADC_CLK() */
#define BL808_CLK_AUDIO_DAC	21 /* GLB_Set_Audio_DAC_CLK() */
#define BL808_CLK_AUDIO_PDM	22 /* GLB_Set_Audio_PDM_CLK() */
#define BL808_CLK_ADC		23 /* GLB_ADC_CLK_Type */
#define BL808_CLK_DAC		24 /* GLB_DAC_CLK_Type */
#define BL808_CLK_CSI		25 /* GLB_CAM_CLK_Type */
#define BL808_CLK_CLKOUT0	26 /* GLB_CHIP_CLK_OUT_0_Type */
#define BL808_CLK_CLKOUT1	27 /* GLB_CHIP_CLK_OUT_1_Type */
#define BL808_CLK_CLKOUT2	28 /* GLB_CHIP_CLK_OUT_2_Type */
#define BL808_CLK_CLKOUT3	29 /* GLB_CHIP_CLK_OUT_3_Type */
#define BL808_CLK_MM_UART0	30 /* GLB_DSP_UART_CLK_Type */
#define BL808_CLK_MM_UART1	31 /* GLB_DSP_UART_CLK_Type */
#define BL808_CLK_MM_SPI	32 /* GLB_DSP_SPI_CLK_Type */
#define BL808_CLK_MM_I2C0	33 /* GLB_DSP_I2C_CLK_Type */
#define BL808_CLK_MM_I2C1	34 /* GLB_DSP_I2C_CLK_Type */

#define BL808_CLK_PKA		35 /* GLB_PKA_CLK_Type */

#if 0
#define BL808_CLK_PADC??
#define BL808_CLK_MM_NONAME??
#endif

/* GLB_CSI_DSI_CLK_SEL_Type */
/* GLB_DIG_CLK_Type */
/* GLB_512K_CLK_OUT_Type */
/* GLB_DSP_XCLK_Type */
/* GLB_DSP_ROOT_CLK_Type */ /* connected to CLK_CPU ? */
/* GLB_DSP_PBROOT_CLK_Type */
/* GLB_DSP_PLL_CLK_Type */
/* GLB_DISP_CLK_Type -> disp_pll */
/* GLB_DSP_CLK_Type */

#endif /* __DT_BINDINGS_BL808_CLOCK_H */
