#include "file.h"
#include "kernel.h"
#include "my_misc.h"
#define RFID_LIST_FILE "rfidlist"
#define RFID_LIST_FILE_MD5 "rfidlistmd5"
static classFile_t rfidListFile;
static classFile_t rfidListFileMd5;
#define CONFIG_PATH "config"
classFile_t gConfigFile;
#define POWER_OFF_STATE_FILE_PATH "powerFile"
static classFile_t gPowerOffStateFile;
#define SEAT_TYPE_FILE_PATH "seatTypeFile"
classFile_t gSeatTypeConfigFile;
#define SPEED_LIMIT_FILE_PATH "limitFile"
classFile_t gSpeedLimitConfigFile;


bool fileInit()
{
    //1.moutfs
    mountFS();
    //2.init all file
    //2.1init rfid list file
    rfidListFile.filename = RFID_LIST_FILE;
    rfidListFile.timeOut = 0;
    if(!initFile(&rfidListFile))
    {
        err_log("rfidListFile init false\n");
        return false;
    }
    //2.2 init rfid list md5 file
    rfidListFileMd5.filename = RFID_LIST_FILE_MD5;
    rfidListFileMd5.timeOut  = 0;
    if(!initFile(&rfidListFileMd5))
    {
        err_log("rfidListFileMd5 init false\n");
        return false;
    }

    //2.3 init config file
    gConfigFile.filename = CONFIG_PATH;
    gConfigFile.timeOut  = 0;
    if(!initFile(&gConfigFile))
    {
        err_log("init CONFIG file FALSE.\n");
        return false;
    }

    //2.4 init power off state file
    gPowerOffStateFile.filename = POWER_OFF_STATE_FILE_PATH;
    gPowerOffStateFile.timeOut = 0; 
    if(!initFile(&gPowerOffStateFile))
    {
        err_log("init CONFIG file FALSE.\n");
        return false;
    }
    //2.4 init seat type config file
    gSeatTypeConfigFile.filename = SEAT_TYPE_FILE_PATH;
    gSeatTypeConfigFile.timeOut  = 0;
    if(!initFile(&gSeatTypeConfigFile))
    {
        err_log("init  seat type CONFIG file FALSE.\n");
        return false;
    }

    //2.5 init speed limit config fileInit
    gSpeedLimitConfigFile.filename = SPEED_LIMIT_FILE_PATH;
    gSpeedLimitConfigFile.timeOut  = 0;
    if(!initFile(&gSpeedLimitConfigFile))
    {
        err_log("init  speed limit file FALSE.\n");
        return false;
    }

    return true;
}

bool writeRfidList( void *pdata, int len)
{
    if(!writeDataToFile(&rfidListFile, pdata, len))
    {
        return false;
    }
    print_log("write rfidlist ok.\n");
    return true;
}

int  readRfidList(void *pdata, int len)
{
    return readDataFromFile(&rfidListFile, pdata, len);
}

bool writeRFIDListMd5(uint8_t *md5)
{
    if(!writeDataToFile(&rfidListFileMd5, md5, 16))
    {
        return false;
    }
    return true;
}

bool readRFIDListMd5(uint8_t *md5)
{
    return readDataFromFile(&rfidListFileMd5, md5, 16);
}

void deleteRfidFile()
{
    deleteFile(&rfidListFile);
    print_log("Delete RFID list file.\n");
    deleteFile(&rfidListFileMd5);
    print_log("Delete RFID md5 file.\n");
}

void deleteConfigFile(void)
{
    deleteFile(&gConfigFile);
    print_log("DDDDDDDD Delete Config file. DDDDDDDDD\n");
}

bool writePowerOffState(void *pdata, int len)
{
    if(!writeDataToFile(&gPowerOffStateFile, pdata, len))
    {
        return false;
    }
    print_log("write power off state file ok.\n");
    return true;
}

int  readPowerOffState(void *pdata, int len)
{
    return readDataFromFile(&gPowerOffStateFile, pdata, len);
}

void deletePowerOffState(void)
{
    deleteFile(&gPowerOffStateFile);
    print_log("DDDD Delete power state file . DDDD\n");
}

bool writeSpeedLimitState(void *pdata, int len)
{
    if(!writeDataToFile(&gSpeedLimitConfigFile, pdata, len))
    {
        return false;
    }
    print_log("write speed Limit state file ok.\n");
    return true;
}

int  readSpeedLimitState(void *pdata, int len)
{
    return readDataFromFile(&gSpeedLimitConfigFile, pdata, len);
}

void deleteSpeedLimitState(void)
{
    deleteFile(&gSpeedLimitConfigFile);
    print_log("DDDD Delete speed limit state file . DDDD\n");
}

bool FileUnint()
{
    //1. close all files
    
    //2.umout files
    unmountFs();
    
    return true;
}



