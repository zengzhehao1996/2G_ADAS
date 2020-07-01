#include "my_misc.h"
#include "message_center_thread.h"
#include "config.h"
#include "server_interface.h"
#include "rtc.h"
#include "rtc_timestamp.h"
#include "fifo.h"
#include "msg_structure.h"
#include "thread_led.h"
#include <kernel.h>
#include <string.h>
#include "atc.h"
#include "ff.h"
#include "smart_link_version.h"
#include "hw_version.h"
#include "hw_gps_parser.h"


#define MESSAGE_THREAD_STACK_SIZE 2048
K_THREAD_STACK_DEFINE(gMessageThreadStack, MESSAGE_THREAD_STACK_SIZE);
static struct k_thread gMessageThread;
static k_tid_t         gMessageThreadId = 0;

static messagePara_t gConfig;

static volatile bool     g_car_lock;
static volatile uint32_t lockTime = 0;

typedef struct
{
    uint32_t ts;
    uint32_t localMs;
    uint32_t cardId;
    RTC_t    rtc;
    uint8_t  status;
    bool     lock;
} currRFID_t;

#define TMP_RFID_FILE "tmpRfid"
#define MAX_TMP_RFID_DILE_SIZE (1 * 1024 * 1024)
enum
{
    TMP_NONE      = 0,
    TMP_LOCK      = 1,
    TMP_UNLOCK    = 2,
    TMP_DRIVER    = 3,
    TMP_LOCK_TS   = 4,
    TMP_UNLOCK_TS = 5,
    TMP_DRIVER_TS = 6,
    TMP_WRITE     = 7,
    TMP_READ      = 8,
} tmpTs_t;

typedef struct
{
    uint8_t  cmd;
    uint8_t  dirty;
    uint8_t  writeFlag;
    uint8_t  preempted;
    uint32_t getOnTs;
    uint32_t getOnRs;
    uint32_t getOffTs;
    uint32_t getOffRs;
    uint32_t cardId;
    uint8_t  reason;
} tmpRFID_t;

typedef struct
{
    uint32_t            upload_ts;
    uploadFactoryTest_t val;
    bool                upload_flag;
    bool                factory_flag;
    bool                upload_ok;
    bool                local_test_pass;
    bool                server_csq_pass;
} tmpFactory_t;

static currRFID_t   gRfid;
static tmpRFID_t    gTmpRfid;
static tmpFactory_t gTmpFactory;

static uint32_t g_last_message_center_thread_uptime = 0;

static void getGlobalRFID(currRFID_t* ps);
static void setGlobalRFID(currRFID_t* ps);
static void printCan(uploadCAN_t* upload);
static void change_upload_car_state(bool lock, uint32_t rfid, uint32_t ts);
static bool pushRFIDToTmp(void* ps);
static bool popRFIDFromTmp(void* ps);
static void checkUploadTmpRFID(void);

extern void checkFotaOperateACK(void);
extern bool isGetGps(void);
extern hwGpsMsg_t getDFTtailGps();
extern uint8_t isMqttConnect(void);

void initTmpFactory(void)
{
    print_log("msg thread: initTmpFactory...\n ");
    memset(&gTmpFactory, 0, sizeof(gTmpFactory));
    copyHardVersion(&gTmpFactory.val.dv_0, &gTmpFactory.val.dv_1, &gTmpFactory.val.dv_2,
                    &gTmpFactory.val.dv_3);
    gTmpFactory.val.sv_0     = SOFT_VERSION_MAJOR;
    gTmpFactory.val.sv_1     = SOFT_VERSION_MINOR;
    gTmpFactory.val.sv_2     = SOFT_VERSION_TINY;
    gTmpFactory.val.sv_3     = SOFT_VERSION_PATCH;
    gTmpFactory.factory_flag = true;
    gTmpFactory.local_test_pass = false;
    gTmpFactory.server_csq_pass = false;
}

void setUploadFactory(bool val)
{
    //gTmpFactory.factory_flag = val;
    gTmpFactory.upload_flag = val;
}

void setUploadOk(bool val)
{
    gTmpFactory.upload_ok = val;
}

void updateTmpFactoryFlag(void)
{
    if(!gTmpFactory.factory_flag)
    {
        return;
    }

    if(0 != gTmpFactory.val.rfid && gTmpFactory.val.carry && gTmpFactory.val.cv
       && gTmpFactory.val.gps && gTmpFactory.val.mse)
    {
        if(gTmpFactory.val.dv_1 == 4)
        {
            void *p = &gTmpFactory.val.adio;
            /* Determine whether the lower 14 bits are 1 */
            if(0x3fff == *(uint32_t *)p)
            {
                gTmpFactory.local_test_pass =true;
                gTmpFactory.upload_flag = true;
                gTmpFactory.val.ts      = getTimeStamp();
            }
        }
        else
        {
            if(gTmpFactory.val.can)
            {
                gTmpFactory.local_test_pass =true;
                gTmpFactory.upload_flag = true;
                gTmpFactory.val.ts      = getTimeStamp();
            }
        }
    }
}

void setFTCSQ(bool pass)
{
    gTmpFactory.server_csq_pass = pass;
}
bool getADFTlocalTestResult()
{
    return gTmpFactory.local_test_pass;
}
static void change_upload_car_state(bool lock, uint32_t rfid, uint32_t ts)
{
    uploadCarState_t tmp = { 0 };

    if(lock)
    {
        tmp.carState = 0;
        tmp.ts       = ts - 1;
        tmp.rfid     = rfid;
        serverSendForkLockState(&tmp);
        tmp.carState = 1;
        tmp.ts       = ts;
        tmp.rfid     = 0;
        serverSendForkLockState(&tmp);
    }
    else
    {
        tmp.carState = 1;
        tmp.ts       = ts - 1;
        tmp.rfid     = 0;
        serverSendForkLockState(&tmp);
        tmp.carState = 0;
        tmp.ts       = ts;
        tmp.rfid     = rfid;
        serverSendForkLockState(&tmp);
    }
    print_log("changed cat state.\n");
}
static void checkRFIDmsg(void)
{
    rfidFIFO_t rfidMsg = { 0 };
    if(true == rfid2Sim868FifoRcv((char*)&rfidMsg))
    {
        print_log("Recv RFID msg: cardId:[%u], operator:[%d], reason:[%d],timestamp [%u],Ms:[%u]\n",
                  toBigEndian(rfidMsg.cardId), (int)rfidMsg.cmd, (int)rfidMsg.status, rfidMsg.ts,
                  rfidMsg.relativityMs);
        // printRTC(&rfidMsg.rtc);

        switch(rfidMsg.cmd)
        {
            case LOCK:
            {
                if(false == g_car_lock)
                {
                    change_upload_car_state(true, rfidMsg.cardId, rfidMsg.ts);
                }
                g_car_lock = true;
                print_log("Set fork state to lock.<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");

                if(checkTimestamp(rfidMsg.ts))
                {
                    uploadRFID_t upload;
                    uint32_t     msgId = getUniqueMsgId();
                    upload.msgId       = msgId;
                    upload.reason      = rfidMsg.status;
                    upload.cardId      = rfidMsg.cardId;
                    upload.ts          = rfidMsg.ts;
                    memcpy(&upload.rtc, &rfidMsg.rtc, sizeof(upload.rtc));

                    if(0 == upload.cardId && 0 != getCurrCardId())
                    {
                        upload.cardId = getCurrCardId();
                        print_log("WARNING upload.carId = [%u]\n", upload.cardId);
                    }
                    else
                    {
                        print_log("RFID cardID:[%u]\n", rfidMsg.cardId);
                    }
                    strncpy(upload.imei, atcGetIMEI(NULL), sizeof(upload.imei));
                    serverSendRFIDLock(msgId, &upload);
                }
                else
                {
                    if(0 != rfidMsg.relativityMs)
                    {
                        if(!gTmpRfid.preempted)
                        {
                            gTmpRfid.cmd       = TMP_LOCK;
                            gTmpRfid.dirty     = true;
                            gTmpRfid.writeFlag = TMP_WRITE;
                            gTmpRfid.getOnTs   = 0; /* Reserved */
                            gTmpRfid.getOnTs   = 0; /* Reserved */
                            gTmpRfid.getOffTs  = 0;
                            gTmpRfid.getOffRs  = rfidMsg.relativityMs;
                            gTmpRfid.cardId    = rfidMsg.cardId;
                            gTmpRfid.reason    = rfidMsg.status;
                            print_log("wait lock to save ms:[%u]\n", rfidMsg.relativityMs);
                            semTakeWriteOk(100);
                        }
                    }
                    else
                    {
                        warning_log("Lock rfid ts:[%u],relativityMs:[%u]\n", rfidMsg.ts,
                                    rfidMsg.relativityMs);
                    }
                }

                break;
            }

            case UNLOCK:
            {
                if(true == g_car_lock)
                {
                    change_upload_car_state(false, rfidMsg.cardId, rfidMsg.ts);
                }
                g_car_lock = false;  //update status
                print_log("Set fork state to unlock.<<<<<<<<<<<<<<<<<<<,\n");

                /* update grfid */
                currRFID_t grfid;
                uint32_t   ts = rfidMsg.ts;
                uint32_t   ms = getKernelTime_32();
                grfid.cardId  = rfidMsg.cardId;
                grfid.localMs = ms;
                grfid.ts      = ts;
                grfid.status  = rfidMsg.status;
                grfid.lock    = true;
                memcpy(&grfid.rtc, &rfidMsg.rtc, sizeof(grfid.rtc));
                setGlobalRFID(&grfid);

                if(checkTimestamp(rfidMsg.ts))
                {
                    uploadRFID_t upload = { 0 };
                    uint32_t     msgId;
                    msgId         = getUniqueMsgId();
                    upload.msgId  = msgId;
                    upload.reason = rfidMsg.status;
                    upload.cardId = rfidMsg.cardId;
                    upload.ts     = ts;
                    memcpy(&upload.rtc, &rfidMsg.rtc, sizeof(upload.rtc));
                    strncpy(upload.imei, atcGetIMEI(NULL), sizeof(upload.imei));
                    serverSendRFIDUnlock(msgId, &upload);
                }
                else
                {
                    if(0 != rfidMsg.relativityMs)
                    {
                        if(!gTmpRfid.preempted)
                        {
                            gTmpRfid.cmd       = TMP_UNLOCK;
                            gTmpRfid.dirty     = true;
                            gTmpRfid.writeFlag = TMP_WRITE;
                            gTmpRfid.getOnTs   = 0;
                            gTmpRfid.getOnRs   = rfidMsg.relativityMs;
                            gTmpRfid.getOffTs  = 0; /* Reserved */
                            gTmpRfid.getOffRs  = 0; /* Reserved */
                            gTmpRfid.cardId    = rfidMsg.cardId;
                            gTmpRfid.reason    = rfidMsg.status;
                            print_log("wait UNlock to save ms:[%u]\n", rfidMsg.relativityMs);
                            semTakeWriteOk(100);
                        }
                    }
                    else
                    {
                        warning_log("Unlock rfid.ts:[%u],relativityMs:[%u]\n", rfidMsg.ts,
                                    rfidMsg.relativityMs);
                    }
                }

                break;
            }

            case UNKNOW:
            {
                uploadUnknowRFID_t upload;
                upload.cardId    = rfidMsg.cardId;
                upload.status    = rfidMsg.status;
                upload.timeStamp = rfidMsg.ts;
                serverSendRFIDUnknow(&upload);

                /* Factory test */
                if(gTmpFactory.factory_flag && !gTmpFactory.upload_flag)
                {
                    if(rfidMsg.cardId != 0)
                    {
                        gTmpFactory.val.rfid = rfidMsg.cardId;
                        print_log("rfid test ok,rfid........................\n");
                    }
                }
                break;
            }

            default:
            {
                warning_log("Not switch.\n");
                break;
            }
        }
    }

    drvRcrdFifo_t driverRecord;
    if(true == DrvRecordFifoRcv((char*)&driverRecord))
    {
        print_log("Recv rfid[%u] onReason:[%d] onTime:[%u] offReason:[%d] offTime:[%u] OnMs:[%u] OffMs:[%u]\n",
                  toBigEndian(driverRecord.cardId), driverRecord.on_reason,driverRecord.getOnTs, driverRecord.off_reason, driverRecord.getOffTs,
                  driverRecord.getOnRelativityMs, driverRecord.getOffRelativityMs);
        if(checkTimestamp(driverRecord.getOnTs) && checkTimestamp(driverRecord.getOffTs))
        {
            uploadDrivingRecord_t driver;
            // print_log("driverRecord.getOnRTC: ");
            // printRTC(&driverRecord.getOnRTC);
            // print_log("driverRecord.getOffRTC: ");
            // printRTC(&driverRecord.getOffRTC);
            driver.cardId   = driverRecord.cardId;
            driver.msgId    = getUniqueMsgId();
            driver.getOnTs  = driverRecord.getOnTs;
            driver.getOffTs = driverRecord.getOffTs;
            driver.on_reason = driverRecord.on_reason;
            driver.off_reason = driverRecord.off_reason;
            memcpy(&driver.getOnRTC, &driverRecord.getOnRTC, sizeof(driver.getOnRTC));
            memcpy(&driver.getOffRTC, &driverRecord.getOffRTC, sizeof(driver.getOffRTC));
            serverSendDrivingRecord(driver.msgId, &driver);
        }
        else
        {
            if((checkTimestamp(driverRecord.getOnTs) || 0 != driverRecord.getOnRelativityMs)
               && (checkTimestamp(driverRecord.getOffTs) || 0 != driverRecord.getOffRelativityMs))
            {
                gTmpRfid.cmd       = TMP_DRIVER;
                gTmpRfid.dirty     = true;
                gTmpRfid.writeFlag = TMP_WRITE;
                gTmpRfid.preempted = true;
                gTmpRfid.getOnTs   = driverRecord.getOnTs;
                gTmpRfid.getOnRs   = driverRecord.getOnRelativityMs;
                gTmpRfid.getOffTs  = driverRecord.getOffTs;
                gTmpRfid.getOffRs  = driverRecord.getOffRelativityMs;
                gTmpRfid.cardId    = driverRecord.cardId;
                print_log("wait driver to save ms:[%u]\n", rfidMsg.relativityMs);
                semTakeWriteOk(100);
            }
            else
            {
                warning_log("driver record getOnTs:[%u],getOnMs:[%u];getOffTs:[%u],getOffMs:[%u]\n",
                            driverRecord.getOnTs, driverRecord.getOnRelativityMs,
                            driverRecord.getOffTs, driverRecord.getOffRelativityMs);
            }
        }
    }
}

void checkTmpWriteRFID(void)
{
    if(TMP_WRITE != gTmpRfid.writeFlag)
    {
        return;
    }

    gTmpRfid.writeFlag = false;
    gTmpRfid.preempted = false;

    switch(gTmpRfid.cmd)
    {
        case TMP_LOCK:
            // print_log("WWWW write lock rfid.1\n");
            gTmpRfid.cmd = TMP_LOCK_TS;
            pushRFIDToTmp(&gTmpRfid);
            break;
        case TMP_UNLOCK:
            // print_log("WWWW write unlock rfid.2\n");
            gTmpRfid.cmd = TMP_UNLOCK_TS;
            pushRFIDToTmp(&gTmpRfid);
            break;
        case TMP_DRIVER:
            // print_log("WWWW write driver rfid.3\n");
            gTmpRfid.cmd = TMP_DRIVER_TS;
            pushRFIDToTmp(&gTmpRfid);
            break;
        default:
            warning_log("Nothing to write.");
            break;
    }

    gTmpRfid.cmd = TMP_NONE;
    // print_log("WWW write rfid ok.4\n");
    semGiveWriteOk();
}

void checkTmpReadRFID(void)
{
    if(!gTmpRfid.dirty || !timeIsAlreadySet() || gTmpRfid.cmd || gTmpRfid.writeFlag)
    {
        return;
    }
    if(popRFIDFromTmp(&gTmpRfid))
    {
        // print_log("RRRR read ok rfid.cardId\n");
        gTmpRfid.writeFlag = TMP_READ;
    }
    else
    {
        print_log("EMPTY TMP file. EEEEEEEEEEEEEEEEEEEEEE\n");
        gTmpRfid.dirty = false;
    }
}

static void checkUploadTmpRFID(void)
{
    if(TMP_READ != gTmpRfid.writeFlag || !timeIsAlreadySet())
    {
        return;
    }

    switch(gTmpRfid.cmd)
    {
        case TMP_LOCK_TS:
        {
            uploadRFID_t upload;
            upload.cardId = gTmpRfid.cardId;
            // print_log("lock ms:[%d]\n",gTmpRfid.getOffRs);
            upload.ts     = thrustTimestamp(gTmpRfid.getOffRs);
            upload.msgId  = getUniqueMsgId();
            upload.reason = gTmpRfid.reason;
            timeStamp2RTC(upload.ts, &upload.rtc);
            strncpy(upload.imei, atcGetIMEI(NULL), sizeof(upload.imei));
            print_log("Lock rfid:[%d],Ts:[%d]\n", upload.cardId, upload.ts);
            if(0 != upload.ts)
            {
                serverSendRFIDLock(upload.msgId, &upload);
                // print_log("SENd ts:[%d]", upload.ts);
                // printRTC(&upload.rtc);
            }
            else
            {
                warning_log("Lock ts:[%d]\n", upload.ts);
            }
        }
        break;
        case TMP_UNLOCK_TS:
        {
            uploadRFID_t upload;
            upload.cardId = gTmpRfid.cardId;
            upload.ts     = thrustTimestamp(gTmpRfid.getOnRs);
            // print_log("rfid:[%d] unlock ms:[%d],ts:[%d],curms:[%d]\n",gTmpRfid.cardId,gTmpRfid.getOnRs,upload.ts,k_uptime_get_32());
            upload.msgId  = getUniqueMsgId();
            upload.reason = gTmpRfid.reason;
            timeStamp2RTC(upload.ts, &upload.rtc);
            strncpy(upload.imei, atcGetIMEI(NULL), sizeof(upload.imei));
            print_log("Unlock ,Rfid:[%d],Ts:[%d]\n", upload.cardId, upload.ts);
            if(0 != upload.ts)
            {
                serverSendRFIDUnlock(upload.msgId, &upload);
                // print_log("SENd unlock ts:[%d]",upload.ts);
                // printRTC(&upload.rtc);
            }
            else
            {
                warning_log("Unlock ts:[%d]\n", upload.ts);
            }
        }
        break;
        case TMP_DRIVER_TS:
        {
            uploadDrivingRecord_t upload;
            upload.cardId = gTmpRfid.cardId;
            upload.msgId  = getUniqueMsgId();
            // print_log("OnRs:[%d],OffRs:[%d]=================\n",gTmpRfid.getOnRs,gTmpRfid.getOffRs);
            if(0 == checkTimestamp(gTmpRfid.getOnTs))
            {
                upload.getOnTs = thrustTimestamp(gTmpRfid.getOnRs);
                // print_log("OnTs:[%d]\n",upload.getOnTs);
            }
            else
            {
                upload.getOnTs = gTmpRfid.getOnTs;
                // print_log("OnTs:[%d]\n",upload.getOnTs);
            }

            // print_log("Onts:[%d]\n",upload.getOnTs);
            if(0 == checkTimestamp(gTmpRfid.getOffTs))
            {
                upload.getOffTs = thrustTimestamp(gTmpRfid.getOffRs);
                // print_log("OffTs:[%d]\n",upload.getOffTs);
            }
            else
            {
                upload.getOffTs = gTmpRfid.getOffTs;
                // print_log("OffTs:[%d]\n",upload.getOffTs);
            }

            print_log("Driver,rfid:[%d],Onts:[%d],OffTs:[%d]\n", upload.cardId, upload.getOnTs,
                      upload.getOffTs);
            if((checkTimestamp(upload.getOnTs)) && (checkTimestamp(upload.getOffTs)))
            {
                timeStamp2RTC(upload.getOnTs, &upload.getOnRTC);
                timeStamp2RTC(upload.getOffTs, &upload.getOffRTC);
                serverSendDrivingRecord(upload.msgId, &upload);
                // print_log("SENd driver Onts:[%d],offTs:[%d]",upload.getOnTs,upload.getOffTs);
                // printRTC(&upload.getOnRTC);
                // printRTC(&upload.getOffRTC);
            }
            else
            {
                warning_log("Onts:[%d], offTs:[%d]\n", upload.getOnTs, upload.getOffTs);
            }
        }
        break;
        default:
            break;
    }

    gTmpRfid.cmd       = TMP_NONE;
    gTmpRfid.writeFlag = TMP_NONE;
    gTmpRfid.preempted = false;
    // print_log("SENd send ok 4\n");
}

static void checkCanErrMsg(void)
{
    canErrFIFO_t canErrMsg;
    if(true == canErrFifoRcv((char*)&canErrMsg))
    {
        uploadCanERR_t upload;
        upload.cardId = getCurrCardId();
        upload.msgId  = getUniqueMsgId();
        upload.ts     = getTimeStamp();
        upload.id     = canErrMsg.id;
        upload.code   = canErrMsg.code;
        serverSendCanERR(upload.msgId, &upload);
    }
}

static void checkCanHourMsg(void)
{
    canWorkHourFIFO_t canHourMsg;

    /* Factory test */
    //7164
    if(gTmpFactory.factory_flag)
    {
        if(true == canWorkHourFifoRcv((char*)&canHourMsg))
        {

            print_log("work hour int[%d,] float[ %f]....\n",(int)canHourMsg.hour,canHourMsg.hour);
            print_log("factory_flag[%d],upload_flag[%d]..\n",gTmpFactory.factory_flag,gTmpFactory.upload_flag);
            if(gTmpFactory.factory_flag && !gTmpFactory.upload_flag && 7164 == (int)canHourMsg.hour)
            {              
                gTmpFactory.val.can = 1;  // 1:is pass
                print_log("can test ok...!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
            }
        }
    }

    
    if(false == isLoginOk())
    {
        return;
    }

    if(true == canWorkHourFifoRcv((char*)&canHourMsg))
    {
        uploadWorkhour_t upload;
        upload.ts       = getTimeStamp();
        upload.workHour = canHourMsg.hour;
        if(upload.ts > 0)
        {
            serverSendWorkHour(&upload);
            print_log("Upload work hour:[%d]\n", (int)upload.workHour);
        }
        else
        {
            warning_log("Work Hour Timestamp is 0 !!!\n");
        }


    }
}

static void checkCanOverspeed(void)
{
    overSpeedFIFO_t canOverspeedMsg;
    if(true == overspeedFifoRcv((char*)&canOverspeedMsg))
    {
        uploadOverspeed_t upload;
        upload.timeStamp    = getTimeStamp();
        upload.msgId        = getUniqueMsgId();
        upload.cardId       = getCurrCardId();
        upload.overspeedVal = canOverspeedMsg.speed;
        serverSendOverSpeed(upload.msgId, &upload);
        print_log("Upload Over Speed : [%d]\n", upload.overspeedVal);
    }
}

static void uploadCarStatus(void)
{
    static uint32_t last_ts = 0;

    uint32_t         curr_ts              = k_uptime_get_32();
    uint32_t         interval             = curr_ts - last_ts;
    uint32_t         UPLOAD_LOCK_INTERVAL = 30 * 1000;  // 30 sec
    uploadCarState_t tmp                  = { 0 };

    if(isLoginOk())
    {
        UPLOAD_LOCK_INTERVAL = gConfig.gCarStateInterval * 1000;
    }
    else
    {
        UPLOAD_LOCK_INTERVAL = gConfig.gCarStateOfflineInterval * 1000;
    }

    if(curr_ts < last_ts || curr_ts - last_ts < UPLOAD_LOCK_INTERVAL)
    {
        return;
    }

    last_ts = curr_ts;

    if(carIsLock())
    {
        tmp.carState = 1;  // car is lock
        tmp.rfid     = getCurrCardId();
        tmp.ts       = getTimeStamp();
        print_log("Tell server car lock.\n");
    }
    else
    {
        tmp.carState = 0;  // car is unlock
        tmp.rfid     = getCurrCardId();
        tmp.ts       = getTimeStamp();
        print_log("Tell server car unlock.\n");
    }

    serverSendForkLockState(&tmp);
}

static void checkCANmsg(void)
{
    canFIFO_t canMsg;
    if(true == canFifoRcv((char*)&canMsg))
    {
        print_log("Receive can msg\n");
        uploadCAN_t upload;
        upload.ts                     = getTimeStamp();
        upload.avgVelocity            = canMsg.averageSpeed;
        upload.distance               = canMsg.moveDistance;
        upload.absDistance            = canMsg.moveAbsDistance;
        upload.batteryVolt            = canMsg.batVoltage;
        upload.batteryCurrent         = canMsg.batCurrent;
        upload.batteryState           = canMsg.batState;
        upload.brakeTime              = canMsg.brakePeriod;
        upload.seatTime               = canMsg.seatPeriod;
        upload.forkTime               = canMsg.forkPeriod;
        upload.overlapTime            = canMsg.moveForkPeriod;
        upload.forkCounter            = canMsg.forkNums;
        upload.moveCounter            = canMsg.moveNums;
        upload.forwardCounter         = canMsg.forwardNums;
        upload.reverseCounter         = canMsg.backwardNums;
        upload.directionChangeCounter = canMsg.divertNums;
        upload.movingTime             = canMsg.movePeriod;
        upload.forwardTime            = canMsg.forwardPeriod;
        upload.reverseTime            = canMsg.backwardPeriod;
        upload.statistical_time       = canMsg.collectPeriod;
        upload.forwardDistance        = canMsg.forwardDistance;
        upload.reverseDistance        = canMsg.backwardDistance;
        upload.carry_time             = canMsg.carryPeriod;
        upload.carry_counter          = canMsg.carryNums;
        upload.cardLen                = 4;
        upload.cardId                 = getCurrCardId();
        printCan(&upload);
        serverSendCanInfo(&upload);
    }
}

static void checkCurrDirverReq(void)
{
    if(semTakeCurrDriver())
    {
        uploadCurrentRFID_t tmp  = { 0 };
        currRFID_t          rfid = { 0 };

        getGlobalRFID(&rfid);
        if(!checkTimestamp(rfid.ts))
        {
            rfid.ts = thrustTimestamp(rfid.localMs);
        }

        tmp.msgId    = getUniqueMsgId();
        tmp.cardId   = getCurrCardId();
        tmp.reserved = 0;
        tmp.ts       = rfid.ts;
        strncpy(tmp.imei, atcGetIMEI(NULL), sizeof(tmp.imei));
        if(checkTimestamp(tmp.ts))
        {
            timeStamp2RTC(tmp.ts, &(tmp.rtc));
            serverSendCurrentDriver(&tmp);
        }
        else
        {
            warning_log("Device driver no timestamp.\n");
        }

        print_log("Device report Current Driver.\n");
    }
}

static void checkAppendRFIDack(void)
{
    rfidAckFIFO_t tmp = { 0 };
    if(appendRFIDFifoRcv((char*)&tmp))
    {
        // print_log("Tell servet count:[%d]..........................\n",tmp.rfid.val);
        serverSendRFIDUpdateAck(&(tmp.rfid));
    }
}

static uint8_t count = 0;
static void    updateMseTime(bool f)
{
    static uint32_t last_tm = 0;
    uint32_t        curr_tm = k_uptime_get_32();

    if(0 == count)
    {
        last_tm = curr_tm;
        return;
    }

    if(f)
    {
        last_tm = curr_tm;
    }
    else
    {
        uint32_t diff     = curr_tm - last_tm;
        uint32_t interval = 15 * 1000;

        for(int i = 0; i < count; ++i)
        {
            interval *= 2;
        }
        // print_log("clear diff:[%d] interval:[%d]\n",diff, interval);
        if(diff > interval)
        {
            count   = 0;
            last_tm = curr_tm;
            print_log("CLear mse upload interval.%%%%%%%%%%%%%%%%%%%%%%\n");
        }
    }
}

static bool checkUploadMseInterval()
{

    static uint32_t last_tm  = 0;
    uint32_t        curr_tm  = k_uptime_get_32();
    uint32_t        interval = 15 * 1000;
    uint32_t        diff_tm  = curr_tm - last_tm;

    if(0 == count)
    {
        interval = 0;
    }
    else
    {
        for(int i = 0; i < count; ++i)
        {
            interval *= 2;
        }
    }

    // print_log("mse upload diff_tm:[%d] interval:[%d]\n",diff_tm,interval);
    if(diff_tm >= interval)
    {
        count++;
        if(count > 6)
        {
            count = 6;
        }
        last_tm = curr_tm;
        return true;
    }

    return false;
}

static void checkMseInfo(void)
{
    static uploadMSE_t max_mse = { 0 };
    speedLimitFIFO_t   slMsg   = { 0 };
    if(true == speedLimitFifoRcv((char*)&slMsg))
    {
        print_log("val:[%d], reason:[%d], flag:[%d]\n", slMsg.val, slMsg.reason, slMsg.limit_flag);
        if(TYPE_VIBRATION == slMsg.reason)
        {
            uploadMSE_t upload = { 0 };

            upload.timestamp            = getTimeStamp();
            upload.msgId                = getUniqueMsgId();
            upload.mse                  = slMsg.val;
            upload.forkSpeedLimitStatus = slMsg.limit_flag;
            if(gConfig.authType == AUTH_RFID)
            {
                upload.cardId = getCurrCardId();
            }
            else
            {
                upload.cardId = 0;
            }

            if(upload.mse > max_mse.mse)
            {
                max_mse = upload;
            }
        }
        else
        {
            uploadCRASH_t upload = { 0 };

            upload.timestamp = getTimeStamp();
            if(gConfig.authType == AUTH_RFID)
            {
                upload.cardId = getCurrCardId();
            }
            else
            {
                upload.cardId = 0;
            }
            upload.crash_type           = slMsg.reason;
            upload.crash_val            = slMsg.val;
            upload.forkSpeedLimitStatus = slMsg.limit_flag;
            serverSendCrash(&upload);
            print_log("SEND to Serve Speed Limit:[%d] reason:[ %d ] val:[ %d ]\n",
                      upload.forkSpeedLimitStatus, upload.crash_type, upload.crash_val);
        }
        updateMseTime(true);

        /* Factory test */
        if(gTmpFactory.factory_flag && !gTmpFactory.upload_flag)
        {
            gTmpFactory.val.mse = 1;
            print_log("test mse is ok!....\n");
        }
    }
    else
    {
        updateMseTime(false);
    }

    if(0 != max_mse.msgId && checkUploadMseInterval())
    {
        serverSendMSE(max_mse.msgId, &max_mse);
        print_log("SEND to Server Speed Limit:[%d] val:[%d]\n", max_mse.forkSpeedLimitStatus,
                  max_mse.mse);
        memset(&max_mse, 0, sizeof(max_mse));
    }
}

static void printFactoryPin(uploadTestV14_t upload)
{
    print_log("  \n\tcap_vol:[%d]\tvin_vol:[%d]\tbat_vol:[%d]\tkey_vol:[%d]\n"
              "\tadc_in0:[%d]\tadc_in1:[%d]\tadc_in2:[%d]\tadc_in3:[%d]\n"
              "\tgpi0:[%d]\tgpi1:[%d]\tgpi2:[%d]\tgpi3:[%d]\tint_in0:[%d]\tint1:[%d]\n",
              upload.cap_vol, upload.vin_vol, upload.bat_vol, upload.key_vol, upload.adc_in0,
              upload.adc_in1, upload.adc_in2, upload.adc_in3, upload.gpi0, upload.gpi1, upload.gpi2,
              upload.gpi3, upload.int_in0, upload.int_in1);
}

static void checkFactryV14(void)
{
    factryV14FIFO_t factryMsg;
    if(true == factryTestV14FifoRcv((char*)&factryMsg))
    {
        print_log("@@@@@@@@@@@@@@@@@REV  factryTestV14FifoRcv!!!!!!!!!!!!!!!.....\n");
        uploadTestV14_t upload = { 0 };
        upload.cap_vol = factryMsg.cap_vol;
        upload.vin_vol = factryMsg.vin_vol;
        upload.bat_vol = factryMsg.bat_vol;
        upload.key_vol = factryMsg.key_vol;
        upload.adc_in0 = factryMsg.adc_in0;
        upload.adc_in1 = factryMsg.adc_in1;
        upload.adc_in2 = factryMsg.adc_in2;
        upload.adc_in3 = factryMsg.adc_in3;
        upload.gpi0    = factryMsg.gpi0;
        upload.gpi1    = factryMsg.gpi1;
        upload.gpi2    = factryMsg.gpi2;
        upload.gpi3    = factryMsg.gpi3;
        upload.int_in0 = factryMsg.int_in0;
        upload.int_in1 = factryMsg.int_in1;
        // serverSendFactryTestV14(&upload);
        // printFactoryPin(upload);
        if(!gTmpFactory.upload_flag)
        {
            gTmpFactory.val.adio = upload;
        }
    }
}

static void checkCanType(void)
{
    autoCanFIFO_t can;
    if(true == autoCanResultFifoRcv((char*)&can))
    {
        print_log("@@@@@@@@ recv can type :[%d], pattern:[%d]\n", can.canTypeResult,
                  can.canPatternResult);
        adapationCan_t upload = { 0 };
        upload.can_type       = can.canTypeResult;
        upload.can_pattern    = can.canPatternResult;
        serverSendCanType(&upload);
        print_log("Send can type to server cantype:[%d], canPattern:[%d]\n", upload.can_type,
                  upload.can_pattern);
        restartKionFactory(can.canTypeResult, can.canPatternResult);
    }
}

static void checkFactoryTestUpload(void)
{
    uint32_t upload_interval = 10 * 1000;  //10 sec
    uint32_t curr_ts         = k_uptime_get_32();
    uint32_t diff            = curr_ts - gTmpFactory.upload_ts;

    if(gTmpFactory.upload_ok)
    {
        return;
    }
   // print_log("diff[%d],gTmpFactory.upload_flag [%d]\n",diff,gTmpFactory.upload_flag);
    if(diff > upload_interval && gTmpFactory.upload_flag)
    {
        if(isMqttConnect())
        {
            serverSendFactoryRecord(&gTmpFactory.val);
            print_log("Upload Factory val:\n[ts:%d \t adio:%d \t rfid:%u \t cv:%d \t csq:%d \t deg:%d \t can:%d "
                    "\t carry:%d \t mse:%d \t gps:%d \t dv:%d.%d.%d.%d \t sv:%d.%d.%d.%d]\n",
                    gTmpFactory.val.ts, gTmpFactory.val.adio, gTmpFactory.val.rfid,
                    gTmpFactory.val.cv, gTmpFactory.val.csq, gTmpFactory.val.csq_deg,
                    gTmpFactory.val.can, gTmpFactory.val.carry, gTmpFactory.val.mse,
                    gTmpFactory.val.gps, gTmpFactory.val.dv_0, gTmpFactory.val.dv_1,
                    gTmpFactory.val.dv_2, gTmpFactory.val.dv_3, gTmpFactory.val.sv_0,
                    gTmpFactory.val.sv_1, gTmpFactory.val.sv_2, gTmpFactory.val.sv_3);
            if(gTmpFactory.val.gps)
            {
                print_log("gps.lon[%lu]\t,lat[%lu]\t,hdop[%u]\t,speed[%u]\n",
                        gTmpFactory.val.longitude,gTmpFactory.val.latitude,gTmpFactory.val.hdop,gTmpFactory.val.speed);
            }
            gTmpFactory.upload_ts = curr_ts;
        }
        else
        {
            // print_log("delay mqtt connect ................................\n");
        }
        
    }
}

static void checkFactoryRemainderTerm()
{
    aidongTestResult_t test;
    if(0 == AiDongTestModuleState(&test))
    {
        gTmpFactory.val.carry = test.rs485;
        gTmpFactory.val.cv    = test.contrlRelay;
        print_log("carry =%d,cv=%d..\n",gTmpFactory.val.carry,gTmpFactory.val.cv );
    }

    actGetLastCsq(&gTmpFactory.val.csq, &gTmpFactory.val.csq_deg);

    hwGpsMsg_t  gps = getDFTtailGps();
    //if(isGetGps())
    if(gps.flag)
    {
        gTmpFactory.val.latitude  = (uint64_t)(gps.lat * 1000000);
        gTmpFactory.val.longitude = (uint64_t)(gps.lon * 1000000);
        gTmpFactory.val.speed     = (uint32_t)(gps.speed * 1.852 * 1000);
        gTmpFactory.val.hdop = 1000;
        gTmpFactory.val.gps = 1;
        //print_log("test gps  ok....\n");
   
    }
}

static void checkFactory(void)
{
    if(!gTmpFactory.factory_flag)
    {
        return;
    }

    checkFactoryRemainderTerm();
    checkFactryV14();
    updateTmpFactoryFlag();
    checkFactoryTestUpload();

    return;
}

static void checkAutoFota(void)
{
    autoFotaFIFO_t msg = {0};
    if(true == autoFotaFifoRcv((char*)&msg))
    {
        serverRequestFotaAutoStart(&msg.aFV);
        print_log("Start auto Fota ------------------>\n");
    }
}

#if 0
void tcCrash(void)
{
    static uint32_t last_ts = 0;
    uint32_t curr_ts = k_uptime_get_32();
    uint32_t diff = curr_ts - last_ts;
    if(diff < 10000)
    {
        return ;
    }
    last_ts = curr_ts;
    uploadCRASH_t upload = {0};
    upload.cardId = 1;
    upload.timestamp = getTimeStamp();
    upload.crash_type = 1;
    upload.crash_val = 2000;
    upload.forkSpeedLimitStatus = 1;
    serverSendCrash(&upload);
    print_log("send crash........\n");
}
#endif

#ifdef DEBUG_THREAD_FILE //if debug define it in my_misc.h
bool fileflag = false;
void checkfile(void)
{
    uint32_t curr_ms = k_uptime_get_32();
    static uint32_t last_ms = 0;
    const uint32_t interval = 5*1000;
    uint32_t diff = curr_ms - last_ms;

    if(!fileflag)
    {
        return ;
    }

    if(diff < interval)
    {
        return ;
    }
    
    stopThreadFile();
    curr_ms = last_ms;
}
#endif

static void messageThreadEntry(void* p1)
{
    /* step uptime */
    g_last_message_center_thread_uptime = k_uptime_get_32();

    /* set config */
    memcpy(&gConfig, p1, sizeof(gConfig));
    print_log("auth type: [%u]\n", gConfig.authType);
    if(0 != gConfig.authType)
    {
        g_car_lock = true;
        print_log("Set fork state to lock.<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
    }
    else
    {
        g_car_lock = false;
        print_log("Set fork state to unlock.<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
    }

    while(1)
    {
        checkRFIDmsg();
        checkUploadTmpRFID();
        uploadCarStatus();
        checkCANmsg();
        checkCanErrMsg();
        checkCanHourMsg();
        checkCanOverspeed();
        checkCurrDirverReq();
        checkAppendRFIDack();
        checkFotaOperateACK();
        checkMseInfo();
        checkCanType();
        checkFactory();
        checkAutoFota();
        
        #ifdef DEBUG_THREAD_FILE
        checkfile();
        #endif

        // print_log("in message thread loop.\n");
        g_last_message_center_thread_uptime = k_uptime_get_32();
        k_sleep(100);
    }
}


bool messageCenterTHreadStart(messagePara_t para)
{
    bool ret;
    g_last_message_center_thread_uptime = k_uptime_get_32();

    gMessageThreadId = k_thread_create(
        &gMessageThread, gMessageThreadStack, MESSAGE_THREAD_STACK_SIZE,
        (k_thread_entry_t)messageThreadEntry, &para, NULL, NULL, K_PRIO_COOP(10), 0, 0);

    if(gMessageThreadId != 0)
    {
        ret = 0;
        print_log("Create Message THREAD Id:[ %p ]; Stack:[ %p ]; Size:[ %p ]\n", gMessageThreadId,
                  gMessageThreadStack, MESSAGE_THREAD_STACK_SIZE);
    }
    else
    {
        ret = -1;
        err_log("Create Thread Message Failed.\n\n");
    }

    return ret;
}

void messageCenterTHreadSafetyStop(void)
{
    if(gMessageThreadId != 0)
    {
        k_thread_abort(gMessageThreadId);
        gMessageThreadId = 0;
    }
}

static void setGlobalRFID(currRFID_t* ps)
{
    memcpy(&gRfid, ps, sizeof(gRfid));
}

bool carIsLock(void)
{
    return g_car_lock;
}

uint32_t getCurrCardId(void)
{
    return gRfid.cardId;
}

static void getGlobalRFID(currRFID_t* ps)
{
    memcpy(ps, &gRfid, sizeof(currRFID_t));
}

/* thread file call it */
static bool pushRFIDToTmp(void* ps)
{
    FIL      flp;
    bool     val;
    uint32_t size       = 0;
    uint32_t write_size = 0;
    uint32_t len        = sizeof(tmpRFID_t);
    char*    filename   = TMP_RFID_FILE;

    if(!ps)
    {
        return false;
    }

    if(FR_OK != f_open(&flp, filename, FA_OPEN_ALWAYS | FA_WRITE | FA_READ))
    {
        err_log("Can't open file:%s.\n", filename);
        return false;
    }
    size = f_size(&flp);
    if(size + len > MAX_TMP_RFID_DILE_SIZE)
    {
        val = false;
        goto END;
    }

    f_lseek(&flp, size);
    if(FR_OK == f_write(&flp, ps, len, &write_size))
    {
        val = true;
        print_log("Push [%d] bytes to file %s.\n", write_size, filename);
    }
    else
    {
        val = false;
    }

END:
    f_close(&flp);
    return val;
}

/* thread file call it */
static bool popRFIDFromTmp(void* ps)
{
    FIL      flp;
    bool     val;
    uint32_t ops       = 0;
    uint32_t size      = 0;
    uint32_t read_size = 0;
    uint32_t len       = sizeof(tmpRFID_t);
    char*    filename  = TMP_RFID_FILE;

    if(!ps)
    {
        return false;
    }

    if(FR_OK != f_open(&flp, filename, FA_OPEN_ALWAYS | FA_WRITE | FA_READ))
    {
        err_log("Can't open file:%s.\n", filename);
        val = false;
        goto END;
    }

    size = f_size(&flp);
    print_log("%s file size:%d\n", filename, size);
    if(size < len)
    {
        val = false;
        goto END;
    }
    else
    {
        ops = size - len;
    }

    f_lseek(&flp, ops);
    if(FR_OK != f_read(&flp, ps, len, &read_size))
    {
        val = false;
        goto END;
    }
    else
    {
        if(read_size != len)
        {
            val = false;
            goto END;
        }
        else
        {
            val = true;
        }
    }

    f_lseek(&flp, ops);
    if(FR_OK != f_truncate(&flp))
    {
        err_log("Truncate file %s Failed!\n", filename);
    }

END:
    f_close(&flp);
    if(val)
    {
        print_log("Pop [%d]Bytes from file %s.\n", read_size, filename);
    }

    return val;
}

void deleteTmpRFIDFile(void)
{
    FIL   flp;
    char* filename = TMP_RFID_FILE;

    if(FR_OK != f_open(&flp, filename, FA_CREATE_ALWAYS | FA_WRITE | FA_READ))
    {
        err_log("Can't delete file:%s.\n", filename);
        return;
    }

    f_close(&flp);

    f_unlink(filename);

    print_log("DELETE file %s\n", filename);
    return;
}

void detect_thread_message_center(uint32_t ts)
{
    if((ts > g_last_message_center_thread_uptime)
       && (ts - g_last_message_center_thread_uptime > THREAD_LOOP_TIME_5SEC))
    {
        messagePara_t para = gConfig;
        warning_log("Restart message center thread ts:[%d] uptime:[%d].\n", ts,
                    g_last_message_center_thread_uptime);
        messageCenterTHreadSafetyStop();
        messageCenterTHreadStart(para);
    }
}


static void printCan(uploadCAN_t* upload)
{
    print_log("upload can msg:\n"
              "\tts:[ %u ]\tRFID:[ %u ]\tstatisticalTime:[ %d ]\n"
              "\tavgSpeed:[ %d ]\tdistance   :[ %d ]\tabsDistance:[ %d ]\n"
              "\tforkTime:[ %d ]\toverlapTime:[ %d ]\tforkCounter:[ %d ]\n"
              "\tseatTime:[ %d ]\tbrakeTime  :[ %d ]\t                   \n"
              "\tmoveTime:[ %d ]\tforwardTime:[ %d ]\treverseTime:[ %d ]\n"
              "\tbatteryState:[ %d ]\tbatteryVolt:[ %d ]\tbatteryCurrent:[ %d ]   \n"
              "\tmoveCounter :[ %d ]\tforwardCounter:[ %d ]\treverseCounter:[ %d ]\n"
              "\tforwardDistance:[ %d ]\treverseDistance[ %d ]\tdirectionChangeCounter[ %d ]\n"
              "\tcarryTime:[ %d ]\tcarryCounter[ %d ]\n",
              upload->ts, toBigEndian(upload->cardId), upload->statistical_time,
              upload->avgVelocity, upload->distance, upload->absDistance, upload->forkTime,
              upload->overlapTime, upload->forkCounter, upload->seatTime, upload->brakeTime,
              upload->movingTime, upload->forwardTime, upload->reverseTime, upload->batteryState,
              upload->batteryVolt, upload->batteryCurrent, upload->moveCounter,
              upload->forwardCounter, upload->reverseCounter, upload->forwardDistance,
              upload->reverseDistance, upload->directionChangeCounter, upload->carry_time,
              upload->carry_counter);
}
