#include "testProcessThread.h"
#include "modbus_rtu_slave.h"
#include "thread_led.h"

#include "my_misc.h"

static volatile uint8_t m_timeoutFlag = 0;

#define TEST_PROCESS_THREAD_STACK_SIZE 2048
K_THREAD_STACK_DEFINE(g_testProcesshreadStack, TEST_PROCESS_THREAD_STACK_SIZE);
static struct k_thread g_testProcessThread;
static k_tid_t         g_testProcessThreadId;

static char hlevelCnt = 0;
static char llevelCnt = 0;

#define TEST_TIMEOUT (90 * 1000)
/*
*buf[0] :timeout;1:timeout
*buf[1]:relay_stat;1:pass
*buf[2]: master_cmd;1:set_pass;2:set_failed;3:start_test
*/
enum CMD_MASTER
{
    INIT = 0,
    PASS = 1,
    FILED = 2,
    START = 3,
};
    #if 0
typedef struct 
{
    
    bool gpioPass;
    //bool m_timeoutFlag;
    bool timerStart;
    bool timeoutSetFlag;
    uint32_t testStartTime;
    

}testPars_t;
testPars_t testPars;
#endif
static bool gpioPass = false;
//static uint8_t m_timeoutFlag = 0;
static bool timerStart = false;
static bool timeoutSetFlag = false;
static uint32_t testStartTime = 0;

static void testProcessThreadRun(void* p);
static void rs485Process();
static void testGpio();
static uint16_t getMasterCmd();//1:pass ;2.failed;3:start.
static bool setTestTimeout(uint16_t isTimeout);// 1.timeout;
static bool setRelayTestResult(uint16_t relay);
//bool cleanTimeout();
static void testTimerRun();
extern bool isRs485ThreradStart();
extern bool isGpioThreadStart();
extern  bool getTestGpiVal();
extern bool ledThreadReay();
extern bool getStartAck();
extern bool resetStartAck();


bool startTestProcessThread(void)
{
    bool ret;

    g_testProcessThreadId =
        k_thread_create(&g_testProcessThread, g_testProcesshreadStack, TEST_PROCESS_THREAD_STACK_SIZE,
                        (k_thread_entry_t)testProcessThreadRun, NULL, NULL, NULL, K_PRIO_COOP(5), 0, 0);
    if(g_testProcessThreadId != 0)
    {
        ret = true;
        print_log("Create test THREAD Id:[ %p ]; Stack:[ %p ]; Size:[ %p ]\n", g_testProcessThreadId,
                  g_testProcesshreadStack, TEST_PROCESS_THREAD_STACK_SIZE);
    }
    else
    {
        ret = false;
        err_log("Create test thread Failed.\n\n");
    }

    return ret;
}
void stopTestProcessThread(void)
{
    if(0 != g_testProcessThreadId)
    {
        
        k_thread_abort(g_testProcessThreadId);
        g_testProcessThreadId = 0;
        print_log("stop  TestProcess  success !\n");
    }
}
static void testProcessThreadRun(void* p)
{
    //memset(&testPars,0,sizeof(testPars));

    gpioPass = false;
    m_timeoutFlag = 0;
    timerStart = false;
    timeoutSetFlag = false;
    testStartTime = 0;
    while(!isRs485ThreradStart() || !isGpioThreadStart() || !ledThreadReay())
    {
        print_log("wait 485 and gpio ,led thead\n");
        k_sleep(100);
    }
    while(1)
    {
        //print_log("111111\n");
        //print_log("m_timeoutFlag[%d]!!!!!!!!!!!!!!\n",m_timeoutFlag);
        uint16_t cmd  = getMasterCmd();

        //print_log("111111\n");
        if(cmd == INIT)
        {
            //close led;
            thread_led_stat_set(LED_STAT_OFF, 1);
            //reset global state
            //memset(&testPars,0,sizeof(testPars));
            gpioPass = false;
            m_timeoutFlag = 0;
            timerStart = false;
            timeoutSetFlag = false;
            testStartTime = 0;
        }
        else if(cmd == START)
        {
            //1.TEST GPIO
            //print_log("111111\n");
            if(getStartAck())
            {
                print_log("get start cmd!!!!!!!!!!!!!!!!!!!!\n");
                resetStartAck();
                gpioPass = false;
                m_timeoutFlag = 0;
                timerStart = true;
                timeoutSetFlag = false;
                hlevelCnt = 0;
                llevelCnt = 0;
                ////
                testStartTime = k_uptime_get_32();
                setTestTimeout(0);
                ////
                setRelayTestResult(0);
                thread_led_stat_set(LED_STAT_OFF, 1);
            }

            #if 1
            testGpio();

            if(m_timeoutFlag == 0)
            {
                //print_log("timerStart[%d]!!!!!!!!!!!!!!\n",timerStart);
                testTimerRun();

            }
            else if(timeoutSetFlag)
            {
                //send timeout
                print_log("time out!!!!!!!!!!!!!!!!!\n");
                setTestTimeout(1);
                timeoutSetFlag = false;
            }
            else
            {
                
            }
            #endif
        }
        else if(cmd == PASS)
        {
            #if 1
            //open led
            thread_led_stat_set(LED_STAT_NO_CAN_SIGNAL, 0);
            thread_led_stat_set(LED_STAT_ON, 1);
            //reset global state
            //memset(&testPars,0,sizeof(testPars));
            gpioPass = false;
            m_timeoutFlag = 0;
            timerStart = false;
            timeoutSetFlag = false;
            testStartTime = 0;
            setRelayTestResult(0);
            setTestTimeout(0);
            #endif
        }
        else if(cmd == FILED)
        {
            #if 1
            //blink led
            thread_led_stat_set(LED_STAT_NO_CAN_SIGNAL, 1);  //no net
            ////reset global state
           // memset(&testPars,0,sizeof(testPars));

            gpioPass = false;
            m_timeoutFlag = 0;
            timerStart = false;
            timeoutSetFlag = false;
            testStartTime = 0;
            setRelayTestResult(0);
            setTestTimeout(0);
            #endif
        }
        
        //print_log("111111\n");

        k_sleep(20);
    }
}


static uint16_t getMasterCmd()
{
     //1:pass ;2.failed;3:start.
     uint16_t cmd = 0;
     ModbusGetData(&cmd,2,1);
     //print_log("master cmd is %d\n",cmd);
     return cmd;
}
static bool setTestTimeout(uint16_t isTimeout)
{
    modbusSetData(&isTimeout,0,1);
    return true;
}
static bool setRelayTestResult(uint16_t relay)
{
    modbusSetData(&relay,1,1);
    return true;

}

static void testGpio()
{
    static bool gpioHFlag = false;
    static bool gpioLFlag = false;

    if(gpioPass)
    {
        gpioHFlag = false;
        gpioLFlag = false;
        hlevelCnt = 0;
        llevelCnt = 0;
        return;
    }
    ////////
    #if 1
    if(getTestGpiVal())
    {
        llevelCnt = 0;
        if(hlevelCnt < 100){hlevelCnt++;}
        
    }
    else
    {
        if(hlevelCnt > 20)
        {
            llevelCnt++;
        }
            
    }
    #endif

    ///////////////
    if(hlevelCnt > 20 && llevelCnt >20)
    {
        print_log("test gpio pass\n");
        gpioPass = true;
        setRelayTestResult(1);
    }
    
    
}
static void testTimerRun()
{
    //print_log("timerStart[%d]!!!!!!!!!!!!!!\n",timerStart);
    //print_log("run testTimerRun,timerStart=[ %d],m_timeoutFlag[%d],curr_time[%u],start time [%u],diff[%u]\n",
              // timerStart,m_timeoutFlag,k_uptime_get_32(),testStartTime,k_uptime_get_32()-testStartTime);
    if(timerStart == true)
    {
        if(k_uptime_get_32() - testStartTime > TEST_TIMEOUT )
        {
            m_timeoutFlag = 1;
            print_log("time out!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
            timeoutSetFlag = true;
        }
    }
}


