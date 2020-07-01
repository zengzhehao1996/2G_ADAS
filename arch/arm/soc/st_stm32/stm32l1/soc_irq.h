/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 BayLibre, SAS
 * Copyright (c) 2018 Rainbonic Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32L1_SOC_IRQ_H_
#define _STM32L1_SOC_IRQ_H_

/*
 * We cannot use the enum present in the ST headers for the IRQs because
 * of the IRQ_CONNECT macro. The macro exepects a number or a symbol that can
 * be processed by the preprocessor.
 */
/* 
 * Cat.1 devices:
 *	STM31L100C6, STM32L100R8, STM32L100RB
 *	STM31L15xx6, STM32L15xx8, STM32L15xxB
 *
 * Cat.2 devices:
 *	STM31L100C6-A, STM32L100R8-A, STM32L100RB-A
 *	STM31L15xx6-A, STM32L15xx8-A, STM32L15xxB-A
 */
#define STM32L1_IRQ_WWDG		0
#define STM32L1_IRQ_PVD			1
#define STM32L1_IRQ_TAMPER		2
#define STM32L1_IRQ_RTC			3
#define STM32L1_IRQ_FLASH		4
#define STM32L1_IRQ_RCC			5
#define STM32L1_IRQ_EXTI0		6
#define STM32L1_IRQ_EXTI1		7
#define STM32L1_IRQ_EXTI2		8
#define STM32L1_IRQ_EXTI3		9
#define STM32L1_IRQ_EXTI4		10
#define STM32L1_IRQ_DMA1_CH1		11
#define STM32L1_IRQ_DMA1_CH2		12
#define STM32L1_IRQ_DMA1_CH3		13
#define STM32L1_IRQ_DMA1_CH4		14
#define STM32L1_IRQ_DMA1_CH5		15
#define STM32L1_IRQ_DMA1_CH6		16
#define STM32L1_IRQ_DMA1_CH7		17
#define STM32L1_IRQ_ADC1		18
#define STM32L1_IRQ_USB_HP		19
#define STM32L1_IRQ_USB_LP		20
#define STM32L1_IRQ_DAC			21
#define STM32L1_IRQ_COMP_TSC		22
#define STM32L1_IRQ_EXTI9_5		23
#define STM32L1_IRQ_LCD			24
#define STM32L1_IRQ_TIM9		25
#define STM32L1_IRQ_TIM10		26
#define STM32L1_IRQ_TIM11		27
#define STM32L1_IRQ_TIM2		28
#define STM32L1_IRQ_TIM3		29
#define STM32L1_IRQ_TIM4		30
#define STM32L1_IRQ_I2C1_EV		31
#define STM32L1_IRQ_I2C1_ER		32
#define STM32L1_IRQ_I2C2_EV		33
#define STM32L1_IRQ_I2C2_ER		34
#define STM32L1_IRQ_SPI1		35
#define STM32L1_IRQ_SPI2		36
#define STM32L1_IRQ_USART1		37
#define STM32L1_IRQ_USART2		38
#define STM32L1_IRQ_USART3		39
#define STM32L1_IRQ_EXTI15_10		40
#define STM32L1_IRQ_RTC_ALARM		41
#define STM32L1_IRQ_USB_FS_WKUP		42
#define STM32L1_IRQ_TIM6		43
#define STM32L1_IRQ_TIM7		44

#if 0
/* Cat.3 devices */
#define STM32L1_IRQ_TIM5		45
#define STM32L1_IRQ_SPI3		46
#define STM32L1_IRQ_DMA2_CH1		47
#define STM32L1_IRQ_DMA2_CH2		48
#define STM32L1_IRQ_DMA2_CH3		49
#define STM32L1_IRQ_DMA2_CH4		50
#define STM32L1_IRQ_DMA2_CH5		51
#define STM32L1_IRQ_AES			52
#define STM32L1_IRQ_RNG			53
#endif

#if 0
/* Cat.4, Cat.5 and Cat.6 devices */
#define STM32L1_IRQ_SDIO		45
#define STM32L1_IRQ_TIM5		46
#define STM32L1_IRQ_SPI3		47
#define STM32L1_IRQ_UART4		48
#define STM32L1_IRQ_UART5		49
#define STM32L1_IRQ_DMA2_CH1		50
#define STM32L1_IRQ_DMA2_CH2		51
#define STM32L1_IRQ_DMA2_CH3		52
#define STM32L1_IRQ_DMA2_CH4		53
#define STM32L1_IRQ_DMA2_CH5		54
#define STM32L1_IRQ_AES			55
#define STM32L1_IRQ_RNG			56
#endif

#endif	/* _STM32L1_SOC_IRQ_H_ */
