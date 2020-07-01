/*
 * Copyright (c) 2016 Linaro Limited.
 * Copyright (c) 2018 Rainbonic Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DMA_LEVEL

#include <board.h>
#include <device.h>
#include <dma.h>
#include <errno.h>
#include <init.h>
#include <logging/sys_log.h>
#include <stdio.h>
#include <string.h>

#include <clock_control/stm32_clock_control.h>

#define DMA_STM32_MAX_STREAMS	8	/* Number of streams per controller */
#define DMA_STM32_MAX_DEVS	2	/* Number of controllers */
#define DMA_STM32_1		0	/* First  DMA controller */
#define DMA_STM32_2		1	/* Second DMA controller */

#define DMA_STM32_IRQ_PRI	CONFIG_DMA_0_IRQ_PRI

struct dma_stm32_stream_reg {
	/* Shared registers */
	u32_t isr;
	u32_t ifcr;

	/* Per channel registers */
	u32_t ccr;
	u32_t cndtr;
	u32_t cpar;
	u32_t cmar;
};

struct dma_stm32_stream {
	u32_t direction;
	struct device *dev;
	struct dma_stm32_stream_reg regs;
	bool busy;

	void (*dma_callback)(struct device *dev, u32_t id,
			     int error_code);
};

static struct dma_stm32_device {
	u32_t base;
	struct device *clk;
	struct dma_stm32_stream stream[DMA_STM32_MAX_STREAMS];
} device_data[DMA_STM32_MAX_DEVS];

struct dma_stm32_config {
	struct stm32_pclken pclken;
	void (*config)(struct dma_stm32_device *);
};

/* DMA burst length */
#define BURST_TRANS_LENGTH_1			0

/* DMA direction */
#define DMA_STM32_DEV_TO_MEM			0
#define DMA_STM32_MEM_TO_DEV			1
#define DMA_STM32_MEM_TO_MEM			2

/* DMA priority level */
#define DMA_STM32_PRIORITY_LOW			0
#define DMA_STM32_PRIORITY_MEDIUM		1
#define DMA_STM32_PRIORITY_HIGH			2
#define DMA_STM32_PRIORITY_VERY_HIGH		3

/* DMA FIFO threshold selection */
#define DMA_STM32_FIFO_THRESHOLD_1QUARTERFULL	0
#define DMA_STM32_FIFO_THRESHOLD_HALFFULL	1
#define DMA_STM32_FIFO_THRESHOLD_3QUARTERSFULL	2
#define DMA_STM32_FIFO_THRESHOLD_FULL		3

/* Maximum data sent in single transfer (Bytes) */
#define DMA_STM32_MAX_DATA_ITEMS		0xffff

#define BITS_PER_LONG		32
#define GENMASK(h, l)	(((~0UL) << (l)) & (~0UL >> (BITS_PER_LONG - 1 - (h))))

#define DMA_STM32_1_BASE	0x40026000
#define DMA_STM32_2_BASE	0x40026400

/* Shared registers */
#define DMA_STM32_ISR		0x00   /* DMA int status reg		      */
/* DMA int flag clear Register */
#define DMA_STM32_IFCR		0x04
#define   DMA_STM32_GI		BIT(0) /* Global interrupt		      */
#define   DMA_STM32_TCI		BIT(1) /* Transfer complete interrupt	      */
#define   DMA_STM32_HTI		BIT(2) /* Transfer half complete interrupt    */
#define   DMA_STM32_TEI		BIT(3) /* Transfer error interrupt	      */

/* DMA Channel x Configuration Register */
#define DMA_STM32_CCR(x)		(0x08 + 0x14 * (x))
#define   DMA_STM32_CCR_EN		BIT(0)  /* Stream Enable	      */
#define   DMA_STM32_CCR_TCIE		BIT(1)  /* Transfer Comp Int En       */
#define   DMA_STM32_CCR_HTIE		BIT(2)  /* Transfer 1/2 Comp Int En   */
#define   DMA_STM32_CCR_TEIE		BIT(3)  /* Transfer Error Int En      */
#define   DMA_STM32_CCR_DIR		BIT(4)	/* Transfer direction.	      */
						/* 0: to memory, 1: to periph */
#define   DMA_STM32_CCR_CIRC		BIT(5)  /* Circular mode	      */
#define   DMA_STM32_CCR_PINC		BIT(6)  /* Peripheral increment mode  */
#define   DMA_STM32_CCR_MINC		BIT(7)	/* Memory increment mode      */
#define   DMA_STM32_CCR_PSIZE_MASK	GENMASK(9, 8)   /* Periph data size   */
#define   DMA_STM32_CCR_MSIZE_MASK	GENMASK(11, 10) /* Memory data size   */
#define   DMA_STM32_CCR_PL_MASK		GENMASK(13, 12) /* Priority level     */
#define   DMA_STM32_CCR_MEM2MEM		BIT(14) /* Target in double buffer    */

/*	  Setting MACROS */
#define   DMA_STM32_CCR_PSIZE(n)	((n & 0x3) << 8)
#define   DMA_STM32_CCR_MSIZE(n)	((n & 0x3) << 10)
#define   DMA_STM32_CCR_PL(n)		((n & 0x3) << 12)

/*	  Getting MACROS */
#define   DMA_STM32_CCR_PSIZE_GET(n)    ((n & DMA_STM32_CCR_PSIZE_MASK) >> 8)
#define   DMA_STM32_CCR_CFG_MASK	(DMA_STM32_CCR_PINC  \
					| DMA_STM32_CCR_MINC \
					| DMA_STM32_CCR_PL_MASK)
#define   DMA_STM32_CCR_IRQ_MASK	(DMA_STM32_CCR_TCIE \
					| DMA_STM32_CCR_TEIE \
					| DMA_STM32_CCR_HTIE)

/* DMA stream x number of data register (len) */
#define DMA_STM32_CNDTR(x)		(0x0c + 0x14 * (x))

/* DMA stream peripheral address register (source) */
#define DMA_STM32_CPAR(x)		(0x10 + 0x14 * (x))

/* DMA stream x memory 0 address register (destination) */
#define DMA_STM32_CMAR(x)		(0x14 + 0x14 * (x))

#define SYS_LOG_U32			__attribute((__unused__)) u32_t

static void dma_stm32_1_config(struct dma_stm32_device *ddata);

static u32_t dma_stm32_read(struct dma_stm32_device *ddata, u32_t reg)
{
	return sys_read32(ddata->base + reg);
}

static void dma_stm32_write(struct dma_stm32_device *ddata,
			    u32_t reg, u32_t val)
{
	sys_write32(val, ddata->base + reg);
}

static void dma_stm32_dump_reg(struct dma_stm32_device *ddata, u32_t id)
{
	SYS_LOG_INF("Using channel: %d\n", id + 1);
	SYS_LOG_INF("CCR:   0x%x \t(config)\n",
		    dma_stm32_read(ddata, DMA_STM32_CCR(id)));
	SYS_LOG_INF("CNDTR: 0x%x \t(length)\n",
		    dma_stm32_read(ddata, DMA_STM32_CNDTR(id)));
	SYS_LOG_INF("CPAR:  0x%x \t(source)\n",
		    dma_stm32_read(ddata, DMA_STM32_CPAR(id)));
	SYS_LOG_INF("CMAR:  0x%x \t(destination)\n",
		    dma_stm32_read(ddata, DMA_STM32_CMAR(id)));
}

static u32_t dma_stm32_irq_status(struct dma_stm32_device *ddata, u32_t id)
{
	u32_t irqs;

	irqs = dma_stm32_read(ddata, DMA_STM32_ISR);
	return (irqs >> (id << 2));
}

static void dma_stm32_irq_clear(struct dma_stm32_device *ddata, u32_t id,
				u32_t irqs)
{
	irqs = irqs << (id << 2);
	dma_stm32_write(ddata, DMA_STM32_IFCR, irqs);
}

static void dma_stm32_irq_handler(void *arg, u32_t id)
{
	struct device *dev = arg;
	struct dma_stm32_device *ddata = dev->driver_data;
	struct dma_stm32_stream *stream = &ddata->stream[id];
	u32_t irqstatus, config, ifcr;

	irqstatus = dma_stm32_irq_status(ddata, id);
	config = dma_stm32_read(ddata, DMA_STM32_CCR(id));
	ifcr = dma_stm32_read(ddata, DMA_STM32_IFCR);

	/* Silently ignore spurious transfer half complete IRQ */
	if (irqstatus & DMA_STM32_HTI) {
		dma_stm32_irq_clear(ddata, id, DMA_STM32_HTI);
		return;
	}

	stream->busy = false;

	if ((irqstatus & DMA_STM32_TCI) && (config & DMA_STM32_CCR_TCIE)) {
		dma_stm32_irq_clear(ddata, id, DMA_STM32_TCI);

		stream->dma_callback(stream->dev, id, 0);
	} else {
		SYS_LOG_ERR("Internal error: IRQ status: 0x%x\n", irqstatus);
		dma_stm32_irq_clear(ddata, id, irqstatus);

		stream->dma_callback(stream->dev, id, -EIO);
	}
}

static int dma_stm32_disable_stream(struct dma_stm32_device *ddata,
				  u32_t id)
{
	u32_t config;
	int count = 0;
	int ret = 0;

	for (;;) {
		config = dma_stm32_read(ddata, DMA_STM32_CCR(id));
		/* Stream already disabled */
		if (!(config & DMA_STM32_CCR_EN)) {
			return 0;
		}

		/* Try to disable stream */
		dma_stm32_write(ddata, DMA_STM32_CCR(id),
				config &= ~DMA_STM32_CCR_EN);

		/* After trying for 5 seconds, give up */
		k_sleep(K_SECONDS(5));
		if (count++ > (5 * 1000) / 50) {
			SYS_LOG_ERR("DMA error: Stream in use\n");
			return -EBUSY;
		}
	}

	return ret;
}

static int dma_stm32_config_devcpy(struct device *dev, u32_t id,
				   struct dma_config *config)

{
	struct dma_stm32_device *ddata = dev->driver_data;
	struct dma_stm32_stream_reg *regs = &ddata->stream[id].regs;
	u32_t src_bus_width  = dma_width_index(config->source_data_size);
	u32_t dst_bus_width  = dma_width_index(config->dest_data_size);
	enum dma_channel_direction direction = config->channel_direction;
#if 1
	if (config->source_data_size == 1)
		src_bus_width = 0;
	if (config->dest_data_size == 1)
		dst_bus_width = 0;
#endif	
	switch (direction) {
	case MEMORY_TO_PERIPHERAL:
		regs->ccr = DMA_STM32_CCR_PSIZE(dst_bus_width) |
			DMA_STM32_CCR_MSIZE(src_bus_width) |
			DMA_STM32_CCR_DIR |
			DMA_STM32_CCR_TCIE |
			DMA_STM32_CCR_TEIE |
			DMA_STM32_CCR_MINC;
		break;
	case PERIPHERAL_TO_MEMORY:
		regs->ccr = DMA_STM32_CCR_PSIZE(src_bus_width) |
			DMA_STM32_CCR_MSIZE(dst_bus_width) |
			DMA_STM32_CCR_TCIE |
			DMA_STM32_CCR_TEIE |
			DMA_STM32_CCR_MINC;
		if (config->head_block->dest_reload_en) 
			regs->ccr |= DMA_STM32_CCR_CIRC;
		break;
	default:
		SYS_LOG_ERR("DMA error: Direction not supported: %d",
			    direction);
		return -EINVAL;
	}

	return 0;
}

static int dma_stm32_config_memcpy(struct device *dev, u32_t id)
{
	struct dma_stm32_device *ddata = dev->driver_data;
	struct dma_stm32_stream_reg *regs = &ddata->stream[id].regs;

	regs->ccr = DMA_STM32_CCR_MEM2MEM |
		DMA_STM32_CCR_MINC |		/* Memory increment mode */
		DMA_STM32_CCR_PINC |		/* Peripheral increment mode */
		DMA_STM32_CCR_TCIE |		/* Transfer comp IRQ enable */
		DMA_STM32_CCR_TEIE;		/* Transfer error IRQ enable */
	
	return 0;
}

static int dma_stm32_config(struct device *dev, u32_t id,
			    struct dma_config *config)
{
	struct dma_stm32_device *ddata = dev->driver_data;
	struct dma_stm32_stream *stream = &ddata->stream[id];
	struct dma_stm32_stream_reg *regs = &ddata->stream[id].regs;
	int ret;

	if (id >= DMA_STM32_MAX_STREAMS) {
		return -EINVAL;
	}

	if (stream->busy) {
		return -EBUSY;
	}

	if (config->head_block->block_size > DMA_STM32_MAX_DATA_ITEMS) {
		SYS_LOG_ERR("DMA error: Data size too big: %d\n",
		       config->head_block->block_size);
		return -EINVAL;
	}

	stream->busy		= true;
	stream->dma_callback	= config->dma_callback;
	stream->direction	= config->channel_direction;

	if (stream->direction == MEMORY_TO_PERIPHERAL) {
		regs->cmar = (u32_t)config->head_block->source_address;
		regs->cpar = (u32_t)config->head_block->dest_address;
	} else {
		regs->cpar = (u32_t)config->head_block->source_address;
		regs->cmar = (u32_t)config->head_block->dest_address;
	}

	if (stream->direction == MEMORY_TO_MEMORY) {
		ret = dma_stm32_config_memcpy(dev, id);
	} else {
		ret = dma_stm32_config_devcpy(dev, id, config);
	}

	regs->cndtr = config->head_block->block_size;

	return ret;
}

static int dma_stm32_start(struct device *dev, u32_t id)
{
	struct dma_stm32_device *ddata = dev->driver_data;
	struct dma_stm32_stream_reg *regs = &ddata->stream[id].regs;
	u32_t irqstatus;
	int ret;

	if (id >= DMA_STM32_MAX_STREAMS) {
		return -EINVAL;
	}

	ret = dma_stm32_disable_stream(ddata, id);
	if (ret) {
		return ret;
	}

	dma_stm32_write(ddata, DMA_STM32_CCR(id),   regs->ccr);
	dma_stm32_write(ddata, DMA_STM32_CPAR(id),  regs->cpar);
	dma_stm32_write(ddata, DMA_STM32_CMAR(id),  regs->cmar);
	dma_stm32_write(ddata, DMA_STM32_CNDTR(id), regs->cndtr);

	/* Clear remanent IRQs from previous transfers */
	irqstatus = dma_stm32_irq_status(ddata, id);
	if (irqstatus) {
		dma_stm32_irq_clear(ddata, id, irqstatus);
	}

	dma_stm32_dump_reg(ddata, id);

	/* Push the start button */
	dma_stm32_write(ddata, DMA_STM32_CCR(id),
			regs->ccr | DMA_STM32_CCR_EN);

	return 0;
}

static int dma_stm32_stop(struct device *dev, u32_t id)
{
	struct dma_stm32_device *ddata = dev->driver_data;
	struct dma_stm32_stream *stream = &ddata->stream[id];
	u32_t ccr, irqstatus;
	int ret;

	if (id >= DMA_STM32_MAX_STREAMS) {
		return -EINVAL;
	}

	/* Disable all IRQs */
	ccr = dma_stm32_read(ddata, DMA_STM32_CCR(id));
	ccr &= ~DMA_STM32_CCR_IRQ_MASK;
	dma_stm32_write(ddata, DMA_STM32_CCR(id), ccr);

	/* Disable stream */
	ret = dma_stm32_disable_stream(ddata, id);
	if (ret)
		return ret;

	/* Clear remanent IRQs from previous transfers */
	irqstatus = dma_stm32_irq_status(ddata, id);
	if (irqstatus) {
		dma_stm32_irq_clear(ddata, id, irqstatus);
	}

	/* Finally, flag stream as free */
	stream->busy = false;

	return 0;
}

static int dma_stm32_init(struct device *dev)
{
	struct dma_stm32_device *ddata = dev->driver_data;
	const struct dma_stm32_config *cdata = dev->config->config_info;
	int i;

	for (i = 0; i < DMA_STM32_MAX_STREAMS; i++) {
		ddata->stream[i].dev  = dev;
		ddata->stream[i].busy = false;
	}

	/* Enable DMA clock */
	ddata->clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);

	__ASSERT_NO_MSG(ddata->clk);

	clock_control_on(ddata->clk, (clock_control_subsys_t *) &cdata->pclken);

	/* Set controller specific configuration */
	cdata->config(ddata);

	return 0;
}

static const struct dma_driver_api dma_funcs = {
	.config		 = dma_stm32_config,
	.start		 = dma_stm32_start,
	.stop		 = dma_stm32_stop,
};

static void dma_stm32_irq_0(void *arg) { dma_stm32_irq_handler(arg, 0); }
static void dma_stm32_irq_1(void *arg) { dma_stm32_irq_handler(arg, 1); }
static void dma_stm32_irq_2(void *arg) { dma_stm32_irq_handler(arg, 2); }
static void dma_stm32_irq_3(void *arg) { dma_stm32_irq_handler(arg, 3); }
static void dma_stm32_irq_4(void *arg) { dma_stm32_irq_handler(arg, 4); }
static void dma_stm32_irq_5(void *arg) { dma_stm32_irq_handler(arg, 5); }
static void dma_stm32_irq_6(void *arg) { dma_stm32_irq_handler(arg, 6); }

const struct dma_stm32_config dma_stm32_1_cdata = {
	.pclken = { .bus = STM32_CLOCK_BUS_AHB1,
		    .enr = LL_AHB1_GRP1_PERIPH_DMA1 },
	.config = dma_stm32_1_config,
};

DEVICE_AND_API_INIT(dma_stm32_1, CONFIG_DMA_1_NAME, &dma_stm32_init,
		    &device_data[DMA_STM32_1], &dma_stm32_1_cdata,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    (void *)&dma_funcs);

static void dma_stm32_1_config(struct dma_stm32_device *ddata)
{
	ddata->base = DMA_STM32_1_BASE;

	IRQ_CONNECT(STM32L1_IRQ_DMA1_CH1, DMA_STM32_IRQ_PRI,
		    dma_stm32_irq_0, DEVICE_GET(dma_stm32_1), 0);
	irq_enable(STM32L1_IRQ_DMA1_CH1);

	IRQ_CONNECT(STM32L1_IRQ_DMA1_CH2, DMA_STM32_IRQ_PRI,
		    dma_stm32_irq_1, DEVICE_GET(dma_stm32_1), 0);
	irq_enable(STM32L1_IRQ_DMA1_CH2);

	IRQ_CONNECT(STM32L1_IRQ_DMA1_CH3, DMA_STM32_IRQ_PRI,
		    dma_stm32_irq_2, DEVICE_GET(dma_stm32_1), 0);
	irq_enable(STM32L1_IRQ_DMA1_CH3);

	IRQ_CONNECT(STM32L1_IRQ_DMA1_CH4, DMA_STM32_IRQ_PRI,
		    dma_stm32_irq_3, DEVICE_GET(dma_stm32_1), 0);
	irq_enable(STM32L1_IRQ_DMA1_CH4);

	IRQ_CONNECT(STM32L1_IRQ_DMA1_CH5, DMA_STM32_IRQ_PRI,
		    dma_stm32_irq_4, DEVICE_GET(dma_stm32_1), 0);
	irq_enable(STM32L1_IRQ_DMA1_CH5);

	IRQ_CONNECT(STM32L1_IRQ_DMA1_CH6, DMA_STM32_IRQ_PRI,
		    dma_stm32_irq_5, DEVICE_GET(dma_stm32_1), 0);
	irq_enable(STM32L1_IRQ_DMA1_CH6);

	IRQ_CONNECT(STM32L1_IRQ_DMA1_CH7, DMA_STM32_IRQ_PRI,
		    dma_stm32_irq_6, DEVICE_GET(dma_stm32_1), 0);
	irq_enable(STM32L1_IRQ_DMA1_CH7);
}

#if 0
/*
 * DMA2 not present in STM32L1X Cat.1 & Cat.2 devices
 */
static void dma_stm32_2_config(struct dma_stm32_device *ddata);

static const struct dma_stm32_config dma_stm32_2_cdata = {
	.pclken = { .bus = STM32_CLOCK_BUS_AHB1,
		    .enr = LL_AHB1_GRP1_PERIPH_DMA2 },
	.config = dma_stm32_2_config,
};

DEVICE_AND_API_INIT(dma_stm32_2, CONFIG_DMA_2_NAME, &dma_stm32_init,
		    &device_data[DMA_STM32_2], &dma_stm32_2_cdata,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    (void *)&dma_funcs);

static void dma_stm32_2_config(struct dma_stm32_device *ddata)
{
	ddata->base = DMA_STM32_2_BASE;

	IRQ_CONNECT(STM32L1_IRQ_DMA2_CH1, DMA_STM32_IRQ_PRI,
		    dma_stm32_irq_0, DEVICE_GET(dma_stm32_2), 0);
	irq_enable(STM32L1_IRQ_DMA2_CH1);

	IRQ_CONNECT(STM32L1_IRQ_DMA2_CH2, DMA_STM32_IRQ_PRI,
		    dma_stm32_irq_1, DEVICE_GET(dma_stm32_2), 0);
	irq_enable(STM32L1_IRQ_DMA2_CH2);

	IRQ_CONNECT(STM32L1_IRQ_DMA2_CH3, DMA_STM32_IRQ_PRI,
		    dma_stm32_irq_2, DEVICE_GET(dma_stm32_2), 0);
	irq_enable(STM32L1_IRQ_DMA2_CH3);

	IRQ_CONNECT(STM32L1_IRQ_DMA2_CH4, DMA_STM32_IRQ_PRI,
		    dma_stm32_irq_3, DEVICE_GET(dma_stm32_2), 0);
	irq_enable(STM32L1_IRQ_DMA2_CH4);

	IRQ_CONNECT(STM32L1_IRQ_DMA2_CH5, DMA_STM32_IRQ_PRI,
		    dma_stm32_irq_4, DEVICE_GET(dma_stm32_2), 0);
	irq_enable(STM32L1_IRQ_DMA2_CH5);
}

#endif
