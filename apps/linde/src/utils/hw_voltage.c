/*
 * hw_batvol.c
 *
 *  Created on: Nov 15, 2017
 *      Author: root
 */

#include <board.h>
#include "stm32f4xx.h"
#include "stm32f4xx_hal_adc.h"
#include "stm32f4xx_hal_gpio.h"
#include "hw_voltage.h"
#include "my_misc.h"


bool adc_pin_setup(ADC_HandleTypeDef *pHandler, int adc_index)
{
	switch(adc_index){
    case 1:
			pHandler->Instance = ADC1;
		break;
		case 2:
			pHandler->Instance = ADC2;
		break;
		case 3:
			pHandler->Instance = ADC3;
		break;
		default:return FALSE;
	}
  
	if (HAL_ADC_DeInit(pHandler) != HAL_OK)
		return false;

	pHandler->Init.ClockPrescaler        = ADC_CLOCKPRESCALER_PCLK_DIV4;          /* Asynchronous clock mode, input ADC clock not divided */
	pHandler->Init.Resolution            = ADC_RESOLUTION_12B;             /* 12-bit resolution for converted data */
	pHandler->Init.DataAlign             = ADC_DATAALIGN_RIGHT;           /* Right-alignment for converted data */
	pHandler->Init.ScanConvMode          = DISABLE;                       /* Sequencer disabled (ADC conversion on only 1 channel: channel set on rank 1) */
	pHandler->Init.EOCSelection          = DISABLE;           			/* EOC flag picked-up to indicate conversion end */
	pHandler->Init.ContinuousConvMode    = DISABLE;                       /* Continuous mode disabled to have only 1 conversion at each conversion trig */
	pHandler->Init.NbrOfConversion       = 1;                             /* Parameter discarded because sequencer is disabled */
	pHandler->Init.DiscontinuousConvMode = DISABLE;                       /* Parameter discarded because sequencer is disabled */
	pHandler->Init.NbrOfDiscConversion   = 0;                             /* Parameter discarded because sequencer is disabled */
	pHandler->Init.ExternalTrigConv      = ADC_SOFTWARE_START;            /* Software start to trig the 1st conversion manually, without external event */
	pHandler->Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE; /* Parameter discarded because software trigger chosen */
	pHandler->Init.DMAContinuousRequests = DISABLE;                       /* DMA one-shot mode selected (not applied to this example) */

	if (HAL_ADC_Init(pHandler) != HAL_OK){
		err_log("Failed to init Handler.\n");
		return false;
	}

  return true;
}

/**
 * In the AiDong Linde-PA board, pin PA1 act as ADC (ADC1 channel 1) to detect
 * the voltage of battery.  The ADC is configured 12-bit, and the value detected
 * is 1/2 of 'battery voltage'.
 *
 * @return value in mV; or negative value on errors.
 */
int adc_get_vol(ADC_HandleTypeDef *pHandler, unsigned int Channel)
{
	int val = 0;
	ADC_ChannelConfTypeDef ADC_ChanConf;
	ADC_ChanConf.Channel=Channel;
	ADC_ChanConf.Rank=1;
	ADC_ChanConf.SamplingTime=ADC_SAMPLETIME_3CYCLES;
	ADC_ChanConf.Offset=0;
	if (HAL_ADC_ConfigChannel(pHandler, &ADC_ChanConf) != HAL_OK){
    err_log("Fail read Channel [ %d ] voltage.\n", Channel);
		return 0;
  }

	if (HAL_ADC_Start(pHandler) != HAL_OK){
    err_log("Fail read Channel [ %d ] voltage.\n", Channel);
		return 0;
	}

	if (HAL_ADC_PollForConversion(pHandler, 100) != HAL_OK){
    err_log("Fail read Channel [ %d ] voltage.\n", Channel);
		return 0;
	}

	val = HAL_ADC_GetValue(pHandler);
	HAL_ADC_Stop(pHandler);

	return val;
}

