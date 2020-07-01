#include <stdio.h>
#include <kernel.h>
#include "hw_can.h"
#include "handler_caninfo.h"
#include "proto_adapt.h"
#include "thread_can.h"
#include "rs485Thread.h"

#include "fifo.h"

#include "sempho.h"

#include "msg_structure.h"

#include "thread_led.h"

#define DISC_NET_TIMEOUT 250  // offline 250 sec upload

#define CAN_THREAD_SIZE 1024

#define DISC_CAN_PERIOD 1000


K_THREAD_STACK_DEFINE(g_canThreadStack, CAN_THREAD_SIZE);
static struct k_thread g_canThread;
static k_tid_t         g_canThreadId = 0;

static canWorkHourFIFO_t canworkinghours;
static canErrFIFO_t      errorcodecan;
static canFIFO_t         canworkingdata;
static overSpeedFIFO_t   overmaxspeed;
static carryFIFO_t       carrydata;

static bool reupload = false;

static workDataPara_t gCanConfig;
static uint32_t       gLastCanThreadMs;

//static bool restartFlag = true; // add no can type answer lock vechicle request

void setAdaptationCanType(uint8_t canType, uint8_t canPattern);
void clearAdapationCantype(void);
void tellServerDeviceEnterDiagnosisMode(void);
//void checkInfoInvalid(uint16_t upinterval);

void canThreadEntry(void* p1)
{
    uint8_t  cantype                  = ((workDataPara_t*)p1)->canType;  // param in
    uint8_t  canPattern               = ((workDataPara_t*)p1)->canPattern;
    uint8_t  authType                 = ((workDataPara_t*)p1)->authType;
    uint16_t interval                 = ((workDataPara_t*)p1)->interval;          // param in sec
    uint16_t leaveseatmax             = ((workDataPara_t*)p1)->seatTimeOut;       // param in sec
    uint32_t speedmax                 = 1000 * ((workDataPara_t*)p1)->overSpeed;  // param in km/h
    bool     pressure                 = ((workDataPara_t*)p1)->pressEnble;
    uint16_t uploadinterval           = interval;
    uint16_t offlineCanRecordInterval = ((workDataPara_t*)p1)->canOfflineInterval;  // in sec

    print_log("cantype:[%d],canPattern:[%d],canIntrval:[%d],canOfflineInterval:[%d],seatTimeout:[%"
              "d],overSpeed:[%d].\n",
              cantype, canPattern, uploadinterval, offlineCanRecordInterval, leaveseatmax,
              speedmax);

    uint8_t voltlose    = 0;
    bool    connect     = 0;
    uint8_t loop        = 0;
    uint8_t recvcount   = 0;
    bool    ret         = 0;
    bool    issuccess   = 0;
    int8_t  index       = 0;
    bool    isdiagnosis = 0;

    CanRxMsgTypeDef* messageparse;

    /* init NoCan led blink */
    thread_led_stat_set(LED_STAT_NO_CAN_SIGNAL, 1);

    if(0 == authType)
    {
        setVehicleUnlock(1);
    }
    else
    {
        setVehicleUnlock(0);
    }

    while(loop < MAX_CAN_INIT_NUM && ret == FALSE)
    {
        ret = initDeviceCan(cantype);
        if(!ret)
        {
            print_log("CAN initDeviceCan failedXXXXXXXXXXXXXXXXXXXXXXX\n");
            k_sleep(1000);
        }
        loop++;
    }

    issuccess = ret;

    if(issuccess)
    {
        print_log("Init Can device ok........\n");
        paraCanlib_t canlibPara = { cantype, canPattern, speedmax, setAdaptationCanType };
        ret                     = setProtoType(canlibPara);
        print_log("set can type:[%d] ok.\n", cantype);
        issuccess = ret;
    }

    if(LINDE_ADAPTATION == cantype)
    {
        clearAdapationCantype();
    }

    if(issuccess)
    {
        kalmanInit();

        loop = 0;
        ret  = 0;

        while(loop < MAX_CAN_INIT_NUM && ret == FALSE)
        {
            ret = setRecvFilter();
            loop++;
        }
        print_log("set recv filter:::%d\n", ret);

        forkCanInfoInit();
        print_log("init canINFO.\n");

        setMaxSpeed(speedmax);
        issuccess = ret;
    }


    while(1)
    {
        /* step uptime */


        gLastCanThreadMs = k_uptime_get_32();

        if(FALSE == isdiagnosis)
        {
            isdiagnosis = getDiagnosis();
        }

        if(FALSE == isdiagnosis)
        {
            reqCurrentInfo();
        }
        else
        {
            tellServerDeviceEnterDiagnosisMode();
        }

        if(connect && issuccess)
        {
            connect = connectCan();

            if(connect == FALSE)
            {
                print_log("XXXXXXXXXXXXXXXXx CAN xxxxxxxxxxxxxxxxxxx\n");
                semGiveCanBroken();
                semGivePressureStop();
                thread_led_stat_set(LED_STAT_NO_CAN_SIGNAL, 1);

                // Can broken upload can info
                if(reupload)
                {
                    handlerPushInfo(&canworkingdata);
                    //checkInfoInvalid(uploadinterval);
                    if(canworkingdata.batState > 0)
                    {

                        voltlose = 3;
                    }
                    else if(voltlose < 3)
                    {
                        voltlose++;
                    }
                    if(canworkingdata.batVoltage != 0 && canworkingdata.batState != 0
                       && voltlose >= 3)
                    {
                        canFifoSend((char*)&canworkingdata);
                        print_log("send can to box.\n");
                    }
                    forkCanInfoInit();
                    canworkingdata.carryNums   = 0;
                    canworkingdata.carryPeriod = 0;
                }
            }

            recvcount = getCanRecvCount();

            //print_log("recvcount::::::::::::::::::::::%d\n",recvcount);


            for(loop = 0; loop < recvcount; loop++)
            {
                messageparse = getCanRecvMessage(loop);
                parseMessage(messageparse->StdId, messageparse->Data, messageparse->DLC);
            }

            resetCanRecvBuff();
            handlerLoopFresh();  // one loop update once immediate info

            //print_log("req currentInfo  ok.....\n");

            if(FALSE == isdiagnosis)
            {
                if(reqPeriodTimeout())  // request working hours timeout
                {
                    reqPeriodInfo();
                }
            }


            if(pressure)
            {
                if(carryDataFifoRcv((char*)&carrydata) && getVehicleUnlock())
                {
                    print_log("vvvvvvvvvvvvvvvvvvv.carry data coming vvvvvvvvvvvvvv\n");
                    canworkingdata.carryNums   = carrydata.carryCounter;
                    canworkingdata.carryPeriod = carrydata.carryTime;
                    handlerPushInfo(&canworkingdata);

                    if(canworkingdata.batVoltage > 10000)
                    {

                        voltlose = 3;
                    }
                    else if(voltlose < 3)
                    {
                        voltlose++;
                    }
                    if(canworkingdata.batVoltage != 0 && canworkingdata.batState != 0
                       && voltlose >= 3)
                    {
                        canFifoSend((char*)&canworkingdata);
                        print_log("send can to box.\n");
                    }

                    /* check over speed */
                    overmaxspeed.speed = getOverSpeed();
                    if(overmaxspeed.speed != 0)
                    {
                        overspeedFifoSend((char*)&overmaxspeed);
                        clearOverSpeed();
                    }

                    forkCanInfoInit();
                    canworkingdata.carryNums   = 0;
                    canworkingdata.carryPeriod = 0;
                    if(isMqttConnect() == 1)
                    {
                        uploadinterval = interval;
                    }
                    else
                    {
                        uploadinterval = offlineCanRecordInterval;
                    }
                }
            }

            ret = uploadInfoTimeout(uploadinterval);
            if(ret && getVehicleUnlock())  // can collect message send
            {
                print_log("vvvvvvvvvvvvvvvvvvv.upload info time vvvvvvvvvvvvvv\n");
                handlerPushInfo(&canworkingdata);

                if(canworkingdata.batVoltage > 10000)
                {

                    voltlose = 3;
                }
                else if(voltlose < 3)
                {
                    voltlose++;
                }
                if(canworkingdata.batVoltage != 0 && canworkingdata.batState != 0 && voltlose >= 3)
                {
                    canFifoSend((char*)&canworkingdata);
                    print_log("send can to box.\n");
                }
                /* check over speed */
                overmaxspeed.speed = getOverSpeed();
                if(overmaxspeed.speed != 0)
                {
                    overspeedFifoSend((char*)&overmaxspeed);
                    clearOverSpeed();
                }

                forkCanInfoInit();
                canworkingdata.carryNums   = 0;
                canworkingdata.carryPeriod = 0;
            }

            if(getWorkingHoursFlag())
            {
                canworkinghours.hour = getWorkinghours();
                canWorkHourFifoSend((char*)&canworkinghours);
                print_log("send hour: %d -----------------------------------------\n",
                          (int)canworkinghours.hour);
                resWorkingHoursFlag();
            }
            index = getNewErrorIndex();
            if(index >= 0)
            {
                print_log("index : [%d]\n", index);
                errorcodecan.id   = getErrorId(index);
                errorcodecan.code = getErrorCode(index);
                canErrFifoSend((char*)&errorcodecan);
            }

            if(seatOffTimeout(leaveseatmax))
            {
                semGiveSeatTimeout();
                resSeatOffTime();
                print_log("Seat Time Out : [%d] <<<<<<<<<<<<<<<<<<<<\n", leaveseatmax);
            }
            //print_log("seat time out :%d\n",leaveseatmax);
        }
        else if(0 == connect && issuccess)
        {
            connect = connectCan();
            if(connect)
            {
                forkCanInfoInit();
                semGiveCanOnline();
                semGivePressureStart();
                thread_led_stat_set(LED_STAT_NO_CAN_SIGNAL, 0);
                print_log("vvvvvvvvvvvvvvvvvv CAN vvvvvvvvvvvvvvvvvvvvv\n");
            }
        }

        if(semTakeSafeLockReq())
        {
            if(discanPeriod() >= DISC_CAN_PERIOD && connect == TRUE && issuccess)
            {
                print_log("disconnect can and give safe lock "
                          "......................aaaaaaaaaaaaaaaaaaaaa..\n");
                semGiveSafeLockRep();
                setVehicleUnlock(0);
                setConnectCan(FALSE);

                // lock vechicle upload can info
                handlerPushInfo(&canworkingdata);
                if(canworkingdata.batVoltage > 10000)
                {

                    voltlose = 3;
                }
                else if(voltlose < 3)
                {
                    voltlose++;
                }
                if(canworkingdata.batVoltage != 0 && canworkingdata.batState != 0 && voltlose >= 3)
                {
                    canFifoSend((char*)&canworkingdata);
                    print_log("send can to box.\n");
                }
                forkCanInfoInit();
                reupload                   = false;
                canworkingdata.carryNums   = 0;
                canworkingdata.carryPeriod = 0;
            }
            else if(connect == TRUE && issuccess)  // can is connect
            {
                if(getSafeLock())
                {
                    print_log("connect can give safe lock..............\n");
                    semGiveSafeLockRep();
                    setVehicleUnlock(0);

                    // lock vechicle upload can info
                    handlerPushInfo(&canworkingdata);
                    if(canworkingdata.batVoltage > 10000)
                    {

                        voltlose = 3;
                    }
                    else if(voltlose < 3)
                    {
                        voltlose++;
                    }
                    if(canworkingdata.batVoltage != 0 && canworkingdata.batState != 0
                       && voltlose >= 3)
                    {
                        canFifoSend((char*)&canworkingdata);
                        print_log("send can to box.\n");
                    }
                    forkCanInfoInit();
                    reupload                   = false;
                    canworkingdata.carryNums   = 0;
                    canworkingdata.carryPeriod = 0;
                }
                else
                {
                    bool forking = getForkLast();
                    unlockCauseFIFO_t msg  = { 0 };
                    if(forking)
                    {
                        /* have fork */
                        msg.cause = HAVE_FORK_UNLOCK;
                    }
                    else
                    {
                        /* have speed */
                        msg.cause = HAVE_SPEED_UNLOCK;
                    }
                    unlockCauseFifoSend((char*)&msg);
                }
            }
            else
            {
                semGiveSafeLockRep();
                setVehicleUnlock(0);
            }
        }

        if(semTakeSeatTimerStart())
        {
            forkCanInfoInit();
            canworkingdata.carryNums   = 0;
            canworkingdata.carryPeriod = 0;
            resSeatOffTime();
            setVehicleUnlock(1);
            reupload = true;
        }


        if(isMqttConnect() == 1)
        {
            uploadinterval = interval;
        }
        else 
        {
            uploadinterval = offlineCanRecordInterval;
        }

        k_sleep(50);
        //k_yield();
    }

    return 0;
}

void tellServerDeviceEnterDiagnosisMode(void)
{
    static bool upload = true;
    if(upload)
    {
        char buf[64];
        sprintf(buf, "timestamp:%d Device Enter Diagnosis Mode.", getTimeStamp());
        serverSendErrLog(buf);
        upload = false;
    }
}

bool startCanThread(workDataPara_t ps)
{
    bool ret;
    // print_log("start can thread\n");
    memcpy(&gCanConfig, &ps, sizeof(gCanConfig));
    gLastCanThreadMs = k_uptime_get_32();
    g_canThreadId    = k_thread_create(&g_canThread, g_canThreadStack, CAN_THREAD_SIZE,
                                    (k_thread_entry_t)canThreadEntry, &gCanConfig, NULL, NULL,
                                    K_PRIO_COOP(11), 0, 0);
    if(g_canThreadId != 0)
    {
        ret = true;
        print_log("Create CAN THREAD Id:[ %p ]; Stack:[ %p ]; Size:[ %p ]\n", g_canThreadId,
                  g_canThreadStack, CAN_THREAD_SIZE);
    }
    else
    {
        ret = false;
        err_log("Create Thread CAN Failed.\n");
    }

    /*start rs485 thread*/
    print_log("pressEnble is %d \n", ps.pressEnble);
    if(ps.canType != 200 && ps.pressEnble == 1)
    {
        static rs485para_t rs485Para;
        rs485Para.carryThreshold    = ps.carry_threshold;
        rs485Para.overLoadThreshold = ps.overload_threshold;
        rs485Para.overLoadThreshold = NORMAL_MODE;
        startRs485Thread(&rs485Para);
    }

    return ret;
}

void stopCanThread(void)
{
    print_log("begin stop can thread\n");
    if(0 != g_canThreadId)
    {
        k_thread_abort(g_canThreadId);
        g_canThreadId = 0;
        print_log("v133 stop can Thread  success !\n");
    }
    //stop 485 thread
    stopRs485Thread();
}

extern bool is_test_mode(void);
void        detect_can_thread(uint32_t ts)
{
    // no detect in test mode
    if(is_test_mode())
    {
        return;
    }

    if((ts > gLastCanThreadMs) && (ts - gLastCanThreadMs > THREAD_LOOP_TIME_5SEC))
    {
        warning_log("Rstart can thread. +++++++++++++++++++\n");
        workDataPara_t para = gCanConfig;
        stopCanThread();
        startCanThread(para);
    }
}

static autoCanFIFO_t adap = { 0 };
void                 setAdaptationCanType(uint8_t canType, uint8_t canPattern)
{
    if(adap.canTypeResult != canType || adap.canPatternResult != canPattern)
    {
        adap.canTypeResult    = canType;
        adap.canPatternResult = canPattern;
        print_log("Self-adaptation canType:[%d], canPattern:[%d] :-)\n", canType, canPattern);
        if(false == autoCanResultFifoSend((char*)&adap))
        {
            k_sleep(100);
            if(true == autoCanResultFifoSend((char*)&adap))
            {
                print_log("resend adaptation can ok.\n");
            }
            else
            {
                err_log("send adaptation failed.\n");
            }
        }
        else
        {
            print_log("send adaptation can ok.\n");
        }
    }
}

void clearAdapationCantype(void)
{
    memset(&adap, 0, sizeof(adap));
}

//void checkInfoInvalid(uint16_t upinterval)
//{
//    unint32_t maxperiod = uplinterval*1000+CAN_PERIOD_BIAS;
//    if(maxperiod<canworkingdata.collectPeriod)
//    {
//        canworkingdata.isvalid = 0;
//    }
//    if(maxperiod<canworkingdata.)
//
