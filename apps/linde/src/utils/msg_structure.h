#ifndef __MSG_STRUCTURE_H__
#define __MSG_STRUCTURE_H__
#include <zephyr.h>
#include "rtc.h"
#include "server_interface.h"

typedef struct
{
    /* can ERROR code FIFO */
    void*    _core; /* used for core */
    uint16_t id;
    uint16_t code;
} canErrFIFO_t;

typedef struct
{
    void* _core; /* used for core */
    float hour;
} canWorkHourFIFO_t;

enum
{                /* Enumeration values for rfidFIFO */
  LOCK   = 0x01, /* lock the car */
  UNLOCK = 0x02, /* unlock the car */
  UNKNOW = 0x03  /* unknow rfid */
};
enum lockReason
{
    RFID_LOCK         = 0,
    SEAT_TIMEOUT_LOCK = 1,
    BREAK_CAN_LOCK    = 2,
    PWR_OFF_LOCK      = 3,
    INIT_LOCK         = 4,
};
enum unlockReason
{
    RFID_UNLOCK         = 0,
    KEEP_DRIVING_UNLOCK = 1,
    HAVE_SPEED_UNLOCK   = 2,
    HAVE_FORK_UNLOCK    = 3,
    HAVE_MSE_UNLOCK     = 4,
    READ_CFG_ERR_UNLOCK = 5,
};
typedef struct
{
    /* data */
    void*    _core; /* used for core */
    uint32_t cardId;
    uint32_t ts;
    uint32_t relativityMs;
    uint8_t  cmd; /* 0x01:unkown card,0x02: vehicle unlock;0x03:vehicle unlock*/
    uint8_t
               status; /* if "cmd" == 0x01,"status" is vehicle  current status;if cmd ==0x02,;if cmd ==0x03,status =: 0 is rfid lock ,1 is seat time out lock,2 key lock */
    localRTC_t rtc;
} rfidFIFO_t;
typedef struct
{
    /* data */
    void*      _core; /* used for core */
    localRTC_t getOnRTC;
    uint8_t    on_reason;
    localRTC_t getOffRTC;
    uint8_t    off_reason;
    uint32_t   getOnTs;
    uint32_t   getOnRelativityMs;
    uint32_t   getOffTs;
    uint32_t   getOffRelativityMs;
    uint32_t   cardId;
} drvRcrdFifo_t;

typedef struct
{
    void*    _core;
    uint32_t cardId;
    bool     rfidValible;

} rfid2CtrlFIFO_t;


typedef struct
{
    void* _core; /* used for core */
    //velocity
    int32_t  averageSpeed;     //average speed since last upload at m/h
    int32_t  moveDistance;     //unit in "mm"
    uint32_t moveAbsDistance;  //absolute distance since last upload. unit in "mm"
    uint32_t
        movePeriod;  //sum of the time that velocity is not zero since last upload. time is "ms"

    //battery status
    uint32_t batVoltage;  //forklift 's battery voltage at "mV"
    uint32_t batCurrent;  //forklift's average current since last upload at "mA"
    uint32_t batState;    //florklift's battery percentage, from 0 to 100

    //period
    uint32_t collectPeriod;   // can statistic period
    uint32_t brakePeriod;     //brake active time since last upload. "ms"
    uint32_t seatPeriod;      //seat active time since last upload. "ms"
    uint32_t forkPeriod;      //fork active time since last upload. "ms"
    uint32_t moveForkPeriod;  //fork & move time overlap in "ms"


    //counter
    uint16_t forkNums;      //how many times the forLINDE_CAN_GENERALk moves since the last move
    uint16_t moveNums;      //how many times this veLINDE_CAN_GENERALhicle moved since last upload
    uint16_t forwardNums;   //how many times the fork move forward
    uint16_t backwardNums;  //how many times the fork move reverse
    uint16_t divertNums;    //how_many times this vehicle change the direction since last upload

    //forward and reverse period and distance
    uint32_t forwardPeriod;     // how long the fork move forward
    uint32_t backwardPeriod;    // how long the fork move reverse
    uint32_t forwardDistance;   //how far the fork move forward
    uint32_t backwardDistance;  //how far the fork move reverse

    uint32_t carryPeriod;  // how long the fork carry goods
    uint16_t carryNums;    // how many times carry goods
} canFIFO_t;


typedef struct
{
    void*   _core;
    int32_t speed;
} overSpeedFIFO_t;

typedef struct
{
    void*  _core;
    double lat;
    double lon;
    float  hdop;
    float  speed;
} gpsFIFO_t;

typedef struct
{
    void*           _core;
    updateRFIDack_t rfid;
} rfidAckFIFO_t;

typedef struct
{
    void*    _core;
    uint32_t carryTime;
    uint16_t carryCounter;
} carryFIFO_t;

typedef struct
{
    void*   _core;
    int32_t val;
    uint8_t reason;
    uint8_t limit_flag;
} speedLimitFIFO_t;

typedef struct
{
    void*    _core;
    uint32_t cap_vol : 1;
    uint32_t vin_vol : 1;
    uint32_t bat_vol : 1;
    uint32_t key_vol : 1;
    uint32_t adc_in0 : 1;
    uint32_t adc_in1 : 1;
    uint32_t adc_in2 : 1;
    uint32_t adc_in3 : 1;
    uint32_t gpi0 : 1;
    uint32_t gpi1 : 1;
    uint32_t gpi2 : 1;
    uint32_t gpi3 : 1;
    uint32_t int_in0 : 1;
    uint32_t int_in1 : 1;
    uint32_t : 18;
} factryV14FIFO_t;

typedef struct
{
    void*   _core;
    uint8_t canTypeResult;
    uint8_t canPatternResult;
} autoCanFIFO_t;

typedef struct
{
    void* _core;
    uint8_t cause;      //  see to enum unlockReason
}unlockCauseFIFO_t;

typedef struct
{
    void* _core;
    autoFotaVersion_t aFV;
}autoFotaFIFO_t;

/****************************************************/
typedef struct
{
    int8_t  mode;      //1:aidong_test,2:KION_test
    uint8_t can_type;  // can type
    uint8_t cmd;  // 0:不需要自适配; 1:需要自适配7和17协议(当且仅当can_type==7)
    uint8_t can_pattern;
} testStart_t;

enum
{
    OFFICAL_RUN = 0,
    TEST_AIDONG = 1,
    TEST_KION   = 2

};
typedef struct
{
    int8_t rs485;        //0:test fail;1:test pass;
    int8_t contrlRelay;  //0:test fail;1:test pass;
} aidongTestResult_t;
#endif
