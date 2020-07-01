#include "sensor_decoder.h"
#include "my_misc.h"
#include "my_task.h"
#include "move_sensor_microSwitch.h"
#include "move_sensor_mpu.h"
#include "move_sensor_hoare.h"
#include "fork_updown_sensor_microSwitch.h"
#include "fork_sensor_hoare.h"
#include "fork_sensor_pressure.h"
#include "genenal_sensor.h"
#include "move_sensor_speed.h"


typedef struct
{
    bool (*moveSensorSetup)(gpSensorTpye_t* conf);
    bool (*forkSensorSetup)(gpSensorTpye_t* conf);
    bool (*generalSensorSetup)(gpSensorTpye_t* conf);
    void (*moveSensorRun)(void);
    void (*forkSensorRun)(void);
    void(*gernelSensorRun)(void);
    void(*getMoveData)(sensorData_t* data);
    void (*getForkData)(sensorData_t* data);
    void (*getGenerData)(sensorData_t* data);
    uint32_t (*getMoveSensorRunInterval)(void);
    uint32_t (*getForkSensorRunInterval)(void);
    uint32_t (*getGenelSensorRunInterval)(void);
}gpSensorParser_t;

typedef struct 
{
    uint32_t moveSensor_interval;
    uint32_t forkSensor_interval;
    uint32_t generalSensor_interval;
}SensorTaskInteravl_t;

static gpSensorParser_t gpSensorParser;
static gpSensorTpye_t sensorConfig;
//static SensorTaskInteravl_t sensorTaskInterval;
static my_task_t g_taskMovesensor;
static my_task_t g_taskForkSensor;
static my_task_t g_taskGenerSensor;


static bool gpSensorSetParser(gpSensorParser_t* ptr_parser);

bool gpSensorSetType(gpSensorTpye_t* sensorType)
{
    gpSensorParser_t myParser;
    if(!sensorType)
    {
        err_log("sensorType is null\n");
        return false;
    }
    sensorConfig = *sensorType;
    print_log("sensorConfig:forkSensorType[%d],movSensorType[%d],seatType[%d],fork_threshold[%d],move_threshold[%d],speed_enable[%d]\n",
              sensorConfig.forkSensorType,sensorConfig.movSensorType,
              sensorConfig.seatType,sensorConfig.fork_threshold,sensorConfig.move_threshold,
              sensorConfig.speed_enable);
    //init move sensor
    switch(sensorConfig.movSensorType)
    {
        case MOV_SENSOR_SWITCH:
            myParser.moveSensorSetup = moveMicroSwtichSetup;
            myParser.moveSensorRun = moveMicroSwtichRun;
            myParser.getMoveData = getmoveMicroSwtichData;
            myParser.getMoveSensorRunInterval = getMoveMicroSwtichRunInterval;
            print_log("mov sensor is MOV_SENSOR_SWITCH\n ");
            //sensorTaskInterval.moveSensor_interval = 1;
        break;
        case MOV_SENSOR_HALL:
            myParser.moveSensorSetup = moveHoareSetup;
            myParser.moveSensorRun = moveHoareRun;
            myParser.getMoveData = getmoveHoareData;
            myParser.getMoveSensorRunInterval = getMoveHoareRunInterval;
            print_log("mov sensor is MOV_SENSOR_HALL\n ");
        break;
        case MOV_SENSOR_MPU:
            myParser.moveSensorSetup = moveMpuSetup;
            myParser.moveSensorRun = moveMpuRun;
            myParser.getMoveData = getmoveMpuData;
            myParser.getMoveSensorRunInterval = getMoveMpuRunInterval;
            print_log("mov sensor is MOV_SENSOR_MPU\n ");

        break;
        case MOV_SENSOR_SPEED:
            myParser.moveSensorSetup = moveSpeedSetup;
            myParser.moveSensorRun = moveSpeedRun;
            myParser.getMoveData = getmoveSpeedData;
            myParser.getMoveSensorRunInterval = getMoveSpeedRunInterval;
            print_log("mov sensor is MOV_SENSOR_SPEED\n ");
            //sensorTaskInterval.moveSensor_interval = 1;
        break;
            
        default:
            return false;
    }
    //init fork sensor
    switch(sensorConfig.forkSensorType)
    {
        case FORK_SENSOR_SWTICH:
            myParser.forkSensorSetup = forkUpdownMicroSwtichSetup;
            myParser.forkSensorRun = forkUpdownMicroSwtichRun;
            myParser.getForkData = getForkUpdownMicroSwtichData;
            myParser.getForkSensorRunInterval = getForkUpdownMicroSwtichRunInterval;
            //sensorTaskInterval.forkSensor_interval = 1;
            print_log("fork sensor is FORK_SENSOR_SWTICH\n ");
            break;
        case FORK_SENSOR_HALL:
            myParser.forkSensorSetup = forkHoareSetup;
            myParser.forkSensorRun = forkHoareRun;
            myParser.getForkData = getForkHoareData;
            myParser.getForkSensorRunInterval = getForkHoareRunInterval;
            //sensorTaskInterval.forkSensor_interval = 1;
            print_log("fork sensor is FORK_SENSOR_HALL\n ");
            break;
        case FORK_SENSOR_PRESSURE:
            myParser.forkSensorSetup = forkPressureSetup;
            myParser.forkSensorRun = forkPressureRun;
            myParser.getForkData = getforkPressureData;
            myParser.getForkSensorRunInterval = getforkPressurehRunInterval;
            break;
        default:
            return false;
    }
    //init gerneral sensor
    myParser.generalSensorSetup = genenalSensorSetup;
    myParser.gernelSensorRun = genenalSensorRun;
    myParser.getGenerData = getGenenalSensorData;
    myParser.getGenelSensorRunInterval = getGenenalRunInterval;
    print_log("genel sensor is setup ok\n");
   // sensorTaskInterval.generalSensor_interval = 1;
    return  gpSensorSetParser(&myParser);
}

static bool gpSensorSetParser(gpSensorParser_t* ptr_parser)
{
    if(!ptr_parser)
    {
        err_log("invalid gp sensor parser struct!\n");
        return false;
    }
    memcpy((uint8_t*)&gpSensorParser,(uint8_t*)ptr_parser,sizeof(gpSensorParser_t));
    if(gpSensorParser.moveSensorSetup)
    {
        print_log("before move moveSensorSetup\n");
        gpSensorParser.moveSensorSetup(&sensorConfig);
        print_log("move moveSensorSetup success!\n");
    }
    else
    {
        err_log("moveSensorSetup point is NULL\n");
        return false;
    }
    if(gpSensorParser.forkSensorSetup)
    {
        gpSensorParser.forkSensorSetup(&sensorConfig);
        print_log("forkSensorSetup success!\n");
    }
    else
    {
        err_log("forkSensorSetup is NULL\n");
        return false;

    }
    if(gpSensorParser.generalSensorSetup)
    {
        gpSensorParser.generalSensorSetup(&sensorConfig);
        print_log("generalSensorSetup success!\n");

    }
    else
    {
        err_log("generalSensorSetup is NULL\n");
        return false;
    }

   //init task!!
    if(!task_init(&g_taskMovesensor,gpSensorParser.getMoveSensorRunInterval(),NULL,gpSensorParser.moveSensorRun))
   // if(!task_init(&g_taskMovesensor,1,NULL,NULL))
    {
        err_log("Fail to init moveSensorRun task.\n");
        return false;
    }
    else
    {
        print_log("moveSensorRun task init ok\n");

    }
    if(!task_init(&g_taskForkSensor,gpSensorParser.getForkSensorRunInterval(),NULL,gpSensorParser.forkSensorRun))
    {
        err_log("Fail to init forkSensorRun task.\n");
        return false;
    }
    if(!task_init(&g_taskGenerSensor,gpSensorParser.getGenelSensorRunInterval(),NULL,gpSensorParser.gernelSensorRun))
    {
        err_log("Fail to init forkSensorRun task.\n");
        return false;
    } 
    print_log("task init ok\n");
    return true;
}

void gpSensorParseLoop(s64_t ts)
{
    
    task_trigger(&g_taskMovesensor,ts);//move sensor
    task_trigger(&g_taskForkSensor,ts);
    task_trigger(&g_taskGenerSensor,ts);
}

bool getGpsensorData(sensorData_t *sensorData)
{
    //get move data
    sensorData_t data;
    memset(&data,0,sizeof(sensorData_t));
    gpSensorParser.getMoveData(&data);
    sensorData->moveStat = data.moveStat;
    sensorData->speed = data.speed;
    sensorData->moveLevel = data.moveLevel;
    
    gpSensorParser.getForkData(&data);
    sensorData->forkStat = data.forkStat;
    sensorData->carryStat = data.carryStat;
    sensorData->goodWeight = data.goodWeight;
    
    gpSensorParser.getGenerData(&data);
    sensorData->battery_volt = data.battery_volt;
    sensorData->battery_stat = data.battery_stat;
    sensorData->keyStat = data.keyStat;
    sensorData->seatStat = data.seatStat;
    sensorData->seatAdapStat = data.seatAdapStat;
    //print_sensorData(sensorData);
    return true;
}

bool getspeedEnable()
{
    return sensorConfig.speed_enable;
}

void pushBoolArry(bool pb[],int arry_size,bool val)
{
    for(int i = 0; i<arry_size-1; ++i)
    {
        pb[i] = pb[i+1];
    }
    pb[arry_size-1] = val;

}
int8_t getBoolArryStat(bool pb[],int arry_size)
{
    int trueCnt = 0;
    for(int i = 0; i<arry_size ;++i)
    {
        if(pb[i])
        {
            trueCnt++;
        }
    }
    if(trueCnt == arry_size)
    {
        return 1;
    }
    else if(trueCnt == 0)
    {
        return 0;
    }
    else
    {
        return -1;
    }    
}

void print_sensorData(sensorData_t *sensorData)
{
    if(!sensorData)
    {
        return;
    }
      print_log("sensorData->speed= [%d]\n",sensorData->speed);
      print_log("sensorData->moveStat= [%d]\n",sensorData->moveStat);
      print_log("sensorData->forkStat= [%d]\n",sensorData->forkStat);
      print_log("sensorData->carryStat= [%d]\n",sensorData->carryStat);
      print_log("sensorData->battery_volt= [%d]\n",sensorData->battery_volt);
      print_log("sensorData->battery_stat= [%d]\n",sensorData->battery_stat);
      print_log("sensorData->seatStat= [%d]\n",sensorData->seatStat);
      print_log("sensorData->keyStat= [%d]\n",sensorData->keyStat);
  
}

void getSensorConfigData(gpSensorTpye_t* sensorType)
{
    * sensorType = sensorConfig;
}
void setSensorSeatTypeConfig(int8_t seatType)
{
    sensorConfig.seatType = seatType;
}

