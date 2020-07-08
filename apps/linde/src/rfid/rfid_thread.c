#include "rfid_thread.h"
#include "my_misc.h"
#include "rfid_protocol_wg34.h"
#include "file.h"
#include "sempho.h"
#include "fifo.h"
#include "msg_structure.h"
#include "my_tool.h"
#include "kernel.h"

#define RFID_RCV_MAX_TIME 250
#define RFID_RCV_PARS_TIME 100
#define RFID_THREAD_STACK_SIZE 2048
#define RFID_CARD_NUM 256
#define MAX_READ_FILE 4

K_THREAD_STACK_DEFINE(g_rfidThreadStack, RFID_THREAD_STACK_SIZE);
static struct k_thread g_rfidThread;
static k_tid_t         g_rfidThreadId;
static uint32_t        rfidThreadRunlast_time  = 0;
static uint32_t        rfidList[RFID_CARD_NUM] = { 0 };
static int             rfidNUM                 = -1;
static bool            rfidReadFailFlag = false;
static uint8_t         rfidFileRead = 0;
static struct k_mutex  rfidListmutex;

static void            rfidThreadRun(void* p);
static uint32_t        isRfidAdvent();
static bool            checkRfidIdAvail(uint32_t rfid);
static bool            sendRfidMsg(uint32_t rfid, bool rfidAvail);
static int             rfidListGet(uint32_t* rfidList);
static void            printRfidList();
static uint32_t        stb_rfid = 0;

bool rfidThreadStart(void)
{
    bool ret;

    g_rfidThreadId =
        k_thread_create(&g_rfidThread, g_rfidThreadStack, RFID_THREAD_STACK_SIZE,
                        (k_thread_entry_t)rfidThreadRun, NULL, NULL, NULL, K_PRIO_COOP(5), 0, 0);
    if(g_rfidThreadId != 0)
    {
        ret = true;
        print_log("Create RFID THREAD Id:[ %p ]; Stack:[ %p ]; Size:[ %p ]\n", g_rfidThreadId,
                  g_rfidThreadStack, RFID_THREAD_STACK_SIZE);
    }
    else
    {
        ret = false;
        err_log("Create Thread RFID Failed.\n\n");
    }

    return ret;
}
void rfidThreadStop(void)
{
    if(0 != g_rfidThreadId)
    {
        rfidWg34UnInit();
        k_thread_abort(g_rfidThreadId);
        g_rfidThreadId = 0;
        print_log("rfidThreadStop  success !");
    }
}
s64_t rfidThreadRunLastTime()
{
    return rfidThreadRunlast_time;
}

static uint32_t isRfidAdvent()
{
    //print_log("isRfidAdvent   enter...........1\n");
    uint32_t     retRfidId       = 0;
    static bool  isWg34ParsReady = false;
    char         wgBit           = 0;
    int          wgRcvTime       = RFID_RCV_MAX_TIME;
    unsigned int serilId         = 0;
    if(isWg34ParsReady)
    {
        wgBit = rfidWg34RcvBitCnt();
        if(wgBit != 34)
        {
            //print_log("isRfidAdvent   enter...........1\n");
            rfidWg34Reset();
            //retRfidId = 0;
            k_sleep(wgRcvTime);
            return 0;
        }
        //print_log("isWg34ParsReady  = %d\n", isWg34ParsReady);
        serilId = rfidWg34GetCardId();
        if(serilId != 0)
        {
            print_log("serilId = [ %u ]\n", toBigEndian(serilId));
            retRfidId = serilId;
        }
        rfidWg34Reset();
        //print_log("isRfidAdvent   enter...........1\n");
        isWg34ParsReady = false;
        k_sleep(wgRcvTime);
        stb_rfid = retRfidId;
        return retRfidId;
    }
    wgBit = rfidWg34RcvBitCnt();
    //print_log("isRfidAdvent   enter...........1\n");
    if(wgBit == 0)
    {
        wgRcvTime = RFID_RCV_MAX_TIME;
        //print_log("wgBit = %d\n",wgBit);
    }
    else if(wgBit < 0 || wgBit > 34)
    {
        wgRcvTime = RFID_RCV_MAX_TIME;
        //print_log("wgBit = %d\n",wgBit);
        rfidWg34Reset();
    }
    else if(wgBit <= 34 && wgBit > 0)
    {
        wgRcvTime       = RFID_RCV_PARS_TIME;
        isWg34ParsReady = true;
        //print_log("isRfidAdvent   enter...........3\n");
    }
    //k_sleep(wgRcvTime);
    return 0;
}

uint32_t RFID_get()
{
    extern  uint32_t stb_rfid;
    return stb_rfid;
}

static bool checkRfidIdAvail(uint32_t rfid)
{
    if(rfid == 0)
    {
        return false;
    }
    if(k_mutex_lock(&rfidListmutex,100)==0)
    {
        for(int i = 0; i < rfidNUM; i++)
         {
             if(rfidList[i] == 0)
            {
                k_mutex_unlock(&rfidListmutex);
                return false;
            }
             if(rfidList[i] == rfid)
            {
                k_mutex_unlock(&rfidListmutex);
                return true;
            }
             else
            {
                 continue;
            }
        }
        k_mutex_unlock(&rfidListmutex);
        
    }

    return false;
}



static bool sendRfidMsg(uint32_t rfid, bool rfidAvail)
{
    rfid2CtrlFIFO_t txRfidMsg;
    txRfidMsg.rfidValible = rfidAvail;
    txRfidMsg.cardId      = rfid;
    print_log("rfidAvail is %d,rfid = [ %u ]\n", (int)txRfidMsg.rfidValible,
              toBigEndian(txRfidMsg.cardId));
    for(int i = 0; i < 2; i++)
    {
        if(!rfid2CtrlFifoSend(&txRfidMsg))
        {
            err_log("rfid2CtrlFifoSend failed\n");
            k_sleep(100);
            continue;
        }
        break;
    }
    k_sleep(500);
    return true;
}

static void rfidThreadRun(void* p)
{

    int      ret        = -1;
    uint32_t rfidID     = 0;
    bool     isExit     = false;
    bool     SendSucess = false;
    ret                 = rfidWg34Setup();
    ret = 0;
    if(ret)
    {
        err_log("rfidWg34Setup fail,ret = %d\n", ret);
    }
    //give vehicle thread a sempha that wg34 init finished
    //k_sem_give(&g_sem_wg34Ready);

    for(char i = 0; i < 3; i++)
    {
        if(!semGiveWg34Ready())
        {
            k_sleep(100);
            err_log("semGiveWg34Ready is failed\n");
            continue;
        }
        break;
    }

     k_mutex_init(&rfidListmutex); 

    //get msg from emmc
    printRfidList();
    print_log("rfid init success!!!!!!!!!!!!!!!!!!!!\n");

    while(1)
    {      
        rfidThreadRunlast_time = k_uptime_get_32();
        //1.check  rfid list updata semaphore wether come
        //2.check rfid card wether come
        rfidID = isRfidAdvent();
        if(rfidID == 0)
        {
            //print_log("rfid is not coming\n");
            k_sleep(200);
            continue;
        }
        print_log("rfid = [ %u ]\n", toBigEndian(rfidID));
        //3.check rfid wether belong rfid list
        isExit = checkRfidIdAvail(rfidID);
        //4.upload rfid to vehicle thread
        if(rfidReadFailFlag)
        {
            isExit = true;
        }
        SendSucess = sendRfidMsg(rfidID, isExit);
        k_sleep(500);
    }
}


uint8_t readRfidListFile(void)
{
     int byteNum = 0;
     if(MAX_READ_FILE < rfidFileRead)
     {
         rfidReadFailFlag = true;
        return rfidFileRead;
     }
     else    // else if (rfidFileRead==0 || MAX_READ_FILE >= rfidFileRead)
     {
        if(k_mutex_lock(&rfidListmutex,K_NO_WAIT)==0)
        {
            byteNum = readRfidList(rfidList, RFID_CARD_NUM * 4);
            if(byteNum<=0)
            {
                rfidFileRead++;
                rfidReadFailFlag = true;
                print_log("read Rfid list file failed ................\n");
                char buf[64];
                sprintf(buf,"WARN: %d RFID read filelist failed code:4",getTimeStamp());
                serverSendErrLog(buf);
            }
            else
            {
                rfidNUM = byteNum/4;
                rfidFileRead = 0;
                rfidReadFailFlag = false;
                print_log("read Rfid list Num::%d ................\n",rfidNUM);
            }
            k_mutex_unlock(&rfidListmutex);
        }
        else
        {
            print_log("get RFid list failed .............\n");
            return 1;// get lock failed
        }
     }
     return rfidFileRead;
}




static void printRfidList()
{
    print_log("RFID List %d:\n", rfidNUM);
    for(int i = 0; i < rfidNUM; i++)
    {
        printk("[%u] ", toBigEndian(rfidList[i]));
    }
    printk("\n");
}
void debugRfidErrFlagSet(bool flag)
{
    rfidReadFailFlag = flag;
}


