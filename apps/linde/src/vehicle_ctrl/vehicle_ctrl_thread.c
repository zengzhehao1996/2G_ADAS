#include "vehicle_ctrl_relay.h"
#include "vehicle_ctrl_led.h"
#include "vehicle_ctrl_beep.h"
#include "vehicle_ctrl_thread.h"
#include <stdint.h>
#include <stdbool.h>
#include <kernel.h>
#include "my_misc.h"
#include <gpio.h>
#include <device.h>
#include "msg_structure.h"
#include "my_file.h"
#include "rtc.h"
#include "hw_power.h"
#include "thread_work_data.h"
#include "fifo.h"

#define ABNORMAL_TRY_LOCK_INTERVAL (10*1000)
#define VEHCL_CTRL_STACK_SIZE 2048
#define VEHCL_STATUS_LOCK 1
#define VEHCL_STATUS_UNLOCK 2
#define POWER_OFF_TIME (10)
#define LAST_RFID_MSG_FILE "rfidMsgFile"
#define MAX_WRITE_CNT 4

#define WRITE_FILE_PERIOD 100

typedef struct lastRfidMsg_s
{
    uint32_t rfidId;
    char     lastStatus;
    uint8_t  reason;
    uint32_t lastTimeStamp;
    uint32_t relativityMs;
    localRTC_t rtc;
    uint32_t num_cnt;
} lastRfidMsg_t;



static classFile_t g_lastRfidFile;
K_THREAD_STACK_DEFINE(g_vehclCtrlStack, VEHCL_CTRL_STACK_SIZE);
static struct k_thread g_vehclCtrlThread;
static k_tid_t         g_vehclCtrlThreadId;
static s64_t           g_vehclCtrlThreadLastRunTime = 0;
static powerOffFile_t        powerOffTime = {0};
static bool abnormalStartFlag = false;
static rfidFIFO_t g_driverMsg = { .cmd = 1, .status = 0, .cardId = 0 };
static lastRfidMsg_t g_lastRfidMsg = { .rfidId = 0, .lastStatus = VEHCL_STATUS_LOCK, .reason = RFID_UNLOCK, .lastTimeStamp = 0,.relativityMs = 0};
static uint8_t updateLastRfid = 0;
static bool reboot_flag = false;// use for :if device reboot ,any card can lock vehicle!

static void (*runMode)(void);
static void factoryTestMode();
static void officalMode();
static void abnormalMode();
void print_rfidMsg(rfidFIFO_t* lockMsg);
static void vehclCtrlThreadRun(void* p);
static bool vehclCtrlSetup(vehPara_t vehPar);
static void UfoRfidProcess();
static void rfidReadyProcess();
static bool getLastRfidMsg();
static void rebootLockFaild();
static void rfidUnlockProcess();
static void addPwrOffRecord();
static bool safeLock();
static void uploadLockRecord(uint8_t reason);
static void uploadUnlockRecord();
static void uploadUFOMsg(rfidFIFO_t* ufoMsg);
static void uploadAnyRfidRecord(uint32_t card);

static uint8_t updateLastRfidFlag(void);

void readPowerOffFile(void);

void initLastRfidFile(void);

void readLastRfidFile(void);

uint8_t writeLastRfidFile(void);


bool vehclCtrlThreadStart(vehPara_t* para)
{
    bool ret;
    g_vehclCtrlThreadId = k_thread_create(
        &g_vehclCtrlThread, g_vehclCtrlStack, VEHCL_CTRL_STACK_SIZE,
        (k_thread_entry_t)vehclCtrlThreadRun,para, NULL, NULL, K_PRIO_COOP(2), 0, 0);
    if(g_vehclCtrlThreadId != 0)
    {
        ret = true;
        print_log("Create VehclCtrl THREAD Id:[ %p ]; Stack:[ %p ]; Size:[ %p ]\n",
                  g_vehclCtrlThreadId, g_vehclCtrlStack, VEHCL_CTRL_STACK_SIZE);
    }
    else
    {
        ret = false;
        err_log("Create Thread VehclCtrl Failed.\n\n");
    }

    return ret;
}
void vehclThreadStop(void)
{
    if(0 != g_vehclCtrlThreadId)
    {
        k_thread_abort(g_vehclCtrlThreadId);
        vehclBeepClose();//fix issue:avoid open beep,but vehclthread stop!the beep on beeping always!
        vehclLedClose();
        warning_log("\t\nvehclThreadStop ok...................\n");
    }
}
s64_t vehclThreadRunLastTime(void)
{
    return g_vehclCtrlThreadLastRunTime;
}
//vehPara_t



void initLastRfidFile(void)
{
    g_lastRfidFile.filename   = LAST_RFID_MSG_FILE;
    g_lastRfidFile.timeOut    = 0;
    if(!initFile(&g_lastRfidFile))
    {
       err_log("init last rfid File failed!!!!!!!!!!!!!!!!!!!!!!\n");
    }
}



void readLastRfidFile(void)
{
    int length=0;
    length = readDataFromFile(&g_lastRfidFile, &g_lastRfidMsg, sizeof(lastRfidMsg_t));
    if(length != sizeof(lastRfidMsg_t))
    {
        g_lastRfidMsg.rfidId     = 0;
        g_lastRfidMsg.lastStatus = VEHCL_STATUS_UNLOCK;
        g_lastRfidMsg.reason     = READ_CFG_ERR_UNLOCK;
        g_lastRfidMsg.lastTimeStamp = getTimeStamp();
        getRTC(&(g_lastRfidMsg.rtc));
        err_log("g_lastRfidMsg failed@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
    }
    g_lastRfidMsg.relativityMs = k_uptime_get_32();
    return;
}


uint8_t writeLastRfidFile (void)
{
    if(MAX_WRITE_CNT > updateLastRfid && 0<updateLastRfid)
    {
        if(writeDataToFile(&g_lastRfidFile, &g_lastRfidMsg, sizeof(lastRfidMsg_t)))
        {
            updateLastRfid = 0;
        }
        else
        {
            updateLastRfid++;
            err_log("saveLastRfidMsg failed@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ \n");
        }

    }
    
    return updateLastRfid;

}


static uint8_t updateLastRfidFlag(void)
{
    if(updateLastRfid==0)
    {
        updateLastRfid=1;
    }
    else if(MAX_WRITE_CNT > updateLastRfid && 0<updateLastRfid)
    {
        k_sleep(WRITE_FILE_PERIOD);
        if(updateLastRfid==0)
        {
            updateLastRfid=1;
        }
    }
    print_log("update last Rfid flag::::::::%d \n");
    return updateLastRfid;
}

void readPowerOffFile(void)
{
    int length=0;
    length = readPowerOffState(&powerOffTime, sizeof(powerOffTime));
    if(length != sizeof(powerOffTime))
    {
        powerOffTime.ts = getTimeStamp();
        getRTC(&powerOffTime.rtc);
        //no power off  file or file is broken
        err_log("get power off msg failed and reset  power off config!!!!\n"); 
     }
     print_log("power off ts = [%d],power off ts ：SSSSSSSSSSSSSSSSSSSSSSSSS\n",powerOffTime.ts);   
    //  printRTC(&powerOffTime.rtc);
     deletePowerOffState();
}




static bool vehclCtrlSetup(vehPara_t vehPar)
{
    print_log("~~~~~~~~~~~~~~~~workMode = %d\n",vehPar.mode);
    int ret = 0;
    //1.init relay ,led,beep
    if(!vehclCtrlrelaySetup())
    {
        return false;
    }
    if(!vehclCtrlLedSetup())
    {
        return false;
    }
    if(!vehclCtrlBeepSetup())
    {
        return false;
    }
    //2.wait rfid_thread ready
    while(!semTakeWg34Ready())
    {
        err_log("vehicle ctrl thread wait rfid thread time out@@@@@@@\n");
        k_sleep(100);
    }
    //3. beep 3 times reminder users rfid  is ready
    rfidReadyProcess();
    if(vehPar.mode == MODE_FACTORY_TEST)
    {
        for(int i = 0; i < 2; i++)
        {
            if(vehclLock())
            {
                vehclLedClose();
                print_log("in factory test mode ,init vehical state LOCK!!\n");
                break;
            }
            k_sleep(50);
        }
    }
    else if(vehPar.mode == MODE_ABNORMAL)
    {
        //1.unlock vehicle
        vehclOpen();
        vehclLedOpen();
        //2.set last state
        g_lastRfidMsg.lastStatus = VEHCL_STATUS_UNLOCK;
        
        
    }
    else if(vehPar.mode == MODE_OFFICAL)
    {
        
        //setup file to save last rfid
    
        //4. get last status of vehicle frome emmc thread
        /* if get fail ,reset last rfid msg*/
    
        print_log("g_lastRfidMsg.rfidId = %u,g_lastRfidMsg.lastStatus = %d,g_lastRfidMsg.last time = %u@@@@@@@@\n",
                  toBigEndian(g_lastRfidMsg.rfidId), g_lastRfidMsg.lastStatus,g_lastRfidMsg.lastTimeStamp);

        // printRTC(&g_lastRfidMsg.rtc);
        print_log(" print g_lastRfidMsg  end.....\n");
        //5.set vehicle
        if(g_lastRfidMsg.lastStatus == VEHCL_STATUS_LOCK)
        {
            //delete powoff lock file
            print_log("lastRfidMsg.lastStatus == VEHCL_STATUS_LOCK....\n");
            //deletePowerOffState();     
            // if(safeLock())
            if(1)
            {
                print_log(" safeLock condition is ok, lock vehical\n");
                vehclLock();
                vehclLedClose();
            }
            else
            {
                print_log("abnormal start!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
                reboot_flag = true; 
                abnormalStartFlag = true;
                unlockCauseFIFO_t msg = {0};
                if(unlockCauseFifoRcv((char*)&msg))
                {

                    g_lastRfidMsg.reason = msg.cause;
                }
                else
                {
                    g_lastRfidMsg.reason = HAVE_SPEED_UNLOCK; // default
                }
                semGiveSeatTimerStart();
                vehclOpen();
                vehclLedOpen();
                rebootLockFaild();
                updateLastRfidFlag(); 
            }
        }
        
        else if(g_lastRfidMsg.lastStatus == VEHCL_STATUS_UNLOCK)
        {        
    
            uint32_t currTime = getTimeStamp();
            //POWER_OFF_TIME
            //1. curr_time -powoff_time < default_time
            print_log("in power off ctrol,currTime:[%u],powerOffTime[%u],!!!!!!!!!!!!!!!!!!!\n",currTime, powerOffTime.ts);
            // printRTC(&powerOffTime.rtc);
            print_log(" print powerOffTime msg end....\n");
            if(currTime - powerOffTime.ts < POWER_OFF_TIME)
            {
                reboot_flag = true;
                g_lastRfidMsg.reason = KEEP_DRIVING_UNLOCK;
                semGiveSeatTimerStart();
                vehclOpen();
                vehclLedOpen();
                rfidUnlockProcess();
            }
            else
            {   
                print_log("power off to lock vehicleXXXXXXXXXX\n");
                // send lock msg and drive record msg
                addPwrOffRecord();    
                // if(safeLock())
                if(1)
                {
                    print_log("power off 10s ,lock vehical SUCCESS");
                    vehclLock();
                    vehclLedClose();
                    g_lastRfidMsg.rfidId =0;
                    g_lastRfidMsg.lastTimeStamp = getTimeStamp();
                    g_lastRfidMsg.lastStatus = LOCK;
                    g_lastRfidMsg.reason     = RFID_UNLOCK;
                    getRTC(&g_lastRfidMsg.rtc);
                    updateLastRfidFlag();
                    
                }
                else
                {
                    print_log(" power off condition is not satisfied, abnormal start vechicle!!!!!!!!!!!!!!!!!!\n");
                    reboot_flag = true;
                    abnormalStartFlag = true;
                    unlockCauseFIFO_t msg = {0};
                    if(unlockCauseFifoRcv((char*)&msg))
                    {

                        g_lastRfidMsg.reason = msg.cause;
                    }
                    else
                    {
                        g_lastRfidMsg.reason = HAVE_SPEED_UNLOCK; // default
                    }
                    semGiveSeatTimerStart();
                    vehclOpen();
                    vehclLedOpen();
                    rebootLockFaild();
                    updateLastRfidFlag();                
                }        
            }
        }
   }
    return true;
}
static void UfoRfidProcess(uint32_t rfidId)
{
    //1.1process led and card
    for(int i = 0; i < 3; i++)
    {
        vehclBeepOpen();
        vehclLedOpen();
        k_sleep(200);
        vehclBeepClose();
        vehclLedClose();
        k_sleep(200);
    }
    // 1.2reset last status
    if(g_lastRfidMsg.lastStatus == LOCK)
    {
        vehclBeepClose();
        vehclLedClose();
    }
    else if(g_lastRfidMsg.lastStatus == UNLOCK)
    {
        vehclLedOpen();
        vehclBeepClose();
    }
    //2.send to sim868
    rfidFIFO_t ufoDriver = { .cmd = UNKNOW, .status = g_lastRfidMsg.lastStatus, .cardId = rfidId,.ts =getTimeStamp(),.relativityMs = k_uptime_get_32()};
    uploadUFOMsg(&ufoDriver);

}
static void rfidReadyProcess()
{
    print_log("@@@@rfidReadyProcess\n");
    for(int i = 0; i < 3; i++)
    {
        //print_log("^^^^^^^^^^open= %d^^^^^^^^^^^^^^^^\n",i);
        vehclBeepOpen();
        vehclLedOpen();
        k_sleep(110);
        vehclBeepClose();
        vehclLedClose();
        k_sleep(110);
        //print_log("^^^^^^^^^^close= %d^^^^^^^^^^^^^^^^\n",i);
    }
}
static void vehclCtrlThreadRun(void* p)
{
    char workMode = ((vehPara_t*)p)->mode;
    print_log("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~workMode = %d\n",workMode);
    if(!vehclCtrlSetup(*(vehPara_t*)p))
    {
        err_log("vehicle ctrl setup failed\n");
        return false;
    }
    //runMode = 
    if(workMode == MODE_FACTORY_TEST)
    {
        runMode = factoryTestMode;

    }
    else if(workMode == MODE_OFFICAL)
    {
        runMode = officalMode;

    }
    else if(workMode == MODE_ABNORMAL)
    {
        runMode = abnormalMode;

    }
    else
    {
        runMode = officalMode;

    }
    while(1)
    {
        runMode();
        k_sleep(100);
    }
}



static void factoryTestMode()
{
    //print_log("factoryTestMode Run@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
    //1.wait rfid
    static rfid2CtrlFIFO_t rfidMsg;
    static char lastVehiStat = LOCK;
    if(!rfid2CtrlFifoRcv((char*)(&rfidMsg)))
    {
        k_sleep(50);
        return;
    }
   
    //2. send to sim 868
    if(lastVehiStat == LOCK)
    {
        g_driverMsg.cmd    = UNLOCK;
        g_driverMsg.cardId = rfidMsg.cardId;
        g_driverMsg.status = 0;
        g_driverMsg.ts = getTimeStamp();
        getRTC(&g_driverMsg.rtc);
        
        lastVehiStat = UNLOCK;
        vehclOpen();
        vehclLedOpen();
        vehclLockBeepHint();

    }
    else if(lastVehiStat == UNLOCK)
    {
        g_driverMsg.cmd    = LOCK;
        g_driverMsg.cardId = rfidMsg.cardId;
        g_driverMsg.status = RFID_LOCK;
        g_driverMsg.ts = getTimeStamp();
        getRTC(&g_driverMsg.rtc);
        lastVehiStat = LOCK;
        vehclLock();
        vehclLedClose();
        vehclLockBeepHint();
    }
    //rfid2Sim868FifoSend(&g_driverMsg);
    rfidFIFO_t ufoDriver = { .cmd = UNKNOW, .status = g_driverMsg.status, .cardId = g_driverMsg.cardId,.ts =g_driverMsg.ts, .relativityMs = g_driverMsg.relativityMs};
    uploadUFOMsg(&ufoDriver);
    //received rfid
}
static void abnormalMode()
{   
    static int count = 0;
    count++;
    if(count % 100 == 0)
    {
        print_log("In Abnormal Mode.\n");
    }
    rfid2CtrlFIFO_t rfidMsg;
    if(!rfid2CtrlFifoRcv((char*)(&rfidMsg)))
    {
        k_sleep(50);
        return;
    }
    //VEHCL_STATUS_LOCK
    if(g_lastRfidMsg.lastStatus == VEHCL_STATUS_UNLOCK )
    {
        g_lastRfidMsg.lastStatus = VEHCL_STATUS_LOCK;
        vehclLock();
        vehclLedClose();
    }
    else if(g_lastRfidMsg.lastStatus == VEHCL_STATUS_LOCK)
    {
        g_lastRfidMsg.lastStatus = VEHCL_STATUS_UNLOCK;
        vehclOpen();
        vehclLedOpen();
    }
    
}

static void officalMode()
{
    static rfid2CtrlFIFO_t rfidMsg;
    uint32_t   currTime = k_uptime_get_32();
    static uint32_t lastTryLockTime = 0;
    //1.wait seat timeout sempha
    if(semTakeSeatTimeout())
    {
        print_log("seat timeout ###############################\n");
        if(g_lastRfidMsg.lastStatus == UNLOCK)
        {
            if(!safeLock())
            {
                err_log("seat timeout ,but safelock condition is not satisfied\n");
                return;
            }
            else
            {
                print_log("seat timeout ,lock vehicle\n");
                vehclLock();
                vehclLedClose();
                uploadLockRecord(SEAT_TIMEOUT_LOCK);
                updateLastRfidFlag();
                abnormalStartFlag = false;
                reboot_flag = false;
                
            }
        }
    }
    //2.wait can timeout sempho
    if(semTakeCanTimeOut())
    {
        print_log("can time out sem coming!!the last status = %dXXXXXXXXXX\n",g_lastRfidMsg.lastStatus);
        if(g_lastRfidMsg.lastStatus == UNLOCK)
        {
            print_log("semTakeCanTimeOut is coming!!!!!!!\n"); 
            //setVehicleUnlock(0);
            setPublicVehicleUnlock(0);
            vehclLock();
            vehclLedClose();
            uploadLockRecord(BREAK_CAN_LOCK);
            updateLastRfidFlag();
            abnormalStartFlag = false;
            reboot_flag = false;
        }
    }
    //3.if  abnormal StartFlag ,try lock vehicle
    if( abnormalStartFlag && (currTime - lastTryLockTime > ABNORMAL_TRY_LOCK_INTERVAL) )
    {
        print_log("vehicle thread try to lock vehicle,currTime= %u,lastTryLockTime = %u$$$$$$$$$$$$$\n",currTime,lastTryLockTime);
        lastTryLockTime = currTime;
        if(safeLock())
        {
            vehclLock();
            vehclLedClose();
            uploadLockRecord(INIT_LOCK); // init lock
            updateLastRfidFlag();
            abnormalStartFlag = false;
            reboot_flag = false;
        }
    }
    //4.wait rfid thread sempha
    if(!rfid2CtrlFifoRcv((char*)(&rfidMsg)))
    {
        k_sleep(50);
        return;
    }
    //3.process rfid msg
    if(rfidMsg.rfidValible)
    {
        // LOCK VEHICLE
        if(g_lastRfidMsg.lastStatus == UNLOCK)
        {
            // resovle: if smartlink start with vehicle opening,every  avalible card can lock it
            if(g_lastRfidMsg.rfidId == rfidMsg.cardId || g_lastRfidMsg.rfidId == 0)
            {
                if(!safeLock())
                {
                    print_log("rfid lock type, safelock condition is not satisfied,return\n");
                    return;
                }
                g_lastRfidMsg.rfidId = rfidMsg.cardId;
                vehclLock();
                vehclLedClose();
                vehclLockBeepHint();
                uploadLockRecord(RFID_LOCK);
                updateLastRfidFlag();
                abnormalStartFlag = false;
                reboot_flag = false;
                
            }
            if(reboot_flag)
            {
                if(!safeLock())
                {
                    print_log("rfid lock type, safelock condition is not satisfied,return\n");
                    return;
                }
                vehclLock();
                vehclLedClose();
                vehclLockBeepHint();
                uploadAnyRfidRecord(rfidMsg.cardId);
                updateLastRfidFlag();
                abnormalStartFlag = false;
                reboot_flag = false;
            }
        }
        //UNLOCK
        else if(g_lastRfidMsg.lastStatus == LOCK)
        {
            semGiveSeatTimerStart();
            vehclOpen();
            vehclLedOpen();
            vehclUnlockBeepHint();
            g_lastRfidMsg.rfidId = rfidMsg.cardId;
            g_lastRfidMsg.lastStatus = UNLOCK;
            g_lastRfidMsg.lastTimeStamp = getTimeStamp();
            g_lastRfidMsg.reason = RFID_UNLOCK;
            getRTC(&g_lastRfidMsg.rtc);
            updateLastRfidFlag();
            uploadUnlockRecord();
        }
    }
    else
    {
        // if unlock rfid card is killed ,the card can lock the vehicle;
        if(g_lastRfidMsg.rfidId == rfidMsg.cardId)
        {
            if(!safeLock())
            {
                err_log("safe lock condition is not satisfied\n");
                return;
            }
            g_lastRfidMsg.rfidId = rfidMsg.cardId;
            vehclLock();
            vehclLedClose();
            vehclLockBeepHint();
            uploadLockRecord(RFID_LOCK);
            updateLastRfidFlag();
            abnormalStartFlag = false; 
        }
        else
        {
            UfoRfidProcess(rfidMsg.cardId);
        }
    }

}

uint32_t vehicleGetCurrRfid()
{
    return g_lastRfidMsg.rfidId;
}

void print_rfidMsg(rfidFIFO_t* lockMsg)
{
   print_log("rfid msg:\n");
   printk("lockMsg->cardId = %u ,lockMsg->ts = %u,lockMsg->status = %d\n",
   toBigEndian(lockMsg->cardId),lockMsg->ts,lockMsg->status);
}
static void rebootLockFaild()
{
    //pack fifo
    rfidFIFO_t rebootMsg;
    g_lastRfidMsg.lastStatus = UNLOCK;
    g_lastRfidMsg.lastTimeStamp = getTimeStamp();
    g_lastRfidMsg.relativityMs  = k_uptime_get_32();
    getRTC(&g_lastRfidMsg.rtc);
    g_lastRfidMsg.rfidId = 0;
    rebootMsg.cardId = g_lastRfidMsg.rfidId;
    rebootMsg.cmd = g_lastRfidMsg.lastStatus;
    rebootMsg.rtc = g_lastRfidMsg.rtc;
    rebootMsg.status = g_lastRfidMsg.reason;
    rebootMsg.ts = g_lastRfidMsg.lastTimeStamp;
    rebootMsg.relativityMs = g_lastRfidMsg.relativityMs;
    // send fifo
    rfid2Sim868FifoSend(&rebootMsg);
}

static void rfidUnlockProcess()
{
    //pack fifo
    rfidFIFO_t unlockMsg;
    unlockMsg.cardId = g_lastRfidMsg.rfidId;
    unlockMsg.cmd = UNLOCK;
    unlockMsg.rtc = g_lastRfidMsg.rtc;
    unlockMsg.status = g_lastRfidMsg.reason;
    unlockMsg.ts = g_lastRfidMsg.lastTimeStamp;
    unlockMsg.relativityMs = g_lastRfidMsg.relativityMs;
    // send fifo
    rfid2Sim868FifoSend(&unlockMsg); 
}
static void addPwrOffRecord()
{
   rfidFIFO_t       lockMsg={0};
   drvRcrdFifo_t    drvRcrdMsg={0};
   
   lockMsg.cardId    = g_lastRfidMsg.rfidId;
   lockMsg.cmd       = LOCK;
   lockMsg.ts        = powerOffTime.ts;
   lockMsg.rtc       = powerOffTime.rtc;
   lockMsg.status    = PWR_OFF_LOCK;

   drvRcrdMsg.cardId    = g_lastRfidMsg.rfidId;
   drvRcrdMsg.getOnRTC  = g_lastRfidMsg.rtc;
   drvRcrdMsg.on_reason = g_lastRfidMsg.reason;
   drvRcrdMsg.getOnTs   = g_lastRfidMsg.lastTimeStamp;
   drvRcrdMsg.getOffRTC = powerOffTime.rtc;
   drvRcrdMsg.getOffTs  = powerOffTime.ts;
   drvRcrdMsg.off_reason = PWR_OFF_LOCK;

   rfid2Sim868FifoSend(&lockMsg);
   DrvRecordFifoSend(&drvRcrdMsg);

   
   
}
static bool safeLock()
{
    semResetSafeLockRep();
    print_log("req safe lock .....................bbbbbbbbbbbbbbb\n");
    semGiveSafeLockReq();
    if(!semTakeSafeLockRep())
    {
        err_log("semTakeSafeLockRep  failed@@@@@@@@@@@@@@@@@\n");
        return false;
    }
    return true;
}
static void uploadLockRecord(uint8_t reason)
{
    rfidFIFO_t       lockMsg;
    drvRcrdFifo_t    drvRcrdMsg;
    uint32_t getOffTime = getTimeStamp();
    uint32_t getOffMs = k_uptime_get_32();
    localRTC_t getOffRtc;
    getRTC(&getOffRtc);
    lockMsg.cardId    = g_lastRfidMsg.rfidId;
    lockMsg.cmd       = LOCK;
    lockMsg.ts        =  getOffTime;
    lockMsg.rtc       =getOffRtc;
    lockMsg.relativityMs = getOffMs;
    lockMsg.status    = reason;
    
    drvRcrdMsg.cardId    = g_lastRfidMsg.rfidId;
    drvRcrdMsg.getOnRTC  = g_lastRfidMsg.rtc;
    drvRcrdMsg.getOnTs   = g_lastRfidMsg.lastTimeStamp;
    drvRcrdMsg.on_reason = g_lastRfidMsg.reason;
    drvRcrdMsg.getOffRTC = getOffRtc;
    drvRcrdMsg.getOffTs  = getOffTime;
    drvRcrdMsg.getOnRelativityMs = g_lastRfidMsg.relativityMs;
    drvRcrdMsg.getOffRelativityMs = getOffMs;
    drvRcrdMsg.off_reason = reason;
    
    rfid2Sim868FifoSend(&lockMsg);
    DrvRecordFifoSend(&drvRcrdMsg);

    g_lastRfidMsg.rfidId = 0;
    getRTC(&g_lastRfidMsg.rtc);
    g_lastRfidMsg.lastTimeStamp = getTimeStamp();
    g_lastRfidMsg.relativityMs  = k_uptime_get_32();
    g_lastRfidMsg.lastStatus    = LOCK;
}

static void uploadAnyRfidRecord(uint32_t card)
{
    rfidFIFO_t       lockMsg;
    drvRcrdFifo_t    drvRcrdMsg;
    uint32_t getOffTime = getTimeStamp();
    uint32_t getOffRMs = k_uptime_get_32();
    localRTC_t getOffRtc;
    getRTC(&getOffRtc);
    lockMsg.cardId    = card;
    lockMsg.cmd       = LOCK;
    lockMsg.ts        =  getOffTime;
    lockMsg.rtc       =getOffRtc;
    lockMsg.status = RFID_LOCK;
    lockMsg.relativityMs = getOffRMs;
    
    drvRcrdMsg.cardId    = g_lastRfidMsg.rfidId;
    drvRcrdMsg.getOnRTC  = g_lastRfidMsg.rtc;
    drvRcrdMsg.getOnTs   = g_lastRfidMsg.lastTimeStamp;
    drvRcrdMsg.on_reason = g_lastRfidMsg.reason;
    drvRcrdMsg.getOffRTC = getOffRtc;
    drvRcrdMsg.getOffTs  = getOffTime;
    drvRcrdMsg.getOnRelativityMs = g_lastRfidMsg.relativityMs;
    drvRcrdMsg.getOffRelativityMs = getOffRMs;
    drvRcrdMsg.off_reason = RFID_LOCK;
    
    rfid2Sim868FifoSend(&lockMsg);
    DrvRecordFifoSend(&drvRcrdMsg);

    g_lastRfidMsg.rfidId = 0;
    getRTC(&g_lastRfidMsg.rtc);
    g_lastRfidMsg.lastTimeStamp = getTimeStamp();
    g_lastRfidMsg.relativityMs = k_uptime_get_32();
    g_lastRfidMsg.lastStatus = LOCK;
    g_lastRfidMsg.reason     = RFID_UNLOCK;
}

static void uploadUnlockRecord()
{
    rfidFIFO_t       unlockMsg;
    unlockMsg.cardId    = g_lastRfidMsg.rfidId;
    unlockMsg.cmd       = UNLOCK;
    unlockMsg.ts        = g_lastRfidMsg.lastTimeStamp;
    unlockMsg.rtc       = g_lastRfidMsg.rtc; 
    unlockMsg.status    = g_lastRfidMsg.reason;
    unlockMsg.relativityMs = g_lastRfidMsg.relativityMs;

    rfid2Sim868FifoSend(&unlockMsg);
    
}
static void uploadUFOMsg(rfidFIFO_t* ufoMsg)
{
    rfidFIFO_t       UFOMsg;
    UFOMsg.cardId    = ufoMsg->cardId;
    UFOMsg.cmd       = ufoMsg->cmd;
    UFOMsg.status       = RFID_UNLOCK;
    UFOMsg.ts     = getTimeStamp();
    UFOMsg.relativityMs = k_uptime_get_32();
    getRTC(&UFOMsg.rtc);
    rfid2Sim868FifoSend(&UFOMsg);
}


