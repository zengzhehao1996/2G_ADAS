#include "fork_sensor_hoare.h"
#include <kernel.h>
#include <board.h>
#include "stm32f4xx.h"
#include "stm32f4xx_hal_adc.h"
#include "stm32f4xx_hal_gpio.h"
#include "my_misc.h"
#include "hw_voltage.h"
#define  FORK_HOARE_RATE 60
#define  FORK_HOARE_ADC_NUM 10
#define FORK_HOARE_ADC_RUN_INTERVAL 1
static ADC_HandleTypeDef AdcIn0Handler;
static uint8_t forkHoareAdcCnt = 0;
static uint32_t forkHoareCurrSum = 0;
static uint32_t forkHoareCurr = 0;
static gpSensorTpye_t forkHoareConfig;
static sensorData_t  forkHoareData;

bool forkHoareSetup(gpSensorTpye_t* conf)
{
    GPIO_InitTypeDef GPIO_InitStruct;
	__HAL_RCC_ADC3_CLK_ENABLE();
	__HAL_RCC_GPIOF_CLK_ENABLE();
    GPIO_InitStruct.Pin = ADC_IN0_GPIO_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
    if(!adc_pin_setup(&AdcIn0Handler,3))
    {
		err_log("Failed to init AdcIn1.\n");
        return false;
    }
    forkHoareConfig = *conf;
    return true;
}
void forkHoareRun(void)
{
    int val = 0;
    int vol = 0;
    int curr = 0;
    if(forkHoareAdcCnt >= FORK_HOARE_ADC_NUM)
    {
        forkHoareCurr = 1.0 * forkHoareCurrSum / forkHoareAdcCnt;
        forkHoareCurrSum = 0;
        forkHoareAdcCnt = 0;
        if(forkHoareCurr > forkHoareConfig.fork_threshold * 1)
        {
            forkHoareData.forkStat = FORK_UP_DOWN;
            forkHoareData.goodWeight = 0;
            forkHoareData.carryStat = 0;

        }
        else
        {
            forkHoareData.forkStat = FORK_STOP;
            forkHoareData.goodWeight = 0;
            forkHoareData.carryStat = 0;

        }
        
    }
    else
    {
        val = adc_get_vol(&AdcIn0Handler, ADC_IN0_ADC_CHANNEL);
       // print_log(" fork AdcIn0Handler,val = %d \n",val);
    	vol = val*(51+10)*3300.0/4096/10;
       // print_log("  reality fork vol AdcIn0Handler,val = %d \n",vol);
        curr = vol * FORK_HOARE_RATE;
        forkHoareCurrSum += curr;
        forkHoareAdcCnt++;
    }
   // print_log("forkHoareData sensor\n");
  //  print_sensorData(&forkHoareData);

    
}
void getForkHoareData(sensorData_t* data)
{
    *data = forkHoareData;
}

uint32_t getForkHoareRunInterval(void)
{
    return FORK_HOARE_ADC_RUN_INTERVAL;
}

void getForkHoareCurr(uint32_t *current)
{
    *current = forkHoareCurr;

}

