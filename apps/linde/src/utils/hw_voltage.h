/*
 * hw_batvol.h
 *
 *  Created on: Nov 15, 2017
 *      Author: root
 */
#ifndef HW_VOLTAGE
#define HW_VOLTAGE

#include "stm32f4xx_hal_adc.h"
#include "kernel.h"
bool adc_pin_setup(ADC_HandleTypeDef *pHandler, int type);
int adc_get_vol(ADC_HandleTypeDef *pHandler, unsigned int Channel);

#endif //HW_VOLTAGE
