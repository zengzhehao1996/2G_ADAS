/*
 * Copyright (c) 2018 Rainbonic Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* Input - Detect power supply, from external or by battery */
#define PWR_EXT_GPIO_PORT	"GPIOD"
#define PWR_EXT_GPIO_PIN	10
/* Output - Power Hold switch */
#define PWR_HOLD_GPIO_PORT	"GPIOD"
#define PWR_HOLD_GPIO_PIN	11

/* Input - System initialization configuration pin */
#define CFG_INIT_GPIO_PORT	"GPIOA"
#define CFG_INIT_GPIO_PIN	12
/* Output - Status LED */
#define LED_STAT_GPIO_PORT	"GPIOI"
#define LED_STAT_GPIO_PIN0	4
#define LED_STAT_GPIO_PIN1	5
#define LED_STAT_GPIO_PIN	LED_STAT_GPIO_PIN0

/*
 * GSM & GNSS/GPS module pins
 */
/* Output - GSM Enable swith */
#define GSM_EN_GPIO_PORT	"GPIOD"
#define GSM_EN_GPIO_PIN		15
/* Output - GSM Enable switch */
#define GSM_PWR_GPIO_PORT	"GPIOD"
#define GSM_PWR_GPIO_PIN	13
/* Output - GNSS Enable switch */
#define GNSS_EN_GPIO_PORT	"GPIOD"
#define GNSS_EN_GPIO_PIN	14

/*
 * RF 433MHz module pins
 */
/* Output - RF TXEN pin */
#define RF_TXEN_GPIO_PORT	"GPIOH"
#define RF_TXEN_GPIO_PIN	12
/* Output - RF RXEN pin */
#define RF_RXEN_GPIO_PORT	"GPIOH"
#define RF_RXEN_GPIO_PIN	13
/* Input - RF GDO0 pin */
#define RF_GDO0_GPIO_PORT	"GPIOH"
#define RF_GDO0_GPIO_PIN	14
/* Input - RF GDO2 pin */
#define RF_GDO2_GPIO_PORT	"GPIOH"
#define RF_GDO2_GPIO_PIN	15

/* 
 * RFID GPIOs 
 */
 #define VEHICLE_LED_OPEN_VAL   0
#define VEHICLE_LED_CLOSE_VAL  1
/* Input - RFID D0 pin */
#define GPIO_RFID_WG34_D0	    "GPIOI"
/* Input - RFID D0 pin */
#define GPIO_RFID_WG34_D0_PIN	0
/* Input - RFID D1 pin */   
#define GPIO_RFID_WG34_D1       GPIO_RFID_WG34_D0
#define GPIO_RFID_WG34_D1_PIN	1
/* Output - RFID BEEP pin */
#define GPIO_VEHCL_CTRL_BEEP_PORT GPIO_RFID_WG34_D0
#define GPIO_VEHCL_CTRL_BEEP_PIN  2
/* Output - RFID LED pin */
#define GPIO_VEHCL_CTRL_LED_PORT  "GPIOI"
#define GPIO_VEHCL_CTRL_LED_PIN	  5

#define GPIO_VEHCL_CTRL_DELAY_PORT      "GPIOF"
#define GOIO_VEHCL_CTRL_DALAY_PIN       15

/*
 * Misc pins
 */
/* Output - RS485_DE pin */
#define RS485_DE_GPIO_PORT	"GPIOE"
#define RS485_DE_GPIO_PIN	9
/* Output - WiFi Enable switch */
#define WIFI_EN_GPIO_PORT	"GPIOD"
#define WIFI_EN_GPIO_PIN	9

/*
 * External GPIO pins - J16 port
 */
/* External input GPIO pins */
#define EXT_IN_GPIO_PORT	"GPIOG"
#define EXT_IN_GPIO_PIN0	0
#define EXT_IN_GPIO_PIN1	1
#define EXT_IN_GPIO_PIN2	2
#define EXT_IN_GPIO_PIN3	3
/* External int intput gpio pins */
#define EXT_INT_GPIO_PORT       "GPIOA"
#define EXT_INT_GPIO_PIN0	0
#define EXT_INT_GPIO_PIN1	1
/* External output GPIO pins */
#define EXT_OUT_GPIO_PORT	"GPIOF"
#define EXT_OUT_GPIO_PIN0	14
#define EXT_OUT_GPIO_PIN1	15

/*
 * CAN bus pins
 */
#define	CAN1_GPIO		GPIOD
#define CAN1_GPIO_TX		GPIO_PIN_1
#define CAN1_GPIO_RX		GPIO_PIN_0
#define CAN1_GPIO_ENABLE	__HAL_RCC_GPIOD_CLK_ENABLE

/* Define aliases to make the basic samples work */
#define SW0_GPIO_NAME		CFG_INIT_GPIO_PORT
#define SW0_GPIO_PIN		CFG_INIT_GPIO_PIN
#define LED0_GPIO_PORT		LED_STAT_GPIO_PORT
#define LED0_GPIO_PIN		LED_STAT_GPIO_PIN0

/*
 * ADC for voltage measurement, etc.
 */
#define VOL_ADCx		ADC3
#define VOL_GPIO_PORT		GPIOF
#define VOL_CAP_GPIO_PIN	GPIO_PIN_3
#define VOL_CAP_ADC_CHANNEL	ADC_CHANNEL_9
#define VOL_BAT_GPIO_PIN	GPIO_PIN_4
#define VOL_BAT_ADC_CHANNEL	ADC_CHANNEL_14
#define VOL_VIN_GPIO_PIN	GPIO_PIN_5
#define VOL_VIN_ADC_CHANNEL	ADC_CHANNEL_15
#define VOL_KEY_GPIO_PIN	GPIO_PIN_6
#define VOL_KEY_ADC_CHANNEL	ADC_CHANNEL_4

#define ADC_IN0_GPIO_PIN	GPIO_PIN_7
#define ADC_IN0_ADC_CHANNEL	ADC_CHANNEL_5
#define ADC_IN1_GPIO_PIN	GPIO_PIN_8
#define ADC_IN1_ADC_CHANNEL	ADC_CHANNEL_6
#define ADC_IN2_GPIO_PIN	GPIO_PIN_9
#define ADC_IN2_ADC_CHANNEL	ADC_CHANNEL_7
#define ADC_IN3_GPIO_PIN	GPIO_PIN_10
#define ADC_IN3_ADC_CHANNEL	ADC_CHANNEL_8

/* GPIO OUT */
#define GPIO_OUT_PORT		"GPIOF"
#define GPIO_OUT0_PIN           14
#define GPIO_OUT1_PIN           15

/* Uart port */
#define UART_PORT_CON   "UART_1"
#define UART_PORT_GSM   "UART_2"
#define UART_PORT_GPS   "UART_3"
#define UART_PORT_RF433 "UART_6"

#endif /* __INC_BOARD_H */
