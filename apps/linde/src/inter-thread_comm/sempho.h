#ifndef __SEMPHA_H__
#define __SAMPHA_H__
#include <kernel.h>

#define SAVE_2_FILE_MAX 3

bool semInit();
// 1.g_sem_wg34Ready
bool semGiveWg34Ready();
bool semTakeWg34Ready();

//2.g_sem_seatTimeout
bool semGiveSeatTimeout();
bool semTakeSeatTimeout();

//3.g_sem_seatTimerStart
bool semGiveSeatTimerStart();
bool semTakeSeatTimerStart();

//4.g_sem_safeLockReq
bool semGiveSafeLockReq();
bool semTakeSafeLockReq();

//5.g_sem_safeLockRep
bool semGiveSafeLockRep();
bool semTakeSafeLockRep();
bool semResetSafeLockRep();

//6.g_sem_rfidListUpdata
bool semGiveRfidListUpdata();
bool semTakeRfidListUpdata();

//6.2 g_sem_rfidKeepUnlock
bool semGiveRfidKeepUnlock();
bool semTakeRfidKeepUnlock();

//7.g_sem_canbroken
bool semGiveCanBroken();
bool semTakeCanBroken();

//8.g_sem_canonline
bool semGiveCanOnline();
bool semTakeCanOnline();

//9.g_sem_curr_driver
bool semGiveCurrDriver();
bool semTakeCurrDriver();

//10.g_sem_can_timeout
bool semGiveCanTimeOut();
bool semTakeCanTimeOut();

bool semGiveImuState();
bool semTakeImuState();

bool semGiveImuReq();
bool semTakeImuReq();

bool semGivePowerOff();
bool semTakePowerOff();

bool semGiveInitFsOk();
bool semTakeInitFsOk();
// can thread send semphor to 485 thread,to stop or start pressure sensor work
bool semGivePressureStart();
bool semTakePressureStart();
bool semGivePressureStop();
bool semTakePressureStop();

bool semGiveWriteOk();
bool semTakeWriteOk(uint32_t ms);


#endif
