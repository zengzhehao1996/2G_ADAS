/*
 * Copyright (c) 2017 Linaro, Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>
#ifdef CONFIG_BOARD_AIDONG_LINDE429V13
#include <dfu/mcuboot.h>
#include "bootfm.h"
int swap_type;
#endif

#define TEST_IMAGE_ADDR		0x80000
#define TEST_SLOT1_ADDR		0x40000
#define TEST_IMAGE_SIZE		0x20000
#define CHUNK_SIZE		512

void main(void)
{
#ifdef CONFIG_BOARD_AIDONG_LINDE429V13
	unsigned char buf[CHUNK_SIZE];
	uint32_t off;
	uint32_t delay_secs;
#endif
	
	printk("Hello World from %s on %s!\n",
	       MCUBOOT_HELLO_WORLD_FROM, CONFIG_BOARD);
	
#ifdef CONFIG_BOARD_AIDONG_LINDE429V13
	if (bootfm_init() != 0) {
		printk("bootfm_init() failed\n");
		return;
	}
	
	swap_type = boot_swap_type();
	printk("swap_type: %d\n", swap_type);
	switch (swap_type) {
	case BOOT_SWAP_TYPE_NONE:
		printk("swap_type: none\n");
		if (bootfm_check_magic(TEST_IMAGE_ADDR) == 0) {
			boot_erase_img_bank(FLASH_AREA_IMAGE_1_OFFSET);
			for (off = 0; off < TEST_IMAGE_SIZE; off += CHUNK_SIZE) {
				bootfm_flash_read(TEST_IMAGE_ADDR + off, buf, CHUNK_SIZE);
				bootfm_flash_write(TEST_SLOT1_ADDR + off, buf, CHUNK_SIZE);
			}
			boot_request_upgrade(0);
			printk("copied image in 0x80000 to slot1\n");
			k_sleep(1000);
			sys_reboot(0);
		} else {
			printk("bootfm_check_magic() failed\n");
		}
		break;

	case BOOT_SWAP_TYPE_REVERT:
		printk("Mark image okay, and thus the image will be run contineously.\n");
		bootfm_set_confirmed();
		break;
	}
	delay_secs = 17;
	printk("reboot after about %d seconds\n", delay_secs);
	k_sleep(1000 * delay_secs);
	sys_reboot(0);
#endif /* CONFIG_BOARD_AIDONG_LINDE429V13 */
}
