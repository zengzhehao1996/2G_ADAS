#ifndef __MOVE_SENSOR_MICROSWITCH_H__
#define __MOVE_SENSOR_MICROSWITCH_H__
#include "sensor_decoder.h"
bool moveMicroSwtichSetup(gpSensorTpye_t* conf);
void moveMicroSwtichRun(void);
void getmoveMicroSwtichData(sensorData_t* data);
uint32_t getMoveMicroSwtichRunInterval(void);



#endif


