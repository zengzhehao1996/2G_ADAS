#ifndef __TRAY_PRESSURE_H__
#define __TRAY_PRESSURE_H__
#include "kernel.h"
typedef struct trayData_s
{
    uint16_t carryCounter;
    bool isForkMov;
    bool CarryBeerFlag;
}trayData_t;
bool pressureInit(uint16_t carry,uint16_t overLoad);
trayData_t pressureProcess();
void resetPressure();
void pressurePrint(trayData_t* pt);
void getPressureSensorData(uint16_t* data,uint16_t* pressureData);
void processCarryAffair();
void setPressureTimeOut(uint16_t t_s);
//bool isPressForkMove();
#endif
