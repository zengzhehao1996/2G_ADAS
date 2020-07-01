#ifndef __SENSOR_DECODER_H__
#define __SENSOR_DECODER_H__
#include <kernel.h>

enum GP_MOV_SENSOR_TYPE
{
    MOV_SENSOR_SWITCH = 0,//micro_swtich/optoelectronic_switch
    // 1 is disable
    MOV_SENSOR_HALL = 2,//hall_sensor that measure current
    MOV_SENSOR_MPU=3, //inside mpu sensor:bmi160
    MOV_SENSOR_SPEED = 4,//impulse_swtich
};

enum GP_FORK_SENSOR_TYPE
{
    FORK_SENSOR_PRESSURE = 1,//impulse_swtich
    FORK_SENSOR_SWTICH = 3,//micro swtich/optoelectronic_swtich 
    FORK_SENSOR_HALL = 4,//hall_sensor that measure current
};

enum GP_SENSOR_SEAT_STAT
{
    SEAT_OFF = 0,
    SEAT_ON  = 1,
};
enum GP_SENSOR_KEY_STAT
{
    KEY_OFF = 0,
    KEY_ON = 1,
};
enum GP_MOVE_SENSOR_STAT
{
    MOVE_STOP = 0,
    MOVE_FORWARD = 1,
    MOVE_BACKWARD =2,
};
enum GP_MOVE_LEVEL_STAT
{
    MOVE_LEVEL_DISABLE = 0,
    MOVE_LEVEL_STOP = 1,
    MOVE_LEVEL_SLOW = 2,
    MOVE_LEVEL_FAST = 3,
};
enum GP_FORK_SENSOR_STAT
{
    FORK_STOP = 0,
    FORK_UP_DOWN = 1,
    FORK_SHUFFLING =2,
    FORK_SEESAW = 3,
    FORK_HUG = 4,
};
enum GP_SENSOR_CARRY_STAT
{
    CARRY_NOTHING = 0,
    CARRY_GOOD = 1,
};
typedef struct 
{
    uint8_t movSensorType;
    uint8_t forkSensorType;
    uint16_t move_threshold;    //nocan config
    uint16_t fork_threshold;    //nocan config
    uint8_t seatType;//0:seat on stat is null;1:seat on stat is high level,2:seat on stat is low level
    bool speed_enable; // if move sensor have no speed ,please disable this flag
    uint16_t carry_threshold; 
}gpSensorTpye_t;

typedef struct
{

}gp_moveSensorConf_t;

typedef struct
{

}gp_forkSensorConf_t;

typedef struct
{
    uint8_t seatType;//1:seat_on value = 1;0:seat_on value = 0;

}gp_genelSensorConf_t;

typedef struct {
  //move sensor
  int32_t speed;
  int8_t moveStat;
  int8_t moveLevel;
  //fork sensor
  int8_t forkStat;
  int8_t carryStat;
  int16_t goodWeight;//in "kg"

  //overlap time:move and fork
  //int8_t overlapStat;
  //gerneral sensor
  
  //battery sensor
  uint32_t battery_volt;
  uint8_t  battery_stat;
  //seat sensor 
  int8_t seatStat;
  int8_t seatAdapStat;//use for self-adapting seat type
  //ACC sensor
  int8_t keyStat;
}sensorData_t;


bool gpSensorSetType(gpSensorTpye_t* sensorType);
void gpSensorParseLoop(s64_t ts);
bool getGpsensorData(sensorData_t *sensorData);
bool getspeedEnable();
void pushBoolArry(bool pb[],int arry_size,bool val);
int8_t getBoolArryStat(bool pb[],int arry_size);
void print_sensorData(sensorData_t *sensorData);
void getSensorConfigData(gpSensorTpye_t* sensorType);
void setSensorSeatTypeConfig(int8_t seatType);
#endif

