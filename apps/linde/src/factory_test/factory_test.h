#ifndef __FACTORY_TEST_H__
#define __FACTORY_TEST_H__


#include <kernel.h>
#include "vehicle_ctrl_thread.h"
#include "msg_structure.h"
#define FACTORY_GPS_INTERVAL (10*1000)
bool factoryTestThreadStart(testStart_t para);
void factoryTestThreadStop();
s64_t factoryTestThreadTime();
int8_t AiDongTestModuleState(aidongTestResult_t* AiDongtestResult); // 0:success, -1: failed
bool setAidondTestResult(bool pass);
bool isFTStart();
extern void startFactoryTest(void);
extern void factoryTestTimeout(void);

#endif

