#ifndef HANDLER_CAN_INFO_H
#define HANDLER_CAN_INFO_H
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



#define  CAN_ERROR_TRACTION  0
#define  CAN_ERROR_DISPLAY 1
#define  CAN_ERROR_STEER  2
#define  CAN_ERROR_LIFT 3



#define MAX_REAL_INTER  2000  // 2000ms real info recv max interval time

#define MIN_CAN_INTER   1000   // 1000MS can connect

#define MAX_CAN_INTER   10000  // 10000ms can disconnect

#define MAX_UPLOAD_INTER 30000 //30s fork stop max upload to server timeout

#define WORKING_STOP_INTER 3000  // 3s no fork or speed upload server data

#define MAX_ERROR_TYPE  10  // in 1 period the max nums of error type

#define REQ_PERIOD_INTER 600000  // get working hours interval 10 minutes

#define SPEED_BELOW_SAFE 50

#define MAX_SEND_NUM  3 // sovle discan loop long time issue

#define MOVINGSPEED   500   // 0.5km/h

#define BAT_FILTER  5  // state can be changed  +-  5%

#define DIAGNOSIS_KEEP 8000  // diagnosis tools quit time 8 seconds no message


typedef struct forkWorkData
{
    int32_t speed;    // speed forwad > 0  backward < 0
    bool fork;        // fork active is true and static is false
    bool seat;        // driver on is true and driver off is false
    bool brake;       // brake active is true and invalid is false
} forkworkdatas;

typedef struct forkBatData
{
    uint32_t volt;    // battery voltage
    uint32_t state;   // battery state
} forkbatdatas;


float vehicleWorkingHours;


typedef struct forkCollectData
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

    //counter
    uint16_t forkNums;//how many times the forLINDE_CAN_GENERALk moves since the last move
    uint16_t moveNums;//how many times this veLINDE_CAN_GENERALhicle moved since last upload
    uint16_t forwardNums;//how many times the fork move forward
    uint16_t backwardNums;//how many times the fork move reverse
    uint16_t divertNums;//how_many times this vehicle change the direction since last upload

    //forward and reverse period and distance
    uint32_t forwardPeriod;// how long the fork move forward
    uint32_t backwardPeriod;// how long the fork move reverse
    int32_t forwardDistance;//how far the fork move forward
    int32_t backwardDistance;//how far the fork move reverse
}forkcollectdatas;


typedef struct errorCodeMessage
{
    uint16_t errorType;
    uint16_t errorCode;
} errorcodemessages;


typedef struct kalman_param_s{
  // Kalman filter variables 
  float q; //process noise covariance
  float r; //measurement noise covariance
  float x; //value
  float p; //estimation error covariance
  float k; //kalman gain
}kalmanparamt;


void forkCanInfoInit(void);

bool uploadInfoTimeout (uint16_t interval) ;

void handlerFreshSpeed(float velocity);// one period only one fresh value

void handlerSpeedInfo(float velocity);


void handlerFreshFork(bool forking);// one period only one fresh value

void handlerForkInfo(bool forking);


void handlerSeatInfo(bool seating);

void handlerFreshSeat(bool seating);


void resetSeatOffPeriod(void);


void handlerFreshBrake(bool braking);

void handlerBrakeInfo(bool braking);

void handlerFreshBatVolt(uint32_t voltage);
void handlerBatVoltInfo(uint32_t voltage);
uint32_t averageBatVolt(uint32_t volt[], uint8_t num);


void handlerBatFreshState(uint8_t state);
void handlerBatStateInfo(uint8_t state);
uint8_t averageBatState(uint8_t state[], uint8_t num);

void handlerLoopFresh(void);

void handlerPushInfo(canFIFO_t *canworkdata);

uint8_t handlerErrorCode(uint16_t type, uint16_t code); 

uint8_t getCurrentErrorNum(void);

uint16_t getErrorCode(uint8_t index);

uint16_t getErrorId(uint8_t index);

uint8_t handlerWorkingHours(float hours);

bool getWorkingHoursFlag(void);

float getWorkinghours(void);

void resWorkingHoursFlag(void);

bool reqPeriodTimeout(void);

bool disconnectTimeout(uint16_t discinter);

bool connectCan(void);

void setConnectCan(bool connect);

uint32_t discanPeriod(void);

void setVehicleUnlock(uint8_t status);

uint8_t getVehicleUnlock(void);

int8_t getNewErrorIndex(void);

void setMaxSpeed(uint32_t speed);

// speed in m/h
void initOverSpeed(int32_t speed);

int32_t getOverSpeed(void);

void clearOverSpeed(void);

uint8_t getStatefromVolt(uint32_t volt,uint32_t vmin,uint32_t vmax);

bool seatOffTimeout(uint16_t leaveinter);

float getKalmanFilteredValue(kalmanparamt* param,float measurement) ;

void kalmanInit(void);

void setDiagnosis(bool value);

bool getDiagnosis(void);

bool getForkLast(void);

int32_t getLastSpeed(void);

bool getSafeLock(void);

#ifdef __cplusplus
}
#endif
#endif //HANDLER_CAN_INFO_H

