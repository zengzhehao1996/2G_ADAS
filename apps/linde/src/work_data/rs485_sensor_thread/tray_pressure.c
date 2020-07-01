#include "tray_pressure.h"
#include "modbus_rtu.h"
#include "config.h"
#include "fifo.h"
#include "sempho.h"
#include "msg_structure.h"

#define FORK_MOVE_VALUE (2*20) //in kpa,sensor value ?
#define PRESSURE_BUFF_SIZE 5
#define FLUCT_ABS 50
#define PRESS_SENSOR_FACTOR 20
#define PRESS_ABS(a) (((a)>=0)?(a):(-(a)))
#define PRESS_TIME_OUT (2* 60)

static char last_status = 0;
static char curr_status = 0;
static uint16_t carryThreshold = 0;
static uint16_t overLoadThreshold = 0;
static bool empty2BeerFlag= false;
static bool Beer2EmptyFlag = false;
static uint16_t buff[PRESSURE_BUFF_SIZE] = {0};
static uint16_t buffNum = 0;
extern sysconfig_t g_sys_config;
static trayData_t trayData;
static s64_t lastHoldBeerTime = 0;
static s64_t TotolBeerTime = 0;
static bool pressForkMove = false;
static bool carryFlag = false;
static uint16_t lastPressDate = 0;
static uint16_t lastSensorData = 0;
static bool firstRunFlag = true;
static uint16_t forkCarryCounter = 0;
static carryFIFO_t sendCarryData = {0};
static carryFIFO_t carryDataTmp = {0};
static bool isWorkStart = false;
static bool circlStopFlag = false;
static uint32_t pressureLastTime = 0;
static uint32_t pressTimeout = PRESS_TIME_OUT;
static uint32_t carryTimeDiff = 0;
static bool lastCarryBeerFlag = false;
typedef enum
{
    UFO_DATA = 0,
    TRAY_BEER = 1,
    TRAY_EMPTY =2,
    NO_TRAY = 3
    
};
static uint16_t getPressure(uint16_t data);

bool pressureInit(uint16_t carry,uint16_t overLoad)
{
    carryThreshold = carry;
    overLoadThreshold = overLoad;
    sendCarryData.carryCounter = 0;
    sendCarryData.carryTime = 0;
    print_log("pressure Init success,carryThreshold = %d,overLoadThreshold= %d\n",carryThreshold,overLoadThreshold );
    return true;
}
trayData_t pressureProcess()
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
   if(!modbusMaster0304Req(&pressureRep,&pressureReq))
    {
        memset(&ret,0,sizeof(trayData_t));
        err_log("read pressure data is failed#########################\n");
        return ret;
   }
   // process beer data
   
   sensorData = pressureRep.response0304.values[0];
   pressureData = getPressure(sensorData);
   int mm =PRESS_ABS(pressureData -lastPressDate);
   if(curr_time - last_time > 5*1000)
    {
        last_time = curr_time;       
        err_log("@@@@@@@@@@@@@@@@@@@@read pressureProcess rcv data is [%d],pressure is [%d]kpa\n",sensorData,pressureData);
        print_log("pressForkMove[%d],forkCarryCounter[%d],carryFlag[%d]xxxxx\n",pressForkMove,forkCarryCounter,carryFlag);
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
   
   if(pressureData > carryThreshold)
    {
        buff[buffNum % PRESSURE_BUFF_SIZE] = TRAY_BEER;
        buffNum++;

   }

   else if(pressureData <=carryThreshold )
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
        err_log("pressureProcess   parse carry counter is not ready\n");
        ret.isForkMov = pressForkMove;
        ret.carryCounter = forkCarryCounter;
        ret.CarryBeerFlag = carryFlag;
        return ret;
    }
  
   // if(last_status == TRAY_EMPTY && curr_status == TRAY_BEER)
   if(last_status == TRAY_BEER )
    {
        empty2BeerFlag = true;
        carryFlag = true;
    }
    else if(curr_status == TRAY_EMPTY)
    {
        Beer2EmptyFlag = true;
        carryFlag = false;
    }
    if(curr_status != last_status)
    { 
        if(curr_status == TRAY_EMPTY && last_status == TRAY_BEER)
        {
            forkCarryCounter++;
        } 
        last_status = curr_status;
    }

    ret.isForkMov = pressForkMove;
    ret.carryCounter = forkCarryCounter;
    ret.CarryBeerFlag = carryFlag;
    return ret;
    
}
void resetPressure()
{
    lastHoldBeerTime = 0;
    TotolBeerTime = 0;
    empty2BeerFlag = false;
    Beer2EmptyFlag = false;
    carryFlag = false;
    last_status = UFO_DATA;
    curr_status = UFO_DATA;
    forkCarryCounter = 0;
    memset(buff,0,PRESSURE_BUFF_SIZE);
    
}

static uint16_t getPressure(uint16_t data)
{
    return (data*20);// 40*1000 /2000
}

void pressurePrint(trayData_t* pt)
{
    print_log("pressure data is carryCounter[%d], CarryBeerFlag[%d],isForkMov[%d]\n",pt->carryCounter,pt->CarryBeerFlag,pt->isForkMov);
}
void getPressureSensorData(uint16_t* data,uint16_t* pressureData)
{
    *data = lastPressDate/PRESS_SENSOR_FACTOR;
    *pressureData = lastPressDate;
}
void processCarryAffair()
{
    uint32_t currTime = k_uptime_get_32();
    uint32_t diff = currTime - carryTimeDiff;
    //print_log("DDDDDDDDDDDDDdif [%u],curr[%u],last_time [%u]\n",diff,currTime,pressureLastTime);
    //1.wait start semphor
    if(semTakePressureStart())
    {
        isWorkStart = true;
        circlStopFlag = false;
        diff = 0;
        print_log("semTakePressureStart comingUUUUUUUUUUUUUUUUUUUUUUUUUU\n");
    }
    //2.wait stop semphor
    if(semTakePressureStop())
    {
        isWorkStart = false;
        circlStopFlag = true;
        print_log("semTakePressureStop comingUUUUUUUUUUUUUUUUUUUUUUUUUU\n");
    }
    //4.need work?
    if(!isWorkStart)
    {
        resetPressure();
        pressureLastTime = currTime;
        carryTimeDiff = currTime;
        
        if(circlStopFlag)
        {
            circlStopFlag = false;
            print_log(" circlStopFlag is coming ,pressure sensor send data t0 can thread :carryTime = %d,carryCounter = %d,WWWWWWWWWWWWWWWWWWW\n",
                       sendCarryData.carryTime,sendCarryData.carryCounter);
            carryDataFifoSend(&sendCarryData);
            memset(&sendCarryData,0,sizeof(sendCarryData));
        }
        return;
    }
    //3.send fifo
    if(circlStopFlag || (currTime - pressureLastTime > (pressTimeout*1000)))
    {
        pressureLastTime = currTime;
        circlStopFlag = false;
        print_log("pressure sensor send data to can thread :carryTime = %d,carryCounter = %d,WWWWWWWWWWWWWWWWWWW\n",
                   sendCarryData.carryTime,sendCarryData.carryCounter);
        carryDataFifoSend(&sendCarryData);
        memset(&sendCarryData,0,sizeof(sendCarryData));
        //memset(&carryDataTmp,0,sizeof(carryDataTmp));
    }

    //5. process pressure sensor data!
    trayData_t  trayData = pressureProcess();
    if(trayData.CarryBeerFlag != lastCarryBeerFlag)
    {
        if(lastCarryBeerFlag)
        {
            circlStopFlag = true;
        }
        lastCarryBeerFlag = trayData.CarryBeerFlag;
    }

    if(trayData.carryCounter)
    {
        sendCarryData.carryCounter = trayData.carryCounter;
        resetPressure();

    }

    
    if(trayData.CarryBeerFlag)
    {
       // print_log("DDDDDDDDDDDDDDDDDDDDDIF diff[%u] ,carryTime[%u]\n",diff,sendCarryData.carryTime);
        
        sendCarryData.carryTime += diff;
        carryTimeDiff = currTime;       
    }

    
}

void setPressureTimeOut(uint16_t t_s)
{
    pressTimeout = (t_s);

}


