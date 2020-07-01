
#include "sempho.h"

static struct k_sem    g_sem_updataRfidList; /*used for emmc thread tell rfid thread message that rfid list need updata*/
static struct k_sem    g_sem_rfidKeepUnlock; /* used for vehicle thread tell message center that keep unlock */
static struct k_sem    g_sem_wg34Ready; /*rfid thread tell vehicle_ctrl_thread that rfid init finished!*/
static struct k_sem    g_sem_seatTimeout; /* can thread  send vehicle ctrl thread a sempha that seat is timeout*/
static struct k_sem    g_sem_seatTimerStart; /*vehicle thread send can thread a sempha that seat timer need reset*/
static struct k_sem    g_sem_safeLockReq; /* vehicle thread requst  a semph that  whether forklift can be lock*/
static struct k_sem    g_sem_safeLockRep; /* can thread inform  ctrl thread that vehicle can be lock*/
static struct k_sem    g_sem_canbroken;   /* can thread tell can network broken to message center */
static struct k_sem    g_sem_canonline;   /* can thread tell can network online to message center */
static struct k_sem    g_sem_currDriver;  /* sever call back functin tell message center */
static struct k_sem    g_sem_canTimeOut;/*if lost can,can thread give the sem to vehicle thread,lock forklift*/
static struct k_sem    g_sem_imuReq;    // can disconnect req imu if vechicle is moving
static struct k_sem    g_sem_imuState;  // if vehicle is static imu give signal allow lock vehicle
static struct k_sem    g_sem_powerOff;  // used for poweroff
static struct k_sem    g_sem_initFs;
static struct k_sem    g_sem_pressureStart;
static struct k_sem    g_sem_pressureStop;
static struct k_sem    g_sem_writeFileOk;



bool semInit()
{
    k_sem_init(&g_sem_wg34Ready, 0, 1);
    k_sem_init(&g_sem_seatTimeout, 0, 1);
    k_sem_init(&g_sem_seatTimerStart, 0, 1);
    k_sem_init(&g_sem_safeLockReq, 0, 1);
    k_sem_init(&g_sem_safeLockRep, 0, 1);
    k_sem_init(&g_sem_updataRfidList, 0, 1);
    k_sem_init(&g_sem_rfidKeepUnlock, 0, 1);
    k_sem_init(&g_sem_canbroken, 0, 1);
    k_sem_init(&g_sem_canonline, 0, 1);
    k_sem_init(&g_sem_currDriver, 0, 1);
    k_sem_init(&g_sem_canTimeOut, 0, 1);
    k_sem_init(&g_sem_imuReq, 0, 1);
    k_sem_init(&g_sem_imuState, 0, 1);
    k_sem_init(&g_sem_powerOff, 0, 1);
    k_sem_init(&g_sem_initFs, 0, 1);
    k_sem_init(&g_sem_pressureStart, 0, 1);
    k_sem_init(&g_sem_pressureStop, 0, 1);
    k_sem_init(&g_sem_writeFileOk, 0, 1);

    return true;
}


// 1.g_sem_wg34Ready
bool semGiveWg34Ready()
{
    k_sem_give(&g_sem_wg34Ready);
    return true;
}
bool semTakeWg34Ready()
{
    if(!k_sem_take(&g_sem_wg34Ready, 100))
    {
        return true;
    }
    return false;
}
//2.g_sem_seatTimeout
bool semGiveSeatTimeout()
{
    k_sem_give(&g_sem_seatTimeout);
    return true;
}
bool semTakeSeatTimeout()
{
    if(!k_sem_take(&g_sem_seatTimeout, 0))
    {
        return true;
    }
    return false;
}
//3.g_sem_seatTimerStart
bool semGiveSeatTimerStart()
{
    k_sem_give(&g_sem_seatTimerStart);
    return true;
}
bool semTakeSeatTimerStart()
{
    if(!k_sem_take(&g_sem_seatTimerStart, 0))
    {
        return true;
    }
    return false;
}
//4.g_sem_safeLockReq
bool semGiveSafeLockReq()
{
    k_sem_give(&g_sem_safeLockReq);
    return true;
}
bool semTakeSafeLockReq()
{
    if(!k_sem_take(&g_sem_safeLockReq, 0))
    {
        return true;
    }
    return false;
}
//5.g_sem_safeLockRep
bool semGiveSafeLockRep()
{
    k_sem_give(&g_sem_safeLockRep);
    return true;
}
bool semResetSafeLockRep()
{
    k_sem_reset(&g_sem_safeLockRep);
    return true;
}

bool semTakeSafeLockRep()
{
    if(!k_sem_take(&g_sem_safeLockRep, 2000))
    {
        return true;
    }
    return false;
}
//6.g_sem_rfidListUpdata
bool semGiveRfidListUpdata()
{
    k_sem_give(&g_sem_updataRfidList);
    return true;
}
bool semTakeRfidListUpdata()
{
    if(!k_sem_take(&g_sem_updataRfidList, 0))
    {
        return true;
    }
    return false;
}

bool semGiveRfidKeepUnlock()
{
    k_sem_give(&g_sem_rfidKeepUnlock);
    return true;
}

bool semTakeRfidKeepUnlock()
{
    if(!k_sem_take(&g_sem_rfidKeepUnlock, 0))
    {
        return true;
    }
    return false;
}

//7.g_sem_canbroken
bool semGiveCanBroken()
{
    k_sem_give(&g_sem_canbroken);
    return true;
}

bool semTakeCanBroken()
{
    if(!k_sem_take(&g_sem_canbroken, 0))
    {
        return true;
    }
    return false;
}

//8.g_sem_canonline
bool semGiveCanOnline()
{
    k_sem_give(&g_sem_canonline);
    return true;
}

bool semTakeCanOnline()
{
    if(!k_sem_take(&g_sem_canonline, 0))
    {
        return true;
    }
    return false;
}

bool semGiveCurrDriver()
{
    k_sem_give(&g_sem_currDriver);
    return true;
}

bool semTakeCurrDriver()
{
    if(!k_sem_take(&g_sem_currDriver, 0))
    {
        return true;
    }
    return false;
}
//10.g_sem_can_timeout
bool semGiveCanTimeOut()
{
    k_sem_give(&g_sem_canTimeOut);
    return true;

}
bool semTakeCanTimeOut()
{
    if(!k_sem_take(&g_sem_canTimeOut, 0))
    {
        return true;
    }
    return false;

}

bool semGiveImuState()
{
    k_sem_give(&g_sem_imuState);
    return true;
}


bool semTakeImuState()
{
    if(!k_sem_take(&g_sem_imuState,500))
    {
        return true;
    }
    return false;
}


bool semGiveImuReq()
{
    k_sem_give(&g_sem_imuReq);
    return true;
}

bool semTakeImuReq()
{
    if(!k_sem_take(&g_sem_imuReq, 0))
    {
        return true;
    }
    return false;
}


bool semGivePowerOff()
{
    k_sem_give(&g_sem_powerOff);
    return true;
}

bool semTakePowerOff()
{
    if(!k_sem_take(&g_sem_powerOff, 0))
    {
        return true;
    }
    return false;
}

bool semGiveInitFsOk()
{
    k_sem_give(&g_sem_initFs);
    return true;
}

bool semTakeInitFsOk()
{
    if(!k_sem_take(&g_sem_initFs, K_FOREVER))
    {
        return true;
    }
    return false;
}
bool semGivePressureStart()
{
    k_sem_give(&g_sem_pressureStart);
    return true;

}
bool semTakePressureStart()
{
    if(!k_sem_take(&g_sem_pressureStart, 0))
    {
        return true;
    }
    return false;
}
bool semGivePressureStop()
{
    k_sem_give(&g_sem_pressureStop);
    return true;

}
bool semTakePressureStop()
{
    if(!k_sem_take(&g_sem_pressureStop, 0))
    {
        return true;
    }
    return false;

}

bool semGiveWriteOk()
{
    k_sem_give(&g_sem_writeFileOk);
    return true;
}

bool semTakeWriteOk(uint32_t ms)
{
    if(!k_sem_take(&g_sem_pressureStop, ms))
    {
        return true;
    }
    return false;
}