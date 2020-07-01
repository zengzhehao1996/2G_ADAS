#ifndef __SYSCONFIG_H__
#define __SYSCONFIG_H__
#include <kernel.h>
#include "smart_link_version.h"

#if SERVER_SWITCH
//Official server
#define SYS_CONFIG_DEFAULT_SERVER_ADDRESS "212.129.230.17"
#define SYS_CONFIG_DEFAULT_SERVER_PORT 1883
#define CHECK_VALUE 0x4F84B2
#else
// test server
#define SYS_CONFIG_DEFAULT_SERVER_ADDRESS "111.231.40.143"
#define SYS_CONFIG_DEFAULT_SERVER_PORT 1883
#define CHECK_VALUE 0x4F5DA2
#endif

#define SYS_CONFIG_LINDE_DEVID_SIZE 24
#define SYS_CONFIG_LINDE_SERVER_ADDR_SIZE 32

#define HB_INTERVAL_MIN 60                  /* 1 min  */
#define GPS_INTERVAL_MIN 10                 /* 10 sec */
#define CAN_INTERVAL_MIN 5                  /* 5 sec  */
#define CAN_OFFLINE_INTERVAL_MIN 30         /* 30 sec */
#define CAN_INTERVAL_MAX 2000               /* 33 min */
#define CAR_STATE_INTERVAL_MIN 30           /* 30 sec */
#define CAR_STATE_INTERVAL_MAX 1000         /* 16 min */
#define CAR_STATE_OFFLINE_INTERVAL_MIN 250  /* 250 sec */
#define CAR_STATE_OFFLINE_INTERVAL_MAX 1000 /* 16 min */

typedef struct
{
    uint8_t  devId[SYS_CONFIG_LINDE_DEVID_SIZE];
    uint8_t  serverAddr[SYS_CONFIG_LINDE_SERVER_ADDR_SIZE];  // server address
    uint16_t serverPort;                                     // server port
    uint8_t  authType;                  // authentication service. 0 means none. 1.means 2d code. 2 means RFID
    uint8_t  canType;                   // can type id;
    uint16_t canMaxuploadInterval;      // upload can data every n secs
    uint16_t seatoffTimeout;            // sec ,init 20sec
    int32_t  mseThreshold;
    int32_t  overspeedAlarmThreshold;
    uint16_t gpsUploadInterval;         // upload gps data every n secs
    uint16_t hbTimeout;
    uint16_t hbInterval;                // heart beat every N secs
    uint8_t  moveType;                  // no can config 1:hall sensor,0:switch
    uint8_t  forkType;                  // no can config 1:hall sensor,0:switch
    uint16_t moveThreshold;             // no can config 0.1A
    uint16_t forkThreshold;             // no can config 0.1A
    uint16_t carry_threshold;           // 单位：KPA；0：不启用载货时间功能，＞0：启用载货时间功能
    uint16_t overload_threshold;        // 单位：KPA  0：不启用超载报警功能，＞0：启用超载功能
    uint16_t onlineStateInterval;       // [30sec,1000sec]
    uint16_t offlineStateInterval;      // [250sec,1000sec]
    uint8_t  crashSwitch;               // 碰撞限速开关. 0:关闭,1:轻微碰撞限速,
                                        //             2:中度碰撞限速,3:高度碰撞限速
    uint8_t  canPattern;                // can Pattern
    uint8_t  seatType;                  //server config seat_on volt level; SEAT_ON_INVAL = 0;SEAT_ON_HIGH = 1;//SEAT_ON_LOW =2;
    uint8_t  autoFotaSwitch;            // 0:off,1:on
    int32_t  vibrationLimitThreshold;   // 振动限速阈值,默认1000000
    int32_t  crashLowThreshold;         // 轻微碰撞阈值
    int32_t  crashMiddleThreshold;      // 中度碰撞阈值
    int32_t  crashHighThreshold;        // 高度碰撞阈值
    uint16_t offlineCanInterval;        // in sec
    
} sysconfig_t;

typedef struct 
{
    uint8_t  pressEnable;       // 0:disable;1:enable;
}memconfig_t;

typedef struct 
{
    int8_t seatType;
}seatTypeConfig_t;

typedef struct
{
    uint8_t limit_flag; // 0:no limit, 1:limit
}speedLimitFlag_t;

typedef struct
{
    int32_t vibration_threshold;
    int32_t vibration_limit_threshold;
    int32_t crash_low_threshold;
    int32_t crash_mid_threshold;
    int32_t crash_hig_threshold;
    uint8_t carsh_switch;
} speedLimitConfig_t;

enum
{    
    SEAT_ON_INVAL = 0,
    SEAT_ON_HIGH = 1,
    SEAT_ON_LOW =2,
};
enum
{
    AUTH_NONE = 0,
    AUTH_CODE = 1,
    AUTH_RFID = 2
};

enum
{
    CLEAN = 0,
    DIRTY = 1,
    SPEED_LIMIT_OFF = 0,
    SPEED_LIMIT_ON  = 1,
    TYPE_VIBRATION  = 0,
    TYPE_CRASH_OFF  = 0,
    TYPE_CRASH_LOW  = 1,
    TYPE_CRASH_MID  = 2,
    TYPE_CRASH_HIG  = 3
};

extern bool        gConfigFlag;
extern sysconfig_t gSysconfig;
extern sysconfig_t gSysconfigBackup;
extern memconfig_t gMemconfig;
extern seatTypeConfig_t gSeatTypeConfig;

void setSaveConfig2File(void);
bool saveConfig(sysconfig_t* ps);
bool loadConfig(void);
void resetConfig(void);
void resetServerIp(void);
void printConfig(sysconfig_t* ps);
void checkPressAvail();

void setSaveSeatTypeConfig2File(void);
bool saveSeatTypeConfig(seatTypeConfig_t* ps);
bool loadSeatTypeConfig(void);
void deleteSeatTypeConfigFile(void);
#endif