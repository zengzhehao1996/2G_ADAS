/*
 * Copyright (c) 2018 Ilya Tagunov <tagunil@gmail.com>
 * Copyright (c) 2018 Rainbonic Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/*
 * GSM & GNSS/GPS module pins
 */
/* Output - GSM Enable switch */
#define GSM_PWR_GPIO_PORT	"GPIOA"
#define GSM_PWR_GPIO_PIN	1
/* Output - GSM Enable swith */
#define GSM_EN_GPIO_PORT	"GPIOB"
#define GSM_EN_GPIO_PIN		15
/* Output - GNSS Enable switch */
#define GNSS_EN_GPIO_PORT	"GPIOA"
#define GNSS_EN_GPIO_PIN	12
/* Output - GSM/GNSS Output Enable */
#define GSM_OE_GPIO_PORT	"GPIOA"
#define GSM_OE_GPIO_PIN		7

#endif /* __INC_BOARD_H */
