/*
 * Copyright (c) 2018 Rainbonic Technology Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <misc/printk.h>
#include <flash.h>
#include <dfu/mcuboot.h>
#include "bootfm.h"

#ifdef CONFIG_BOARD_AIDONG_LINDE429V13

#define BOOT_MAGIC_VAL_W0 0xf395c277
#define BOOT_MAGIC_VAL_W1 0x7fefd260
#define BOOT_MAGIC_VAL_W2 0x0f505235
#define BOOT_MAGIC_VAL_W3 0x8079b62c
#define BOOT_MAGIC_VALUES {BOOT_MAGIC_VAL_W0, BOOT_MAGIC_VAL_W1,\
			   BOOT_MAGIC_VAL_W2, BOOT_MAGIC_VAL_W3 }

const u32_t img_magic[4] = BOOT_MAGIC_VALUES;
static struct device *flash_dev;

int bootfm_init(void)
{
	flash_dev = device_get_binding(FLASH_DEV_NAME);
	return 0;
}

int bootfm_flash_read(uint32_t off, void *dst, uint32_t len)
{
	return flash_read(flash_dev, off, dst, len);
}

int bootfm_flash_write(uint32_t off, const void *src, uint32_t len)
{
	int rc = 0;
	
	flash_write_protection_set(flash_dev, false);
	rc = flash_write(flash_dev, off, src, len);
	flash_write_protection_set(flash_dev, true);
	return rc;
}

int bootfm_check_magic(uint32_t off)
{
	u32_t readout[ARRAY_SIZE(img_magic)];
	int ret;

	if ((ret = flash_read(flash_dev,
					off + FLASH_AREA_IMAGE_0_SIZE - sizeof(img_magic),
					&readout,
					sizeof(img_magic))) != 0)
		return -1;

	if (memcmp(img_magic, readout, sizeof(img_magic)) != 0)
		return -1;

	return 0;
}

int bootfm_set_confirmed(void)
{
	u32_t readout;
	int ret;
	
	if ((ret = bootfm_check_magic(FLASH_AREA_IMAGE_0_OFFSET)) != 0)
		return -1;

	if ((ret = bootfm_flash_write(FLASH_AREA_IMAGE_0_OFFSET
						+ FLASH_AREA_IMAGE_0_SIZE - 16,
						img_magic, 16)) != 0)
		return -1;

	if ((ret = boot_write_img_confirmed()) != 0)
		return -1;

	if ((ret = flash_read(flash_dev, FLASH_AREA_IMAGE_0_OFFSET +
					FLASH_AREA_IMAGE_0_SIZE - 24, &readout,
					sizeof(readout))) != 0)
		return -1;

	if ((readout & 0xff) != 1)
		return -1;

	return 0;
}

#endif /* CONFIG_BOARD_AIDONG_LINDE429V13 */
