#include "fork_updown_sensor_microSwitch.h"
#include "hw_gpio.h"
#include "board.h"
#include "stm32f4xx_hal_gpio.h"
#include "my_misc.h"

#define FORK_UPDOWN_SENSOR_INTERVAL_MS 1  
static struct device *g_gpio_hydraulic_status;   //EXT_IN_GPIO_PIN1
static sensorData_t forkData;
static bool origbuff[10] = {0};
bool forkUpdownMicroSwtichSetup(gpSensorTpye_t* conf)
{
    g_gpio_hydraulic_status = hwGpioPinInit(EXT_IN_GPIO_PORT, EXT_IN_GPIO_PIN2, GPIO_DIR_IN);
	if (!g_gpio_hydraulic_status) 
    {
		print_log("GPIO hydraulic configure failed.\n");
		return false;
	}
    memset(&forkData,0,sizeof(sensorData_t));
    return true;
}
void forkUpdownMicroSwtichRun(void)
{
    bool val = 0;
    int8_t ret = 0;
    gpio_pin_read(g_gpio_hydraulic_status,EXT_IN_GPIO_PIN1, &val);
    pushBoolArry(origbuff,sizeof(origbuff),val);
    ret = getBoolArryStat(origbuff,sizeof(origbuff));
    if(ret == 0)
    {
        forkData.forkStat = FORK_STOP;
        forkData.goodWeight = 0;
        forkData.carryStat = 0;
    }
    else if(ret == 1)
    {
        forkData.forkStat = FORK_UP_DOWN;
        forkData.goodWeight = 0;
        forkData.carryStat = 0;
    }
   // print_log("fork sensor\n");
    //print_sensorData(&forkData);

}
void getForkUpdownMicroSwtichData(sensorData_t* data)
{
     *data =  forkData;
}

uint32_t getForkUpdownMicroSwtichRunInterval(void)
{
    return FORK_UPDOWN_SENSOR_INTERVAL_MS;
}


