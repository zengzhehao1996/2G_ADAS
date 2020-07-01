/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32L1X_FLASH_REGISTERS_H_
#define _STM32L1X_FLASH_REGISTERS_H_

/**
 * @brief
 *
 * Based on reference manual:
 *
 * Chapter 3.4: Embedded Flash Memory
 */

union __flash_acr {
	u32_t val;
	struct {
		u32_t latency :1 __packed;
		u32_t prften :1 __packed;
		u32_t acc64 :1 __packed;
		u32_t sleeppd :1 __packed;
		u32_t runpd :1 __packed;
		u32_t rsvd__5_31 :27 __packed;
	} bit;
};

/* 3.8.7 Embedded flash registers */
struct stm32l1x_flash {
	volatile union __flash_acr acr;
	volatile u32_t pecr;
	volatile u32_t pdkeyr;
	volatile u32_t pekeyr;
	volatile u32_t prgkeyr;
	volatile u32_t optkeyr;
    volatile u32_t sr;
    volatile u32_t obr;
    volatile u32_t wrpr;
};

#endif	/* _STM32L1X_FLASHREGISTERS_H_ */
