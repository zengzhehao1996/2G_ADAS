#include "fork_sensor_pressure.h"
#include "modbus_rtu.h"
#include "hw_rs485.h"
#include "my_misc.h"
#define FORK_PRESSURE_RUN_INTERVAL 200
#define FORK_MOVE_VALUE (2*20) //in kpa,sensor value ?
#define PRESSURE_BUFF_SIZE 5
#define FLUCT_ABS 50
#define PRESS_SENSOR_FACTOR 20
#define PRESS_ABS(a) (((a)>=0)?(a):(-(a)))
#define PRESS_TIME_OUT (2* 60)
static uint8_t  commErrCnt = 0;
static uint32_t firstCommErrTime = 0;
typedef enum
{
    UFO_DATA = 0,
    TRAY_BEER = 1,
    TRAY_EMPTY =2,
    NO_TRAY = 3 
};
typedef struct trayData_s
{
    uint16_t carryCounter;
    bool isForkMov;
    bool CarryBeerFlag;
}trayData_t;

typedef struct pressure_s
{
    bool commStatus;
    uint16_t oriData;
    uint16_t pressData;
}pressure_t;
static char last_status = 0;
static char curr_status = 0;
static bool empty2BeerFlag= false;
static bool Beer2EmptyFlag = false;
static uint16_t buff[PRESSURE_BUFF_SIZE] = {0};
static uint16_t buffNum = 0;
static trayData_t trayData;
static bool pressForkMove = false;
static bool carryFlag = false;
static uint16_t lastPressDate = 0;
static uint16_t lastSensorData = 0;
static bool firstRunFlag = true;
static bool lastCarryBeerFlag = false;
static gpSensorTpye_t pressureConfig;
static sensorData_t  forkPressData;
static pressure_t sensorLog;
static uint16_t v14getPressure(uint16_t data);
static trayData_t v14pressureProcess();
static void v14pressurePrint(trayData_t* pt);

static trayData_t v14pressureProcess()
{
    trayData_t ret;
    s64_t curr_time = k_uptime_get();
    static s64_t last_time = 0;
   memset(&ret,0,sizeof(trayData_t));
   bool isReadyParse = false;
   ModbusParser pressureReq;
   ModbusParser pressureRep;
   uint16_t pressureData;
   uint16_t sensorData = 0;
   memset(&pressureReq,0,sizeof(ModbusParser));
   memset(&pressureRep,0,sizeof(ModbusParser));
   pressureReq.request0304.address = 1;
   pressureReq.request0304.function = 3;
   pressureReq.request0304.index = 0;
   pressureReq.request0304.count = 1;
   //modbusMaster0304ExchangeReq(ModbusParser * m0304Rep,ModbusParser *m0304Req);
   if(!modbusMaster0304ExchangeReq(&pressureRep,&pressureReq))
    {
        commErrCnt++;
        if(commErrCnt == 1){firstCommErrTime = k_uptime_get_32();}
        if(commErrCnt > 200){commErrCnt = 200;}
   }
   else
   {
       commErrCnt = 0;
   }
   if(commErrCnt > 5 && (k_uptime_get_32() - firstCommErrTime > 2000))
   {
       memset(&ret,0,sizeof(trayData_t));
       err_log("read pressure data is failed#########################\n");
       sensorLog.commStatus = false;
       sensorLog.oriData = 0;
       sensorLog.pressData = 0;
       return ret;
   }
   sensorLog.commStatus = true;
   // process beer data
   
   sensorData = pressureRep.response0304.values[0];
   // press sensor data
   pressureData = v14getPressure(sensorData);
   if(sensorLog.commStatus == false)
   {
       sensorLog.oriData = 0;
       sensorLog.pressData = 0;
   }
   else
   {
       sensorLog.oriData = sensorData;
       sensorLog.pressData = pressureData;
   }

   
   int mm =PRESS_ABS(pressureData -lastPressDate);
   if(curr_time - last_time > 5*1000)
    {
        last_time = curr_time;       
        err_log("@@@@@@@@@@@@@@@@@@@@read v14pressureProcess rcv data is [%d],pressure is [%d]kpa\n",sensorData,pressureData);
        print_log("pressForkMove[%d],forkCarryCounter[%d],carryFlag[%d]xxxxx\n",pressForkMove,pressForkMove,carryFlag);
        print_log("mm =%d,pressureData=%d,lastPressDate=%d,xxxxxxxxxxxxxx\n",mm,pressureData,lastPressDate);
   }
   
   //paras fork move
   if(firstRunFlag)
   {
        firstRunFlag = false;
        lastPressDate = pressureData;
   }
   //int mm =PRESS_ABS(pressureData - lastPressDate);
   if(mm >= FORK_MOVE_VALUE)
   //if(PRESS_ABS(pressureData - lastPressDate) >= FORK_MOVE_VALUE )   
   {
        pressForkMove = true;
   }
   else
   {
        pressForkMove = false;
   }
   
   lastPressDate = pressureData;
   
   if(pressureData > pressureConfig.carry_threshold)
    {
        buff[buffNum % PRESSURE_BUFF_SIZE] = TRAY_BEER;
        buffNum++;

   }

   else if(pressureData <=pressureConfig.carry_threshold )
   {
       buff[buffNum % PRESSURE_BUFF_SIZE] = TRAY_EMPTY;
       buffNum++;
   }
   else
   {
       buff[buffNum % PRESSURE_BUFF_SIZE] = UFO_DATA;
       buffNum++;
   }

    if(buffNum == 5)
    {
        buffNum == 0;
    }

    //process data
    for(int i =0 ;i<PRESSURE_BUFF_SIZE-1;i++)
    {
        if(buff[i]!= buff[i+1])
        {
            isReadyParse = false;
            break;
        }
        curr_status = buff[0];
        isReadyParse = true;
    }
    if(!isReadyParse)
    {       
        err_log("v14pressureProcess   parse carry counter is not ready\n");
        ret.isForkMov = pressForkMove;
        ret.CarryBeerFlag = carryFlag;
        return ret;
    }
  
   // if(last_status == TRAY_EMPTY && curr_status == TRAY_BEER)
   if(last_status == TRAY_BEER )
    {
        carryFlag = true;
    }
    else if(curr_status == TRAY_EMPTY)
    {
        carryFlag = false;
    }
    if(curr_status != last_status)
    {
        last_status = curr_status;
    }    
    ret.isForkMov = pressForkMove;
    ret.CarryBeerFlag = carryFlag;
    return ret;
    
}

static uint16_t v14getPressure(uint16_t data)
{
    return (data*20);// 40*1000 /2000
}

static void v14pressurePrint(trayData_t* pt)
{
    print_log("pressure data is carryCounter[%d], CarryBeerFlag[%d],isForkMov[%d]\n",pt->carryCounter,pt->CarryBeerFlag,pt->isForkMov);
}

///////////////////////////////////////////////////////////////////////////////////
bool forkPressureSetup(gpSensorTpye_t* conf)
{
    //init config data
    if(!conf)
    {
        err_log("fork pressure conf is NULL\n");
        return false;
    }
    pressureConfig = *conf;
    print_log("carry_threshold = %dxxxxxxxxxxxxxxxxxx\n",pressureConfig.carry_threshold);
    //init rs485
    if(!hw_rs485_init())
    {
        err_log(" rs485 init failed \n");
        return false;
    }
    return true;
}

void getforkPressureData(sensorData_t* data)
{
    *data =forkPressData;
}
uint32_t getforkPressurehRunInterval(void)
{
    return FORK_PRESSURE_RUN_INTERVAL;
}

void forkPressureRun(void)
{
    //rcv rs485 data
    trayData_t  trayData = v14pressureProcess();
    if(trayData.isForkMov)
    {
        forkPressData.forkStat = FORK_UP_DOWN;
    }
    else
    {
        forkPressData.forkStat = FORK_STOP;
    }
    if(trayData.CarryBeerFlag)
    {
        forkPressData.carryStat = CARRY_GOOD;
    }
    else
    {
        forkPressData.carryStat = CARRY_NOTHING;
    }
    // parse data
}

void getPressSensorStatus(bool *status,uint16_t *origData,uint16_t *pressData)
{
    *status = sensorLog.commStatus;
    *origData = sensorLog.oriData;
    *pressData = sensorLog.pressData;
}

