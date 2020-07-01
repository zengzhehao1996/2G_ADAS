#include "server_interface.h"
#include "message_center_thread.h"
#include "network_thread.h"
#include "protocol.h"
#include "my_misc.h"
#include "rtc.h"
#include "config.h"
#include "file.h"
#include "my_md5.h"
#include "config.h"
#include "smart_link_version.h"
#include "thread_wdt.h"
#include "fota_upgrade_thread.h"
#include <string.h>
#include "sempho.h"
#include "fifo.h"
#include "msg_structure.h"
#include "atc.h"
#include "hw_version.h"
#include "factory_test.h"

extern sysconfig_t gSysconfig;
enum SERVER_CMDID
{
    SERVER_LOGIN_REQ = 0x0c,
    SERVER_LOGIN_ACK = 0x0c01,

    SERVER_TIME_REQ = 0x1,  //TODO may be delete
    SERVER_TIME_ACK = 0x2,  //TODO may be delete

    SERVER_HB_REQ = 0x5,  //TODO may be delete
    SERVER_HB_ACK = 0x6,  //TODO may be delete

    SERVER_CAN_ERROR_REQ     = 0x32,
    SERVER_IS_CAR_LOCKED_REQ = 0x34,

    SERVER_SIM_CARD_UPLOAD_REQ = 0x51,
    SERVER_GET_SIM_CARD_CMD    = 0x54,

    SERVER_CMD_REBOOT = 0xae,  //reboot the device

    SERVER_TRANSFER_CONFIG_REQ = 0x1a, /* request config */
    SERVER_TRANSFER_CONFIG_ACK = 0x1b,

    SER_GET_DEVICE_CONFIG_ACK = 0x80,
    SER_GET_DEVICE_CONFIG     = 0x81,

    SERVER_UPLOAD_LOG_REQ = 0x101,
    SERVER_UPLOAD_CAN_REQ = 0x103,
    SERVER_UPLOAD_GPS_REQ = 0x105,

    SERVER_MSE_UPLOAD_REQ = 0x123,
    DEV_UPLOAD_CRASH      = 0x125,

    //RFID related
    CMD_RFID_CLEAR               = 0x130,
    SERVER_RFID_UNKOWN_REQ       = 0x133,
    SERVER_RFID_LOCK_REQ         = 0x134,
    SERVER_RFID_UNLOCK_REQ       = 0x135,
    SERVER_RFID_BLOCK_UPDATE_REQ = 0x138,
    CMD_RFID_BLOCK_UPDATE        = 0x137,
    CMD_RFID_BLOCK_UPDATE_ACK    = 0x136,

    SERVER_FORK_WORK_HOUR_UPLOAD = 0x150,
    REPORTING_OVERSPEED_VAL      = 0x172,


    CMD_GET_CURRENT_DRIVER_ACK = 0x190,
    CMD_GET_CURRENT_DRIVER     = 0x191,

    SERVER_GET_VERSION_ACK = 0x200,
    SERVER_GET_VERSION_REQ = 0x201,

    SERVER_GET_RFID_MD5_ACK = 0x210,
    SERVER_GET_RFID_MD5_REQ = 0x211,

    SERVER_CMD_SPEED_LIMIT_DISABLE_ACK                   = 0x220,
    SERVER_CMD_SPEED_LIMIT_DISABLE                       = 0x221,
    SERVER_RELIEVES_THE_VIBRATION_SPEED_LIMIT_STATUS_ACK = 0x222,
    SERVER_RELIEVES_THE_VIBRATION_SPEED_LIMIT_STATUS     = 0x223,

    SERVER_RFID_DRIVER_RECORD = 0x230,

    CMD_FOTA_START_ACK                   = 0x300,
    CMD_FOTA_START                       = 0x301,
    SERVER_SEND_FILE_BYTES_ACK           = 0x302,
    SERVER_SEND_FILE_BYTES               = 0x303,
    SERVER_FOTA_VERSION_VERIFICATION_ACK = 0X304,
    SERVER_FOTA_VERSION_VERIFICATION     = 0X305,
    FOTA_EVENTS                          = 0X306,
    SERVER_TERMINATES_FOTA_DOWNLOAD_ACK  = 0x308,
    SERVER_TERMINATES_FOTA_DOWNLOAD      = 0x309,
    DEV_FACTRY_TEST_IO_RESULT            = 0x30b,
    DEV_FOTA_REQ_VERSION                 = 0x310,
    DEV_FOTA_REQ_VERSION_ACK             = 0x311,
    DEV_FOTA_AUTO_DOWNLOAD               = 0x312,
    // 0x313 Reserved

    SER_KION_FACRORY_TEST_START_ACK = 0x500,
    SER_KION_FACTORY_TEST_START     = 0x501,
    DEV_REPORT_CANTYPE              = 0x502,
    DEV_REPORT_CANTYPE_ACK          = 0x503,
    SER_KION_FACTORY_TEST_STOP_ACK  = 0x504,
    SER_KION_FACTORY_TEST_STOP      = 0x505,
    KION_FACTORY_TEST_CAN           = 0x510,
    KION_FACTORY_TEST_RFID_UNKNOW   = 0X512,


    SERVER_CMD_FOR_DEBUG_ACK = 0x1001, /* Only used for debug */

    SER_FACTORY_TEST_START     = 0x1119,
    SER_FACTORY_TEST_START_ACK = 0x1124,
    SER_FACTORY_TEST_STOP      = 0x1120,
    SER_FACTORY_TEST_STOP_ACK  = 0x1125,
    FACTORY_TEST_CAN           = 0x1121,
    FACTORY_TEST_MSE           = 0x1122,
    FACTORY_TEST_GPS           = 0x1123,
    FACTORY_TEST_RECORD        = 0x1124,
};

static uint32_t g_last_message_time  = 0;
static uint8_t  g_run_mode           = NORMAL_MODE;
static int32_t  debugCmd             = 0;
static uint8_t  g_run_test_mode_once = NORMAL_MODE;

#define SERVER_BUFFER_SIZE 1024
static enum {
    WRITE_RFID_FLAG = 1,
    WRITE_FOTA_FLAG = 2,
};
static struct server_buffer
{
    uint8_t  buffer[SERVER_BUFFER_SIZE];
    uint8_t  md5[16];
    uint16_t pos;
    uint8_t  counter;
    uint8_t  flag; /* WRITE_RFID_FLAG:rfid, WRITE_FOTA_FLAG:fota */
} serverBuf;

static void set_run_test_mode(void);

int checkSaveRFIDList(void)
{
    if(WRITE_RFID_FLAG != serverBuf.flag || serverBuf.counter == 0
       || SAVE_2_FILE_MAX < serverBuf.counter)
    {
        return 0;
    }

    rfidAckFIFO_t tmp = { 0 };

    if(0 == serverBuf.pos)
    {
        if(writeRfidList(serverBuf.buffer, serverBuf.pos))
        {
            if(1 == serverBuf.counter)
            {
                appendRFIDFifoSend((char*)&tmp);
            }
            serverBuf.counter = 0;
            serverBuf.flag    = 0;
            writeRFIDListMd5(serverBuf.md5);  //TODO
        }
        else
        {
            if(1 == serverBuf.counter)
            {
                appendRFIDFifoSend((char*)&tmp);
            }
            serverBuf.counter++;
        }
    }
    else
    {
        if(writeRfidList(serverBuf.buffer, serverBuf.pos))
        {
            if(1 == serverBuf.counter)
            {
                tmp.rfid.val = serverBuf.pos / 4;
                memcpy(tmp.rfid.md5, serverBuf.md5, sizeof(tmp.rfid.md5));
                appendRFIDFifoSend((char*)&tmp);
                // print_log("++++++++++++++++++++++++++++++++++++++++++++++++++ count:[%d]\n",tmp.rfid.val);
            }
            serverBuf.counter = 0;
            serverBuf.flag    = 0;
            writeRFIDListMd5(serverBuf.md5);  //TODO
        }
        else
        {
            if(1 == serverBuf.counter)
            {
                tmp.rfid.val = 0;
                appendRFIDFifoSend((char*)&tmp);
            }
            serverBuf.counter++;
        }
    }

    //tell read thread
    semGiveRfidListUpdata();

    return serverBuf.counter;
}

void updateBlockRFID(const uint8_t* pdata, uint32_t len)
{
    uint8_t md5[16];
    if(len % 4 == 0 && len != 0)
    {
        print_log("RFID card numbers : %d\n", len / 4);
        for(int i = 0; i < len; i++)
        {
            printk("0x%02x,", pdata[i]);
        }
        printk("\n");
        md5CheckBlock(pdata, len, md5);
        if(0 == serverBuf.flag)
        {
            memcpy(serverBuf.buffer, pdata, len);
            memcpy(serverBuf.md5, md5, sizeof(serverBuf.md5));
            serverBuf.pos     = len;
            serverBuf.counter = 1;
            serverBuf.flag    = WRITE_RFID_FLAG;
        }
        else
        {
            serverSendLog("ERROR: UPDATE RFID 1");
            warning_log("UPDATE RFID 1");
        }
    }
    else if(0 == len)
    {
        if(0 == serverBuf.flag)
        {
            memset(serverBuf.buffer, 0, sizeof(serverBuf.buffer));
            memset(serverBuf.md5, 0, sizeof(serverBuf.md5));
            serverBuf.pos     = 4;
            serverBuf.counter = 1;
            serverBuf.flag    = WRITE_RFID_FLAG;
            serverSendLog("Server send 0 card.");
        }
        else
        {
            serverSendLog("ERROR: UPDATE RFID 2");
            warning_log("UPDATE RFID 2");
        }
    }
    else
    {
        updateRFIDack_t tmp = { 0 };
        tmp.val             = -1;
        serverSendRFIDUpdateAck(&tmp);
    }
}

static void resetUploadConfig(downloadConfig_t* ps)
{
    if(!ps)
    {
        warning_log("Para Error.\n");
    }
    memset(ps, 0, sizeof(gSysconfigBackup.serverAddr));
    strncpy(ps->server_IP, gSysconfigBackup.serverAddr, sizeof(ps->server_IP));
    ps->server_Port            = gSysconfigBackup.serverPort;
    ps->can_type               = gSysconfigBackup.canType;
    ps->can_pattern            = gSysconfigBackup.canPattern;
    ps->auth_type              = gSysconfigBackup.authType;
    ps->hb_interval            = gSysconfigBackup.hbInterval;
    ps->can_upload_interval    = gSysconfigBackup.canMaxuploadInterval;
    ps->gps_upload_interval    = gSysconfigBackup.gpsUploadInterval;
    ps->seat_timeout           = gSysconfigBackup.seatoffTimeout;
    ps->over_speed             = gSysconfigBackup.overspeedAlarmThreshold;
    ps->mse_threshold          = gSysconfigBackup.mseThreshold;
    ps->move_type              = gSysconfigBackup.moveType;
    ps->fork_type              = gSysconfigBackup.forkType;
    ps->move_threshold         = gSysconfigBackup.moveThreshold;
    ps->fork_threshold         = gSysconfigBackup.forkThreshold;
    ps->carry_threshold        = gSysconfigBackup.carry_threshold;
    ps->overload_threshold     = gSysconfigBackup.overload_threshold;
    ps->online_state_interval  = gSysconfigBackup.onlineStateInterval;
    ps->offline_state_interval = gSysconfigBackup.offlineStateInterval;
}

static void getCurrConfig(downloadConfig_t* pconfig)
{
    strncpy(pconfig->server_IP, gSysconfig.serverAddr, sizeof(gSysconfig.serverAddr));
    pconfig->server_Port               = gSysconfig.serverPort;
    pconfig->can_type                  = gSysconfig.canType;
    pconfig->can_pattern               = gSysconfig.canPattern;
    pconfig->auth_type                 = gSysconfig.authType;
    pconfig->hb_interval               = gSysconfig.hbInterval;
    pconfig->can_upload_interval       = gSysconfig.canMaxuploadInterval;
    pconfig->gps_upload_interval       = gSysconfig.gpsUploadInterval;
    pconfig->seat_timeout              = gSysconfig.seatoffTimeout;
    pconfig->over_speed                = gSysconfig.overspeedAlarmThreshold;
    pconfig->mse_threshold             = gSysconfig.mseThreshold;
    pconfig->move_type                 = gSysconfig.moveType;
    pconfig->fork_type                 = gSysconfig.forkType;
    pconfig->move_threshold            = gSysconfig.moveThreshold;
    pconfig->fork_threshold            = gSysconfig.forkThreshold;
    pconfig->carry_threshold           = gSysconfig.carry_threshold;
    pconfig->overload_threshold        = gSysconfig.overload_threshold;
    pconfig->online_state_interval     = gSysconfig.onlineStateInterval;
    pconfig->offline_state_interval    = gSysconfig.offlineStateInterval;
    pconfig->crash_switch              = gSysconfig.crashSwitch;
    pconfig->vibration_limit_threshold = gSysconfig.vibrationLimitThreshold;
    pconfig->crash_low_threshold       = gSysconfig.crashLowThreshold;
    pconfig->crash_middle_threshold    = gSysconfig.crashMiddleThreshold;
    pconfig->crash_high_threshold      = gSysconfig.crashHighThreshold;
}

static void updateConfig(const uint8_t* pdata, uint32_t len)
{
    uint32_t          copy_len = 0;
    downloadConfig_t* pconfig  = (downloadConfig_t*)pdata;
    downloadConfig_t  tmp;
    resetUploadConfig(&tmp);
    // printConfig(&tmp);
    copy_len = sizeof(tmp) >= len ? len : sizeof(tmp);
    print_log("copy len is [%d]\n", copy_len);
    memcpy(&tmp, pconfig, copy_len);
    if(0 != copy_len)
    {
        memset(gSysconfigBackup.serverAddr, 0, sizeof(gSysconfigBackup.serverAddr));
        strncpy(gSysconfigBackup.serverAddr, pconfig->server_IP,
                sizeof(gSysconfigBackup.serverAddr));
        gSysconfigBackup.serverPort              = tmp.server_Port;
        gSysconfigBackup.canType                 = tmp.can_type;
        gSysconfigBackup.canPattern              = tmp.can_pattern;
        gSysconfigBackup.seatType                = tmp.seatType;
        gSysconfigBackup.authType                = tmp.auth_type;
        gSysconfigBackup.hbInterval              = tmp.hb_interval;
        gSysconfigBackup.canMaxuploadInterval    = tmp.can_upload_interval;
        gSysconfigBackup.gpsUploadInterval       = tmp.gps_upload_interval;
        gSysconfigBackup.seatoffTimeout          = tmp.seat_timeout;
        gSysconfigBackup.overspeedAlarmThreshold = tmp.over_speed;
        gSysconfigBackup.mseThreshold            = tmp.mse_threshold;
        gSysconfigBackup.moveType                = tmp.move_type;
        gSysconfigBackup.forkType                = tmp.fork_type;
        gSysconfigBackup.moveThreshold           = tmp.move_threshold;
        gSysconfigBackup.forkThreshold           = tmp.fork_threshold;
        gSysconfigBackup.carry_threshold         = tmp.carry_threshold;
        gSysconfigBackup.overload_threshold      = tmp.overload_threshold;
        gSysconfigBackup.onlineStateInterval     = tmp.online_state_interval;
        gSysconfigBackup.offlineStateInterval    = tmp.offline_state_interval;
        gSysconfigBackup.crashSwitch             = tmp.crash_switch;
        gSysconfigBackup.vibrationLimitThreshold = tmp.vibration_limit_threshold;
        gSysconfigBackup.crashLowThreshold       = tmp.crash_low_threshold;
        gSysconfigBackup.crashMiddleThreshold    = tmp.crash_middle_threshold;
        gSysconfigBackup.crashHighThreshold      = tmp.crash_high_threshold;
        gSysconfigBackup.autoFotaSwitch          = tmp.autoFotaSwitch;
        gSysconfigBackup.offlineCanInterval      = tmp.offlineCanInterval;

        print_log("Server Update serverIP              : [ %s ]\n", gSysconfigBackup.serverAddr);
        print_log("Server Update serverPort            : [ %d ]\n", gSysconfigBackup.serverPort);
        print_log("Server Update can type              : [ %d ]\n", gSysconfigBackup.canType);
        print_log("Server Update can Pattern           : [ %d ]\n", gSysconfigBackup.canPattern);
        print_log("Server Update auth type             : [ %d ]\n", gSysconfigBackup.authType);
        print_log("Server Update hb interval           : [ %d ]\n", gSysconfigBackup.hbInterval);
        print_log("Server Update can interval          : [ %d ]\n",
                  gSysconfigBackup.canMaxuploadInterval);
        print_log("Server Update gps interval          : [ %d ]\n",
                  gSysconfigBackup.gpsUploadInterval);
        print_log("Server Update seat timeout          : [ %d ]\n",
                  gSysconfigBackup.seatoffTimeout);
        print_log("Server Update over speed            : [ %d ]\n",
                  gSysconfigBackup.overspeedAlarmThreshold);
        print_log("Server Update mse threshold         : [ %d ]\n", gSysconfigBackup.mseThreshold);
        print_log("Server Update move type             : [ %d ]\n", gSysconfigBackup.moveType);
        print_log("Server Update fork type             : [ %d ]\n", gSysconfigBackup.forkType);
        print_log("Server Update move threshold        : [ %d ]\n", gSysconfigBackup.moveThreshold);
        print_log("Server Update fork threshold        : [ %d ]\n", gSysconfigBackup.forkThreshold);
        print_log("Server Update carry_threshold       : [ %d ]\n",
                  gSysconfigBackup.carry_threshold);
        print_log("Server Update overload_threshold    : [ %d ]\n",
                  gSysconfigBackup.overload_threshold);
        print_log("Server Update online state interval : [ %d ]\n",
                  gSysconfigBackup.onlineStateInterval);
        print_log("Server Update Offline state interval: [ %d ]\n",
                  gSysconfigBackup.offlineStateInterval);
        print_log("Server Update crash switch          : [ %d ]\n", gSysconfigBackup.crashSwitch);
        print_log("Server Update vibration limit       : [ %d ]\n",
                  gSysconfigBackup.vibrationLimitThreshold);
        print_log("Server Update crash low threshold   : [ %d ]\n",
                  gSysconfigBackup.crashLowThreshold);
        print_log("Server Update crash middle threshold: [ %d ]\n",
                  gSysconfigBackup.crashMiddleThreshold);
        print_log("Server Updata crash high threshold  : [ %d ]\n",
                  gSysconfigBackup.crashHighThreshold);
        print_log("Server Updata seat type             : [ %d ]\n", gSysconfigBackup.seatType);
        print_log("Server Update sauto fota            : [ %d ]\n",
                  gSysconfigBackup.autoFotaSwitch);
        print_log("Server Update offline can interval  : [ %d ]\n",
                  gSysconfigBackup.offlineCanInterval);
        uint8_t can_type = gSysconfigBackup.canType;
        if(gSysconfig.canType != 200 && can_type == 200)
        {
            /* delete all file */
            //TODO reset rfid
            deleteAllCache();            //cache
            deleteSeatTypeConfigFile();  // delete seat type file
            gSysconfig.canType = can_type;
        }

        /* update mpu config */
#if 0 /* support in the future */
        speedLimitConfig_t mpu_config;
        getMpuConfig(&mpu_config);
        mpu_config.carsh_switch              = gSysconfigBackup.crashSwitch;
        mpu_config.vibration_threshold       = gSysconfigBackup.mseThreshold;
        mpu_config.vibration_limit_threshold = gSysconfigBackup.vibrationLimitThreshold;
        mpu_config.crash_low_threshold       = gSysconfigBackup.crashLowThreshold;
        mpu_config.crash_mid_threshold       = gSysconfigBackup.crashMiddleThreshold;
        mpu_config.crash_hig_threshold       = gSysconfigBackup.crashHighThreshold;
        setMpuConfig(&mpu_config);
#endif

        setSaveConfig2File();
    }
    else
    {
        serverSendLog("config len ERROR");
        warning_log("config ERROR len:[%d]\n", len);
    }
}

static void checkAutoFotaVersion(hardVersion_t hV, softVersion_t sV)
{
    hardVersion_t  lhV = { 0 };
    softVersion_t  lsV = { SOFT_VERSION_MAJOR, SOFT_VERSION_MINOR, SOFT_VERSION_TINY,
                          SOFT_VERSION_PATCH };
    autoFotaFIFO_t aF  = { 0 };

    getHardVersion(&lhV.major, &lhV.minor, &lhV.tiny, &lhV.least);

    if(hV.major != lhV.major || hV.minor != lhV.minor || hV.tiny != lhV.tiny
       || hV.least != lhV.least)
    {
        char buf[96] = { 0 };
        sprintf(buf, "Hard Version ERROR. server:[%d.%d.%d.%d], dev:[%d.%d.%d.%d]\n", hV.major,
                hV.minor, hV.tiny, hV.least, lhV.major, lhV.minor, lhV.tiny, lhV.least);
        serverSendLog(buf);
        print_log(buf);
        return;
    }

    if(sV.major != lsV.major || sV.minor != lsV.minor || sV.tiny != lsV.tiny || sV.least < lsV.least
       || sV.least >= 100)
    {
        char buf[96] = { 0 };
        sprintf(buf, "Hard Version ERROR. server:[%d.%d.%d.%d], dev:[%d.%d.%d.%d]\n", sV.major,
                sV.minor, sV.tiny, sV.least, lsV.major, lsV.minor, lsV.tiny, lsV.least);
        serverSendLog(buf);
        print_log(buf);
        return;
    }
    else if(sV.least == lsV.least)
    {
        print_log("FOTA Version consistency does not require an upgrade...\n");
        return;
    }

    /* upgrade */
    aF.aFV.ts          = getTimeStamp();
    aF.aFV.hardVersion = hV;
    aF.aFV.softVersion = sV;
    //send to message
    autoFotaFifoSend((char*)&aF);
}

void serverCmdParser(uint16_t cmdid, uint16_t version, const uint8_t* pdata, uint32_t len)
{
    print_log("CmdParser, cmdId:[0x%04x],len:[%u]\n", cmdid, len);
    g_last_message_time = k_uptime_get_32();

    switch(cmdid)
    {
        case SERVER_LOGIN_ACK:
        case SERVER_HB_ACK:
        {
            setLoginOk(true);
            print_log("Log in OK or hb ack!!!!!!!!!!!!!!!!!!!\n");

            uint32_t timeStamp = *(uint32_t*)pdata;
            setTimeStamp(timeStamp);
            localRTC_t rtc = { 0 };
            timeStamp2RTC(timeStamp, &rtc);
            setRTC(&rtc);
            char buf[64] = { 0 };
            sprintf(buf, "server set ts:[%d]\n", timeStamp);
            // print_log("timeStamp:[%u]\n", timeStamp);
            print_log(buf);
            printRTC(&rtc);
            break;
        }

        case SERVER_GET_SIM_CARD_CMD:
        {
            print_log("Server Get SIM card.\n");
            serverSendSimCard(atcGetCCID(NULL));
            print_log("Tell server sim card : [%s]\n", atcGetCCID(NULL));
            break;
        }

        case SERVER_CMD_REBOOT:
        {
            thread_wdt_stop();
            break;
        }

        case SERVER_TRANSFER_CONFIG_ACK:
        {
            print_log("Transfer config version : [%d]\n", version);
            updateConfig(pdata, len);
            break;
        }

        case SER_GET_DEVICE_CONFIG:
        {
            downloadConfig_t config = { 0 };
            getCurrConfig(&config);
            serverSendCurrConfig(&config);
            print_log("Send current config to server.\n");
        }

        case CMD_RFID_CLEAR:
        {
            uint32_t tmp = 0;
            writeRfidList(&tmp, 1);
            print_log("Clear RFID.\n");
            break;
        }
        case CMD_RFID_BLOCK_UPDATE:
        {
            updateBlockRFID(pdata, len);
            print_log("rcv server cmd "
                      "updateBlockRFID############################################################"
                      "\n");
            break;
        }
        case CMD_GET_CURRENT_DRIVER:
        {
            semGiveCurrDriver();
            print_log("Derver Get cuttent driver.\n");
            break;
        }

        case SERVER_GET_VERSION_REQ: /* step start network thread */
        {
            uploadVersion_t version = { g_hw_major,        g_hw_minor,         g_hw_tiny,
                                        g_hw_patch,        SOFT_VERSION_MAJOR, SOFT_VERSION_MINOR,
                                        SOFT_VERSION_TINY, SOFT_VERSION_PATCH };
            strncpy(version.text, g_hw_text, sizeof(version.text));
            serverSendVersion(&version);
            print_log("Send version...\n");
            break;
        }

        case SERVER_GET_RFID_MD5_REQ:
        {
            //TODO move to file thread
            uint8_t md5[MD5_SIZE] = { 0 };
            readRFIDListMd5(md5);
            serverSendRFIDmd5(md5);
            break;
        }

        case SERVER_CMD_SPEED_LIMIT_DISABLE:
        {
#if 0 /* support in the future */
            uploadReleaseLimitRet_t val = { 0 };
            print_log("SERVER CMD disable speed limit.\n");

            uint32_t msgId = *(uint32_t*)pdata;
            val.msgId      = msgId;
            int8_t ret     = syncSpeedLimitOff();
            if(0 == ret)
            {
                val.ret = 1;
            }
            else if(1 == ret)
            {
                val.ret = 2;
            }
            else
            {
                val.ret = 0;
            }

            serverSendLimitDisable(&val);
            print_log("tell server disable speed limit ret:[%d]\n", val.ret);
#else
            serverSendLog("The device does not support the speed limit function");
#endif
            break;
        }

        case SERVER_RELIEVES_THE_VIBRATION_SPEED_LIMIT_STATUS:
        {
#if 0 /* support in the future */
            uploadLimitStatus_t val  = { 0 };
            speedLimitFlag_t    flag = { 0 };

            print_log("SERVER CMD limit status........\n");

            uint32_t msgId = *(uint32_t*)pdata;
            val.msgId      = msgId;

            getCurrSpeedLimit(&flag);
            val.limit_flag = flag.limit_flag;
            serverSendLimitStatus(&val);
            print_log("tell server limit status:[%d]\n", val.limit_flag);
#else
            serverSendLog("The device does not support the speed limit function");
#endif
            break;
        }

        case CMD_FOTA_START:
        {
            print_log("<<<<<<<<<<<<<<<<<<<<<<<< Server start fota.\n");
            doFotaStart(pdata, len);
            break;
        }

        case SERVER_SEND_FILE_BYTES:
        {
            print_log("<<<<<<<<<<<<<<<<<<<<<<<< server send bytes.\n");
            pushFotaBytes(pdata, len);
            break;
        }

        case SERVER_FOTA_VERSION_VERIFICATION:
        {
            doFotaSolidifyFirm();
            break;
        }
        case SERVER_TERMINATES_FOTA_DOWNLOAD:
        { /* step start network thread */
            doFotaStop();
            break;
        }

        case DEV_FOTA_REQ_VERSION_ACK:
        {
            autoFotaVersion_t* pV = (autoFotaVersion_t*)pdata;
            checkAutoFotaVersion(pV->hardVersion, pV->softVersion);
        }
        break;

        case SER_KION_FACTORY_TEST_START:
        {
            int             ret = -1;
            factoryStart_t* p   = (factoryStart_t*)pdata;
            print_log("Server Start Kion factory can_type:[%d], can_pattern:[%d], cmd:[%d]\n",
                      pdata[0], pdata[1], pdata[2]);

            if(NORMAL_MODE == g_run_mode)
            {
                testStart_t para = { KION_FACTORY_MODE, p->can_type, p->cmd, p->can_pattern };
                set_run_test_mode();
                factoryTestThreadStart(para);
                g_run_mode = KION_FACTORY_MODE;
                ret        = 0;
            }
            else
            {
                ret = -1;
                print_log("device run mode:[%d].\n", g_run_mode);
            }

            serverSendKionFactoryStartACk(ret);
            print_log("Start Kion factory ret:[%d]\n", ret);
            break;
        }
        case SER_KION_FACTORY_TEST_STOP:
        {
            int ret = 0;
            print_log("Server Stop Kion factory test.\n");

            if(KION_FACTORY_MODE == g_run_mode)
            {
                factoryTestThreadStop();
                g_run_mode = NORMAL_MODE;
                ret        = 0;
            }
            else
            {
                ret = -1;
                print_log("device run mode:[%d]\n", g_run_mode);
            }
            serverSendKionFactoryStopACk(ret);
            print_log("Stop Kion factory ret:[%d]\n", ret);
            break;
        }
        case SER_FACTORY_TEST_START:
        {
            // uint16_t factoryMsg = *(uint16_t*)pdata;
            // int      ret        = -1;
            // print_log("\tWWWWWWWWWWWWWWWWWWrev SER_FACTORY_TEST_START msg body = %ld !!!!!!!\n",
            //           factoryMsg);
            // if(g_run_mode == NORMAL_MODE && gSysconfig.canType == 200)
            // {
            //     testStart_t para = { FACTORY_MODE, 0, 0 , 0};
            //     factoryTestThreadStart(para);
            //     g_run_mode = FACTORY_MODE;
            //     ret        = 0;
            //     print_log("factory test   start!! ,factoryTsetFlag = %d,canType = %d\n", g_run_mode,
            //               gSysconfig.canType);
            // }
            // else
            // {
            //     print_log("factory test can not start!! ,factoryTsetFlag = %d,canType = %d\n",
            //               g_run_mode, gSysconfig.canType);
            //     ret = -1;
            // }

            // serverSendFactoryStartACK(ret);
            // print_log("Start Factory ret:[%d]\n", ret);
            serverSendLog("Abandon protocal 0x1119.");
            break;
        }

        case SER_FACTORY_TEST_STOP:
        {
            uint16_t factoryMsg = *(uint16_t*)pdata;
            int      ret        = 0;
            print_log("\tWWWWWWWWWWWWWWWWWWWWrev SER_FACTORY_TEST_STOP msg body = %ld !!!!!!!\n",
                      factoryMsg);
            // if(g_run_mode == FACTORY_MODE && gSysconfig.canType == 200)
            // {
            //     factoryTestThreadStop();
            //     g_run_mode      = NORMAL_MODE;
            //     ret             = 0;
            //     print_log("factory test  stop!! ,factoryTsetFlag = %d,canType = %d\n",
            //               g_run_mode, gSysconfig.canType);
            // }
            // else
            // {
            //     print_log("factory test can not  stop!! ,factoryTsetFlag = %d,canType = %d\n",
            //               g_run_mode, gSysconfig.canType);
            //     ret = -1;
            // }
            g_run_mode = NORMAL_MODE;
            if(1 == factoryMsg)
            {

                //setAidongTestPass();
                setFTCSQ(true);
                if(getADFTlocalTestResult())
                {
                    //test ok!
                    setAidondTestResult(true);
                }
                else
                {
                    //test failed!
                    setAidondTestResult(false);
                }
            }
            else if(0 == factoryMsg)
            {
                setFTCSQ(false);
                //test failed
                setAidondTestResult(false);
            }

            setUploadOk(true);

            serverSendFactoryStopACK(ret);
            print_log("Stop Factory ret:[%d]\n", ret);

            break;
        }
#if 1

        case SERVER_CMD_FOR_DEBUG_ACK:
        {
            /* debug code in here */
            debugCmd = *(int32_t*)pdata;

#ifdef DEBUG_THREAD_FILE
            extern bool fileflag;
            if(0 == debugCmd)
            {
                fileflag = false;
            }
            else
            {
                fileflag = true;
            }
#endif

            serverSendLog("###################  debug.\n");
            print_log("Debug CMD = %d.>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>..\n", debugCmd);
            print_log("Debug..done_______________________________\n");

            break;
        }
#endif
        default:
            break;
    }
    //print_log("END CmdParser, cmdId:[0x%04x],len:[%u]\n", cmdid, len);
}
extern void initTmpFactory(void);
void        startFactoryTest(void)
{
    if(g_run_mode == NORMAL_MODE)
    {
        g_run_mode = FACTORY_MODE;
        initTmpFactory();
    }
    else
    {
        warning_log("Not in Normal mode.\n");
    }

    return;
}

void factoryTestTimeout(void)
{
    if(g_run_mode == FACTORY_MODE)
    {
        g_run_mode = NORMAL_MODE;
        setUploadFactory(true);
    }
    else
    {
        warning_log("Not in Factory mode.\n");
    }
    return;
}

enum run_mode isRunMode(void)
{
    return g_run_mode;
}

bool serverSendBytes(uint32_t msgId, uint16_t cmdId, uint16_t version, uint8_t* pdata, uint32_t len)
{
    bool ret;

    if(false == isLoginOk() && msgId < 2) /* >=2: have offline cache */
    {
        return false;
    }

    uint8_t head[12] = { 0 };

    if(-1 == cpPaserFillHead(cmdId, version, len, head, cpHeadSize()))
    {
        err_log("Can't fill head.\n");
        return false;
    }
    int val;
    val = pushCache(msgId, len, pdata, cpHeadSize(), head);

    if(val != 0)
    {
        ret = false;
    }
    else
    {
        ret = true;
    }
    print_log("push to cache val[%d].\n", val);

    return ret;
}

bool serverSendCanBytes(uint32_t msgId, uint16_t cmdId, uint16_t version, uint8_t* pdata,
                        uint32_t len)
{
    bool ret;

    if(false == isLoginOk() && msgId < 2) /* >=2: have offline cache */
    {
        return false;
    }

    uint8_t head[12] = { 0 };

    if(-1 == cpPaserFillHead(cmdId, version, len, head, cpHeadSize()))
    {
        err_log("Can't fill head.\n");
        return false;
    }
    int val;
    val = pushCanCache(msgId, len, pdata, cpHeadSize(), head);

    if(val != 0)
    {
        ret = false;
    }
    else
    {
        ret = true;
    }
    print_log("push to can cache val[%d].\n", val);

    return ret;
}

bool SendImTopicBytes(uint32_t msgId, uint16_t cmdId, uint16_t version, uint8_t* pdata,
                      uint32_t len)
{
    bool ret;

    if(false == isLoginOk() && msgId < 2)
    {
        return false;
    }

    uint8_t head[12] = { 0 };

    if(-1 == cpPaserFillHead(cmdId, version, len, head, cpHeadSize()))
    {
        err_log("Can't fill head.\n");
        return false;
    }
    int val;
    val = pushImCache(msgId, len, pdata, cpHeadSize(), head);

    if(val != 0)
    {
        ret = false;
    }
    else
    {
        ret = true;
    }
    print_log("push to cache val[%d].\n", val);

    return ret;
}


bool serverLoginInMQTT(const uint8_t* pstr, uint8_t* outBuf)
{
    uint8_t buf[15 + 12] = { 0 };
    bool    ret;

    if(15 != strlen(pstr))
    {
        print_log("IMEI ERROR.\n");
        return false;
    }

    if(-1
       == cpPaserFillHead(SERVER_LOGIN_REQ, 1, strlen(pstr), buf, cpHeadSize()))  // modified to 2
    {
        err_log("Can't fill head.\n");
        return false;
    }

    memcpy(&buf[cpHeadSize()], pstr, strlen(pstr));

    memcpy(outBuf, buf, sizeof(buf));

    return ret;
}

bool serverGetTimestamp(void)
{
    return serverSendBytes(UPLOAD_ONCE, SERVER_TIME_REQ, 1, NULL, 0);
}

bool serverHeart(uint8_t* pstr)
{
    return serverSendBytes(UPLOAD_ONCE, SERVER_HB_REQ, 1, pstr, strlen(pstr));
}

bool serverSendCanERR(uint32_t msgId, uploadCanERR_t* ps)
{
    return serverSendBytes(msgId, SERVER_CAN_ERROR_REQ, 3, ps, sizeof(uploadCanERR_t));
}

bool serverSendForkLockState(uploadCarState_t* ps)
{
    return serverSendBytes(getUniqueMsgId(), SERVER_IS_CAR_LOCKED_REQ, 2, ps,
                           sizeof(uploadCarState_t));
}

bool serverSendSimCard(uint8_t* pstr)
{
    return serverSendBytes(UPLOAD_ONCE, SERVER_SIM_CARD_UPLOAD_REQ, 1, pstr, strlen(pstr));
}

bool serverRequestConfig(uint8_t* pstr)
{
    uint16_t version = 6;
    print_log("Request config version [%d].\n", version);
    return SendImTopicBytes(UPLOAD_ONCE, SERVER_TRANSFER_CONFIG_REQ, version, pstr, 15);
}

bool serverSendLog(uint8_t* pstr)
{
    return serverSendBytes(UPLOAD_ONCE, SERVER_UPLOAD_LOG_REQ, 1, pstr, strlen(pstr));
}

bool serverSendErrLog(uint8_t* pstr)
{
    return serverSendBytes(UPLOAD_REPORT, SERVER_UPLOAD_LOG_REQ, 1, pstr, strlen(pstr));
}

#ifdef DEBUG_OFFINE_THREAD
bool serverSendstring(uint8_t* pstr)
{
    uint32_t msgid = getUniqueMsgId();
    print_log("msgid = %d.......................\n", msgid);
    return serverSendBytes(msgid, SERVER_UPLOAD_LOG_REQ, 1, pstr, strlen(pstr));
}
#endif

bool serverSendCanInfo(uploadCAN_t* ps)
{
    uint16_t msgid;
    uint32_t upload_count;
    if(FACTORY_MODE == g_run_mode)
    {
        msgid        = FACTORY_TEST_CAN;
        upload_count = UPLOAD_ONCE;
        return;
    }
    else if(KION_FACTORY_MODE == g_run_mode)
    {
        msgid        = KION_FACTORY_TEST_CAN;
        upload_count = UPLOAD_ONCE;
    }
    else
    {
        msgid        = SERVER_UPLOAD_CAN_REQ;
        upload_count = UPLOAD_CAN;
    }

    return serverSendCanBytes(upload_count, msgid, 5, ps, sizeof(uploadCAN_t));
    // return serverSendBytes(upload_count, msgid, 5, ps, sizeof(uploadCAN_t)); //TODO
}

bool serverSendGpsInfo(uploadGPS_t* ps)
{
    uint16_t proid = SERVER_UPLOAD_GPS_REQ;

    if(FACTORY_MODE == g_run_mode)
    {
        proid = FACTORY_TEST_GPS;
        return;
    }
    else
    {
        proid = SERVER_UPLOAD_GPS_REQ;
    }
    //UPLOAD_REPORT
    return serverSendBytes(UPLOAD_ONCE, proid, 1, ps, sizeof(uploadGPS_t));  //TODO
    //return serverSendBytes(UPLOAD_REPORT, proid, 1, ps, sizeof(uploadGPS_t));
}

bool serverSendVersion(uploadVersion_t* ver)
{
    return serverSendBytes(UPLOAD_ONCE, SERVER_GET_VERSION_ACK, 3, ver, sizeof(uploadVersion_t));
}

bool serverSendMSE(uint32_t msgId, uploadMSE_t* ps)
{
    uint16_t proid;
    if(FACTORY_MODE == g_run_mode)
    {
        proid = FACTORY_TEST_MSE;
        msgId = UPLOAD_ONCE;
        return;
    }
    else
    {
        proid = SERVER_MSE_UPLOAD_REQ;
    }

    return serverSendBytes(msgId, proid, 3, ps, sizeof(uploadMSE_t));
}

bool serverSendCrash(uploadCRASH_t* ps)
{
    return serverSendBytes(UPLOAD_REPORT, DEV_UPLOAD_CRASH, 1, ps, sizeof(uploadCRASH_t));
}

bool serverSendLimitStatus(uploadLimitStatus_t* ps)
{
    return serverSendBytes(UPLOAD_ONCE, SERVER_RELIEVES_THE_VIBRATION_SPEED_LIMIT_STATUS_ACK, 1, ps,
                           sizeof(uploadLimitStatus_t));
}

bool serverSendLimitDisable(uploadReleaseLimitRet_t* ps)
{
    return serverSendBytes(UPLOAD_ONCE, SERVER_CMD_SPEED_LIMIT_DISABLE_ACK, 1, ps,
                           sizeof(uploadReleaseLimitRet_t));
}

bool serverSendRFIDUnknow(uploadUnknowRFID_t* ps)
{
    uint16_t proid;
    if(KION_FACTORY_MODE == g_run_mode)
    {
        proid = KION_FACTORY_TEST_RFID_UNKNOW;
    }
    else
    {
        proid = SERVER_RFID_UNKOWN_REQ;
    }
    return serverSendBytes(UPLOAD_ONCE, proid, 2, ps, sizeof(uploadUnknowRFID_t));
}

bool serverSendRFIDLock(uint32_t msgId, uploadRFID_t* ps)
{
    return serverSendBytes(msgId, SERVER_RFID_LOCK_REQ, 4, ps, sizeof(uploadRFID_t));
}

bool serverSendRFIDUnlock(uint32_t msgId, uploadRFID_t* ps)
{
    return serverSendBytes(msgId, SERVER_RFID_UNLOCK_REQ, 4, ps, sizeof(uploadRFID_t));
}

bool serverSendCurrentDriver(uploadCurrentRFID_t* ps)
{
    return serverSendBytes(UPLOAD_ONCE, CMD_GET_CURRENT_DRIVER_ACK, 1, ps,
                           sizeof(uploadCurrentRFID_t));
}

bool serverSendDrivingRecord(uint32_t msgId, uploadDrivingRecord_t* ps)
{
    return serverSendBytes(msgId, SERVER_RFID_DRIVER_RECORD, 2, ps, sizeof(uploadDrivingRecord_t));
}

bool serverRequestRFIDList(uint8_t* md5)
{
    return SendImTopicBytes(UPLOAD_ONCE, SERVER_RFID_BLOCK_UPDATE_REQ, 4, md5, 16);
}

bool serverSendRFIDUpdateAck(updateRFIDack_t* ps)
{
    return serverSendBytes(UPLOAD_ONCE, CMD_RFID_BLOCK_UPDATE_ACK, 4, ps, sizeof(updateRFIDack_t));
}

bool serverSendWorkHour(uploadWorkhour_t* ps)
{
    return serverSendBytes(UPLOAD_ONCE, SERVER_FORK_WORK_HOUR_UPLOAD, 2, ps,
                           sizeof(uploadWorkhour_t));
}

// may be delete it
// bool serverSetOverspeedACK(int val)
// {
//     return serverSendBytes(UPLOAD_ONCE, CMD_SET_OVERSPEED_THRESHOLD_VAL_ACK, 1, &val, sizeof(int));
// }

bool serverSendOverSpeed(uint32_t msgId, uploadOverspeed_t* ps)
{
    return serverSendBytes(UPLOAD_CAN, REPORTING_OVERSPEED_VAL, 2, ps, sizeof(uploadOverspeed_t));
}

bool serverSendRFIDmd5(uint8_t* md5)
{
    return serverSendBytes(UPLOAD_ONCE, SERVER_GET_RFID_MD5_ACK, 4, md5, MD5_SIZE);
}

bool serverSendCCID(uint8_t* str)
{
    return serverSendBytes(UPLOAD_ONCE, SERVER_SIM_CARD_UPLOAD_REQ, 2, str,
                           20);  //TODO some issue no important
}

bool serverSendFotaStartACK(uint32_t val)
{
    fotaStartACK_t msg = { 0 };
    msg.ret            = val;
    msg.timeStamp      = getTimeStamp();
    // print_log("Fota start ack val:[%d] ts:[%d]\n",msg.ret,msg.timeStamp);
    return SendImTopicBytes(UPLOAD_ONCE, CMD_FOTA_START_ACK, 2, &msg, sizeof(msg));
}

#define FOTA_SAME_ACK_INTERVAL \
    (15 * 1000)  // 10 sec  need to change accorting to communicate circle
bool serverSendFotaLineACK(uint32_t val)
{
    bool            ret;
    static uint32_t last_val          = 0;
    static uint32_t last_send_time    = 0;
    uint32_t        current_send_time = k_uptime_get_32();  //ms
    uint32_t        interval_time     = current_send_time - last_send_time;
    //print_log("FOTA interval : [%u]\n",interval_time);
    if(last_val != val || interval_time > FOTA_SAME_ACK_INTERVAL)
    {
        fotaFileBytesACK_t msg = { 0 };
        msg.length             = val;
        msg.timeStamp          = getTimeStamp();
        // print_log("Fota Line ack val:[%d] ts:[%d]\n",msg.length,msg.timeStamp);
        ret = SendImTopicBytes(UPLOAD_ONCE, SERVER_SEND_FILE_BYTES_ACK, 2, &msg, sizeof(msg));
    }
    else
    {
        ret = false;
        err_log("The same ACK for FOTA.\n");
    }
    last_send_time = current_send_time;
    last_val       = val;
    return ret;
}

bool serverSendFotaSolidify(uint32_t val)
{
    fotaVerificationACK_t msg = { 0 };
    msg.ret                   = val;
    msg.timeStamp             = getTimeStamp();
    // print_log("Fota Solidify ack val:[%d] ts:[%d]\n",msg.ret,msg.timeStamp);
    return SendImTopicBytes(UPLOAD_ONCE, SERVER_FOTA_VERSION_VERIFICATION_ACK, 2, &msg,
                            sizeof(msg));
}

bool serverSendFotaEvent(uint32_t val)
{
    fotaEvent_t msg = { 0 };
    msg.event       = val;
    msg.timeStamp   = getTimeStamp();
    // print_log("Fota event ack val:[%d] ts:[%d]\n",msg.event,msg.timeStamp);
    return SendImTopicBytes(UPLOAD_ONCE, FOTA_EVENTS, 2, &msg, sizeof(msg));
}

bool serverSendFotaStopACK(uint32_t val)
{
    fotaStopACK_t msg = { 0 };
    msg.ret           = val;
    msg.timeStamp     = getTimeStamp();
    // print_log("Fota stop ack val:[%d] ts:[%d]\n",msg.ret,msg.timeStamp);
    return SendImTopicBytes(UPLOAD_ONCE, SERVER_TERMINATES_FOTA_DOWNLOAD_ACK, 2, &msg, sizeof(msg));
}

bool serverRequestFotaVersion(autoFotaRequest_t* ps)
{
    return SendImTopicBytes(UPLOAD_ONCE, DEV_FOTA_REQ_VERSION, 1, ps, sizeof(autoFotaRequest_t));
}

bool serverRequestFotaAutoStart(autoFotaVersion_t* ps)
{
    return SendImTopicBytes(UPLOAD_ONCE, DEV_FOTA_AUTO_DOWNLOAD, 1, ps, sizeof(autoFotaVersion_t));
}

bool serverSendFactryTestV14(uploadTestV14_t* ps)
{
    return serverSendBytes(UPLOAD_ONCE, DEV_FACTRY_TEST_IO_RESULT, 1, ps, sizeof(uploadTestV14_t));
}

bool serverSendFactoryStartACK(int val)
{
    return serverSendBytes(UPLOAD_ONCE, SER_FACTORY_TEST_START_ACK, 1, &val, sizeof(int));
}

bool serverSendFactoryStopACK(int val)
{
    return serverSendBytes(UPLOAD_ONCE, SER_FACTORY_TEST_STOP_ACK, 1, &val, sizeof(int));
}

bool serverSendFactoryRecord(uploadFactoryTest_t* ps)
{
    // return serverSendBytes(UPLOAD_ONCE_RFID, FACTORY_TEST_RECORD, 1, ps, sizeof(uploadFactoryTest_t));
    return SendImTopicBytes(UPLOAD_ONCE_RFID, FACTORY_TEST_RECORD, 1, ps,
                            sizeof(uploadFactoryTest_t));
}

bool serverSendKionFactoryStartACk(int val)
{
    return serverSendBytes(UPLOAD_ONCE, SER_KION_FACRORY_TEST_START_ACK, 1, &val, sizeof(int));
}

bool serverSendKionFactoryStopACk(int val)
{
    return serverSendBytes(UPLOAD_ONCE, SER_KION_FACTORY_TEST_STOP_ACK, 1, &val, sizeof(int));
}

bool serverSendCanType(adapationCan_t* ps)
{
    gSysconfigBackup.canType    = ps->can_type;
    gSysconfigBackup.canPattern = ps->can_pattern;
    setSaveConfig2File();
    return serverSendBytes(UPLOAD_REPORT, DEV_REPORT_CANTYPE, 1, ps, sizeof(adapationCan_t));
}

void restartKionFactory(uint8_t cantype, uint8_t canPattern)
{
    factoryTestThreadStop();
    testStart_t para = { KION_FACTORY_MODE, cantype, 0, canPattern };
    factoryTestThreadStart(para);
}

bool serverSendCurrConfig(downloadConfig_t* ps)
{
    return serverSendBytes(UPLOAD_ONCE, SER_GET_DEVICE_CONFIG_ACK, 4, ps, sizeof(downloadConfig_t));
}


/* 1 reserved for non-retransmission messa; 2 reserved for can; 3 reserved for once rfid 4: reserver for report*/
static volatile uint32_t gMsgId = 5;

uint32_t getUniqueMsgId(void)
{
    return gMsgId++;
}

uint32_t messageInterval(void)
{
    uint32_t curr_time = k_uptime_get_32();
    uint32_t diff      = 0;
    if(curr_time > g_last_message_time)
    {
        diff = curr_time - g_last_message_time;
    }
    else
    {
        diff = 0;
    }
    return diff;
}

void updateHbLastTime(void)
{
    g_last_message_time = k_uptime_get_32();
}

static bool g_isLoginOk = false;
bool        isLoginOk(void)
{
    return g_isLoginOk;
}
void setLoginOk(bool val)
{
    g_isLoginOk = val;
}
int32_t getServerDebugCmd()
{
    return debugCmd;
}

static void set_run_test_mode(void)
{
    g_run_test_mode_once = !NORMAL_MODE;
}

bool is_test_mode(void)
{
    return (NORMAL_MODE == g_run_test_mode_once);
}
