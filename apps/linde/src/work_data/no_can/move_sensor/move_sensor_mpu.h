#ifndef __MOVE_SENSOR_MPU_H__
#define __MOVE_SENSOR_MPU_H__
#include "sensor_decoder.h"
bool moveMpuSetup(gpSensorTpye_t* conf);
void moveMpuRun(void);
void getmoveMpuData(sensorData_t* data);
uint32_t getMoveMpuRunInterval(void);



#endif


