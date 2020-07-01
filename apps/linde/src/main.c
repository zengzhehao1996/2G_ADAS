#include <stdio.h>
#include <kernel.h>
#include <string.h>

#include "my_misc.h"
#include "rtc.h"
#include "config.h"
#include "rfid_thread.h"
#include "vehicle_ctrl_thread.h"
#include "fifo.h"
#include "file.h"
#include "sempho.h"
#include "msg_structure.h"
#include "thread_mpu.h"
#include "config.h"
#include "message_center_thread.h"
#include "thread_work_data.h"
#include "thread_wdt.h"
#include "thread_led.h"
#include "smart_link_version.h"
#include "fota_upgrade_thread.h"
#include "network_thread.h"
#include "thread_file.h"
#include "my_tool.h"
#include "hw_version.h"
#include "thread_stb.h"

sysconfig_t gSysconfigBackup;
sysconfig_t gSysconfig;

static void checkPowerOffMsg();
extern void detect_thread_message_center(uint32_t ts);
extern void detect_thread_offline_thread(uint32_t ts);

int main(int argc, char* argv[])
{
    /* step first power hold enable */
    hwPowerEnable();

    /* get hw version */
    getHardVersion(&g_hw_major, &g_hw_minor, &g_hw_tiny, &g_hw_patch);

    /* print version */
    print_version();
    print_hw_version();

    /* step init led */
    thread_led_stat_init();
    thread_led_stat_set(LED_STAT_ON, 1);
    thread_led_stat_set(LED_STAT_NO_COMM_SIGNAL, 1);  //no net

    /* step init RTC */
    initRTC();
    localRTC_t rtc;
    if(getRTC(&rtc))
    {
        printRTC(&rtc);
    }

    /* step init sem */
    if(!semInit())
    {
        err_log("semInit failed");
        return -1;
    }
    /* step init fifo */
    if(!fifoInit())
    {
        err_log("fifoInit failed");
        return -2;
    }

    /* step start file thread */
    startThreadFile();

    /* step delay fs init ok */
    print_log("wait for Init fs and load config.\n");
    semTakeInitFsOk();
    print_log("load config and main loop.\n");

    /* step power callback */
    hwPowerCallbackInit(gSysconfig.canType, gSysconfig.authType);

    /* step strt wdt */
    thread_wdt_start(6000);

    /* step start network thread */
    uint16_t      hbTimeOut = check_min(gSysconfig.hbTimeout, HB_INTERVAL_MIN);
    networkPara_t netPra    = { .hb_timeout       = hbTimeOut,
                             .auto_fota_switch = gSysconfig.autoFotaSwitch };
    networkThreadStart(netPra);

    /* step start message center thread */
    uint16_t      gpsInterval = check_min(gSysconfig.gpsUploadInterval, GPS_INTERVAL_MIN);
    messagePara_t para        = { .gGpsIntrval = gpsInterval,
                           .gHbTimeOut  = hbTimeOut,
                           .gHbInterval = gSysconfig.hbInterval,
                           .gCarStateInterval =
                               check_range(gSysconfig.onlineStateInterval, CAR_STATE_INTERVAL_MIN,
                                           CAR_STATE_INTERVAL_MAX),
                           .gCarStateOfflineInterval = check_range(gSysconfig.offlineStateInterval,
                                                                   CAR_STATE_OFFLINE_INTERVAL_MIN,
                                                                   CAR_STATE_OFFLINE_INTERVAL_MAX),
                           .authType                 = gSysconfig.authType };
    messageCenterTHreadStart(para);

    /* step start gps thred */
    startGpsThread(&gpsInterval);

    /* step start BMI160 thread */
    speedLimitConfig_t mseConfig = {
        gSysconfig.mseThreshold,       gSysconfig.vibrationLimitThreshold,
        gSysconfig.crashLowThreshold,  gSysconfig.crashMiddleThreshold,
        gSysconfig.crashHighThreshold, gSysconfig.crashSwitch
    };
    print_log("MSE Threshold:[%d].\n", mseConfig.vibration_threshold);
    threadMpuStart(&mseConfig);

    /* step start can thread */
    print_log("Can type: %d,gConfigFlag =%d\n", gSysconfig.canType, gConfigFlag);
    if(gSysconfig.canType != 200 && false == gConfigFlag)
    {
        uint16_t canInterval =
            check_range(gSysconfig.canMaxuploadInterval, CAN_INTERVAL_MIN, CAN_INTERVAL_MAX);
        uint16_t offlineCanInterval =
            check_range(gSysconfig.offlineCanInterval, CAN_OFFLINE_INTERVAL_MIN, CAN_INTERVAL_MAX);
        int8_t seatType = 0;
        if(gSysconfig.seatType == 1 || gSysconfig.seatType == 2)
        {
            seatType = gSysconfig.seatType;
        }
        else
        {
            seatType = gSeatTypeConfig.seatType;
        }


        workDataPara_t canPara = {
            .canType            = gSysconfig.canType,
            .canPattern         = gSysconfig.canPattern,
            .authType           = gSysconfig.authType,
            .interval           = canInterval,
            .seatTimeOut        = gSysconfig.seatoffTimeout,
            .overSpeed          = gSysconfig.overspeedAlarmThreshold,
            .pressEnble         = gMemconfig.pressEnable,
            .carry_threshold    = gSysconfig.carry_threshold,
            .overload_threshold = gSysconfig.overload_threshold,
            .move_type          = gSysconfig.moveType,
            .fork_type          = gSysconfig.forkType,
            .move_threshold     = gSysconfig.moveThreshold,  //nocan config ---mA
            .fork_threshold     = gSysconfig.forkThreshold,
            .seatType           = seatType,
            .canOfflineInterval = offlineCanInterval,
        };
        startWorkDataThread(canPara);
    }

    /* chiose auth type */
    print_log("Auth type: %d\n", gSysconfig.authType);
    if((gSysconfig.authType == AUTH_RFID && gSysconfig.canType != 200) || (gConfigFlag == true))
    {
        /* step start RFID thread */
        rfidThreadStart();

        /* step start vehicle ctrl thread */
        vehPara_t para = { .mode = MODE_OFFICAL };
        if(gConfigFlag == true)
        {
            para.mode = MODE_ABNORMAL;
        }
        vehclCtrlThreadStart(&para);
    }

    //aidong factory test
    print_log("canType[%d],authType[%d],gConfigFlag[%d]\n", gSysconfig.canType, gSysconfig.authType,
              gConfigFlag);
    if(gSysconfig.canType == 200 && gSysconfig.authType == AUTH_NONE && false == gConfigFlag)
    {
        print_log("enter aidong factory test mode\n");
        testStart_t testPars = { FACTORY_MODE, 0, 0, 0 };
        factoryTestThreadStart(testPars);
    }
    //print_log("test1");
    startThreadSTB();
    //print_log("test2");

    int i       = 0;
    int counter = 0;
    while(1)
    {
        uint32_t curr_ts = k_uptime_get_32();

        detect_thread_mpu(curr_ts);
        detect_thread_message_center(curr_ts);
        detect_thread_offline_thread(curr_ts);
        // detect_thread_nerwork_thread(curr_ts);
        detect_thread_file(curr_ts);

        if(200 != gSysconfig.canType)
        {
            detect_workdata_thread(curr_ts);
        }


        counter++;
        if(counter % 20 == 0)
        {
            print_log("main loop counter [%d] ----------.\n", ++i);
        }

        k_sleep(2000);
    }

    return 0;
}
