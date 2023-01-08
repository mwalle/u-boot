/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 Michael Walle <michael@walle.cc>
 */
#ifndef PINCTRL_BL808_H
#define PINCTRL_BL808_H

#define BL808_FUNC_SDH				0
#define BL808_FUNC_SPI0				1
#define BL808_FUNC_SFLASH			2
#define BL808_FUNC_I2S				3
#define BL808_FUNC_PDM				4
#define BL808_FUNC_I2C0				5
#define BL808_FUNC_I2C1				6
#define BL808_FUNC_UART0_RTS			(7 |  (0 << 8))
#define BL808_FUNC_UART0_CTS			(7 |  (1 << 8))
#define BL808_FUNC_UART0_TX			(7 |  (2 << 8))
#define BL808_FUNC_UART0_RX			(7 |  (3 << 8))
#define BL808_FUNC_UART1_RTS			(7 |  (4 << 8))
#define BL808_FUNC_UART1_CTS			(7 |  (5 << 8))
#define BL808_FUNC_UART1_TX			(7 |  (6 << 8))
#define BL808_FUNC_UART1_RX			(7 |  (7 << 8))
#define BL808_FUNC_UART2_RTS			(7 |  (8 << 8))
#define BL808_FUNC_UART2_CTS			(7 |  (9 << 8))
#define BL808_FUNC_UART2_TX			(7 | (10 << 8))
#define BL808_FUNC_UART2_RX			(7 | (11 << 8))
#define BL808_FUNC_RMII				8
#define BL808_FUNC_CAM				9
#define BL808_FUNC_ADC				10
#define BL808_FUNC_GPIO				11
#define BL808_FUNC_PWM0				16
#define BL808_FUNC_PWM1				17
#define BL808_FUNC_SPI1				18
#define BL808_FUNC_I2C2				19
#define BL808_FUNC_I2C3				20
#define BL808_FUNC_UART3			21
#define BL808_FUNC_DBI_B			22
#define BL808_FUNC_DBI_C			23
#define BL808_FUNC_DPI				24
#define BL808_FUNC_JTAG_LP			25
#define BL808_FUNC_JTAG_M0			26
#define BL808_FUNC_JTAG_D0			27
#define BL808_FUNC_CLK_OUT_CAM_REF_CLK		(31 | (0 << 8))
#define BL808_FUNC_CLK_OUT_I2S_REF_CLK		(31 | (1 << 8))
#define BL808_FUNC_CLK_OUT0_AUDIO_ADC_CLK	(31 | (2 << 8))
#define BL808_FUNC_CLK_OUT0_AUDIO_DAC_CLK	(31 | (3 << 8))
#define BL808_FUNC_CLK_OUT1_AUDIO_ADC_CLK	(31 | (2 << 8))
#define BL808_FUNC_CLK_OUT1_AUDIO_DAC_CLK	(31 | (3 << 8))
#define BL808_FUNC_CLK_OUT2_ANA_XTAL_CLK	(31 | (2 << 8))
#define BL808_FUNC_CLK_OUT2_PLL_32M_CLK		(31 | (3 << 8))
#define BL808_FUNC_CLK_OUT3_NONE		(31 | (2 << 8))
#define BL808_FUNC_CLK_OUT3_PLL_48M_CLK		(31 | (3 << 8))

#define BL808_PINMUX(pin, func)			(((pin) << 16) | (func))

#endif /* PINCTRL_BL808_H */
