/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Sample app to utilize GPIO on AiDong Linde-PA board
 *
 */

#include <zephyr.h>
#include <string.h>
#include <stdio.h>
#include <device.h>
#include <gpio.h>
#include <uart.h>
#include <dma.h>
#include <misc/util.h>

/* boards/${arch}/${board}/board.h */
#include "board.h"
#include "atc.h"
#include "gpd.h"
#include "my_misc.h"

#define DEV_DEBUG

//#ifdef CONFIG_BOARD_AIDONG_LINDE429V14
//#define GPS_UART_PORT		"UART_6"
//#define GPS_UART_BASE		USART6_BASE
//#define GPS_DMA_DEV		    CONFIG_DMA_2_NAME
//#define GPS_DMA_SLOT_RX		1	/* stream 1 */	
//#define GPS_DMA_SLOT_TX		6	/* stream 6 */
//#define GPS_DMA_RX_NDTR		(DMA2_BASE + 0x18 * GPS_DMA_SLOT_RX + 0x14)

//#elif CONFIG_BOARD_AIDONG_LINDE429V13
#ifdef CONFIG_BOARD_AIDONG_LINDE429V13
#define GPS_UART_PORT		"UART_3"
#define GPS_UART_BASE		USART3_BASE
#define GPS_DMA_DEV		    CONFIG_DMA_1_NAME
#define GPS_DMA_SLOT_RX		1	/* stream 1 */	
#define GPS_DMA_SLOT_TX		3	/* stream 3 */
#define GPS_DMA_RX_NDTR		(DMA1_BASE + 0x18 * GPS_DMA_SLOT_RX + 0x14)

#elif defined(CONFIG_BOARD_AIDONG_LINDE429V14)
#define GPS_UART_PORT		"UART_3"
#define GPS_UART_BASE		USART3_BASE
#define GPS_DMA_DEV		    CONFIG_DMA_1_NAME
#define GPS_DMA_SLOT_RX		1	/* stream 1 */	
#define GPS_DMA_SLOT_TX		3	/* stream 3 */
#define GPS_DMA_RX_NDTR		(DMA1_BASE + 0x18 * GPS_DMA_SLOT_RX + 0x14)

#elif defined(CONFIG_BOARD_RAINBONIC_L151CB) || defined(CONFIG_BOARD_RAINBONIC_L151CC)
#define GPS_UART_PORT		"UART_3"
#define GPS_UART_BASE		USART3_BASE
#define GPS_DMA_DEV		    CONFIG_DMA_1_NAME
#define GPS_DMA_SLOT_RX		2	/* channel 3 */
#define GPS_DMA_SLOT_TX		1	/* channel 2 */
#define GPS_DMA_RX_NDTR		(DMA1_BASE + 0x14 * GPS_DMA_SLOT_RX + 0x0c)
#else
#error "Unsupported board"
#endif


#define USART_SR		(GPS_UART_BASE + 0x00)
#define USART_DR		(GPS_UART_BASE + 0x04)
#define USART_CR3		(GPS_UART_BASE + 0x14)

#define DEST_BUFSIZE		1024
static struct device	       *uart_gps_dev;
static char			dest_buf[DEST_BUFSIZE];
static struct device		*dma;

static struct dma_config	dma_tx_cfg;
static struct dma_config	dma_rx_cfg;
static struct dma_block_config	dma_tx_blk_cfg;
static struct dma_block_config	dma_rx_blk_cfg;

static volatile u8_t		gpd_tx_done;

static inline void uart_clear_sr(void)
{
	volatile u32_t *reg = (volatile u32_t *)USART_SR;
	u32_t val;
	
	val = *reg;
	val &= ~USART_SR_TC;
	*reg = val;
}

static inline void uart_dma_rx_enable(void)
{
	volatile u32_t *reg = (u32_t *)USART_CR3;
	u32_t val;

	uart_clear_sr();
	val = *reg;
	val |= USART_CR3_DMAR;
	*reg = val;
}

static inline void uart_dma_tx_enable(void)
{
	volatile u32_t *reg = (u32_t *)USART_CR3;
	u32_t val;

	val = *reg;
	val |= USART_CR3_DMAT;
	*reg = val;
}

static void start_rx(void)
{
	if (dma_config(dma, GPS_DMA_SLOT_RX, &dma_rx_cfg)) {
		printf("%s: Error config RX DMA\n", __func__);
		return;
	}
	uart_clear_sr();
	if (dma_start(dma, GPS_DMA_SLOT_RX)) {
		printf("%s: Error start DMA\n", __func__);
		return;
	}
}

static void dma_rx_callback(struct device *dev, u32_t id, int error_code)
{
	if (error_code) {
		printk("%s: error code %d.\n", __func__, error_code);
		return;
	}
}

static void dma_tx_callback(struct device *dev, u32_t id, int error_code)
{
	if (error_code) {
#ifdef DEV_DEBUG
		printf("%s: detected error code %d\n", __func__, error_code);
#endif		
		return;
	}
	if (id != GPS_DMA_SLOT_TX) {
#ifdef DEV_DEBUG		
		printf("%s: stream ID error, expected %d, got %d.\n",
			__func__, GPS_DMA_SLOT_TX, id);
#endif		
		return;
	}
	gpd_tx_done = 1;
}

static void dma_cfg_init(struct dma_config *cfg, struct dma_block_config *blk_cfg)
{
	cfg->source_data_size = 1;
	cfg->dest_data_size = 1;
	cfg->source_burst_length = 1;
	cfg->dest_burst_length = 1;
	cfg->complete_callback_en = 0;
	cfg->error_callback_en = 1;
	cfg->block_count = 1;
	cfg->head_block = blk_cfg;
}

static int uart_dma_rx_init(void)
{
	dma_cfg_init(&dma_rx_cfg, &dma_rx_blk_cfg);
	dma_rx_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	dma_rx_cfg.dma_callback = dma_rx_callback;
	dma_rx_blk_cfg.source_address = USART_DR;
	dma_rx_blk_cfg.dest_address = (u32_t)dest_buf;
	dma_rx_blk_cfg.block_size = DEST_BUFSIZE;
	dma_rx_blk_cfg.dest_reload_en = 1;

	uart_dma_rx_enable();
	uart_dma_tx_enable();
	start_rx();

	return 0;
}

static int uart_dma_tx_init(void)
{
	dma_cfg_init(&dma_tx_cfg, &dma_tx_blk_cfg);
	dma_tx_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	dma_tx_cfg.dma_callback = dma_tx_callback;
	dma_tx_blk_cfg.dest_address = USART_DR;

	return 0;
}


/**
 * @brief	Read the data from circular FIFO contrlled by DMA.  The data
 *		is transmitted from UART port of 115200 8N1, thus 11520 bytes
 *		per second, ths FIFO size and read frequency should be set
 *		properly, in case of data loss.
 *
 * @return	Number of bytes read from the circular FIFO.
 */
int gpd_read(char *buf, unsigned int len)
{
	static	u32_t rpos;
	u32_t	ndt;
	u32_t	wpos;
	char	*p;
	u32_t	total, rest, rlen;

	ndt = *(volatile u32_t *)GPS_DMA_RX_NDTR;
	ndt &= 0xFFFF;
	wpos  = DEST_BUFSIZE - ndt;
	total = 0;
	rest  = len;
	p     = buf;
    // print_log("NDTR:[%d] wpos:[%d] rpos:[%d]\n",ndt,wpos,rpos);
	if (rpos > wpos) {
		rlen = DEST_BUFSIZE - rpos;
		if (rlen > rest)
			rlen = rest;
		memcpy(p, &dest_buf[rpos], rlen);
		total += rlen;
		rest  -= rlen;
		p     += rlen;
		rpos  += rlen;
		if (rpos == DEST_BUFSIZE)
			rpos = 0;
	}
	if (rpos < wpos) {
		rlen = wpos - rpos;
		if (rlen > rest)
			rlen = rest;
		memcpy(p, &dest_buf[rpos], rlen);
		total += rlen;
		rest  -= rlen;
		p     += rlen;
		rpos  += rlen;
	}

	return total;
}

int gpd_send(const char *data, unsigned int len)
{
	gpd_tx_done = 0;
	dma_tx_blk_cfg.source_address = (u32_t)data;
	dma_tx_blk_cfg.block_size = len;
	if (dma_config(dma, GPS_DMA_SLOT_TX, &dma_tx_cfg)) {
		printf("%s: Error config TX DMA\n", __func__);
		gpd_tx_done = 1;
		return -1;
	}
	uart_clear_sr();
	if (dma_start(dma, GPS_DMA_SLOT_TX)) {
		printf("%s: Error start TX DMA\n", __func__);
		gpd_tx_done = 1;
		return -1;
	}
	do {
		k_sleep(K_MSEC(10));
	} while (!gpd_tx_done);
	return len;
}

int gpd_init(void)
{
	if ((uart_gps_dev = device_get_binding(GPS_UART_PORT)) == NULL) {
		printf("Fail to bind U(S)ART device %s\n", GPS_UART_PORT);
		return -1;
	}
	if ((dma = device_get_binding(GPS_DMA_DEV)) == NULL) {
		printf("%s: Cannot get DMA controller.\n", __func__);
		return -1;
	}
	if (uart_dma_rx_init() < 0) {
		printf("%s: uart_dma_rx_init() failed\n", __func__);
		return -1;
	}
	if (uart_dma_tx_init() < 0) {
		printf("%s: uart_dma_rx_init() failed\n", __func__);
		return -1;
	}

	return 0;
}

int gpd_fini(void)
{
	struct device *dev;

	dev = device_get_binding(GNSS_EN_GPIO_PORT);
	if (dev == NULL) {
		printk("Can not find device %s.\n", GNSS_EN_GPIO_PORT);
		return -EIO;
	}
	if (gpio_pin_configure(dev, GNSS_EN_GPIO_PIN, GPIO_DIR_OUT) != 0) {
		printk("Configure device %s pin %d direction failed.\n",
			GNSS_EN_GPIO_PORT, GNSS_EN_GPIO_PIN);
		return -EIO;
	}
	gpio_pin_write(dev, GNSS_EN_GPIO_PIN, 0);
	
	return 0;
}








