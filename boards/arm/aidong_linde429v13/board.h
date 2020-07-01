/*
 * Copyright (c) 2018 Rainbonic Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* Input - Detect power supply, from external or by battery */
#define PWR_EXT_GPIO_PORT	"GPIOB"
#define PWR_EXT_GPIO_PIN	8
/* Output - Power Hold switch */
#define PWR_HOLD_GPIO_PORT	"GPIOC"
#define PWR_HOLD_GPIO_PIN	0

/* Input - System initialization configuration pin */
#define CFG_INIT_GPIO_PORT	"GPIOE"
#define CFG_INIT_GPIO_PIN	4
/* Output - Status LED */
#define LED_STAT_GPIO_PORT	"GPIOC"
#define LED_STAT_GPIO_PIN	1

/* Output - WiFi Enable switch */
#define WIFI_EN_GPIO_PORT	"GPIOB"
#define WIFI_EN_GPIO_PIN	15

/* Output - GSM Enable swith */
#define GSM_EN_GPIO_PORT	"GPIOC"
#define GSM_EN_GPIO_PIN		2
/* Output - GSM Enable switch */
#define GSM_PWR_GPIO_PORT	"GPIOC"
#define GSM_PWR_GPIO_PIN	4
/* Output - GNSS Enable switch */
#define GNSS_EN_GPIO_PORT	"GPIOC"
#define GNSS_EN_GPIO_PIN	3

/* Input - RF GDO0 pin */
#define RF_GDO0_GPIO_PORT	"GPIOE"
#define RF_GDO0_GPIO_PIN	2
/* Input - RF GDO2 pin */
#define RF_GDO2_GPIO_PORT	"GPIOE"
#define RF_GDO2_GPIO_PIN	3
/* Output - RF TXEN pin */
#define RF_TXEN_GPIO_PORT	"GPIOE"
#define RF_TXEN_GPIO_PIN	0
/* Output - RF RXEN pin */
#define RF_RXEN_GPIO_PORT	"GPIOE"
#define RF_RXEN_GPIO_PIN	1
/* Output - RS485_DE pin */
#define RS485_DE_GPIO_PORT	"GPIOE"
#define RS485_DE_GPIO_PIN	9

/* External GPIO port - J1 */
#define RSVDX_GPIO_PORT		"GPIOE"
#define RSVD0_GPIO_PIN		15
#define RSVD1_GPIO_PIN		14
#define RSVD2_GPIO_PIN		13
#define RSVD3_GPIO_PIN		12

/* Define aliases to make the basic samples work */
#define SW0_GPIO_NAME		CFG_INIT_GPIO_PORT
#define SW0_GPIO_PIN		CFG_INIT_GPIO_PIN
#define LED0_GPIO_PORT		LED_STAT_GPIO_PORT
#define LED0_GPIO_PIN		LED_STAT_GPIO_PIN

//////////////////RFID INTERFACE//////////////////////
/*wg34 D0*/

#define VEHICLE_LED_OPEN_VAL    1
#define VEHICLE_LED_CLOSE_VAL  0

#define GPIO_RFID_WG34_D0	"GPIOE"
#define GPIO_RFID_WG34_D0_PIN	13
/*WG32 D1*/
#define GPIO_RFID_WG34_D1	"GPIOE"
#define GPIO_RFID_WG34_D1_PIN	12
/*rfid led*/
#define GPIO_RFID_LED		"GPIOE"
#define	GPIO_RFID_LED_PIN	15
//////////////////RFID INTERFACE//////////////////////
/*vehicle relay interface */
#define GPIO_VEHCL_CTRL_DELAY_PORT      "GPIOD"
#define GOIO_VEHCL_CTRL_DALAY_PIN       12
/* GPIO OUT */
#define GPIO_OUT_PORT		"GPIOD"
#define GPIO_OUT0_PIN           11
#define GPIO_OUT1_PIN           12
/*vehicle ctrl beep interface*/
#define GPIO_VEHCL_CTRL_BEEP_PORT       "GPIOE"
#define GPIO_VEHCL_CTRL_BEEP_PIN        14

/*vehicle ctrl led interface*/
#define GPIO_VEHCL_CTRL_LED_PORT    "GPIOE"
#define GPIO_VEHCL_CTRL_LED_PIN     15



/* Uart port */
#define UART_PORT_CON   "UART_1"
#define UART_PORT_GSM   "UART_2"
#define UART_PORT_GPS   "UART_3"
#define UART_PORT_RF433 "UART_6"


#endif /* __INC_BOARD_H */
