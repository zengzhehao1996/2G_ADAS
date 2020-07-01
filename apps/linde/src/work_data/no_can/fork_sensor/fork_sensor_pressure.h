#ifndef __FORK_SENSOR_PRESSURE_H__
#define __FORK_SENSOR_PRESSURE_H__
#include "kernel.h"
#include "sensor_decoder.h"

bool forkPressureSetup(gpSensorTpye_t* conf);
void forkPressureRun(void);
void getforkPressureData(sensorData_t* data);
uint32_t getforkPressurehRunInterval(void);
void getPressSensorStatus(bool *status,uint16_t *origData,uint16_t *pressData);

#endif
