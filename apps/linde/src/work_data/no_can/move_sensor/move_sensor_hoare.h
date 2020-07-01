#ifndef __MOVE_SENSOR_HOARE_H__
#define __MOVE_SENSOR_HOARE_H__
#include "sensor_decoder.h"
bool moveHoareSetup(gpSensorTpye_t* conf);
void moveHoareRun(void);
void getmoveHoareData(sensorData_t* data);
uint32_t getMoveHoareRunInterval(void);
void getMoveHoareCurr(uint32_t *current);

#endif
