#ifndef __STM324xx_SDIO_MMC_H
#define __STM324xx_SDIO_MMC_H

/* Includes ------------------------------------------------------------------*/
#include <stm32f4xx.h>
	
#include "diskio.h"
#include "integer.h"

#include "stm32f4xx.h"
#include "stm32f4xx_hal_rcc.h"
#include "stm32f4xx_hal_gpio.h"
#include "defines.h"

//#include "tm_stm32f4_delay.h"
#include "tm_stm32f4_fatfs.h"
#include "tm_stm32f4_gpio.h"
#include "tm_stm32f4_dma.h"

//#include "stm32f4xx_ll_sdmmc.h"

#define SD_PRESENT                                 ((uint8_t)0x01)
#define SD_NOT_PRESENT                             ((uint8_t)0x00)

#define BLOCK_SIZE                                  512
#endif

