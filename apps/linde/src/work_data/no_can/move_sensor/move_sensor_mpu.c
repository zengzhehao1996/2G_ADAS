#include "move_sensor_mpu.h"
#include "hw_gpio.h"
#include "thread_mpu.h"
#define MOVE_MPU_SENSOR_INTERVAL_MS 1
static sensorData_t movData;
bool moveMpuSetup(gpSensorTpye_t* conf)
{
    memset(&movData,0,sizeof(sensorData_t));
    return true;
}
void moveMpuRun(void)
{
    uint8_t val = 0;
    val = getMpuStat();
    movData.moveLevel = MOVE_LEVEL_DISABLE;
    if(val == 0)
    {
        movData.moveStat = MOVE_STOP;
        movData.speed = 0;
    }
    else if(val == 1 || val == 3)
    {
        movData.moveStat = MOVE_FORWARD;
        movData.speed = 0;
    }
    else if(0)
    {
        //backword
        movData.moveStat = MOVE_BACKWARD;
        movData.speed = 0;
    }

}
void getmoveMpuData(sensorData_t* data)
{
     *data =  movData;
}

uint32_t getMoveMpuRunInterval(void)
{
    return MOVE_MPU_SENSOR_INTERVAL_MS;
}

