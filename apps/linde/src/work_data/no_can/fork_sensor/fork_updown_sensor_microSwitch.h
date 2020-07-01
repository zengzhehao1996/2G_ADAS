#ifndef __FORK_UPDOWN_SENSOR_MICROSWTICH_H__
#define __FORK_UPDOWN_SENSOR_MICROSWTICH_H__
#include "sensor_decoder.h"

bool forkUpdownMicroSwtichSetup(gpSensorTpye_t* conf);
void forkUpdownMicroSwtichRun(void);
void getForkUpdownMicroSwtichData(sensorData_t* data);
uint32_t getForkUpdownMicroSwtichRunInterval(void);

#endif