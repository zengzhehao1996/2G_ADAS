#ifndef __SERVER_INTERFACE_H__
#define __SERVER_INTERFACE_H__
#include <zephyr.h>

#define IMEI_SIZE 15

#pragma pack(1)
typedef struct
{                    /* used to upload CAN bus error code */
    uint32_t msgId;  /* message ID */
    uint32_t ts;     /* time stamp */
    uint16_t id;     /* ID for can ERROR */
    uint16_t code;   /* ERROR code */
    uint32_t cardId; /* current rfid ID */
} uploadCanERR_t;

typedef struct
{
    uint8_t year;    /*!< Specifies the RTC Date Year.
                         This parameter must be a number between Min_Data = 0 and Max_Data = 99 */
    uint8_t month;   /*!< Specifies the RTC Date Month (in BCD format).
                         This parameter can be a value of @ref RTC_Month_Date_Definitions */
    uint8_t date;    /*!< Specifies the RTC Date.
                         This parameter must be a number between Min_Data = 1 and Max_Data = 31 */
    uint8_t weekDay; /*!< Specifies the RTC Date WeekDay.
                         This parameter can be a value of @ref RTC_WeekDay_Definitions */
    uint8_t hours;   /*!< Specifies the RTC Time Hour.
                         This parameter must be a number between Min_Data = 0 and Max_Data = 12 if the RTC_HourFormat_12 is selected.
                         This parameter must be a number between Min_Data = 0 and Max_Data = 23 if the RTC_HourFormat_24 is selected */
    uint8_t minutes; /*!< Specifies the RTC Time Minutes.
                         This parameter must be a number between Min_Data = 0 and Max_Data = 59 */
    uint8_t seconds; /*!< Specifies the RTC Time Seconds.
                         This parameter must be a number between Min_Data = 0 and Max_Data = 59 */
} RTC_t;

typedef struct
{                                 /* used to update mse config */
    int32_t  threshold;           /* upload threshold */
    uint16_t uploadInterval;      /* interval in sec */
    uint8_t  combineChannel;      /* musk */
    int32_t  speedLimitThreshold; /* turn ON/OFF speed limit */
} mseConfig_t;

typedef struct
{ /* used to upload mse */
    uint32_t msgId;
    uint32_t timestamp;
    int32_t  mse;
    uint32_t cardId;
    uint8_t  forkSpeedLimitStatus;  //1,限速；0,不限速
} uploadMSE_t;

typedef struct
{
    uint32_t timestamp;
    int32_t  crash_val;  //碰撞值
    uint32_t cardId;
    uint8_t  forkSpeedLimitStatus;  //1,限速；0,不限速
    uint8_t  crash_type;            //碰撞类型,1:低度,2:中度,3:高度
} uploadCRASH_t;

typedef struct
{                       /* used to upload unknow rfid card */
    uint8_t  status;    /* 车辆当前状态 */
    uint32_t timeStamp; /* 时间戳 */
    uint32_t cardId;    /* 卡片id */
} uploadUnknowRFID_t;

typedef struct
{                   /* used to upload rfid card */
    uint32_t msgId; /* unique message ID */
    uint8_t  reason;
    RTC_t    rtc;    /* rtc time */
    uint32_t ts;     /* time stamp */
    uint32_t cardId; /* card Id */
    uint8_t  imei[IMEI_SIZE];
} uploadRFID_t;

typedef struct
{
    int     val;      //>=0,被保存卡片张数；=-1，覆盖错误
    uint8_t md5[16];  //本地保存的卡片的校验值
} updateRFIDack_t;

typedef struct
{
    /* used to upload driving record */
    uint32_t msgId;
    uint32_t getOnTs;
    uint8_t  on_reason;
    RTC_t    getOnRTC;
    uint32_t getOffTs;
    uint8_t  off_reason;
    RTC_t    getOffRTC;
    uint32_t cardId;
} uploadDrivingRecord_t;

typedef struct
{                /* used to upload can information */
    uint32_t ts; /* time stamp */

    /* velocity */
    int32_t  avgVelocity;  //average speed since last upload at m/h
    int32_t  distance;     //unit in ""mm""
    uint32_t absDistance;  //absolute distance since last upload. unit in ""mm""
    uint32_t movingTime;  //sum of the time that velocity is not zero since last upload. time is ""ms""

    /* battery status */
    uint32_t batteryVolt;     //forklift 's battery voltage at ""mV""
    uint32_t batteryCurrent;  //forklift's average current since last upload at ""mA""
    uint32_t batteryState;    //florklift's battery percentage, from 0 to 100

    /* time */
    uint32_t brakeTime;         //brake active time since last upload. ""ms""
    uint32_t seatTime;          //seat active time since last upload. ""ms""
    uint32_t forkTime;          //fork active time since last upload. ""ms""
    uint32_t overlapTime;       //fork & move time overlap in ""ms""
    uint32_t carry_time;        // carry things time in "ms"
    uint32_t statistical_time;  // total time in "ms"

    /* counter */
    uint16_t forkCounter;    //how many times the fork moves since the last move
    uint16_t carry_counter;  //the times of carry things
    uint16_t moveCounter;    //how many times this vehicle moved since last upload
    uint16_t forwardCounter;
    uint16_t reverseCounter;
    uint16_t directionChangeCounter;  //how_many times this vehicle change the direction since last upload

    uint32_t forwardTime;
    uint32_t reverseTime;
    uint32_t forwardDistance;
    uint32_t reverseDistance;

    /* current driver */
    uint8_t  cardLen; /* card len */
    uint32_t cardId;  /* card id */

    /* add in V6 */
    //uint32_t flag;
} uploadCAN_t;

typedef struct
{
    /* used to upload workhour */
    float    workHour;
    uint32_t ts;
} uploadWorkhour_t;

typedef struct
{
    uint32_t timestamp;
    uint64_t longitude;
    uint64_t latitude;
    uint32_t hdop;  //horizontal dilution of precision
    uint32_t speed;
} uploadGPS_t;

typedef struct
{
    uint32_t msgId;        /* unique ID and server return it */
    int32_t  overspeedVal; /* m/h */
    uint32_t timeStamp;    /* sec */
    uint32_t cardId;       /* current rfid,0 is no driver */
} uploadOverspeed_t;

enum
{
    MOVE_SWITCH    = 0, /* switch sensor */
    MOVE_TEE       = 1, /* Reserved */
    MOVE_HALL      = 2, /* hall sensor */
    FORK_SWITCH    = 3, /* switch sensor */
    FORK_HALL      = 4, /* hall sensor */
    FORK_HYDRAULIC = 5, /* hydraulic sensor */
    FORK_MOTION    = 6, /* motion sensor */
    MOVE_MOTION    = 7, /* motion sensor */
} sensorENUM_t;
typedef struct
{
    bool electro;       /* true:Electric power forklift;false:Fuel powered forklift */
    int  moveType;      /* 2:hall sensor; 0:switch；1：保留；*/
    int  forkType;      /* 4:hall sensor; 3:switch */
    int  moveThreshold; /* mA,default:15000 */
    int  forkThreshold; /* mA,default:40000 */
} generalConfig_t;

typedef struct
{ /* use for server get current driver */
    uint32_t msgId;
    uint8_t  reserved;
    RTC_t    rtc;      /* rtc */
    uint32_t ts;       /* time stamp */
    uint32_t cardId;   /* card ID */
    uint8_t  imei[15]; /* IMEI */
} uploadCurrentRFID_t;

typedef struct
{
    unsigned char devVersionMajor;
    unsigned char devVersionMinor;
    unsigned char devVersionTiny;
    unsigned char devVersionLeast;
    unsigned char softVersionMajor;
    unsigned char softVersionMinor;
    unsigned char softVersionTiny;
    unsigned char softVersionLeast;
    uint8_t       text[28];
} uploadVersion_t;

typedef struct
{
    uint32_t        fileLenth; /* 文件总长度 */
    char            md5[16];   /* 文件的MD5校验值 */
    uploadVersion_t version;   /* 刷机包的软硬件版本号 */
} fotaStart_t;

#if 0
typedef struct
{
  uint8_t  md5[16];  /* MD5校验值 */
  uint32_t offset;   /* 块偏移起始位置 */
  uint8_t  block[1024]; /* n字节传输块实际数据，一般地n=1024 */
}uploadFOTAops_t
#endif

typedef struct
{
    char     server_IP[16];
    uint16_t server_Port;
    uint8_t  can_type;
    uint8_t  auth_type;            //0:无，2：刷卡
    uint16_t hb_interval;          //sec
    uint16_t gps_upload_interval;  //sec
    uint16_t can_upload_interval;  //sec
    uint16_t seat_timeout;         //sec
    uint16_t over_speed;           // m/h
    uint16_t mse_threshold;        //默认1500
    uint8_t  move_type;            //1:hall sensor; 0:switch；
    uint8_t  fork_type;            //1:hall sensor; 0:switch
    uint16_t move_threshold;       //单位0.1A
    uint16_t fork_threshold;       //单位0.1A
    uint16_t carry_threshold;  //单位：KPA；0：不启用载货时间功能，＞0：启用载货时间功能
    uint16_t overload_threshold;  //单位：KPA  0：不启用超载报警功能，＞0：启用超载功能
    uint16_t online_state_interval;     //sec [30,1000]
    uint16_t offline_state_interval;    //sec [250,1000]
    uint8_t  crash_switch;              //碰撞限速开关;0:关闭,1:轻微碰撞限速,
                                        //           2:中度碰撞限速,3:高度碰撞限速
    uint8_t can_pattern;                //can pattern
    
    int32_t vibration_limit_threshold;  //振动限速阈值,默认10000
    int32_t crash_low_threshold;        //轻微碰撞阈值
    int32_t crash_middle_threshold;     //中度碰撞阈值
    int32_t crash_high_threshold;       //高度碰撞阈值
    uint8_t seatType;
    uint8_t autoFotaSwitch;             // 0:off, 1:on
    uint16_t offlineCanInterval;        // in sec , min 30sec max 2000sec define 600sec 
} downloadConfig_t;

typedef struct
{
    uint8_t  carState;  // 0:不锁车,1:锁车
    uint32_t ts;        // 时间戳
    uint32_t rfid;      // 卡片Id,无卡片时为0
} uploadCarState_t;

typedef struct
{
    /* data */
    uint32_t cap_vol : 1;  //bit 0
    uint32_t vin_vol : 1;  //bit 1
    uint32_t bat_vol : 1;  //bit 2
    uint32_t key_vol : 1;  //bit 3
    uint32_t adc_in0 : 1;  //bit 4
    uint32_t adc_in1 : 1;  //bit 5
    uint32_t adc_in2 : 1;  //bit 6
    uint32_t adc_in3 : 1;  //bit 7
    uint32_t gpi0 : 1;     //bit 8
    uint32_t gpi1 : 1;     //bit 9
    uint32_t gpi2 : 1;     //bit 10
    uint32_t gpi3 : 1;     //bit 11
    uint32_t int_in0 : 1;  //bit 12
    uint32_t int_in1 : 1;  //bit 13
    uint32_t : 18;         //bit 14~31 Reserved
} uploadTestV14_t;

typedef struct
{
    uint32_t msgId;  // return the magId of server
    uint8_t  ret;    // 0,解除失败；1，解除成功;2,设备未限速
} uploadReleaseLimitRet_t;

typedef struct
{
    uint32_t msgId;       // return the magId of server
    uint8_t  limit_flag;  // 0:limit, 1:no limit
} uploadLimitStatus_t;

typedef struct
{
    uint8_t can_type;     // can type
    uint8_t can_pattern;  // can pattern
    uint8_t cmd;  // 0:不需要自适配; 1:需要自适配7和17协议(当且仅当can_type==7)
} factoryStart_t;

typedef struct
{
    uint8_t can_type;
    uint8_t can_pattern;
} adapationCan_t;

typedef struct
{
    uint32_t        ts;        // timestamp
    uint64_t longitude; //原始值*1000000
    uint64_t latitude;  //原始值*1000000
    uint32_t hdop;      //原始值*1000
    uint32_t speed;     //原始值*1000
    uploadTestV14_t adio;      // bitmap see ***
    uint32_t        rfid;      // rfid
    int8_t          cv;        // 控车继电器,0 is failed, 1 is pass
    int8_t          csq;       // >=22 is pass, if not failed
    int8_t          csq_deg;   // normal equal to zero
    int8_t          can;       // 0 is failed, 1 is pass
    int8_t          carry;     // 0 is failed, 1 is pass
    int8_t          mse;       // 0 is failed, 1 is pass
    int8_t          gps;       // 0 is failed, 1 is pass
    int8_t          dv_0;      // device version of major
    int8_t          dv_1;      // device version of minor
    int8_t          dv_2;      // device version of tiny
    int8_t          dv_3;      // device version of patch
    int8_t          sv_0;      // software version of major
    int8_t          sv_1;      // software version of minor
    int8_t          sv_2;      // software version of tiny
    int8_t          sv_3;      // software version of patch
} uploadFactoryTest_t;

typedef struct
{
    uint32_t ret;
    uint32_t timeStamp;
}fotaStartACK_t;

typedef struct
{
    uint32_t length;
    uint32_t timeStamp;
}fotaFileBytesACK_t;

typedef struct
{
    int32_t ret;
    uint32_t timeStamp;
}fotaVerificationACK_t;

typedef struct
{
    uint32_t event;
    uint32_t timeStamp;
}fotaEvent_t;

typedef struct
{
    int32_t ret;
    uint32_t timeStamp;
}fotaStopACK_t;

typedef struct 
{
    uint8_t major;
    uint8_t minor;
    uint8_t tiny;
    uint8_t least;
}softVersion_t;

typedef struct 
{
    uint8_t major;
    uint8_t minor;
    uint8_t tiny;
    uint8_t least;   
}hardVersion_t;

typedef struct
{
    uint32_t      ts;
    hardVersion_t hardVersion;
    softVersion_t softVersion;
}autoFotaVersion_t;

typedef struct
{
    uint32_t      ts;
    hardVersion_t hardVersion;
}autoFotaRequest_t;

#pragma pack()

enum
{
    UPLOAD_ONCE      = 1,
    UPLOAD_CAN       = 2,
    UPLOAD_ONCE_RFID = 3,
    UPLOAD_REPORT    = 4,
};
enum
{
    FOTA_DOWNLOAD_CHECKMD5_OK = 0,   // file download & check md5 ok
    FOTA_OPEN_FILE_ERROR      = 1,   // file can't open
    FOTA_MD5_FILE_CHECK_ERROR = 2,   // file download ok but md5 check failed
    FOTA_FILE_SYSTEM_ERROR    = 3,   // file system crash
    FOTA_RESEND_LINE          = 4,   // Request for retransmission of line
    FOTA_NO_SERVER_CMMAND     = 5,   // can't receive a line and delete file
    FOTA_NOT_IN_MODE          = 6,   //The device is not in fota mode
    FOTA_STOP_FOTA_MODE       = 7,   // Stop fota mode
    FOTA_REBOOT_SYSTEM        = 8,   //The fork is loacked reboot
    FOTA_MD5_LINE_CHECK_ERROR = 10,  // Md5 row check does not pass
    FOTA_MD5_LINE_CHECK_OK    = 11,  // Md5 line check pass
    FOTA_MD5_FILE_CHECK_OK    = 12,  // Md5 file check pass
    FOTA_ALG_256K_OK          = 20,  // Alignment file to 256k ok
    FOTA_ALG_256K_FAILED      = 21   // Alignment file to 256k failed
};


void serverCmdParser(uint16_t cmdid, uint16_t version, const uint8_t* pdata, uint32_t len);
bool serverSendBytes(uint32_t msgId, uint16_t cmdId, uint16_t version, uint8_t* pdata,
                     uint32_t len);
bool serverSendCanBytes(uint32_t msgId, uint16_t cmdId, uint16_t version, uint8_t* pdata, uint32_t len);
bool serverLogin(const uint8_t* pstr);
bool serverLoginInMQTT(const uint8_t* pstr, uint8_t* outBuf);
bool serverRequestConfig(uint8_t* pstr);
bool serverGetTimestamp(void);
bool serverHeart(uint8_t* pstr);
bool serverSendCanERR(uint32_t msgId, uploadCanERR_t* ps);
bool serverSendForkLockState(uploadCarState_t* ps);
bool serverSendSimCard(uint8_t* pstr);
bool serverSendLog(uint8_t* pstr);
bool serverSendErrLog(uint8_t* pstr);
bool serverSendCanInfo(uploadCAN_t* pdata);
bool serverSendGpsInfo(uploadGPS_t* ps);
bool serverSendVersion(uploadVersion_t* ver);
bool serverSendMSE(uint32_t msgId, uploadMSE_t* ps);
bool serverSendCrash(uploadCRASH_t* ps);
bool serverSendLimitStatus(uploadLimitStatus_t* ps);
bool serverSendLimitDisable(uploadReleaseLimitRet_t* ps);
bool serverSendRFIDUnknow(uploadUnknowRFID_t* ps);
bool serverSendRFIDLock(uint32_t msgId, uploadRFID_t* ps);
bool serverSendRFIDUnlock(uint32_t msgId, uploadRFID_t* ps);
bool serverSendCurrentDriver(uploadCurrentRFID_t* ps);
bool serverSendDrivingRecord(uint32_t msgId, uploadDrivingRecord_t* ps);
bool serverRequestRFIDList(uint8_t* md5);
bool serverSendRFIDUpdateAck(updateRFIDack_t* ps);
bool serverSendWorkHour(uploadWorkhour_t* ps);
bool serverSetOverspeedACK(int val);
bool serverSendOverSpeed(uint32_t msgId, uploadOverspeed_t* ps);
bool serverSendRFIDmd5(uint8_t* md5);
bool serverSendCCID(uint8_t* str);
bool serverSendFotaStartACK(uint32_t val);
bool serverSendFotaLineACK(uint32_t val);
bool serverSendFotaSolidify(uint32_t val);
bool serverSendFotaEvent(uint32_t val);
bool serverSendFotaStopACK(uint32_t val);
bool serverSendFactoryStartACK(int val);
bool serverSendFactoryStopACK(int val);
bool serverSendFactoryRecord(uploadFactoryTest_t* ps);
bool serverSendKionFactoryStartACk(int val);
bool serverSendKionFactoryStopACk(int val);
bool serverSendCanType(adapationCan_t* ps);
bool serverSendCurrConfig(downloadConfig_t* ps);
void updateHbLastTime(void);
bool isLoginOk(void);
void setLoginOk(bool val);

int32_t  getServerDebugCmd();
uint32_t getUniqueMsgId(void);
uint32_t messageInterval(void);

bool serverRequestFotaVersion(autoFotaRequest_t *ps);
bool serverRequestFotaAutoStart(autoFotaVersion_t *ps);

void restartKionFactory(uint8_t cantype, uint8_t canPattern);

enum run_mode
{
    NORMAL_MODE       = 0,
    FACTORY_MODE      = 1,
    KION_FACTORY_MODE = 2,
};


#undef DEBUG_OFFINE_THREAD /* debug offline cache */
#ifdef DEBUG_OFFINE_THREAD
bool serverSendstring(uint8_t* pstr);
#endif
#endif