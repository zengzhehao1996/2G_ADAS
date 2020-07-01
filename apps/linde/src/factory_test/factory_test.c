#include "factory_test.h"
#include "rfid_thread.h"
#include "vehicle_ctrl_thread.h"
#include "thread_wdt.h"
#include "config.h"
#include "thread_gps.h"
#include "factory_test_unique.h"
#include "fifo.h"
#include "rs485Thread.h"
#include "modbus_rtu.h"

#define FACTORY_START_TIMEOUT (10*1000)
#define FACTORY_END_TIMEOUT (15*1000)
extern sysconfig_t gSysconfig;
static testStart_t gFactorycfg;

#define FACTORY_TEST_THREAD_STACK_SIZE 2048
K_THREAD_STACK_DEFINE(g_factoryTestThreadStack, FACTORY_TEST_THREAD_STACK_SIZE);
static struct k_thread g_factoryTestThread;
static k_tid_t         g_factoryTestThreadId;
static void            factoryTestThreadRun(void* p);
static aidongTestResult_t AiDongtestResult_1 = {.rs485 = 0,.contrlRelay = 0};
static int8_t aidongtestPassFlag = -1;
static uint32_t waitFactoryTimeout = 0;
static bool sendTestResultFlag = false;
static bool sendTimeoutFlag = false;
static bool testOK = false;
static bool waitFTresultAck = false;
static bool sendTestOKFlag = false;
static bool ADFTstartFlag = false;
//static bool getTBsendAck = false;
static uint32_t waitFTackTimeout = 0;
static bool judgeEnterAidongtest();
static bool sendMsg2TestBox(int8_t pass);
bool factoryTestThreadStart(testStart_t para)
{
    bool ret;
    gFactorycfg = para;
     print_log("Factory test mode:[%d], can_type:[%d], can_pattern:[%d], cmd:[%d]\n",para.mode, para.can_type, para.can_pattern, para.cmd);
    g_factoryTestThreadId =
        k_thread_create(&g_factoryTestThread, g_factoryTestThreadStack, FACTORY_TEST_THREAD_STACK_SIZE,
                        (k_thread_entry_t)factoryTestThreadRun, &gFactorycfg, NULL, NULL, K_PRIO_COOP(15), 0, 0);
    if(g_factoryTestThreadId != 0)
    {
        ret = true;
        print_log("Create factory test THREAD Id:[ %p ]; Stack:[ %p ]; Size:[ %p ]\n", g_factoryTestThreadId,
                  g_factoryTestThreadStack, FACTORY_TEST_THREAD_STACK_SIZE);
    }
    else
    {
        ret = false;
        err_log("Create Thread factory test Failed.\n\n");
    }

    return ret;

}
void factoryTestThreadStop()
{
    if(0 != g_factoryTestThreadId)
    {
        setFatoryGpsInterval(false);
        //2.start unique thread
        factory_test_unique_stop();
        //3.stop rfid thread
        rfidThreadStop();
        //4.stop vehical thread
        vehclThreadStop();
        //1.stop factory test thread
        k_thread_abort(g_factoryTestThreadId);
        g_factoryTestThreadId = 0;
        k_sleep(100);
        
        
    }

}
s64_t factoryTestThreadTime()
{
    return 0;
}


static void factoryTestThreadRun(void* p)
{
    testStart_t testConf;
    memcpy(&testConf, p, sizeof(testConf));
    print_log("factory test enter: mode:[%d],can_type:[%d],can_partten:[%d],cmd:[%d].\n",testConf.mode,testConf.can_type,testConf.can_pattern,testConf.cmd);
    // judge wether need to enter aidong factory test!
    waitFactoryTimeout = k_uptime_get_32();
    sendTestResultFlag = false;
    testOK = false;
    waitFTresultAck = false;
    sendTimeoutFlag = false;
    sendTestOKFlag = false;
    ADFTstartFlag = false;
    //waitFTackTimeout = k_uptime_get_32();
    memset(&AiDongtestResult_1,0,sizeof(AiDongtestResult_1));
    if(testConf.mode == TEST_AIDONG)
    {
        if(!judgeEnterAidongtest())
        {
            print_log("can not detect aidong factory test signal and quit detect mode!!!\n");
            stopRs485Thread();
            factoryTestThreadStop();
            return;

        }
        else
        {
            print_log("begin factory test!!\n");
            //waitFTackTimeout = k_uptime_get_32();
            ADFTstartFlag = true;
        }
    }

    setFatoryGpsInterval(true);

    // init rfid
    rfidThreadStart();
    //init unique thread!
    factory_test_unique_run(testConf);
    //init vehical ctrl thread
    static vehPara_t para;
    para.mode = MODE_FACTORY_TEST;
    print_log("factory run mode = %d\n",para.mode);
    vehclCtrlThreadStart(&para);

    while(1)
    {

        if(testConf.mode == TEST_AIDONG)
        {
            readTestbox data;
            getTestBoxData(&data);
            //1.test gpo and rs485
            if(data.contrlGpoStat && (!AiDongtestResult_1.contrlRelay))
            {
                print_log("contrl relay pass!!!!!!!!\n");
                AiDongtestResult_1.contrlRelay = 1;
            }
            if(data.rs485Stat && (!AiDongtestResult_1.rs485))
            {
                print_log("rs485 pass!!!!!!!!\n");
                AiDongtestResult_1.rs485 = 1;
            }
           
            if(AiDongtestResult_1.contrlRelay == 1 && AiDongtestResult_1.rs485 == 1 && !sendTestResultFlag)
            {
                print_log("AiDongtestResult_1 is pass!\n");
                sendTestResultFlag = true;
                testOK = true;
                //waitFTackTimeout = k_uptime_get_32();
                //sendTimeoutFlag = true;
                
            }
            //2. check test time timeout 
            if(data.timeout == 1 && !sendTimeoutFlag)
            {
                print_log(" test time out!!!!!\n");
                factoryTestTimeout();               
                sendTestResultFlag = true;
                sendTimeoutFlag = true;
                waitFTackTimeout = k_uptime_get_32();
                //waitFTackTimeout = k_uptime_get_32();
                //waitFTresultAck = true;
            }

            //3. send test result to test box
            
            //set test box state 
            //1.timeout 2.rcv pass signal
            
            if(sendTimeoutFlag)
            {                
                if(sendTimeoutFlag && (k_uptime_get_32()- waitFTackTimeout) > FACTORY_END_TIMEOUT)
                {
                        print_log("wait time out ,and set test box time_out\n");
                        if(aidongtestPassFlag == 1)
                        {
                            sendMsg2TestBox(1);
                        }
                        else
                        {
                            sendMsg2TestBox(2); 
                        }
                        
                        waitFTackTimeout = k_uptime_get_32();
                        waitFTresultAck = true;
                   
                }
                
                //aidongtestPassFlag = false;
            }
            else
            {
                if(aidongtestPassFlag == 1)
                {
                    sendMsg2TestBox(1);
                    waitFTackTimeout = k_uptime_get_32();
                    waitFTresultAck = true;
                }
                else if(aidongtestPassFlag == 0)
                {
                    sendMsg2TestBox(2); 
                    waitFTackTimeout = k_uptime_get_32();
                    waitFTresultAck = true;
                }
                
            }
            
            //4. check send result and quit test mode!
            if(waitFTresultAck && setTBsucess())
            {
                print_log("test finished!,test result[%],and abort thread!\n",aidongtestPassFlag);
                //abort thread!
                stopRs485Thread();
                factoryTestThreadStop();
            }
            k_sleep(1*1000);

        }
        else if(testConf.mode == TEST_KION)
        {
            k_sleep(10*1000);

        }
        else
        {
            k_sleep(10*1000);

        }


    }
}
int8_t AiDongTestModuleState(aidongTestResult_t* AiDongtestResult)
{
    if(sendTestResultFlag && !sendTestOKFlag)
    {
        print_log("send test message to msg_center thread,rs485[%d],contrlRelay[%d]!!!!!!!!!!!\n ",AiDongtestResult_1.rs485,AiDongtestResult_1.contrlRelay);
        
        *AiDongtestResult = AiDongtestResult_1;
         sendTestOKFlag = true;
        return 0;

    }
    else
    {
        return -1;
    }
}

static bool judgeEnterAidongtest()
{
    //start 485 thread
 
    static rs485para_t rs485Para;
    rs485Para.carryThreshold    = 0;            
    rs485Para.overLoadThreshold = 0;
    rs485Para.mode = FACTORY_MODE;
    startRs485Thread(&rs485Para);
    while(!rs485TMready())
    {
        k_sleep(30);
    }
    sendMsg2TestBox(3);
    while(k_uptime_get_32() - waitFactoryTimeout < FACTORY_START_TIMEOUT)
    {
        //readTestbox data;
        //getTestBoxData(&data);
        //if(data.rs485Stat == 1)
        if(setTBsucess())
        {
            while(k_uptime_get_32() < 15 * 1000)
            {
                k_sleep(100);
            }
            
            print_log("begin start factory test!!\n");
            startFactoryTest();
            return true;           
        }
        k_sleep(100);
    }
    return false;

    
}

static bool sendMsg2TestBox(int8_t pass)
{
    setTestbox data;
    if(pass == 1){data.testResult = 1;}
    else if(pass == 2){data.testResult = 2;}
    else if(pass == 3){data.testResult = 3;}
    data.sendFlag = true;
    print_log("testResult = %d\n",data.testResult);
    setTestBox_(data);
}

bool isFTStart()
{
    return ADFTstartFlag;
}

bool setAidondTestResult(bool pass)
{
    if(pass)
    {
        aidongtestPassFlag = 1;
    }
    else
    {
        aidongtestPassFlag = 0;

    }
    return true;
}


