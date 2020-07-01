#include "factory_test_unique.h"
#include "v141_adc_test.h"
#include "v141_gpio_test.h"
#include "kernel.h"
#include "my_misc.h"
#include "msg_structure.h"
#include "fifo.h"
#include "modbus_rtu.h"
#include "hw_rs485.h"
//#include "my_misc.h"

#define V14_FAC_UNIQ_THREAD_STACK_SIZE 1024
K_THREAD_STACK_DEFINE(g_v14FacTestThreadStack, V14_FAC_UNIQ_THREAD_STACK_SIZE);
static struct k_thread g_v14FacTestThread;
static k_tid_t         g_v14FacTestThreadId;

#define ADC_RUN_TIME (10*1000)
#define RS485_RUN_TIME (10*1000)
#define ADC_PORT_NUM 14
#define ADC_VALUE_NUM 10
#define ADC_TIME (50)
#define VOL_HIGH (3*1000)
#define VOL_MIN_INTERVAL (1000)
static bool adcHigh[ADC_PORT_NUM] = {false};
static bool adcLow[ADC_PORT_NUM] = {false};
static bool adcResult[ADC_PORT_NUM] = {false};
static char adcName[][10] = {"BAT_VOL","KEY_VOL","ADC_IN0","ADC_IN1","ADC_IN2","ADC_IN3","VIN_VOL","CAP_VOL",
                                "GPI0","GPI1","GPI2","GPI3","GPINT_IN0","GPINT_IN1"};
static int32_t adcValue[ADC_PORT_NUM][ADC_VALUE_NUM]={0};
static char compareHighCnt = 0;
static char compareLowCnt = 0;
static bool rs485TestPass = false;


static void v14FacUniqThreadRun(void* p);
static void getAdcGpioResult();
static void  v14AdcGpioTest();
static bool v14AdcGpioInit();
static bool v14AdcGpioUinit();

static bool v14rs485Init();
static void v14rs485Test();
static void v14rs485Uint();
static void sendServerV14AdcResult();
static void sendServerV14rs485Result();
void v141_factory_test_unique_run(testStart_t para)
{
    bool ret;
    g_v14FacTestThreadId =
    k_thread_create(&g_v14FacTestThread, g_v14FacTestThreadStack, V14_FAC_UNIQ_THREAD_STACK_SIZE,
                      (k_thread_entry_t)v14FacUniqThreadRun, &para, NULL, NULL, K_PRIO_COOP(8), 0, 0);
    if(g_v14FacTestThreadId != 0)
    {
      ret = true;
      print_log("Create g_v14FacTestThread THREAD Id:[ %p ]; Stack:[ %p ]; Size:[ %p ]\n", g_v14FacTestThreadId,
                g_v14FacTestThreadStack, V14_FAC_UNIQ_THREAD_STACK_SIZE);
    }
    else
    {
      ret = false;
      err_log("Create g_v14FacTestThread Failed.\n\n");
    }
    return ret;


}
void v141_factory_test_unique_stop()
{
    v14AdcGpioUinit();
    v14rs485Uint();
    //stop v141 unique thread
    if(0 != g_v14FacTestThreadId)
    {
        k_thread_abort(g_v14FacTestThreadId);
        g_v14FacTestThreadId = 0;
        print_log("v14 stop v14 factory test Thread  success !\n");
    }
}

static void v14FacUniqThreadRun(void* p)
{

    v14AdcGpioInit();
    //v14rs485Init();
    while(1)
    {
       //test adc
       v14AdcGpioTest();
       //test 485
       //v14rs485Test(); 
       k_sleep(30);
    }

}


static void getAdcGpioResult()
{
    for(int i = 0;i < ADC_PORT_NUM;i++)
    {   
        printk("%s:[%d];",adcName[i],adcValue[i][0]);
        if(!((i+1) % 4))
        {
            printk("\n");
        }
    }
    printk("\n\n");

        for(int i = 0;i < ADC_PORT_NUM;i++)
    {   
        printk("%s:adcHigh = [%d],adcLow = [%d];",adcName[i],adcHigh[i],adcLow[i]);
        if(!((i+1) % 4))
        {
            printk("\n");
        }
    }
    printk("\n\n");
    
}

static void  v14AdcGpioTest()
{
    static uint32_t lastAdcTime = 0;
    uint32_t currTime =k_uptime_get_32();
    static int adcCnt = 0;
    static uint32_t adcTestFirstRunTime = 0;
    if(adcTestFirstRunTime == 0){adcTestFirstRunTime = currTime;}
    if(currTime - adcTestFirstRunTime >= ADC_RUN_TIME)
    {
        for(int i = 0;i < ADC_PORT_NUM;i++)
        {
            if(adcHigh[i] && adcLow[i])
            {
                adcResult[i] = true;
            }
            else
            {
                adcResult[i] = false;
            }
        }
        //send  adc test result to server
        
        sendServerV14AdcResult();
        //getAdcGpioResult();
        
        adcTestFirstRunTime = currTime;
    }
    
    if(currTime-lastAdcTime < ADC_TIME)
    {
        return;
    }
    lastAdcTime = currTime;
    //"0_BAT_VOL","1_KEY_VOL","2_ADC_IN0","3_ADC_IN1","4_ADC_IN2","5_ADC_IN3",
    //"6_VIN_VOL","7_CAP_VOL","8_GPI0","9_GPI1","10_GPI2","11_GPI3","12_GPINT_IN0","13_GPINT_IN1"
    //ADC
    if(adcCnt <ADC_VALUE_NUM)
    {
        adcValue[0][adcCnt] = volt_get_bat_ext();
        adcValue[1][adcCnt] = volt_get_key_ext();            
        adcValue[2][adcCnt] = volt_get_hydraulic_ext();
        adcValue[3][adcCnt] = volt_get_forward_ext();            
        adcValue[4][adcCnt] = volt_get_backward_ext();            
        adcValue[5][adcCnt] = volt_get_adc_seat();         
        adcValue[6][adcCnt] = volt_get_power();
        adcValue[7][adcCnt] = volt_get_bat();
        //GPI
        adcValue[8][adcCnt] = gpio_get_seat_status();
        adcValue[9][adcCnt] = gpio_get_hydraulic_status();
        adcValue[10][adcCnt] = gpio_get_forward_status();
        adcValue[11][adcCnt] = gpio_get_backward_status();
        adcValue[12][adcCnt] = gpio_get_gpint_in0_status();
        adcValue[13][adcCnt] = gpio_get_gpint_in1_status();
        //getAdcGpioResult();
        adcCnt++;
        return;

    }
    else
    {
        adcCnt = 0;
    }

    
    //getAdcGpioResult();
    //print_log("ready to process adc data###############################3\n");
    for(int i = 0;i < ADC_PORT_NUM;i++)
    {
        //print_log("&&&&&&&&&&&&&i = [%d]\n",i);
        int j = 0;
        if(i < 6)
        {
            for(j = 0;j < ADC_VALUE_NUM - 1;j++)
            {
            
                //print_log("i = [%d],j=[%d],adcValue[%d][%d] = [%d]\n",i,j,i,j,adcValue[i][j]);
                if(my_abs(adcValue[i][j] -adcValue[i][j+1]) > VOL_MIN_INTERVAL){break;};
                
            }
            if(j == ADC_VALUE_NUM -1)
            {
                if( (adcValue[i][0] > VOL_HIGH)  && (adcHigh[i] == false) )
                {
                    adcHigh[i] = true;
                }
                else if(adcHigh[i] == true && adcLow[i] == false && ((adcValue[i][0] < VOL_HIGH)) )
                {
                    adcLow[i] = true;
                }
                else
                {
                    continue;
                }
            }
            else
            {
                continue;
            }
       }
       else if(i == 7 || i == 6)
       {
                if( (adcValue[i][0] > VOL_HIGH)  && (adcHigh[i] == false) )
                {
                    adcHigh[i] = true;
                    adcLow[i]  = true;
                }
       }
       else if(i >=8)
       {
           //process gpi signal
            for(j = 0;j < ADC_VALUE_NUM - 1;j++)
            {
            
                //print_log("i = [%d],j=[%d],gpiValue[%d][%d] = [%d]\n",i,j,i,j,adcValue[i][j]);
                if(adcValue[i][j] != adcValue[i][j+1] ){break;}
                
            }
            if(j == ADC_VALUE_NUM -1)
            {
                if( (adcValue[i][0]  == 1)  && (adcHigh[i] == false) )
                {
                    adcHigh[i] = true;
                }
                else if(adcHigh[i] == true && adcLow[i] == false && ((adcValue[i][0] == 0)) )
                {
                    adcLow[i] = true;
                }
                else
                {
                    continue;
                }
            }
            else
            {
                continue;
            }

       }
           
        

        
    }

    
    // upload result to server
    
    
}

static bool v14AdcGpioInit()
{
    //adc init
    volt_setup();
    //gpio init
    linde_gpio_v14_setup();

}

static void sendServerV14AdcResult()
{
    factryV14FIFO_t adcMsg;
    /*
    uint32_t cap_vol:1;
    uint32_t vin_vol:1;
    uint32_t bat_vol:1;
    uint32_t key_vol:1;
    uint32_t adc_in0:1;
    uint32_t adc_in1:1;
    uint32_t adc_in2:1;
    uint32_t adc_in3:1;
    uint32_t gpi0:1;
    uint32_t gpi1:1;
    uint32_t gpi2:1;
    uint32_t gpi3:1;
    uint32_t int_in0:1;
    uint32_t int_in1:1;
    */
        //"0_BAT_VOL","1_KEY_VOL","2_ADC_IN0","3_ADC_IN1","4_ADC_IN2","5_ADC_IN3",
    //"6_VIN_VOL","7_CAP_VOL","8_GPI0","9_GPI1","10_GPI2","11_GPI3","12_GPINT_IN0","13_GPINT_IN1"
    //ADC
    adcMsg.cap_vol  = adcResult[7];
    adcMsg.vin_vol  = adcResult[6];
    adcMsg.bat_vol  = adcResult[0];
    adcMsg.key_vol  = adcResult[1];
    adcMsg.adc_in0  = adcResult[2];
    adcMsg.adc_in1  = adcResult[3];
    adcMsg.adc_in2  = adcResult[4];
    adcMsg.adc_in3  = adcResult[5];
    adcMsg.gpi0     = adcResult[8];
    adcMsg.gpi1     = adcResult[9];
    adcMsg.gpi2     = adcResult[10];
    adcMsg.gpi3     = adcResult[11];
    adcMsg.int_in0  = adcResult[12];
    adcMsg.int_in1  = adcResult[13];
    #if 0
    for(int i = 0;i < 14;i++)
    {
        print_log("adcResult[%d] = [%d].......\n",i,adcResult[i]);

    }
    #endif
    //factryTestV14FifoSend
    if(factryTestV14FifoSend((char*)&adcMsg))
    {
        print_log("@@@@@@@@@@@@@@send v14 factory fifo ok..............\n");
    }
    else
    {
        print_log("@@@@@@@@@@@@@@@send  v14 factory fifo failed..............\n");

    }
    
}

static bool v14AdcGpioUinit()
{
    memset(adcHigh,0,sizeof(adcHigh));
    memset(adcLow,0,sizeof(adcLow));
    memset(adcResult,0,sizeof(adcResult));

}


static bool v14rs485Init()
{
    //init rs485
    if(!hw_rs485_init())
    {
        err_log(" rs485 init failed \n");
        return false;
    }
    print_log("rs485 init ok\n");
    return true;
}
static void v14rs485Test()
{
    ModbusParser pressureReq;
    ModbusParser pressureRep;
    uint16_t sensorData = 0;
    static uint32_t lastRs485Time = 0;
    uint32_t currTime =k_uptime_get_32();
    static uint32_t rs485TestFirstRunTime = 0;
    static uint32_t lastRunTime = 0;
    if(rs485TestFirstRunTime == 0){rs485TestFirstRunTime = currTime;}
    if(currTime - lastRs485Time >= RS485_RUN_TIME)
    {
           
        //send  adc test result to server
        sendServerV14rs485Result();
        lastRs485Time = currTime;
    }
    if(currTime - lastRunTime < 2000)
    {  
        return;
    }
    lastRunTime = currTime;
    memset(&pressureReq,0,sizeof(ModbusParser));
    memset(&pressureRep,0,sizeof(ModbusParser));
    pressureReq.request0304.address = 1;
    pressureReq.request0304.function = 3;
    pressureReq.request0304.index = 0;
    pressureReq.request0304.count = 1;
    //modbusMaster0304ExchangeReq(ModbusParser * m0304Rep,ModbusParser *m0304Req);
    if(!modbusMaster0304ExchangeReq(&pressureRep,&pressureReq))
    {
        err_log("read pressure data is failed#########################\n");
        return;
    }
    //k_sleep(300);
    // process beer data
    sensorData = pressureRep.response0304.values[0];
    print_log("sensorData = %d...\n",sensorData);
    if(sensorData > 10)
    {
        rs485TestPass = true;
    }
    
}
static void sendServerV14rs485Result()
{
    print_log("rs485TestPass = %d....\n",rs485TestPass);
    if(!rs485TestPass){return;}
    canFIFO_t carrydata = {
                     .averageSpeed = 0,     //average speed since last upload at m/h
                     .moveDistance = 0,     //unit in "mm"
                     .moveAbsDistance = 0,  //absolute distance since last upload. unit in "mm"
                     .movePeriod = 0,  //sum of the time that velocity is not zero since last upload. time is "ms"

                     .batVoltage = 24000, //forklift 's battery voltage at "mV"
                     .batCurrent = 0,  //forklift's average current since last upload at "mA"
                     .batState   = 80, //florklift's battery percentage, from 0 to 100

                     .collectPeriod = 10000,   // can statistic period
                     .brakePeriod = 0,   //brake active time since last upload. "ms"
                     .seatPeriod = 10000,      //seat active time since last upload. "ms"
                     .forkPeriod = 0,      //fork active time since last upload. "ms"
                     .moveForkPeriod = 0,  //fork & move time overlap in "ms"


                     .forkNums = 0,      //how many times the forLINDE_CAN_GENERALk moves since the last move
                     .moveNums = 0,      //how many times this veLINDE_CAN_GENERALhicle moved since last upload
                     .forwardNums = 0,   //how many times the fork move forward
                     .backwardNums = 0,  //how many times the fork move reverse
                     .divertNums = 0,    //how_many times this vehicle change the direction since last upload

                     .forwardPeriod = 0,     // how long the fork move forward
                     .backwardPeriod = 0,    // how long the fork move reverse
                     .forwardDistance = 0,   //how far the fork move forward
                     .backwardDistance = 0,  //how far the fork move reverse

                     .carryPeriod = 10000, // how long the fork carry goods
                     .carryNums = 1, // how many times carry goods
                };
    /*
    int32_t  averageSpeed;     //average speed since last upload at m/h
    int32_t  moveDistance;     //unit in "mm"
    uint32_t moveAbsDistance;  //absolute distance since last upload. unit in "mm"
    uint32_t movePeriod;  //sum of the time that velocity is not zero since last upload. time is "ms"

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

    uint32_t carryPeriod; // how long the fork carry goods
    uint16_t carryNums; // how many times carry goods

    */
    //carrudata
    canFifoSend((char*)&carrydata);
}

static void v14rs485Uint()
{
    rs485TestPass = false;
    hw_rs485_unint();
}


