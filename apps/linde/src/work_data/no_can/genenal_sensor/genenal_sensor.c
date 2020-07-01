#include "genenal_sensor.h"
#include <kernel.h>
#include <board.h>
#include "stm32f4xx.h"
#include "stm32f4xx_hal_adc.h"
#include "stm32f4xx_hal_gpio.h"
#include "my_misc.h"
#include "hw_voltage.h"
#include "batter_stat_alg.h"
#include "config.h"
#define GENE_ADC_RUN_INTERVAL 1
#define SEAT_ADC_NUM 10
#define KEY_ADC_NUM 10
#define BATTERY_ADC_NUM 10
#define SEAT_ON_HIGH_VOL 3000
#define SEAT_ON_LOW_VOL 3000
#define KEY_ON_VOL 5000
//seat vol
static ADC_HandleTypeDef AdcIn3Handler;
//key vol
static ADC_HandleTypeDef AdcKeyValHandler;
//battery vol 
static ADC_HandleTypeDef AdcBatValHandler;

static uint8_t seatAdcCnt = 0;
static uint32_t seatVolSum = 0;
static uint32_t seatVol = 0;

static uint8_t keyAdcCnt = 0;
static uint32_t keyVolSum = 0;
static uint32_t keyVol = 0;

static uint8_t batteryAdcCnt = 0;
static uint32_t batteryVolSum = 0;
static uint32_t batteryVol = 0;


static gpSensorTpye_t genelSensorConfig;
sensorData_t generalSensorData;

static void seatSensorParse();
static void keySensorParse();
static void batterySensorParse();



bool genenalSensorSetup(gpSensorTpye_t* conf)
{
    GPIO_InitTypeDef GPIO_InitStruct;
	__HAL_RCC_ADC3_CLK_ENABLE();
	__HAL_RCC_GPIOF_CLK_ENABLE();
    GPIO_InitStruct.Pin = ADC_IN3_GPIO_PIN| VOL_BAT_GPIO_PIN | VOL_KEY_GPIO_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
    //seat 
    if(!adc_pin_setup(&AdcIn3Handler,3))
    {
		err_log("Failed to init AdcIn1.\n");
        return false;
    }
    if(!adc_pin_setup(&AdcBatValHandler,3))
    {
        err_log("Failed to init battery Adc.\n");
        return false;
    }
    if(!adc_pin_setup(&AdcKeyValHandler,3))
    {
        err_log("Failed to init key Adc.\n");
        return false;
    }
    genelSensorConfig = *conf;
    return true;
}
void genenalSensorRun(void)
{
    seatSensorParse();
    keySensorParse();
    batterySensorParse();    
}
void getGenenalSensorData(sensorData_t* data)
{
    *data = generalSensorData;
}

uint32_t getGenenalRunInterval(void)
{
    return GENE_ADC_RUN_INTERVAL;
}


static void seatSensorParse()
{
    int val = 0;
    int vol = 0;
    int8_t seat = SEAT_ON;
    if(seatAdcCnt >= SEAT_ADC_NUM)
    {
        seatVol = 1.0* seatVolSum/seatAdcCnt;
        seatAdcCnt = 0;
        seatVolSum = 0;
        //print_log("#######################seat vol is %d\n",seatVol);
        if(genelSensorConfig.seatType == SEAT_ON_LOW)
        {
            if(seatVol <SEAT_ON_LOW_VOL)
            {
                seat = SEAT_ON;
                //generalSensorData.seatStat = SEAT_ON;
            }
            else
            {
                seat = SEAT_OFF;
                //generalSensorData.seatStat = SEAT_OFF;
            }

        }
        else if(genelSensorConfig.seatType == SEAT_ON_HIGH)
        {
            if(seatVol >SEAT_ON_HIGH_VOL)
            {
                seat = SEAT_ON;
                //generalSensorData.seatStat = SEAT_ON;
            }
            else
            {
                seat = SEAT_OFF;
                //generalSensorData.seatStat = SEAT_OFF;
            }
        }
        else if(genelSensorConfig.seatType == SEAT_ON_INVAL)
        {
            int8_t seatAdaptStat = 0;
            seat = SEAT_ON;
            //seatVol upload
            if(seatVol >= SEAT_ON_HIGH_VOL)
            {
                seatAdaptStat = 1;
            }
            else if(seatVol <= SEAT_ON_LOW_VOL)
            {
                seatAdaptStat = 2;
            }
            generalSensorData.seatAdapStat = seatAdaptStat;
            
        }
        else
        {
            seat = SEAT_ON;
            //seatVol upload
            int8_t seatAdaptStat = 0;
            if(seatVol >= SEAT_ON_HIGH_VOL)
            {
                seatAdaptStat = 1;
            }
            else if(seatVol <= SEAT_ON_LOW_VOL)
            {
                seatAdaptStat = 2;
            }
            generalSensorData.seatAdapStat = seatAdaptStat;
        }
        generalSensorData.seatStat = seat;
                
    }
    else
    {
        val = adc_get_vol(&AdcIn3Handler, ADC_IN3_ADC_CHANNEL);
    	vol = val*(899)*3300.0/4096/33;
        //print_log("  reality ADC_IN3_ADC_CHANNEL vol AdcIn0Handler,val = %d \n",vol);
        seatVolSum += vol;
        seatAdcCnt ++;
    }
 // print_log("generalSensorData.seatStat[%d]\n",generalSensorData.seatStat);  
}

/////////////////////////////////
static void batterySensorParse()
{
    int val = 0;
    int vol = 0;
    if(batteryAdcCnt >= BATTERY_ADC_NUM)
    {
        batteryVol = 1.0* batteryVolSum/batteryAdcCnt;
        batteryAdcCnt = 0;
        batteryVolSum = 0;
        generalSensorData.battery_stat = getBatteryStat(batteryVol);
        generalSensorData.battery_volt = batteryVol; 
        
    }
    else
    {
        val = adc_get_vol(&AdcIn3Handler, VOL_BAT_ADC_CHANNEL);
    	vol = val*(899)*3300.0/4096/33;
        //print_log("  reality VOL_BAT_ADC_CHANNEL vol AdcIn0Handler,val = %d \n",vol);
        batteryVolSum += vol;
        batteryAdcCnt ++;
    }
     //print_log("generalSensorData.battery[%d]\n",generalSensorData.battery_volt);
}

static void keySensorParse()
{
    int val = 0;
    int vol = 0;
    if(keyAdcCnt >= SEAT_ADC_NUM)
    {
        keyVol = 1.0* keyVolSum/keyAdcCnt;
        keyAdcCnt = 0;
        keyVolSum = 0;
        if(keyVol > KEY_ON_VOL)
        {
            generalSensorData.keyStat = KEY_ON;
        }
        else
        {
            generalSensorData.keyStat = KEY_OFF;
        }        
    }
    else
    {
        val = adc_get_vol(&AdcIn3Handler, VOL_KEY_ADC_CHANNEL);
    	vol = val*(899)*3300.0/4096/33;
        //print_log("  reality VOL_KEY_ADC_CHANNEL vol AdcIn0Handler,val = %d \n",vol);
        keyVolSum += vol;
        keyAdcCnt ++;
    }
    //print_log("generalSensorData.keyStat[%d]\n",generalSensorData.keyStat);
}
void setSeatOnType(int8_t seat)
{
    genelSensorConfig.seatType = seat;
}
void getSeatOnType(int8_t *seat)
{
    //*seat = gSeatTypeConfig.seatType;
    *seat = genelSensorConfig.seatType;

}
void getSeatVolt(uint8_t* vol)
{
    *vol = seatVol/1000;
}
void getKeyVolt(uint8_t* vol)
{
    if(keyAdcCnt)
    {
        *vol = keyVolSum/keyAdcCnt/1000;

    }
    else
    {
        *vol = 0;
    }

}


