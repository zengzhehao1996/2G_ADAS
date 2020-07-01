#ifndef __MOVE_SENSOR_SPEED_H__
#define __MOVE_SENSOR_SPEED_H__
#include "sensor_decoder.h"
bool moveSpeedSetup(gpSensorTpye_t* conf);
void moveSpeedRun(void);
void getmoveSpeedData(sensorData_t* data);
uint32_t getMoveSpeedRunInterval(void);
void getMoveSpeedStatus(uint32_t* counter,int32_t* speed,uint32_t* wheel);



#endif


