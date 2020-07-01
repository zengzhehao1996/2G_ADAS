#include "move_sensor_microSwitch.h"
#include "hw_gpio.h"
#include "board.h"
#include "my_misc.h"

#define MOVE_MICRO_SWITCH_SENSOR_INTERVAL_MS 1
static struct device *g_gpio_forward_status;   //EXT_IN_GPIO_PIN2
static sensorData_t movData;
static bool origbuff[10] = {0};
bool moveMicroSwtichSetup(gpSensorTpye_t* conf)
{
    print_log("moveMicroSwtichSetup \n ");
    g_gpio_forward_status = hwGpioPinInit(EXT_IN_GPIO_PORT, EXT_IN_GPIO_PIN2, GPIO_DIR_IN);
	if (!g_gpio_forward_status) 
    {
		print_log("GPIO Forward configure failed.\n");
		return false;
	}
    memset(&movData,0,sizeof(sensorData_t));
    return true;
}
void moveMicroSwtichRun(void)
{
    bool val = 0;
    int8_t ret = 0;
    gpio_pin_read(g_gpio_forward_status,EXT_IN_GPIO_PIN2, &val);
    pushBoolArry(origbuff,sizeof(origbuff),val);
    ret = getBoolArryStat(origbuff,sizeof(origbuff));
    movData.moveLevel = MOVE_LEVEL_DISABLE;
    if(ret == 0)
    {
        movData.moveStat = MOVE_STOP;
        movData.speed = 0;
    }
    else if(ret == 1)
    {
        movData.moveStat = MOVE_FORWARD;
        movData.speed = 0;
    }
    //print_log("move sensor\n");
    //print_sensorData(&movData);

}
void getmoveMicroSwtichData(sensorData_t* data)
{
     *data =  movData;
}

uint32_t getMoveMicroSwtichRunInterval(void)
{
    return MOVE_MICRO_SWITCH_SENSOR_INTERVAL_MS;
}

