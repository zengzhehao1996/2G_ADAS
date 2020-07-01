#include "move_sensor_hoare.h"
#include <kernel.h>
#include <board.h>
#include "stm32f4xx.h"
#include "stm32f4xx_hal_adc.h"
#include "stm32f4xx_hal_gpio.h"
#include "my_misc.h"
#include "hw_voltage.h"
#define  MOVE_HOARE_RATE 60
#define  MOVE_HOARE_ADC_NUM 10
#define MOVE_HOARE_INTERVAL 1
#define MOVE_HOARE_LEVEL_MIN (15*1000)
static ADC_HandleTypeDef AdcIn1Handler;
static uint8_t moveHoareAdcCnt = 0;
static uint32_t moveHoareCurrSum = 0;
static uint32_t moveHoareCurr = 0;
static gpSensorTpye_t moveHoareConfig;
sensorData_t moveHoareData;

bool moveHoareSetup(gpSensorTpye_t* conf)
{
    GPIO_InitTypeDef GPIO_InitStruct;
	__HAL_RCC_ADC3_CLK_ENABLE();
	__HAL_RCC_GPIOF_CLK_ENABLE();
    GPIO_InitStruct.Pin = ADC_IN1_GPIO_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
    if(!adc_pin_setup(&AdcIn1Handler,3))
    {
		err_log("Failed to init AdcIn1.\n");
        return false;
    }
    moveHoareConfig = *conf;
    return true;
}
void moveHoareRun(void)
{
    int val = 0;
    int vol = 0;
    int curr = 0;
    if(moveHoareAdcCnt >= MOVE_HOARE_ADC_NUM)
    {
        moveHoareCurr = 1.0 * moveHoareCurrSum / moveHoareAdcCnt;
        moveHoareCurrSum = 0;
        moveHoareAdcCnt = 0;
        if(moveHoareCurr > moveHoareConfig.move_threshold *1)
        {
            moveHoareData.speed = 0;
            moveHoareData.moveStat = MOVE_FORWARD;
            if(moveHoareCurr > moveHoareConfig.move_threshold *2 && moveHoareCurr > MOVE_HOARE_LEVEL_MIN)
            {
                moveHoareData.moveLevel = MOVE_LEVEL_FAST;
            }
            else
            {
                moveHoareData.moveLevel = MOVE_LEVEL_SLOW;
            }
        }
        else
        {
            moveHoareData.speed = 0;
            moveHoareData.moveStat = MOVE_STOP;
            moveHoareData.moveLevel = MOVE_LEVEL_STOP;          
        }
        
    }
    else
    {
        val = adc_get_vol(&AdcIn1Handler, ADC_IN1_ADC_CHANNEL);
        //print_log(" move AdcIn1Handler,val = %d \n",val);
    	vol = val*(51+10)*3300.0/4096/10;
        //print_log(" reality move  vol AdcIn1Handler,val = %d \n",vol);
        curr = vol * MOVE_HOARE_RATE;
       // print_log(" reality move  current AdcIn1Handler,val = %d \n",curr);
        moveHoareCurrSum += curr;
        moveHoareAdcCnt++;
        //print_log("");
    }
    //print_log("moveHoareData sensor\n");
    //print_sensorData(&moveHoareData);

    
}
void getmoveHoareData(sensorData_t* data)
{
    *data = moveHoareData;
}
uint32_t getMoveHoareRunInterval(void)
{
    return MOVE_HOARE_INTERVAL;
}

void getMoveHoareCurr(uint32_t *current)
{
    *current = moveHoareCurr;
}

