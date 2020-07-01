#include "seatType_selfAdapt_alg.h"
#include "sensor_decoder.h"
#include "my_misc.h"
#include "config.h"
#include "genenal_sensor.h"

#define SEAT_BUFF_SIZE 10
#define CALL_TIME_OUT (10*1000)
#define CALL_TIME_INVERVAL (1*1000)
static bool adaptFinishStat = false;
static uint32_t lastTime = 0;
static int8_t seatBuff[SEAT_BUFF_SIZE] = {0}; 
static int8_t seatAdaptType = SEAT_ON_INVAL;
static uint8_t seatSampCounter = 0;
static bool adaptStartFlag = false;
static void saveAdaptSeatType();
void adaptSeatType(seatAdapt_t value)
{
    uint32_t currTime = k_uptime_get_32();
    int32_t diffTime = currTime - lastTime;
    /////////////////
    if(value.moveLevel == MOVE_LEVEL_DISABLE)
    {
        if(value.moveStat != MOVE_STOP )
        {
            adaptStartFlag = true;
        }
    }
    else if(value.moveLevel == MOVE_LEVEL_FAST)
    {
        adaptStartFlag = true;
    }
    else
    {
        adaptStartFlag = false;
    }
    if(adaptFinishStat || !adaptStartFlag)
    {
        lastTime = currTime;
        //print_log("11111111111111111111111111.adaptFinishStat [%d],adaptStartFlag[%d]\n", adaptFinishStat,adaptStartFlag);
        return;
    }
    
    if(diffTime >= CALL_TIME_OUT)
    {
        lastTime = currTime;
        memset(seatBuff,0,sizeof(seatBuff));
        seatSampCounter = 0;
        //print_log("11111111111111111111111111\n");
        return;
    }
    else if(diffTime <= CALL_TIME_INVERVAL)
    {
        return;
    }
    lastTime = currTime;
    if(seatSampCounter < SEAT_BUFF_SIZE)
    {
        seatBuff[seatSampCounter++] = value.seatAdapStat;
        //print_log("11111111111111111111111111\n");
       // print_log("value.seatAdapStat LLLLLLLLLLLLLLLLL[%d]\n",value.seatAdapStat);
        return;
        
    }
 
    for(int i = 0;i < seatSampCounter - 1;i++)
    {
        if(seatBuff[i] != seatBuff[i+1])
        {
            memset(seatBuff,0,sizeof(seatBuff));
            seatSampCounter = 0;
            //print_log("seat buff value do not equil\n");
            break;
        }
        if(i == seatSampCounter - 2)
        {
            
            seatAdaptType = seatBuff[0];
            if(seatAdaptType == SEAT_ON_INVAL)
            {
                memset(seatBuff,0,sizeof(seatBuff));
                seatSampCounter = 0;
                //print_log("seatAdaptType = [%d]\n",seatAdaptType);
                break;
            }
            //print_log("seatAdaptType is [%d]...........\n",seatAdaptType);
            adaptFinishStat = true;
            saveAdaptSeatType();
        }
    }
}

static void saveAdaptSeatType()
{
    gSeatTypeConfig.seatType = seatAdaptType;
    setSaveSeatTypeConfig2File();
    //modified curr config
    setSeatOnType(seatAdaptType);
    setSensorSeatTypeConfig(seatAdaptType);
}


