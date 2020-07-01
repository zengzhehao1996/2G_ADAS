#include <board.h>
#include "stm32f4xx.h"
#include "stm32f4xx_hal_adc.h"
#include "stm32f4xx_hal_gpio.h"
#include "v141_adc_test.h"
#include "hw_voltage.h"
#include "my_misc.h"
#include "board.h"

static ADC_HandleTypeDef AdcBatteryHandler;
static ADC_HandleTypeDef AdcPowerHandler;
static ADC_HandleTypeDef AdcBatValHandler;
static ADC_HandleTypeDef AdcKeyValHandler;
static ADC_HandleTypeDef Adcin0Handler;
static ADC_HandleTypeDef AdcIn1Handler;
static ADC_HandleTypeDef AdcIn2Handler;
static ADC_HandleTypeDef AdcIn3Handler;

static int get_avg_val(int (*p)(void),int num);

bool volt_setup(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	__HAL_RCC_ADC3_CLK_ENABLE();
	__HAL_RCC_GPIOF_CLK_ENABLE();
	//3 is capacity volt. 5 is ext power in.
	GPIO_InitStruct.Pin = VOL_CAP_GPIO_PIN | VOL_VIN_GPIO_PIN | VOL_BAT_GPIO_PIN | VOL_KEY_GPIO_PIN | ADC_IN0_GPIO_PIN | ADC_IN1_GPIO_PIN | ADC_IN2_GPIO_PIN | ADC_IN3_GPIO_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

	if(!adc_pin_setup(&AdcBatteryHandler,3))
  {
		err_log("Failed to init CapAdc.\n");
    return false;
  }
  if(!adc_pin_setup(&AdcPowerHandler,3))
  {
		err_log("Failed to init PowerAdc.\n");
    return false;
  }
  if(!adc_pin_setup(&AdcBatValHandler,3))
  {
		err_log("Failed to init PowerAdc.\n");
    return false;
  }
  if(!adc_pin_setup(&AdcKeyValHandler,3))
  {
		err_log("Failed to init PowerAdc.\n");
    return false;
  }
	if(!adc_pin_setup(&Adcin0Handler,3))
  {
		err_log("Failed to init AdcIn0.\n");
    return false;
  }
  if(!adc_pin_setup(&AdcIn1Handler,3))
  {
		err_log("Failed to init AdcIn1.\n");
    return false;
  }
	if(!adc_pin_setup(&AdcIn2Handler,3))
  {
		err_log("Failed to init AdcIn1.\n");
    return false;
  }
	if(!adc_pin_setup(&AdcIn3Handler,3))
  {
		err_log("Failed to init AdcIn1.\n");
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

int volt_get_bat(void)
{
	int val;
  val = adc_get_vol(&AdcBatteryHandler, VOL_CAP_ADC_CHANNEL);
	return (val * 6600.0 + 2048) / 4096;
}

int volt_get_power(void){
  int val=0;
  val = adc_get_vol(&AdcPowerHandler, VOL_VIN_ADC_CHANNEL);
	return (val*866*3300.0/4096/33);
}

int volt_get_bat_ext(void)
{
  int val = 0;
  val = adc_get_vol(&AdcBatValHandler, VOL_BAT_ADC_CHANNEL);
  return (val*899*3300.0/4096/33);//mqy modify 2018/8/24
}

int volt_get_key_ext(void)
{
  int val = 0;
  val = adc_get_vol(&AdcKeyValHandler, VOL_KEY_ADC_CHANNEL);
	return (val*899*3300.0/4096/33);//mqy modify 2018/8/24
}

int volt_get_hydraulic_ext(void)
{
	int val = 0;
  val = adc_get_vol(&Adcin0Handler, ADC_IN0_ADC_CHANNEL);
	return (val *61*3300.0/4096/10);
}

int volt_get_forward_ext(void)
{
	int val = 0;
  val = adc_get_vol(&AdcIn1Handler, ADC_IN1_ADC_CHANNEL);
	return (val*61*3300.0/4096/10);
}

int volt_get_backward_ext(void)
{
	int val = 0;
  val = adc_get_vol(&AdcIn2Handler, ADC_IN2_ADC_CHANNEL);
	return (val*899*3300.0/4096/33);
}

int volt_get_adc_seat(void)
{	
	int val;
  val = adc_get_vol(&AdcIn3Handler, ADC_IN3_ADC_CHANNEL);
	return (val*899*3300.0/4096/33);
}

