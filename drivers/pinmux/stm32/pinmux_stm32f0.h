/*
 * Copyright (c) 2017 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32F0_PINMUX_H_
#define _STM32F0_PINMUX_H_

/**
 * @file Header for STM32F0 pin multiplexing helper
 */

#define STM32F0_PINMUX_FUNC_PB6_USART1_TX \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUSHPULL_NOPULL)
#define STM32F0_PINMUX_FUNC_PB7_USART1_RX \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUPDR_NO_PULL)

#define STM32F0_PINMUX_FUNC_PA9_USART1_TX \
	(STM32_PINMUX_ALT_FUNC_1 | STM32_PUSHPULL_NOPULL)
#define STM32F0_PINMUX_FUNC_PA10_USART1_RX \
	(STM32_PINMUX_ALT_FUNC_1 | STM32_PUPDR_NO_PULL)

#define STM32F0_PINMUX_FUNC_PA2_USART2_TX \
	(STM32_PINMUX_ALT_FUNC_1 | STM32_PUSHPULL_NOPULL)
#define STM32F0_PINMUX_FUNC_PA3_USART2_RX \
	(STM32_PINMUX_ALT_FUNC_1 | STM32_PUPDR_NO_PULL)

#define STM32F0_PINMUX_FUNC_PA14_USART2_TX \
	(STM32_PINMUX_ALT_FUNC_1 | STM32_PUSHPULL_NOPULL)
#define STM32F0_PINMUX_FUNC_PA15_USART2_RX \
	(STM32_PINMUX_ALT_FUNC_1 | STM32_PUPDR_NO_PULL)

#define STM32F0_PINMUX_FUNC_PD5_USART2_TX \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUSHPULL_NOPULL)
#define STM32F0_PINMUX_FUNC_PD6_USART2_RX \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUPDR_NO_PULL)

#define STM32F0_PINMUX_FUNC_PB8_I2C1_SCL \
	(STM32_PINMUX_ALT_FUNC_1 | STM32_OPENDRAIN_PULLUP)
#define STM32F0_PINMUX_FUNC_PB9_I2C1_SDA \
	(STM32_PINMUX_ALT_FUNC_1 | STM32_OPENDRAIN_PULLUP)

#define STM32F0_PINMUX_FUNC_PA11_I2C2_SCL \
	(STM32_PINMUX_ALT_FUNC_5 | STM32_OPENDRAIN_PULLUP)
#define STM32F0_PINMUX_FUNC_PA12_I2C2_SDA \
	(STM32_PINMUX_ALT_FUNC_5 | STM32_OPENDRAIN_PULLUP)

#define STM32F0_PINMUX_FUNC_PB10_I2C2_SCL \
	(STM32_PINMUX_ALT_FUNC_1 | STM32_OPENDRAIN_PULLUP)
#define STM32F0_PINMUX_FUNC_PB11_I2C2_SDA \
	(STM32_PINMUX_ALT_FUNC_1 | STM32_OPENDRAIN_PULLUP)

#define STM32F0_PINMUX_FUNC_PA4_SPI1_NSS \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUSHPULL_NOPULL)
#define STM32F0_PINMUX_FUNC_PA5_SPI1_SCK \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUSHPULL_NOPULL)
#define STM32F0_PINMUX_FUNC_PA6_SPI1_MISO \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUSHPULL_NOPULL)
#define STM32F0_PINMUX_FUNC_PA7_SPI1_MOSI \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUSHPULL_NOPULL)

#define STM32F0_PINMUX_FUNC_PA15_SPI1_NSS \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUSHPULL_NOPULL)
#define STM32F0_PINMUX_FUNC_PB3_SPI1_SCK \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUSHPULL_NOPULL)
#define STM32F0_PINMUX_FUNC_PB4_SPI1_MISO \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUSHPULL_NOPULL)
#define STM32F0_PINMUX_FUNC_PB5_SPI1_MOSI \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUSHPULL_NOPULL)

#define STM32F0_PINMUX_FUNC_PB12_SPI2_NSS \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUSHPULL_NOPULL)
#define STM32F0_PINMUX_FUNC_PB13_SPI2_SCK \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUSHPULL_NOPULL)
#define STM32F0_PINMUX_FUNC_PB14_SPI2_MISO \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUSHPULL_NOPULL)
#define STM32F0_PINMUX_FUNC_PB15_SPI2_MOSI \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUSHPULL_NOPULL)

/* Available on STM32F030xC devices only. */
#define STM32F0_PINMUX_FUNC_PB9_SPI2_NSS \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUSHPULL_NOPULL)
#define STM32F0_PINMUX_FUNC_PB10_SPI2_SCK \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUSHPULL_NOPULL)
#define STM32F0_PINMUX_FUNC_PC2_SPI2_MISO \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUSHPULL_NOPULL)
#define STM32F0_PINMUX_FUNC_PC3_SPI2_MOSI \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUSHPULL_NOPULL)

#define STM32F0_PINMUX_FUNC_PB8_CAN_RX \
	(STM32_PINMUX_ALT_FUNC_4 | STM32_PUSHPULL_NOPULL)
#define STM32F0_PINMUX_FUNC_PB9_CAN_TX \
	(STM32_PINMUX_ALT_FUNC_4 | STM32_PUSHPULL_NOPULL)
#define STM32F0_PINMUX_FUNC_PD0_CAN_RX \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUPDR_PULL_UP)
#define STM32F0_PINMUX_FUNC_PD1_CAN_TX \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUSHPULL_NOPULL)

#endif /* _STM32F0_PINMUX_H_ */