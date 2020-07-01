#include "thread_no_can.h"
#include "sensor_decoder.h"
#include "handler_nocaninfo.h"
#include <stdio.h>
#include <kernel.h>
#include "fifo.h"
#include "sempho.h"
#include "msg_structure.h"
#include "thread_led.h"
#include "my_tool.h"

#define DISC_NET_TIMEOUT 250

#define NO_CAN_THREAD_SIZE 2048

#define DISC_CAN_PERIOD  1000

K_THREAD_STACK_DEFINE(g_noCanThreadStack, NO_CAN_THREAD_SIZE);
static struct k_thread g_noCanThread;
static k_tid_t         g_nocanThreadId = 0;
static overSpeedFIFO_t   overmaxspeed;
static canFIFO_t         nocanWorkingdata;
static uint32_t lastSensorTime = 0;
static bool reupload = false;

static void check_no_can_data(canFIFO_t *pNoCan,int interval);

extern bool serverSendLog(uint8_t* pstr);
extern void getMoveHoareCurr(uint32_t *current);
extern void getForkHoareCurr(uint32_t *current);
extern void getPressSensorStatus(bool *status,uint16_t *origData,uint16_t *pressData);
extern void getSeatOnType(int8_t *seat);
extern void getMoveSpeedStatus(uint32_t* counter,int32_t* speed,uint32_t* wheel);
extern void getSeatVolt(uint8_t* vol);//V
extern void getKeyVolt(uint8_t* vol);//V

extern int32_t getServerDebugCmd();


static void sendServerSensorDataLog();
static void nocanThreadEntry(void* p1)
{
    uint8_t  cantype        = ((workDataPara_t*)p1)->canType;  // param in
    uint8_t  authType       = ((workDataPara_t*)p1)->authType;
    uint16_t interval = ((workDataPara_t*)p1)->interval;     // param in sec
    uint16_t leaveseatmax   = ((workDataPara_t*)p1)->seatTimeOut;  // param in sec
    uint32_t speedmax       = 1000*((workDataPara_t*)p1)->overSpeed;    // param in km/h
    bool pressure = ((workDataPara_t*)p1)->pressEnble;
    uint8_t seatType = ((workDataPara_t*)p1)->seatType;
    uint16_t uploadinterval = interval;
    print_log("cantype:[%d],uploadIntrval:[%d],seatTimeout:[%d],overSpeed:[%d],pressenable[%d],seatType[%d].\n",
                cantype,uploadinterval,leaveseatmax,speedmax,pressure,seatType);

    bool    connect   = 0;
    uint8_t loop      = 0;
    uint8_t recvcount = 0;
    bool    ret       = 0;
    int8_t  index     = 0;
    uint8_t voltlose  = 0;
    gpSensorTpye_t sensorType;
    sensorType.forkSensorType = ((workDataPara_t*)p1)->fork_type; 
    sensorType.fork_threshold = ((workDataPara_t*)p1)->fork_threshold;
    sensorType.movSensorType = ((workDataPara_t*)p1)->move_type;
    sensorType.move_threshold = ((workDataPara_t*)p1)->move_threshold;
    sensorType.seatType = ((workDataPara_t*)p1)->seatType;
    sensorType.carry_threshold = ((workDataPara_t*)p1)->carry_threshold;
    if(sensorType.movSensorType == MOV_SENSOR_SPEED)
    {
        sensorType.speed_enable = true;
    }
    else
    {
        sensorType.speed_enable = false;
    }
    if(cantype != LINDE_CAN_GENERAL)
    {
        err_log("can_type is not 254!!!!\n");
        return;
    }
    /* init NoCan led blink */
    thread_led_stat_set(LED_STAT_NO_CAN_SIGNAL, 1);
    
    if(0 == authType)
    {
        nocanSetVehicleUnlock(1);
    }
    else
    {
        nocanSetVehicleUnlock(0);
    }
    //print_log("before enter gpSensorSetType\n");
    ret = gpSensorSetType(&sensorType);
    //print_log("after enter gpSensorSetType\n");

    if(ret == FALSE)
    {
        // init can device baudrate failed pthread exit
        err_log("init no can sensor failed!!\n");
        return;
    }
    print_log("Init no can sensor ok........\n");
    print_log("set can type %d\n", cantype);

    loop = 0;
    ret  = 0;

    nocanSetMaxSpeed(speedmax);
    uint16_t loopvalue = 0;
    
    while(1)
    {
        loopvalue++;
        if(loopvalue > 60000)
        {
            loopvalue = 0;
        }
        gpSensorParseLoop(k_uptime_get());
        //print_log("connet = [%d]\n",nocanConnect());
        //upload server some log
        if(getServerDebugCmd() == 10001 && (k_uptime_get_32() - lastSensorTime > 5000) )
        {
            lastSensorTime = k_uptime_get_32();
            sendServerSensorDataLog();
        }
        if(connect)
        {
            connect = nocanConnect();

            if(connect == FALSE)
            {
                print_log("XXXXXXXXXXXXXXXXx CAN xxxxxxxxxxxxxxxxxxx\n");
                semGiveCanBroken();
                thread_led_stat_set(LED_STAT_NO_CAN_SIGNAL, 1);
                // can broken,send can info
                ///////////
                if(reupload)
                {
                    handlerNocanPushInfo(&nocanWorkingdata);
                    if(nocanWorkingdata.batVoltage > 10000)
                    {

                        voltlose = 3;
                    }
                    else if(voltlose < 3)
                    {
                        voltlose++;
                    }
                    if(nocanWorkingdata.batVoltage !=0 && nocanWorkingdata.batState != 0 && voltlose>=3)
                    {
                        canFifoSend((char*)&nocanWorkingdata);
                        print_log("send can to box.\n");
                    }
                }
                ///////////
            }
            noCanParseMessage(); 
            //print_log("req currentInfo  ok.....\n");
            ret = nocanUploadInfoTimeout(uploadinterval);
            // print_log("No Can upload interval:[%d]\n",uploadinterval);
            if(ret == 1)
            {
                print_log("................upload info time out return ::::::::::::%d\n",ret);
            }
          
            if( ret && nocanGetVehicleUnlock())  // can collect message send
            {
                print_log("vvvvvvvvvvvvvvvvvvv.upload info time vvvvvvvvvvvvvv\n");
                handlerNocanPushInfo(&nocanWorkingdata);

                /* check time in range[0,interval] */
                check_no_can_data(&nocanWorkingdata,uploadinterval*1000); // sec to ms
                if(nocanWorkingdata.batVoltage > 10000)
                {

                    voltlose = 3;
                }
                else if(voltlose < 3)
                {
                    voltlose++;
                }
                if(nocanWorkingdata.batVoltage !=0 && nocanWorkingdata.batState != 0 && voltlose>=3)
                {
                    bool ret = canFifoSend((char*)&nocanWorkingdata);
                    if(ret)
                    {
                        print_log("send can to box ok.\n");
                    }
                    else
                    {
                        print_log("send can to box FAILED .\n");
                    }

                }


                /* check over speed */
                overmaxspeed.speed = getNocanOverSpeed();
                if(overmaxspeed.speed != 0)
                {
                    overspeedFifoSend((char*)&overmaxspeed);
                    nocanClearOverSpeed();
                }

                forkNoCanInfoInit();              
            }            
            if(nocanSeatOffTimeout(leaveseatmax))
            {
                semGiveSeatTimeout();
                resSeatOffTime();
                print_log("Seat Time Out : [%d] <<<<<<<<<<<<<<<<<<<<\n", leaveseatmax);
            }
            //print_log("seat time out :%d\n",leaveseatmax);         
            
        }
        else
        {
            connect = nocanConnect();
            if(connect)
            {
                semGiveCanOnline();
                //semGivePressureStart();
                thread_led_stat_set(LED_STAT_NO_CAN_SIGNAL, 0);
                print_log("vvvvvvvvvvvvvvvvvv CAN vvvvvvvvvvvvvvvvvvvvv\n");
            }
        }

        if(semTakeSafeLockReq())
        {
            if(nocanDiscanPeriod() >= DISC_CAN_PERIOD && connect == TRUE)
            {
                print_log("disconnect can and give safe lock ......................aaaaaaaaaaaaaaaaaaaaa..\n");
                semGiveSafeLockRep();
                nocanSetVehicleUnlock(0);
                nocanSetConnect(FALSE);
                reupload = false;
                handlerNocanPushInfo(&nocanWorkingdata);
                if(nocanWorkingdata.batVoltage > 10000)
                {
                
                    voltlose = 3;
                }
                else if(voltlose < 3)
                {
                    voltlose++;
                }
                if(nocanWorkingdata.batVoltage !=0 && nocanWorkingdata.batState != 0 && voltlose>=3)
                {
                    canFifoSend((char*)&nocanWorkingdata);
                    print_log("send can to box.\n");
                }

            }
            else if(connect == TRUE)     // can is connect
            {
                if(getSafeLock())  //TODO
                {
                    print_log("connect can give safe lock..............\n");
                    semGiveSafeLockRep();
                    nocanSetVehicleUnlock(0);
                    reupload = false;
                    handlerNocanPushInfo(&nocanWorkingdata);
                    if(nocanWorkingdata.batVoltage > 10000)
                    {
                    
                        voltlose = 3;
                    }
                    else if(voltlose < 3)
                    {
                        voltlose++;
                    }
                    if(nocanWorkingdata.batVoltage !=0 && nocanWorkingdata.batState != 0 && voltlose>=3)
                    {
                        canFifoSend((char*)&nocanWorkingdata);
                        print_log("send can to box.\n");
                    }
                    
                }
                else
                {
                    bool forking = getForkLast();
                    unlockCauseFIFO_t msg = {0};
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
                nocanSetVehicleUnlock(0);                  
            }
            // upload last can info


        }

        if(semTakeSeatTimerStart())
        {
            forkNoCanInfoInit();
            resSeatOffTime();
            nocanSetVehicleUnlock(1);
            reupload = true;
        }


        if(isMqttConnect()==1)
        {
            uploadinterval = interval;
        }
        else if (interval < DISC_NET_TIMEOUT)
        {
            uploadinterval = DISC_NET_TIMEOUT;
        }
        

        k_sleep(50);
    }

    return 0;
}

bool startNoCanThread(workDataPara_t ps)
{
    bool ret;
    print_log("start can thread\n");
    g_nocanThreadId = k_thread_create(&g_noCanThread, g_noCanThreadStack, NO_CAN_THREAD_SIZE,
                    (k_thread_entry_t)nocanThreadEntry, &ps, NULL, NULL, K_PRIO_COOP(11), 0, 0);
    if(g_nocanThreadId != 0)
    {
        ret = true;
        print_log("Create CAN THREAD Id:[ %p ]; Stack:[ %p ]; Size:[ %p ]\n", g_nocanThreadId,
                  g_noCanThreadStack, NO_CAN_THREAD_SIZE);
    }
    else
    {
        ret = false;
        err_log("Create Thread CAN Failed.\n");
    }

    return ret;
}

static void check_no_can_data(canFIFO_t *pNoCan,int interval)
{
    pNoCan->forkPeriod = check_range(pNoCan->forkPeriod,0,interval);
    pNoCan->movePeriod = check_range(pNoCan->movePeriod,0,interval);
    pNoCan->backwardPeriod = check_range(pNoCan->backwardPeriod,0,interval);
    pNoCan->brakePeriod = check_range(pNoCan->brakePeriod,0,interval);
    pNoCan->carryPeriod = check_range(pNoCan->carryPeriod,0,interval);
    pNoCan->forwardPeriod = check_range(pNoCan->forwardPeriod,0,interval);
    pNoCan->moveForkPeriod = check_range(pNoCan->moveForkPeriod,0,interval);
    pNoCan->seatPeriod = check_range(pNoCan->seatPeriod,0,interval);
}

void stopNoCanThread(void)
{
    if(0 != g_nocanThreadId)
    {
        k_thread_abort(g_nocanThreadId);
        g_nocanThreadId = 0;
    }
}

static void sendServerSensorDataLog()
{
   /*
    extern bool serverSendLog(uint8_t* pstr);
    extern void getMoveHoareCurr(uint32_t *current);
    extern void getForkHoareCurr(uint32_t *current);
    extern void getPressSensorStatus(bool *status,uint16_t *origData,uint16_t *pressData);
    extern void getSeatVolt(uint8_t* vol);//V
    extern void getKeyVolt(uint8_t* vol);//V

    */ 
    char buff[200] = {0};
    uint32_t moveCurr = 0;
    uint32_t ForkCurr = 0;
    uint32_t speedCnt = 0;
    uint32_t wheel = 0;
    int32_t speed = 0;
    bool pressStatus = false;
    uint16_t pressData = 0;
    uint16_t origData = 0;
    int8_t seat = 0;
    uint8_t keyVol = 0;
    uint8_t seatVol = 0;
    getMoveHoareCurr(&moveCurr);
    getForkHoareCurr(&ForkCurr);
    getPressSensorStatus(&pressStatus,&origData,&pressData);
    getSeatOnType(&seat);
    getMoveSpeedStatus(&speedCnt,&speed,&wheel);
    getSeatVolt(&seatVol);
    getKeyVolt(&keyVol);
    sprintf(buff,"keyVolt=%dV;seatVolt=%dV;seatType=%d;pressSensor:status = %d,pressure= %uKPA;MoveCurr=%umA,ForkCurr=%umA,speedSensor:cnt = %d,wheel =%dmm,speed=%dm/h",
            keyVol,seatVol,seat,pressStatus,pressData,moveCurr,ForkCurr,speedCnt,wheel,speed);
    print_log("@@@@@@@@@@@@@@@@@@@@@@@@@buff_size[%d],sensor data = %s\n",strlen(buff),buff);
    serverSendLog(buff);
    
}

