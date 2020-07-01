#include "factory_test_unique.h"
#include "rs485Thread.h"
#include "tray_pressure.h"
#include "thread_work_data.h"
#include "proto_adapt.h"
#include "config.h"
#include "my_misc.h"
extern sysconfig_t gSysconfig;
static workDataPara_t canPara;

void v133_factory_test_unique_run(testStart_t para)
{
    // print_log("factore test run can_type:[%d], can_pattern:[%d], cmd:[%d]\n",para.can_type,para.can_pattern,para.cmd);

    setPressureTimeOut(10);

    //init can
    memset(&canPara, 0, sizeof(canPara));
    if(para.mode == TEST_AIDONG)
    {
        canPara.canType     = LINDE_CAN_1275;
        canPara.pressEnble  = 0;
    }
    else if(para.mode == TEST_KION)
    {
        if(27 == para.can_type && 1 == para.cmd)
        {
            canPara.canType = 25;
        }
        else
        {
            canPara.canType = para.can_type;
        }

        stopWorkDataThread(); //To avoid repeated startup causing driver problems, turn it off before starting
    }
    canPara.canPattern  = para.can_pattern;
    canPara.authType    = AUTH_NONE;
    canPara.interval    = 10;
    canPara.seatTimeOut = gSysconfig.seatoffTimeout;
    canPara.overSpeed   = gSysconfig.overspeedAlarmThreshold;

    print_log("factory call can_type:[%d], can_pattern:[%d].\n",canPara.canType, canPara.canPattern);
    startWorkDataThread(canPara);

}
void v133_factory_test_unique_stop()
{
    //1.stop 485 thread
    print_log("begin stop v133_factory_test_unique_stop..\n");
    stopRs485Thread();
    //2.stop can thread
    stopWorkDataThread();

}

