#ifndef __GENENAL_SENSOR_H__
#define __GENENAL_SENSOR_H__
#include "sensor_decoder.h"

bool genenalSensorSetup(gpSensorTpye_t* conf);
void genenalSensorRun(void);
void getGenenalSensorData(sensorData_t* data);
uint32_t getGenenalRunInterval(void);
void setSeatOnType(int8_t seat);
void getSeatOnType(int8_t *seat);
void getSeatVolt(uint8_t* vol);
void getKeyVolt(uint8_t* vol);


#endif
