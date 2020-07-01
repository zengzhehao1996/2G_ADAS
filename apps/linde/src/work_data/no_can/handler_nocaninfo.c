#ifdef __cplusplus
extern "C" {
#endif

#include <kernel.h>
#include <string.h>

#include "my_misc.h"

#include "handler_nocaninfo.h"
#include "sensor_decoder.h"
#include "config.h"
#include "seatType_selfAdapt_alg.h"

static uint32_t vehiclesumstart = 0; //collect sum info start time

static nocan_forkcollectdatas vehiclesuminfo;// the summary infomation upload to server

static uint8_t vehiclestopperiod = 0;//vehihcle stop 30s this value add 1

static uint32_t speedmax = 100000; // max speed 100km/h
static int32_t overspeed = 0;// over max speed value

static int32_t speedlastget = 0;   // last get speed value
static int32_t forkspeedget = 0; 
static int32_t speedaccusum = 0;  // speed accumuculate sum
static int32_t speedrecvnum = 0;  // speed no zero nums
static uint32_t speedupdatetime = 0;  // Recv speed message time

static int8_t moveLastStat = 0;
static uint32_t moveupdatatime = 0;

static uint32_t speedstillperiod = 0;  // no moving period

static uint32_t forklastget = 0;  // Recv fork avail start time
static uint32_t forkupdatetime = 0;  //Recv fork message time
static uint32_t forkstillperiod = 0;  // no fork period

static uint32_t carrylastget = 0;  // Recv goods message time
static uint32_t carryupdatetime = 0;  //Recv carry goods avail start time

static uint32_t seatupdatetime = 0;  // Recv seat avail time
static uint32_t seatoffperiod = 0; // seat off time used to timeout lock

static uint32_t brakeupdatetime = 0;  //Recv brake avail time

static uint32_t batvolt = 100; //battery voltage
static uint8_t  voltindex = 0;
static uint32_t volttmp[BAT_FILTER]={0}; 
static uint32_t batvoltsum = 0; // battery voltage accum sum in one period
static uint32_t batvoltnum = 0; // battery voltage recv nums

static uint32_t batstate = 100; //battery State
static uint8_t  stateindex = 0;
static uint8_t  statetmp[BAT_FILTER]={0};
static uint32_t batstatesum = 0;  // battery state accum sum in one period
static uint32_t batstatenum = 0;  //battery state recv nums

static uint32_t canLastAlive=0;

static bool isconnect = FALSE;

static uint32_t reqperiodupdate = 0;

static uint32_t infouploadtime;

static uint8_t vehicleunlock = 1;

uint32_t getLastCanAlive(void);
uint32_t averageBatVolt(uint32_t volt[], uint8_t num);
uint8_t averageBatState(uint8_t state[], uint8_t num);

void forkNoCanInfoInit(void)
{
    speedaccusum = 0;  // speed accumuculate sum
    speedrecvnum = 0;  // speed no zero nums
    
    speedstillperiod = 0;  // no moving period
    forkstillperiod = 0;  // no fork period
    
    batvoltsum = 0;
    batvoltnum = 0;

    batstatesum = 0;
    batstatenum = 0;
    
    vehiclesumstart = k_uptime_get_32();
    vehiclesuminfo.averageSpeed = 0;
    vehiclesuminfo.backwardDistance = 0;
    vehiclesuminfo.backwardNums = 0;
    vehiclesuminfo.backwardPeriod = 0;
    vehiclesuminfo.batCurrent = 0;
    vehiclesuminfo.batState = 0;
    vehiclesuminfo.batVoltage = 0;
    vehiclesuminfo.brakePeriod = 0;
    vehiclesuminfo.divertNums = 0;
    vehiclesuminfo.forkNums = 0;
    vehiclesuminfo.forkPeriod = 0;
    vehiclesuminfo.forwardDistance = 0;
    vehiclesuminfo.forwardNums = 0;
    vehiclesuminfo.forwardPeriod = 0;
    vehiclesuminfo.moveAbsDistance = 0;
    vehiclesuminfo.moveDistance = 0;
    vehiclesuminfo.moveForkPeriod = 0;
    vehiclesuminfo.moveNums = 0;
    vehiclesuminfo.movePeriod = 0;
    vehiclesuminfo.seatPeriod = 0;
    vehiclesuminfo.carryPeriod = 0;
    vehiclesuminfo.carryNums = 0;
    //print_log("Reset can Info.......\n");
}

// if upload can info to server timeout
bool nocanUploadInfoTimeout (uint16_t interval) 
{
    uint32_t currentTime = k_uptime_get_32();
    uint32_t uploadInterval = currentTime - infouploadtime;

    // max upload time interval is too long
    if(uploadInterval >= interval*1000)
    {
        infouploadtime = currentTime;
        return TRUE;      
    }

    // in this period there is valid working data produced
    if(vehiclesuminfo.movePeriod > 0 || vehiclesuminfo.forkPeriod > 0)
    {
       // fork work period end
        if(forkstillperiod >= WORKING_STOP_INTER && speedstillperiod >= WORKING_STOP_INTER)
        {
            infouploadtime = currentTime;
            return TRUE;
        }

    }
    
    return FALSE;
}

//  speed unit m/h
void handlerNocanSpeedInfo(float velocity)
{
    
    uint32_t currentTime = k_uptime_get_32();
    int32_t recvInterval = currentTime - speedupdatetime;
    //print_log("currtime:%d, oldtime:%d\n",currentTime, speedupdatetime);
    speedupdatetime = currentTime;
    int32_t speed = (int32_t) velocity;
    
    
    // too long time update speed info or interval error
    if(recvInterval >= MAX_REAL_INTER || recvInterval < 0)
    {
        forkNoCanInfoInit();
        print_log("currtime:%d, oldtime:%d\n",currentTime, speedupdatetime);
        warning_log("NO Speed reset ....... intervak:%d \n", recvInterval);
        speedlastget = speed;
        return;
    }
    //print_log("currtime:%d, oldtime:%d\n",currentTime, speedupdatetime);

    
    if(forklastget)// fork is active at the same time
    {
        forkspeedget = speed; 
    }

    if(speed == 0)// fork still
    {
        speedstillperiod = speedstillperiod+recvInterval;
    }

    else if (speed > 0) // move forward
    {
        speedstillperiod = 0;
        speedaccusum = speedaccusum + speed;
        speedrecvnum++;
        if(speedlastget <= 0)
        {
            vehiclesuminfo.forwardNums++;
        }
        vehiclesuminfo.forwardDistance += speed*recvInterval/3600;   //distance unit mm
        vehiclesuminfo.forwardPeriod += recvInterval;
        vehiclesuminfo.movePeriod += recvInterval;

        if(speed >= speedmax)
        {
            overspeed = speed;
        }
        

    }
    
    else  // move backward speed < 0
    {
        speedstillperiod = 0;
        speedaccusum = speedaccusum + speed;
        speedrecvnum++;
        if(speedlastget >= 0)
        {
            vehiclesuminfo.backwardNums++;
        }
        vehiclesuminfo.backwardDistance += speed*recvInterval/3600;  //distance unit mm
        vehiclesuminfo.backwardPeriod += recvInterval;
        vehiclesuminfo.movePeriod += recvInterval;

        if(speed <= -speedmax)
        {
            overspeed = speed;
        }
   
    }
    speedlastget = speed;
    return;
}

void handlerNospeedMoveInfo(int8_t moveStat,int8_t moveLevel)
{
    uint32_t currentTime = k_uptime_get_32();
    int32_t recvInterval = currentTime - moveupdatatime;
    //print_log("currtime:%d, oldtime:%d\n",currentTime, speedupdatetime);
    moveupdatatime = currentTime;
 
    // too long time update speed info or interval error
    if(recvInterval >= MAX_REAL_INTER || recvInterval < 0)
    {
    forkNoCanInfoInit();
    print_log("currtime:%d, oldtime:%d\n",currentTime, moveupdatatime);
    warning_log("NO move stat reset ....... intervak:%d \n", recvInterval);
    return;
    }
    //print_log("currtime:%d, oldtime:%d\n",currentTime, speedupdatetime);
    //cal move time
    if(moveStat == MOVE_FORWARD)
    {
        speedstillperiod = 0;
        vehiclesuminfo.forwardPeriod += recvInterval;
        vehiclesuminfo.movePeriod += recvInterval;
        if(moveLastStat != moveStat)
        {
            moveLastStat = moveStat;
            vehiclesuminfo.forwardNums++;
        }
        if(forklastget)// fork is active at the same time
        {
            if(moveLevel == MOVE_LEVEL_DISABLE)
            {
                vehiclesuminfo.moveForkPeriod += recvInterval; 
            }
            else
            {
                if(moveLevel  == MOVE_LEVEL_FAST)
                {
                    vehiclesuminfo.moveForkPeriod += recvInterval; 
                }
                else if(moveLevel == MOVE_LEVEL_SLOW)
                {
                    vehiclesuminfo.forwardPeriod -= recvInterval;
                    vehiclesuminfo.movePeriod -= recvInterval;
                }

            }            
        }
    }
    else if(moveStat == MOVE_BACKWARD)
    {
        speedstillperiod = 0;
        vehiclesuminfo.backwardPeriod += recvInterval;
        vehiclesuminfo.movePeriod += recvInterval;
        if(moveLastStat != moveStat)
        {
            moveLastStat = moveStat;
            vehiclesuminfo.backwardNums++;
        }
        if(forklastget)// fork is active at the same time
        {
            if(moveLevel == MOVE_LEVEL_DISABLE)
            {
                vehiclesuminfo.moveForkPeriod += recvInterval; 
            }
            else
            {
                if(moveLevel  == MOVE_LEVEL_FAST)
                {
                    vehiclesuminfo.moveForkPeriod += recvInterval; 
                }
                else if(moveLevel == MOVE_LEVEL_SLOW)
                {
                    vehiclesuminfo.backwardPeriod -= recvInterval;
                    vehiclesuminfo.movePeriod -= recvInterval;
                }

            }            
        }
    }
    else if(moveStat == MOVE_STOP)
    {
        speedstillperiod = speedstillperiod+recvInterval;
    }
    moveLastStat = moveStat;
    vehiclesuminfo.forwardDistance = 0;
    vehiclesuminfo.backwardDistance = 0;
    vehiclesuminfo.averageSpeed = 0; 
    return;
}

void handlerNocanForkInfo(bool forking)
{
    uint32_t currentTime = k_uptime_get_32();
    int32_t recvInterval = currentTime - forkupdatetime;
    forkupdatetime = currentTime;
      
    if(recvInterval >= MAX_REAL_INTER || recvInterval < 0)
    {
        forkNoCanInfoInit();
        warning_log("No fork reset canINFO.\n");
        forklastget = forking;
        return;
    }  

    if (forking)  // fork is active
    {
        forkstillperiod = 0;
        vehiclesuminfo.forkPeriod += recvInterval;
        
        if(forklastget == FALSE)
        {
            vehiclesuminfo.forkNums++;
        }
    }
    else  // fork is still
    {
        forkstillperiod += recvInterval;
    }
    
    forklastget = forking;
    return;
}
void handlerNocanForkOverlapInfo(bool forking)
{
    uint32_t currentTime = k_uptime_get_32();
    int32_t recvInterval = currentTime - forkupdatetime;
    forkupdatetime = currentTime;
      
    if(recvInterval >= MAX_REAL_INTER || recvInterval < 0)
    {
        forkNoCanInfoInit();
        warning_log("No fork reset canINFO.\n");
        forklastget = forking;
        return;
    }  

    if (forking)  // fork is active
    {
        forkstillperiod = 0;
        vehiclesuminfo.forkPeriod += recvInterval;

        // fork and speed ovlaptime ........
        if(forkspeedget > 0 && forkspeedget >= MOVINGSPEED)
        {
            vehiclesuminfo.moveForkPeriod += recvInterval;
        }
        else if(forkspeedget < 0 && forkspeedget+MOVINGSPEED < 0)
        {
            vehiclesuminfo.moveForkPeriod += recvInterval;
        }
       
       
        if(forklastget == FALSE)
        {
            vehiclesuminfo.forkNums++;
        }
    }
    else  // fork is still
    {
        forkspeedget = 0;
        forkstillperiod += recvInterval;
    }
    
    forklastget = forking;

    return;

}

void handlerNocanCarryInfo(bool carryStat)
{
    //forklastget
    //moveLastStat
    //carryLastGet
    
    uint32_t currentTime = k_uptime_get_32();
    int32_t recvInterval = currentTime - carryupdatetime;
    carryupdatetime = currentTime;
      
    if(recvInterval >= MAX_REAL_INTER || recvInterval < 0)
    {
        forkNoCanInfoInit();
        warning_log("No fork reset canINFO.\n");
        carrylastget = carryStat;
        return;
    }  
    //print_log("carrystat = %d,forklastget = %d,moveLastStat = %d\n",carryStat,forklastget,moveLastStat);
    if (carryStat && (forklastget || moveLastStat))  // fork is active
    {

        vehiclesuminfo.carryPeriod += recvInterval;
        

    }
   // print_log("carryStat = %d,carrylastget = %d......\n",carryStat,carrylastget);
    if(carryStat != carrylastget)
    {
        if(carrylastget == true)
        {
            vehiclesuminfo.carryNums++;
        }

    }
  
    carrylastget = carryStat;
}

void handlerNocanSeatInfo(bool seating)//TODO 
{
    uint32_t currentTime = k_uptime_get_32();
    uint32_t recvInterval = currentTime - seatupdatetime;
    seatupdatetime = currentTime;
      
    if(recvInterval >= MAX_REAL_INTER || recvInterval < 0)
    {
        seatoffperiod = 0;
        forkNoCanInfoInit();
        warning_log("No seat reset canINFO.\n");
        return;
    }

    if(seating)
    {
        seatoffperiod = 0;
        vehiclesuminfo.seatPeriod += recvInterval;
    }
    else
    {
        seatoffperiod += recvInterval;
    } 
    return;
}

// receive unlock vechicle action and restart timing seat off period

void resetNocanSeatOffPeriod(void)
{
    seatoffperiod = 0;
}


void handlerNocanBrakeInfo(bool braking)
{
    uint32_t currentTime = k_uptime_get_32();
    int32_t recvInterval = currentTime - brakeupdatetime;
    brakeupdatetime = currentTime;

    if(recvInterval >= MAX_REAL_INTER || recvInterval < 0)
    {
        forkNoCanInfoInit();
        warning_log("NO break reset canINFO.\n");
        return;
    }

    if(braking)
    {
        vehiclesuminfo.brakePeriod += recvInterval;
    }

    return;  

}

void handlerNocanBatVoltInfo(uint32_t voltage)
{
    uint32_t getvalid = 0;
    volttmp[voltindex]=voltage; 
    voltindex++;
    if(voltindex >= BAT_FILTER)
    {
        voltindex = 0;
        getvalid = averageBatVolt(volttmp,BAT_FILTER);
    }

    if(getvalid > 0)
    {
        batvoltsum += getvalid;
        batvoltnum++;
    }

    return;

}



void handlerNocanBatStateInfo(uint8_t state)
{
    uint8_t statevalid = 0;
    if(state<0 || state>100)
    {
        return;
    }

    statetmp[stateindex] = state;
    stateindex++;
    if(stateindex >= BAT_FILTER)
    {
        stateindex = 0;
        statevalid = averageBatState(statetmp,BAT_FILTER);
    }

    if(statevalid > 0)
    {
        batstatesum += statevalid;
        batstatenum++;
    }
    
    return;

}

uint32_t averageBatVolt(uint32_t volt[], uint8_t num)

{
    uint32_t average = 0;
    uint32_t max=volt[0];
    uint32_t min=volt[0];
    uint32_t sum=0;
    uint32_t sqrt = 0;
    uint8_t  i=0;
    for (i=0 ;i<num ;i++)
    {
        sum+=volt[i];
        
        if(volt[i] > max)
        {
            max = volt[i];
        }

        else if (volt[i] < min)
        {
            min = volt[i];
        }

    }
    
    average = (sum-max-min)/(num-2);
    
    for (i=0;i<num;i++)
    {
        if(volt[i]>average)
        {
            sqrt = sqrt + (volt[i] - average);
        }
        else
        {
            sqrt = sqrt + (average - volt[i]);
        }
    }

    sqrt = sqrt + min -max;
    
    
    if(sqrt/(num -2) > average/5)
    {
        average = 0;
    }
     
    return average;

}




uint8_t averageBatState(uint8_t state[], uint8_t num)

{
    uint8_t average = 0;
    uint8_t max=state[0];
    uint8_t min=state[0];
    uint32_t sum=0;
    uint32_t sqrt = 0;
    uint8_t  i=0;
    for (i=0 ;i<num ;i++)
    {
        sum+=state[i];
        
        if(state[i] > max)
        {
            max = state[i];
        }

        else if (state[i] < min)
        {
            min = state[i];
        }

    }

    
    average = (uint8_t) ((sum-max-min)/(num-2));
    
    for (i=0;i<num;i++)
    {
        if(state[i]>average)
        {
            sqrt = sqrt + (state[i] - average);
        }
        else
        {
            sqrt = sqrt + (average - state[i]);
        }
    }

    sqrt = sqrt + min -max;
    
    if(sqrt/(num -2) > average/5)
    {
        average = 0;
    }
     
    return average;

}


void handlerNocanPushInfo(canFIFO_t *canworkdata)
{
//vehiclesuminfo.carryPeriod
    if(speedrecvnum>0)
    {
        vehiclesuminfo.averageSpeed = speedaccusum/speedrecvnum;
    }
    
    vehiclesuminfo.moveDistance = vehiclesuminfo.forwardDistance + vehiclesuminfo.backwardDistance;
    
    vehiclesuminfo.moveAbsDistance = vehiclesuminfo.forwardDistance - vehiclesuminfo.backwardDistance;

    if(vehiclesuminfo.forwardNums < vehiclesuminfo.backwardNums)
    {
        vehiclesuminfo.divertNums = vehiclesuminfo.forwardNums; 
    }
    else
    {
        vehiclesuminfo.divertNums = vehiclesuminfo.backwardNums; 

    }
    
    vehiclesuminfo.moveNums = vehiclesuminfo.forwardNums + vehiclesuminfo.backwardNums;

    // decide if vehicle is still
    if(vehiclesuminfo.movePeriod == 0 && vehiclesuminfo.forkPeriod ==0)
    {
        vehiclestopperiod++;
    }
    else
    {
        vehiclestopperiod = 0;
    }
    
    // update vehicle bat info
    //if(vehiclestopperiod > 1 && batvoltnum > 0 && batstatenum > 0)//TODO
    if(batvoltnum > 0 && batstatenum > 0)
    {
        batvolt = batvoltsum/batvoltnum;
        batstate = batstatesum/batstatenum;
    }

    vehiclesuminfo.batVoltage = batvolt;
    vehiclesuminfo.batState = batstate;
   
    canworkdata->averageSpeed = vehiclesuminfo.averageSpeed;
    canworkdata->backwardDistance = -vehiclesuminfo.backwardDistance;
    canworkdata->backwardNums = vehiclesuminfo.backwardNums;
    canworkdata->backwardPeriod = vehiclesuminfo.backwardPeriod;
    canworkdata->batCurrent = vehiclesuminfo.batCurrent;
    canworkdata->batState = vehiclesuminfo.batState;
    canworkdata->batVoltage = vehiclesuminfo.batVoltage;
    canworkdata->brakePeriod = vehiclesuminfo.brakePeriod;
    canworkdata->divertNums = vehiclesuminfo.divertNums;
    canworkdata->forkNums = vehiclesuminfo.forkNums;
    canworkdata->forkPeriod = vehiclesuminfo.forkPeriod;
    canworkdata->forwardDistance = vehiclesuminfo.forwardDistance;
    canworkdata->forwardNums = vehiclesuminfo.forwardNums;
    canworkdata->forwardPeriod = vehiclesuminfo.forwardPeriod;
    canworkdata->moveAbsDistance = vehiclesuminfo.moveAbsDistance;
    canworkdata->moveDistance = vehiclesuminfo.moveDistance;
    canworkdata->moveForkPeriod = vehiclesuminfo.moveForkPeriod;
    canworkdata->moveNums = vehiclesuminfo.moveNums;
    canworkdata->movePeriod = vehiclesuminfo.movePeriod;
    canworkdata->seatPeriod= vehiclesuminfo.seatPeriod;
    canworkdata ->carryPeriod = vehiclesuminfo.carryPeriod;
    canworkdata ->carryNums = vehiclesuminfo.carryNums;
    //canworkdata ->
    canworkdata->collectPeriod = k_uptime_get_32()-vehiclesumstart;
    if(canworkdata->collectPeriod < canworkdata->seatPeriod)
    {
        canworkdata->collectPeriod = canworkdata->seatPeriod;
    }    
    return;
}

bool getSafeLock(void)
{
    //print_log("moveLastStat = [%d]......... \n",moveLastStat);
    bool movSafeLock = false;
    if(getspeedEnable())
    {
        if(speedlastget >=0  &&  speedlastget<SPEED_BELOW_SAFE)
        {
            movSafeLock = true;
        }
        else if(speedlastget <0 && speedlastget+SPEED_BELOW_SAFE>0)
        {
            movSafeLock = true;
        }
        else
        {
            movSafeLock = false;
        }
    }
    else
    {
        if(!moveLastStat)
        {
            movSafeLock = true;
        }
        else
        {
            movSafeLock = false;
        }
    }
    if(movSafeLock && forklastget==0)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}



bool nocanSeatOffTimeout(uint16_t leaveinter)
{
    //print_log("seatoffpriod:[%d] ; leaverinter:[%d].\n",seatoffperiod, leaveinter);
    if(seatoffperiod > leaveinter*1000)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void resSeatOffTime(void)
{
    seatoffperiod = 0;
    return;
}


bool nocanDisconnectTimeout(uint16_t discinter)
{
    uint32_t currenttime = k_uptime_get_32();
    uint32_t lastrcanrecv = getLastCanAlive();
    uint32_t recvInterval = currenttime - lastrcanrecv;

    if(recvInterval > discinter*1000)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

bool nocanConnect(void)
{
    uint32_t currenttime = k_uptime_get_32();
    uint32_t lastrcanrecv = 0;
    uint32_t recvInterval = 0;
    ////////////////////////////////////////////
    sensorData_t noCanData;
    getGpsensorData(&noCanData);
    if(noCanData.keyStat != 0)
    {
        canLastAlive = currenttime;
        nocanSetConnect(true);

    }
    else
    {
        //nocanSetConnect(false);
    }
    lastrcanrecv = getLastCanAlive();
    recvInterval = currenttime - lastrcanrecv;
    ////////////////////////////////////////////
    if(isconnect)
    {
        if(lastrcanrecv!=0 && recvInterval >= MAX_CAN_INTER)
        {
            isconnect = FALSE;
        }
    
    }
    else
    {
         if(lastrcanrecv!=0 && recvInterval <= MIN_CAN_INTER)
         {
             isconnect = TRUE;
         }
    
    }
    return isconnect;
}


void nocanSetConnect(bool connect)
{
    isconnect=connect;
    return;    
}


uint32_t nocanDiscanPeriod(void)
{
    uint32_t currenttime = k_uptime_get_32();
    uint32_t lastrcanrecv = getLastCanAlive();
    uint32_t recvInterval = currenttime - lastrcanrecv;
    if(lastrcanrecv!=0 && recvInterval >= 0)
    {
        return recvInterval;
    }    
    return 0;
}




void nocanSetVehicleUnlock(uint8_t status)
{
    vehicleunlock = status;
}


uint8_t nocanGetVehicleUnlock(void)
{
    return vehicleunlock;
}

void nocanSetMaxSpeed(uint32_t speed)
{
    speedmax = speed;
}

int32_t getNocanOverSpeed(void)
{
    return overspeed;
}

void nocanClearOverSpeed(void)
{
    overspeed = 0;
    return;
}

uint32_t getLastCanAlive(void)
{
    return canLastAlive;
}

void noCanParseMessage()
{
    sensorData_t noCanData;
    bool forkStat = false;
    bool seatStat = false;
    bool carryStat = false;
    uint32_t vmax =0;
    uint32_t vmin = 0;
    uint8_t  state = 0;
    getGpsensorData(&noCanData);
    //print_sensorData(&noCanData);
    // genenal sensor
    //can last alive alive   
    if(noCanData.seatStat == SEAT_ON)
    {
        seatStat = true;
    }
    else
    {
        seatStat = false;

    }
    handlerNocanSeatInfo(seatStat);
    handlerNocanBatVoltInfo(noCanData.battery_volt);
    getVotRange(noCanData.battery_volt, &vmin, &vmax);
    state = getStatefromVolt(noCanData.battery_volt, vmin, vmax);
    handlerNocanBatStateInfo(state);
     
    //move sensor
    if(getspeedEnable())
    {
        handlerNocanSpeedInfo(noCanData.speed);        
    }
    else
    {
        //print_log("noCanData.moveStat = [%d]\n",noCanData.moveStat);
        handlerNospeedMoveInfo(noCanData.moveStat,noCanData.moveLevel);
    }
    
    //fork sensor
    //print_log("carryStat = [%d]****************\n",noCanData.carryStat);
    if(noCanData.forkStat == FORK_STOP)
    {
        forkStat = false;
    }
    else
    {
        forkStat = true;
    }
    if(getspeedEnable())
    {
        handlerNocanForkOverlapInfo(forkStat);
    }
    else
    {
        handlerNocanForkInfo(forkStat);
    }

    // carry goods
    if(noCanData.carryStat == CARRY_GOOD)
    {
        carryStat = true;
    }
    else if(noCanData.carryStat == CARRY_NOTHING)
    {
        carryStat = false;
    }
    handlerNocanCarryInfo(carryStat);
    // adapt seat type
    gpSensorTpye_t seatTypeConf;
    getSensorConfigData(&seatTypeConf);
    if(seatTypeConf.seatType != SEAT_ON_HIGH || seatTypeConf.seatType != SEAT_ON_LOW)
    //if(1)
    {
        seatAdapt_t value;
        value.moveLevel = noCanData.moveLevel;
        value.moveStat = noCanData.moveStat;
        value.seatAdapStat = noCanData.seatAdapStat;
        //print_log("moveLevel = [%d] ,moveStat [%d],seatAdapStat = [%d]\n",value.moveLevel,value.moveStat,value.seatAdapStat);
        adaptSeatType(value);
    }
}

bool getForkLast(void)
{
    if(0==forklastget)
    {
        return false;
    }
    else
    {
        return true;
    }
}


#ifdef __cplusplus
}
#endif


