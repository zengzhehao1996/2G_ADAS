#ifndef __THREAD_WORK_DATA_H__
#define __THREAD_WORK_DATA_H__

#include "thread_can.h"
#include "handler_caninfo.h"
#define startWorkDataThread(x) startCanThread(x)
#define stopWorkDataThread()   stopCanThread()
#define setPublicVehicleUnlock(x)   setVehicleUnlock(x)
#define detect_workdata_thread(x) detect_can_thread(x)
#endif /* __THREAD_WORK_DATA_H__ */