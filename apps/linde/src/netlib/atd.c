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
#include "atd.h"
#include "atc.h"

#include "my_misc.h"

/**
 * FIXME: On rainbonic_l151 board, dma_start() may take about 5 seconds to 
 * start DMA transmission.  Disable DMA transmission at present.
 */
#define ATD_DMA_TX

#define DEST_BUFSIZE		4096    //same to ATC_RESPBUFSIZE 1152
#define GPS_TOUT_MS		500

#define DMA_DEV_NAME		CONFIG_DMA_1_NAME
#define UART_PORT_GSM		"UART_2"
#define USART_SR		(USART2_BASE + 0x00)
#define USART_DR		(USART2_BASE + 0x04)
#define USART_CR3		(USART2_BASE + 0x14)

#define RX_STREAM_ID		5
#define TX_STREAM_ID		6

#ifdef CONFIG_BOARD_AIDONG_LINDE429V14
#define DMA1_RX_NDTR		(DMA1_BASE + 0x18 * RX_STREAM_ID + 0x14)
#elif CONFIG_BOARD_AIDONG_LINDE429V13
#define DMA1_RX_NDTR		(DMA1_BASE + 0x18 * RX_STREAM_ID + 0x14)
#elif defined(CONFIG_BOARD_RAINBONIC_L151CB) || defined(CONFIG_BOARD_RAINBONIC_L151CC)
#define DMA1_RX_NDTR		(DMA1_BASE + 0x14 * RX_STREAM_ID + 0x0c)
#else
#error "Unsupported board"
#endif

struct write_info {
	volatile u32_t		busy;
	u32_t			len;
	char			*data;
};

static struct device	       *uart_gsm_dev;
static char			dest_buf[DEST_BUFSIZE];
static struct device		*dma;

#ifdef ATD_DMA_TX
static struct write_info	write_info;
static struct dma_config	dma_tx_cfg;
static struct dma_block_config	dma_tx_blk_cfg;
#endif
static struct dma_config	dma_rx_cfg;
static struct dma_block_config	dma_rx_blk_cfg;

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

static void start_rx(void)
{
	if (dma_config(dma, RX_STREAM_ID, &dma_rx_cfg)) {
		printf("%s: Error config RX DMA\n", __func__);
		return;
	}
	uart_clear_sr();
	if (dma_start(dma, RX_STREAM_ID)) {
		printf("%s: Error start DMA\n", __func__);
		return;
	}
}

#ifdef ATD_DMA_TX
static inline void uart_dma_tx_enable(void)
{
	volatile u32_t *reg = (u32_t *)USART_CR3;
	u32_t val;

	val = *reg;
	val |= USART_CR3_DMAT;
	*reg = val;
}

static void start_tx(const char *data, size_t len)
{
	// print_log(" start tx 0\n");
	dma_tx_blk_cfg.source_address = (u32_t)data;
	dma_tx_blk_cfg.block_size = len;
	// print_log(" start tx 1\n");
	if (dma_config(dma, TX_STREAM_ID, &dma_tx_cfg)) {
		err_log("Error config TX DMA\n");
		return;
	}
	// print_log(" start tx 2\n");
	uart_clear_sr();
	// print_log(" start tx 3\n");
	if (dma_start(dma, TX_STREAM_ID)) {
		err_log("Error start TX DMA\n");
		return;
	}
	// print_log(" start tx 4\n");
}

static void dma_tx_callback(struct device *dev, u32_t id, int error_code)
{
#ifdef ATC_DEBUG
    print_log("dma tx  callback.....\n");
	if (error_code) {
		printk("%s: error_code %d\n", __func__, error_code);
		return;
	}
	if (id != TX_STREAM_ID) {
		printk("%s: wrong stream ID %d.\n", __func__, id);
		return;
	}
	if (!write_info.busy) {
		printk("%s: wrong busy val %d.\n", __func__, write_info.busy);
		return;
	}
#endif	
	write_info.busy = 0;
}
#endif /* ATD_DMA_TX */

static void dma_rx_callback(struct device *dev, u32_t id, int error_code)
{
    
#ifdef ATC_DEBUG
    print_log("dma rx callback.....\n");
	if (error_code) {
		printk("%s: error_code %d\n", __func__, error_code);
		return;
	}
	if (id != RX_STREAM_ID) {
		printk("%s: wrong stream ID %d.\n", __func__, id);
		return;
	}
#endif	
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
	dma_rx_blk_cfg.block_size = DEST_BUFSIZE;
	dma_rx_blk_cfg.dest_address = (u32_t)dest_buf;
	dma_rx_blk_cfg.dest_reload_en = 1;

	uart_dma_rx_enable();
	start_rx();

	return 0;
}

#ifdef ATD_DMA_TX
static int uart_dma_tx_init(void)
{
	dma_cfg_init(&dma_tx_cfg, &dma_tx_blk_cfg);
	dma_tx_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	dma_tx_cfg.dma_callback = dma_tx_callback;
	dma_tx_blk_cfg.dest_address = USART_DR;
	uart_dma_tx_enable();

	return 0;
}
#endif

/**
 * @brief	Read the data from circular FIFO contrlled by DMA.  The data
 *		is transmitted from UART port of 115200 8N1, thus 11520 bytes
 *		per second, ths FIFO size and read frequency should be set
 *		properly, in case of data loss.
 *
 * @return	Number of bytes read from the circular FIFO.
 */


int atd_read(char *buf, int len)
{
	static	u32_t rpos;
	u32_t	ntd;
	u32_t	wpos;
	char	*p;
	u32_t	total, rest, rlen;

	ntd = *(volatile u32_t *)DMA1_RX_NDTR;
	ntd &= 0xFFFF;
	wpos  = DEST_BUFSIZE - ntd;
	total = 0;
	rest  = len;
	p     = buf;
    //print_log("ndtr:[%d] wpos:[%d] rpos:[%d]\n",ntd,wpos,rpos);
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


void atd_clean(void)
{
    int relength = -1;
    char dumpbuf[100];
    do
    {
        relength= atd_read(dumpbuf,100);
        
    }
    while(relength);
    print_log("atd clean ok .........\n");
    return;
}


/**
 * @brief	Write the data to UART.  The caller will garranty the 
 */
#ifdef ATD_DMA_TX
int atd_write(const char *buf, int len, int is_cmd)
{
	int size;
	int i;
	char *mem_ptr;
	
	// print_log(" atd write step 0\n");
	if (write_info.busy) {
		err_log("try to write on device busy.\n");
		return -1;
	}

	size = is_cmd ? len + 1 : len;
	mem_ptr = buf;
	if (is_cmd)
		mem_ptr[len]	 = '\r';

	write_info.busy = 1;
	write_info.len = size;
	write_info.data = mem_ptr;

	// print_log("atd write step 1.\n");
	
	start_tx(write_info.data, size);
	// print_log("atd write step 2.\n");
	if (is_cmd) {
        /***
		print_log("Send Cmd: [ ");
		for (i = 0; i < len; i++)
			printk("%c", buf[i]);
		printk(" ]\n");
		***/
	} else {
		print_log("Send: data of %d bytes\n", len);
	}
	// print_log("atd write step 3.\n");
	while (write_info.busy)
		;
	// print_log("atd write step 4.\n");
	return len;
}

#else
/* !defined ATD_DMA_TX */
int atd_write(const char *buf, int len, int is_cmd)
{
	int i;
    printk("\n atd write : data %s\n", buf);

#ifdef	ATC_DEBUG
	if (is_cmd)
		printk("\nSend: {%s}\n", buf);
	else
		printk("\nSend: data of %d bytes\n", len);
#endif	
	for (i = 0; i < len; i++) {
		uart_poll_out(uart_gsm_dev, buf[i]);
	}
	if (is_cmd)
		uart_poll_out(uart_gsm_dev, '\r');
	
	return len;
}
#endif /* ATD_DMA_TX */

int atd_init(void)
{
	if ((uart_gsm_dev = device_get_binding(UART_PORT_GSM)) == NULL) {
		printf("Fail to bind U(S)ART device %s\n", UART_PORT_GSM);
		return -1;
	}
	if ((dma = device_get_binding(DMA_DEV_NAME)) == NULL) {
		printf("%s: Cannot get DMA controller.\n", __func__);
		return -1;
	}
	if (uart_dma_rx_init() < 0) {
		printf("%s: uart_dma_rx_init() failed\n", __func__);
		return -1;
	}
#ifdef ATD_DMA_TX
	if (uart_dma_tx_init() < 0) {
		printf("%s: uart_dma_tx_init() failed\n", __func__);
		return -1;
	}
#endif	

	return 0;
}

