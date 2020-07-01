#ifndef __FORK_SENSOR_HOARE_H__
#define __FORK_SENSOR_HOARE_H__
#include "sensor_decoder.h"
bool forkHoareSetup(gpSensorTpye_t* conf);
void forkHoareRun(void);
void getForkHoareData(sensorData_t* data);
uint32_t getForkHoareRunInterval(void);
void getForkHoareCurr(uint32_t *current);

#endif

