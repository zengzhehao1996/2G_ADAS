#include "rs485Thread.h"
#include "my_misc.h"
#include "modbus_rtu.h"
#include "server_interface.h"
#define RS485_THREAD_STACK_SIZE 1024
K_THREAD_STACK_DEFINE(g_rs485ThreadStack, RS485_THREAD_STACK_SIZE);
static struct k_thread g_rs485Thread;
static k_tid_t         g_rs485ThreadId;

static uint8_t rs485commCnt = 0; 
static readTestbox testBoxStat;
static setTestbox setTestBox1;
static bool setBoxOK = false;
static bool rs485threadTestModeReady = false;
static void rs485ThreadRun(void* p);
static void aidongRs485Test();
static void (*rs485Run)(void);
extern bool isFTStart();
bool startRs485Thread(rs485para_t *ps)
{
    bool ret;

    g_rs485ThreadId =
        k_thread_create(&g_rs485Thread, g_rs485ThreadStack, RS485_THREAD_STACK_SIZE,
                        (k_thread_entry_t)rs485ThreadRun, ps, NULL, NULL, K_PRIO_COOP(5), 0, 0);
    if(g_rs485ThreadId != 0)
    {
        ret = true;
        print_log("Create RS485 THREAD Id:[ %p ]; Stack:[ %p ]; Size:[ %p ]\n", g_rs485ThreadId,
                  g_rs485ThreadStack, RS485_THREAD_STACK_SIZE);
    }
    else
    {
        ret = false;
        err_log("Create RS485 RFID Failed.\n\n");
    }

    return ret;
}
void stopRs485Thread(void)
{
    hw_rs485_unint();
    rs485threadTestModeReady = false;
    if(0 != g_rs485ThreadId)
    {
        
        k_thread_abort(g_rs485ThreadId);
        g_rs485ThreadId = 0;
        print_log("v133 stopRs485 Thread  success !\n");
    }
}
static void rs485ThreadRun(void* p)
{
    int8_t mode = ((rs485para_t*)p)->mode;
    print_log("rs485 thread ,mode = %d..\n",mode);
    //1.init rs485
    if(!hw_rs485_init())
    {
        err_log(" rs485 init failed \n");
        return false;
    }

    if(mode == FACTORY_MODE)
    {
        rs485commCnt = 0;
        memset(&testBoxStat,0,sizeof(testBoxStat));
        memset(&setTestBox1,0,sizeof(setTestbox));
        setBoxOK = false;
        rs485threadTestModeReady = true;
        rs485Run = aidongRs485Test;
    }
    else if(mode == NORMAL_MODE)
    {
        //2.init pressureInit
        if(!pressureInit( ((rs485para_t*)p)->carryThreshold,((rs485para_t*)p)->overLoadThreshold))
        {
            err_log("pressureInit failed XXXXXXXXXXXXXXXXXXXXXXXXXX\n");
        }
        rs485Run = processCarryAffair;
    }

    while(1)
    {
        if(rs485Run){rs485Run();}  
        k_sleep(20);
    }
}

static void aidongRs485Test()
{

   //TODO
   //print_log("setTestBox1.sendFlag [%d]\n",setTestBox1.sendFlag);
   if(setTestBox1.sendFlag)
   {
        setTestBox1.sendFlag = false;
         ModbusParser data;
        char cnt = 0;
        data.request16.address = 2;
        data.request16.function = 0x10;
        data.request16.index = 2;
        data.request16.count = 1;
        data.request16.length = 2*data.request16.count;
        data.request16.values[0] = setTestBox1.testResult;
        print_log("write modbus 16\n");
        while(!modbusMaster16Write(&data))
        {

            cnt++;
            print_log("resend data to test box ,counter[%d],..\n",cnt);
            if(cnt >=5)
            {
                setBoxOK = false;
                return;
            }
            
            k_sleep(100);
        }
        setBoxOK = true;
   }

   /////////////
   if(isFTStart())
   {
        ModbusParser pressureReq;
       ModbusParser pressureRep;
       memset(&pressureReq,0,sizeof(ModbusParser));
       memset(&pressureRep,0,sizeof(ModbusParser));
       pressureReq.request0304.address = 2;
       pressureReq.request0304.function = 3;
       pressureReq.request0304.index = 0;
       pressureReq.request0304.count = 3;
       if(!modbusMaster0304Req(&pressureRep,&pressureReq))
       {
            print_log("modbus read error!\n");
            //k_sleep(100);
            return;
       }
       else
       {
            if(rs485commCnt < 5){rs485commCnt++;}
            
       }
       if(rs485commCnt>=5)
       {
            testBoxStat.rs485Stat = 1;
       }
       //pressureRep.
       testBoxStat.timeout = pressureRep.response0304.values[0];
       testBoxStat.contrlGpoStat = pressureRep.response0304.values[1];
       testBoxStat.reserved = pressureRep.response0304.values[2];
       //send cmd to test box!!

   }
  



}

bool getTestBoxData(readTestbox* readBox)
{
    *readBox = testBoxStat;
    return true;
}
bool setTestBox_(setTestbox cmd)
{
    if(setTestBox1.sendFlag)
    {
        print_log("set test box busy!!\n");
        return false;
    }
    
    setTestBox1 = cmd;
    print_log("cmd.d0[%d],setTestBox1.d0[%d]\n",cmd.testResult,setTestBox1.testResult);
    setBoxOK = false;
    return true;
}
bool setTBsucess()
{
    
    // print_log("send test box result [%d]!\n",setBoxOK);
    return setBoxOK;
}
bool rs485TMready()
{
    return rs485threadTestModeReady;
}