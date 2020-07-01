#ifndef HANDLER_NOCANINFO_H
#define HANDLER_NOCANINFO_H
#ifdef __cplusplus
extern  "C" {
#endif


#include <stdbool.h>
#include <stdint.h>
#include <kernel.h>
#include "msg_structure.h"



#define LINDE_CAN_SENSORBOX  200  //used for factory defaul type
#define LINDE_CAN_GENERAL  254  //used for no can type
#define LINDE_CAN_UFO  255  //used for those unkown can type




#define MAX_REAL_INTER  2000  // 2000ms real info recv max interval time

#define MIN_CAN_INTER   1000   // 1000MS can connect

#define MAX_CAN_INTER   10000  // 10000ms can disconnect

#define MAX_UPLOAD_INTER 30000 //30s fork stop max upload to server timeout

#define WORKING_STOP_INTER 3000  // 3s no fork or speed upload server data

#define SPEED_BELOW_SAFE 50

#define MAX_SEND_NUM  3 // sovle discan loop long time issue

#define MOVINGSPEED   500

#define BAT_FILTER  5  // state can be changed  +-  5%

typedef struct 
{
    //velocity
    int32_t averageSpeed;//average speed since last upload at m/h
    int32_t moveDistance;//unit in "mm"
    int32_t moveAbsDistance;//absolute distance since last upload. unit in "mm"
    uint32_t movePeriod;//sum of the time that velocity is not zero since last upload. time is "ms"
  
    //battery status
    uint32_t batVoltage;//forklift 's battery voltage at "mV"
    uint32_t batCurrent;//forklift's average current since last upload at "mA"
    uint32_t batState;//florklift's battery percentage, from 0 to 100

    //period
    uint32_t brakePeriod;//brake active time since last upload. "ms"
    uint32_t seatPeriod;//seat active time since last upload. "ms"
    uint32_t forkPeriod;//fork active time since last upload. "ms"
    uint32_t moveForkPeriod;//fork & move time overlap in "ms"
    uint32_t carryPeriod;//carry goods time since last upload. in "ms"
    
    //counter
    uint16_t forkNums;//how many times the forLINDE_CAN_GENERALk moves since the last move
    uint16_t moveNums;//how many times this veLINDE_CAN_GENERALhicle moved since last upload
    uint16_t forwardNums;//how many times the fork move forward
    uint16_t backwardNums;//how many times the fork move reverse
    uint16_t divertNums;//how_many times this vehicle change the direction since last upload
    uint16_t carryNums;// how many times carry goods
    
    //forward and reverse period and distance
    uint32_t forwardPeriod;// how long the fork move forward
    uint32_t backwardPeriod;// how long the fork move reverse
    int32_t forwardDistance;//how far the fork move forward
    int32_t backwardDistance;//how far the fork move reverse
}nocan_forkcollectdatas;


void forkNoCanInfoInit(void);

bool nocanUploadInfoTimeout (uint16_t interval) ;

void handlerNocanSpeedInfo(float velocity);
void handlerNospeedMoveInfo(int8_t moveStat,int8_t moveLevel);

void handlerNocanForkInfo(bool forking);
void handlerNocanForkOverlapInfo(bool forking);

void handlerNocanCarryInfo(bool carryStat);

void handlerNocanSeatInfo(bool seating);

void resetNocanSeatOffPeriod(void);

void handlerNocanBrakeInfo(bool braking);

void handlerNocanBatVoltInfo(uint32_t voltage);

void handlerNocanBatStateInfo(uint8_t state);

void handlerNocanPushInfo(canFIFO_t *canworkdata);


bool nocanDisconnectTimeout(uint16_t discinter);

bool nocanConnect(void);

void nocanSetConnect(bool connect);

uint32_t nocanDiscanPeriod(void);

void nocanSetVehicleUnlock(uint8_t status);

uint8_t nocanGetVehicleUnlock(void);


void nocanSetMaxSpeed(uint32_t speed);

int32_t getNocanOverSpeed(void);

void nocanClearOverSpeed(void);

bool nocanSeatOffTimeout(uint16_t leaveinter);
void noCanParseMessage();  

bool getForkLast(void);

#ifdef __cplusplus
}
#endif
#endif //HANDLER_NOCANINFO_H

