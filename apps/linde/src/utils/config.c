#include "config.h"
#include "my_misc.h"
#include "my_file.h"
#include "sempho.h"
#include "server_interface.h"

#define MAX_CONFIG_SIZE 256  /* bytes */

sysconfig_t        gSysconfig;
sysconfig_t        gSysconfigBackup;
memconfig_t        gMemconfig;
seatTypeConfig_t   gSeatTypeConfig;
bool gConfigFlag = false;
static uint8_t saveSeatTypeConfig2FileFlag=0;   
extern classFile_t gConfigFile;
extern classFile_t gSeatTypeConfigFile;

int checkSaveSeatTypeConfig2File(void);
void resetConfig(void)
{
    memset(&gSysconfig, 0, sizeof(gSysconfig));

    memset(gSysconfig.devId, 0, sizeof(gSysconfig.devId));
    strncpy(gSysconfig.serverAddr, SYS_CONFIG_DEFAULT_SERVER_ADDRESS,
            sizeof(gSysconfig.serverAddr));
    gSysconfig.serverPort              = SYS_CONFIG_DEFAULT_SERVER_PORT;
    gSysconfig.authType                = 0;    /* no 0---2 */ 
    gSysconfig.canType                 = 200;    /* 1275 */
    gSysconfig.canMaxuploadInterval    = 60;   /* 60 sec */
    gSysconfig.seatoffTimeout          = 600;  /* 10 min */
    gSysconfig.gpsUploadInterval       = 300;  /* 5min (30~65000)*/
    gSysconfig.hbInterval              = 30;   /* 30 sec (30~1800)*/
    gSysconfig.hbTimeout               = 120;  /* 2 min */
    gSysconfig.mseThreshold            = 1500; /* 15% */
    gSysconfig.overspeedAlarmThreshold = 120000; /* 120km/h (1000~120000)*/
    gSysconfig.carry_threshold         = 0;    /* kPa (0~40000) */
    gSysconfig.overload_threshold      = 0;    /* kPa (0~40000) */
    gSysconfig.onlineStateInterval     = 30;   /* sec (30~1800) */
    gSysconfig.offlineStateInterval    = 250;  /* sec (250~1800) */
    gSysconfig.moveThreshold           = 15000;  /* 15A (1000~65000) */
    gSysconfig.forkThreshold           = 15000;  /* 15A (1000~65000) */
    gSysconfig.moveType                = 2;
    gSysconfig.forkType                = 0;
    gSysconfig.crashSwitch             = 0;
    gSysconfig.vibrationLimitThreshold = 1000000;
    gSysconfig.crashLowThreshold       = 4000;
    gSysconfig.crashMiddleThreshold    = 6000;
    gSysconfig.crashHighThreshold      = 8000;
    gSysconfig.seatType                = 0;

    memcpy(&gSysconfigBackup, &gSysconfig, sizeof(gSysconfig));
}

void resetServerIp(void)
{
    memset(gSysconfig.serverAddr, 0, sizeof(gSysconfig.serverAddr));
    strncpy(gSysconfig.serverAddr, SYS_CONFIG_DEFAULT_SERVER_ADDRESS,
            sizeof(gSysconfig.serverAddr));
    gSysconfig.serverPort              = SYS_CONFIG_DEFAULT_SERVER_PORT;
    warning_log("Reset Server Ip and Port.\n");
    char buf[64];
    sprintf(buf,"WARN: %d reset server IP and Port",getTimeStamp());
    serverSendErrLog(buf);
}

bool saveConfig(sysconfig_t* ps)
{
    uint8_t buf[MAX_CONFIG_SIZE]={0};
    uint16_t *p_size = buf;

    if(!ps)
    {
        err_log("Para ERROR.\n");
        return false;
    }

    *p_size = sizeof(sysconfig_t);

    memcpy(&buf[2],ps,sizeof(sysconfig_t));

    if(true == writeDataToFile(&gConfigFile, buf, sizeof(uint16_t)+sizeof(sysconfig_t)))
    {
        print_log("SAVE config ok!\n");
        return true;
    }
    else
    {
        err_log("Can't write config to emmc.\n");
        return false;
    }
}

static uint8_t saveConfig2FileFlag=0;   /*  canned format */
void setSaveConfig2File(void)           /*  canned format */
{
    saveConfig2FileFlag = 1;            /*  canned format */
}
int checkSaveConfig2File(void)               /* int funcion(void) canned format ,don't define in .h */
{
    /* if is canned format */
    if(0 == saveConfig2FileFlag || SAVE_2_FILE_MAX < saveConfig2FileFlag)
    {
        return saveConfig2FileFlag;
    }

    if(saveConfig(&gSysconfigBackup))
    {
        
        if(1==saveConfig2FileFlag)
        {
            serverSendLog("VVVVVVVV save config ok! VVVVVVVV");
            print_log("VVVVVVVV save config ok! VVVVVVVV\n");
        }

        saveConfig2FileFlag = 0;         /* if true canned format */
    }
    else
    {
        if(1 == saveConfig2FileFlag)
        {
            serverSendLog("XXXXXXXX save config FAILED!!! XXXXXXXX");
            print_log("XXXXXXXX save config FAILED!!! XXXXXXXX\n");
        }

        saveConfig2FileFlag++;           /* if false  canned format */
    }

    return saveConfig2FileFlag;         /*  canned format */
}

#define CONFIG_POS 2
bool loadConfig(void)
{
    int ret = 0;
    char buf[MAX_CONFIG_SIZE] = {0};
    uint16_t *p_size=buf;
    uint16_t copy_size = 0;

    resetConfig();
    // printConfig(&gSysconfig);

    ret = readDataFromFile(&gConfigFile, buf, sizeof(buf));

    if (ret != *p_size+2)
    {
        warning_log("read config ERR.\n");
        return false;
    }

    copy_size = *p_size > sizeof(gSysconfig) ? sizeof(gSysconfig) : *p_size;
    print_log("curr_size:[%d]\n",sizeof(gSysconfig));
    print_log("read size:[%d]\n",*p_size);
    print_log("copy size:[%d]\n",copy_size);

    memcpy(&gSysconfig,&buf[CONFIG_POS],copy_size);


    memcpy(&gSysconfigBackup, &gSysconfig, sizeof(gSysconfig));

    return true;
}

void printConfig(sysconfig_t *ps)
{
    if(!ps)
    {
        return;
    }

    print_log("IMEI         :[ %s ].\n",ps->devId);
    print_log("IP           :[ %s ].\n",ps->serverAddr);
    print_log("Port         :[ %d ].\n",ps->serverPort);
    print_log("Can type     :[ %d ].\n",ps->canType);
    print_log("Can Pattern  :[ %d ].\n",ps->canPattern);
    print_log("Auth type    :[ %d ].\n",ps->authType);
    print_log("Gps interval :[ %d ].\n",ps->gpsUploadInterval);
    print_log("Can interval :[ %d ].\n",ps->canMaxuploadInterval);
    print_log("Mse threshold:[ %d ].\n",ps->mseThreshold);
    print_log("Seat timeOut :[ %d ].\n",ps->seatoffTimeout);
    print_log("Move type    :[ %d ].\n",ps->moveType);
    print_log("Fork type    :[ %d ].\n",ps->forkType);
    print_log("MoveThreshold:[ %d ].\n",ps->moveThreshold);
    print_log("ForkThreshold:[ %d ].\n",ps->forkThreshold);
    print_log("carry        :[ %d ].\n",ps->carry_threshold);
    print_log("over load    :[ %d ].\n",ps->overload_threshold);
    print_log("OL state Intl:[ %d ].\n",ps->onlineStateInterval);
    print_log("OFL stat Intl:[ %d ].\n",ps->offlineStateInterval);
    print_log("crash switch :[ %d ].\n",ps->crashSwitch);
    print_log("vibration    :[ %d ].\n",ps->vibrationLimitThreshold);
    print_log("low    crash :[ %d ].\n",ps->crashLowThreshold);
    print_log("middle crash :[ %d ].\n",ps->crashMiddleThreshold);
    print_log("high   crash :[ %d ].\n",ps->crashHighThreshold);
    print_log("seatType     :[ %d ].\n",ps->seatType);
    print_log("auto fota    :[ %d ].\n",ps->autoFotaSwitch);
}
void checkPressAvail()
{ 
    if(gSysconfig.carry_threshold > 0)
    {
        gMemconfig.pressEnable = 1;
    }
    else
    {
        gMemconfig.pressEnable = 0;

    }
}

void setSaveSeatTypeConfig2File(void)
{
    saveSeatTypeConfig2FileFlag = 1; 
}
bool saveSeatTypeConfig(seatTypeConfig_t* ps)
{
    if(!ps)
    {
        err_log("Para seat type ERROR.\n");
        return false;
    }
    if(true == writeDataToFile(&gSeatTypeConfigFile, ps,sizeof(seatTypeConfig_t)))
    {
        print_log("SAVE seat type config ok!\n");
        return true;
    }
    else
    {
        err_log("Can't write config to emmc.\n");
        return false;
    } 
}
bool loadSeatTypeConfig(void)
{
    int ret = 0;
    gSeatTypeConfig.seatType = SEAT_ON_INVAL;
    ret = readDataFromFile(&gSeatTypeConfigFile, &gSeatTypeConfig, sizeof(gSeatTypeConfig));
    if (ret != sizeof(gSeatTypeConfig))
    {
        err_log("read seat type config ERR,seatType = [%d].\n",gSeatTypeConfig.seatType);
        return false;
    }
    else
    {
        print_log("load seatType [%d]\n",gSeatTypeConfig.seatType);
    }
    return true;
}
int checkSaveSeatTypeConfig2File(void)
{
    /* if is canned format */
    if(0 == saveSeatTypeConfig2FileFlag || SAVE_2_FILE_MAX < saveSeatTypeConfig2FileFlag)
    {
        return saveSeatTypeConfig2FileFlag;
    }

    if(saveSeatTypeConfig(&gSeatTypeConfig))
    {
        
        if(1==saveSeatTypeConfig2FileFlag)
        {
            serverSendLog("VVVVVVVV save config ok! VVVVVVVV");
            print_log("VVVVVVVV save config ok! VVVVVVVV\n");
        }

        saveSeatTypeConfig2FileFlag = 0;         /* if true canned format */
    }
    else
    {
        if(1 == saveSeatTypeConfig2FileFlag)
        {
            serverSendLog("XXXXXXXX save config FAILED!!! XXXXXXXX");
            print_log("XXXXXXXX save config FAILED!!! XXXXXXXX\n");
        }

        saveSeatTypeConfig2FileFlag++;           /* if false  canned format */
    }

    return saveSeatTypeConfig2FileFlag;         /*  canned format */   
}
void deleteSeatTypeConfigFile(void)
{
    deleteFile(&gSeatTypeConfigFile);
    return;
}