#ifndef __THREAD_WORK_DATA_H__
#define __THREAD_WORK_DATA_H__

#include "thread_no_can.h"
#include "handler_nocaninfo.h"
#define startWorkDataThread(x) startNoCanThread(x)
#define stopWorkDataThread()   stopNoCanThread()
#define setPublicVehicleUnlock(x)   nocanSetVehicleUnlock(x)
#define detect_workdata_thread(x)
#endif /* __THREAD_WORK_DATA_H__ */