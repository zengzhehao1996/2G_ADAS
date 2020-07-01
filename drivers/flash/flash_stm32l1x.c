/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <string.h>
#include <flash.h>
#include <init.h>
#include <soc.h>

#include "flash_stm32.h"
#define RAMFUNC __attribute__ ((long_call, section (".datas")))

//#define USERAMFUNC
//#define MULTIWORDPROG

#if 0
#define FLASH_PDKEY1               (0x04152637U) /*!< Flash power down key1 */
#define FLASH_PDKEY2               (0xFAFBFCFDU) /*!< Flash power down key2: used with FLASH_PDKEY1 
                                                              to unlock the RUN_PD bit in FLASH_ACR */

#define FLASH_PEKEY1               (0x89ABCDEFU) /*!< Flash program erase key1 */
#define FLASH_PEKEY2               (0x02030405U) /*!< Flash program erase key: used with FLASH_PEKEY2
                                                               to unlock the write access to the FLASH_PECR register and
                                                               data EEPROM */

#define FLASH_PRGKEY1              (0x8C9DAEBFU) /*!< Flash program memory key1 */
#define FLASH_PRGKEY2              (0x13141516U) /*!< Flash program memory key2: used with FLASH_PRGKEY2
                                                               to unlock the program memory */

#define FLASH_OPTKEY1              (0xFBEAD9C8U) /*!< Flash option key1 */
#define FLASH_OPTKEY2              (0x24252627U) /*!< Flash option key2: used with FLASH_OPTKEY1 to
                                                              unlock the write access to the option byte block */

#define FLASH_PECR_PELOCK          (0x1U<<0)             /*!< FLASH_PECR and Flash data Lock */
#define FLASH_PECR_PRGLOCK         (0x1U<<1)            /*!< Program matrix Lock */
#endif

#define FLASH_PAGE_SIZE            (256U)


#define SET_BIT(REG, BIT)     ((REG) |= (BIT))
#define CLEAR_BIT(REG, BIT)   ((REG) &= ~(BIT))

#define FLASH_GET_FLAG(SR, __FLAG__) ((SR & (__FLAG__)) == (__FLAG__))


bool flash_stm32_valid_range(struct device *dev, off_t offset, u32_t len,
			     bool write)
{
	ARG_UNUSED(write);

	return flash_stm32_range_exists(dev, offset, len);
}

#ifndef MULTIWORDPROG
static int write_word(struct device *dev, off_t offset, u32_t val)
{
	struct stm32l1x_flash *regs = FLASH_STM32_REGS(dev);
	u32_t tmp;
	int rc;

	if (regs->pecr & FLASH_PECR_PELOCK) { //if PRGLOCK
        if (regs->pecr & FLASH_PECR_PELOCK) { //if PELOCK
            regs->pekeyr = FLASH_PEKEY1;
            regs->pekeyr = FLASH_PEKEY2;            
        }

        regs->prgkeyr = FLASH_PRGKEY1;
        regs->prgkeyr = FLASH_PRGKEY2;            
	}
    
	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}
    *(__IO uint32_t *)(uint32_t)(offset + CONFIG_FLASH_BASE_ADDRESS) = val;

	rc = flash_stm32_wait_flash_idle(dev);

    SET_BIT(regs->pecr, FLASH_PECR_PELOCK);
	return rc;
}
#else 
static int write_word(struct stm32l1x_flash *regs, off_t offset, u32_t val)
{
	return 0;
}
#endif

#ifdef USERAMFUNC  

RAMFUNC static int ram_flash_stm32_wait_flash_idle(struct stm32l1x_flash *regs) 
{
    while (FLASH_GET_FLAG(regs->sr, FLASH_FLAG_BSY)) {
        
    }
    
    if (FLASH_GET_FLAG(regs->sr,FLASH_FLAG_EOP)) {
        regs->sr = FLASH_FLAG_EOP;
    }
    
    if ((FLASH_GET_FLAG(regs->sr,FLASH_FLAG_WRPERR)) ||
        (FLASH_GET_FLAG(regs->sr,FLASH_FLAG_OPTVERR)) ||
        (FLASH_GET_FLAG(regs->sr,FLASH_FLAG_PGAERR))) {
            return -1;
    }
    return 0;
}

RAMFUNC static int write_halfpage(struct device *dev, off_t offset, u32_t *data)
{
	struct stm32l1x_flash *regs = FLASH_STM32_REGS(dev);
	u32_t tmp;
	int rc = 0;
    u32_t key;
    
	if (regs->pecr & FLASH_PECR_PELOCK) { //if PRGLOCK
        if (regs->pecr & FLASH_PECR_PELOCK) { //if PELOCK
            regs->pekeyr = FLASH_PEKEY1;
            regs->pekeyr = FLASH_PEKEY2;            
        }

        regs->prgkeyr = FLASH_PRGKEY1;
        regs->prgkeyr = FLASH_PRGKEY2;            
	}

    key = irq_lock();
    SET_BIT(SCnSCB->ACTLR, SCnSCB_ACTLR_DISMCYCINT_Msk);
    rc = ram_flash_stm32_wait_flash_idle(regs);
    if (rc<0) {
        irq_unlock(key);
        return rc;
    }
    SET_BIT(regs->pecr, FLASH_PECR_FPRG);
    SET_BIT(regs->pecr, FLASH_PECR_PROG);
    tmp = 0;
    while (tmp < 32) {
        *(__IO uint32_t *)(uint32_t)(offset + CONFIG_FLASH_BASE_ADDRESS) = *data;
        data++;
        tmp++;
    }
    rc = ram_flash_stm32_wait_flash_idle(regs);
    CLEAR_BIT(regs->pecr, FLASH_PECR_PROG);
    CLEAR_BIT(regs->pecr, FLASH_PECR_FPRG);
    CLEAR_BIT(SCnSCB->ACTLR, SCnSCB_ACTLR_DISMCYCINT_Msk);    
    irq_unlock(key);
    SET_BIT(regs->pecr, FLASH_PECR_PELOCK);
	return rc;
}
#endif

static int erase_page(struct device *dev, struct stm32l1x_flash *regs, u32_t PageAddress) 
{   
    int rc = 0;
    
    SET_BIT(regs->pecr, FLASH_PECR_ERASE);
    SET_BIT(regs->pecr, FLASH_PECR_PROG);
    *(__IO uint32_t *)(uint32_t)(PageAddress & ~(FLASH_PAGE_SIZE - 1)) = 0x00000000;

	rc = flash_stm32_wait_flash_idle(dev);
    CLEAR_BIT(regs->pecr, FLASH_PECR_PROG);
    CLEAR_BIT(regs->pecr, FLASH_PECR_ERASE);
    return rc;
}

static int erase_sector(struct device *dev, u32_t sector)
{
	struct stm32l1x_flash *regs = FLASH_STM32_REGS(dev);
	u32_t i;
	int rc;

	if (regs->pecr & FLASH_PECR_PELOCK) { //if PRGLOCK
        if (regs->pecr & FLASH_PECR_PELOCK) { //if PELOCK
            regs->pekeyr = FLASH_PEKEY1;
            regs->pekeyr = FLASH_PEKEY2;            
        }

        regs->prgkeyr = FLASH_PRGKEY1;
        regs->prgkeyr = FLASH_PRGKEY2;            
	}
    
	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}

    erase_page(dev, regs, sector*256+CONFIG_FLASH_BASE_ADDRESS);
    
    SET_BIT(regs->pecr, FLASH_PECR_PELOCK);

	return rc;
}

int flash_stm32_block_erase_loop(struct device *dev, unsigned int offset,
				 unsigned int len)
{
	struct flash_pages_info info;
	u32_t start_sector, end_sector;
	u32_t i;
	int rc = 0;

	rc = flash_get_page_info_by_offs(dev, offset, &info);
	if (rc) {
		return rc;
	}
	start_sector = info.index;
	rc = flash_get_page_info_by_offs(dev, offset + len - 1, &info);
	if (rc) {
		return rc;
	}
	end_sector = info.index;

	for (i = start_sector; i <= end_sector; i++) {
		rc = erase_sector(dev, i);
		if (rc < 0) {
			break;
		}
	}

	return rc;
}

#ifndef MULTIWORDPROG
int flash_stm32_write_range(struct device *dev, unsigned int offset,
			    const void *data, unsigned int len)
{
	int i, rc = 0;
#ifdef USERAMFUNC  
    int pages = len>>7;

    for (i = 0; i < (pages<<7); i+=(1<<7), offset+=(1<<7)) {        
        rc = write_halfpage(dev, offset, ((const u32_t *) data)+(i>>7));
        if (rc<0) {
            return rc;
        }
    } 
	for (; i < len; i+=4, offset+=4) {
#else        
	for (i=0; i < len; i+=4, offset+=4) {
#endif          
		rc = write_word(dev, offset, ((const u32_t *) data)[i>>2]);
		if (rc < 0) {
			return rc;
		}
	}

	return rc;
}

#else
    
int flash_stm32_write_range(struct device *dev, unsigned int offset,
			    const void *data, unsigned int len)
{
	int i, rc = 0;

	struct stm32l1x_flash *regs = FLASH_STM32_REGS(dev);
	if (regs->pecr & FLASH_PECR_PELOCK) { //if PRGLOCK
        if (regs->pecr & FLASH_PECR_PELOCK) { //if PELOCK
            regs->pekeyr = FLASH_PEKEY1;
            regs->pekeyr = FLASH_PEKEY2;            
        }

        regs->prgkeyr = FLASH_PRGKEY1;
        regs->prgkeyr = FLASH_PRGKEY2;            
	}
    
	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}

	for (i=0; i < len; i+=4, offset+=4) {
        *(__IO uint32_t *)(uint32_t)(offset + CONFIG_FLASH_BASE_ADDRESS) = ((const u32_t *) data)[i>>2];
        rc = flash_stm32_wait_flash_idle(dev);
		if (rc < 0) {
			return rc;
		}
	}
    SET_BIT(regs->pecr, FLASH_PECR_PELOCK);

	return rc;
}
#endif

//STM32Fl51CC
static const struct flash_pages_layout stm32l1_flash_layout[] = {
	{.pages_count = 1024, .pages_size = 256}
};

void flash_stm32_page_layout(struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size)
{
	ARG_UNUSED(dev);

	*layout = stm32l1_flash_layout;
	*layout_size = ARRAY_SIZE(stm32l1_flash_layout);
}
