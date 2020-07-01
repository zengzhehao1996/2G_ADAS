#include <kernel.h>
#include "my_misc.h"
#include "offline_thread.h"
#include "cache_box.h"
#include "network_thread.h"
#include "rtc.h"
#include "server_interface.h"
#include <string.h>

#define OFFLINE_THREAD_STACK_SIZE 2048
K_THREAD_STACK_DEFINE(gOfflineThreadStack, OFFLINE_THREAD_STACK_SIZE);
static struct k_thread gOfflineThread;
static k_tid_t         gOfflineThreadId   = 0;
static bool volatile gOfflineThreadTuning = false;
#define SEND_CAN_SIZE (12 + sizeof(uploadCAN_t))

static uint32_t gLastOfflineThread = 0;

extern int checkBuff(uint8_t* buf, int len);

enum offline_cmd
{
    OFFLINE_NO_       = 0,
    OFFLINE_CAN_READ  = 1,
    OFFLINE_CAN_WRITE = 2,
    OFFLINE_MSG_READ  = 3,
    OFFLINE_MSG_WRITE = 4
};

static struct offline_buffer
{
    uint8_t          buf[252];
    int              ret;
    localRTC_t       rtc;
    volatile uint8_t cmd;
} g_cache;

static bool waitForFileThread(void)
{
    int counter = 0;
    const int max = 50;
    while(g_cache.cmd)
    {
        if(counter >= max)
        {
            return false;
        }
        k_sleep(10);  // equal to thread file main loop
        counter++;
    }

    return true;
}

static void writeCanOfflineDate(void)
{
    pushCanToCache(&g_cache.buf[0], SEND_CAN_SIZE, g_cache.rtc.date, g_cache.rtc.weekDay);
}

static void readCanOfflineDate(void)
{
    memset(g_cache.buf, 0, sizeof(g_cache.buf));
    if(true == popCanFromCache(&g_cache.buf[0], SEND_CAN_SIZE))
    {
        g_cache.ret = 1;
    }
    else
    {
        g_cache.ret = 0;
    }
}

static void writeMsgOfflineDate(void)
{
    pushMessageToCache(&g_cache.buf[0], g_cache.ret, g_cache.rtc.date, g_cache.rtc.weekDay);
}

static void readMsgOfflineDate(void)
{
    memset(g_cache.buf, 0, sizeof(g_cache.buf));
    g_cache.ret = popMessageFromCache(&g_cache.buf[0], sizeof(g_cache.buf));
}

void checkOffline(void)
{
    if(OFFLINE_NO_ == g_cache.cmd)
    {
        return;
    }

    // printk("[%s,%d] operate offline file.\n",__func__, __LINE__);
    printk(""); /* Used to solve the ***bus fault*** issue */
    switch(g_cache.cmd)
    {
        case OFFLINE_CAN_READ:
            readCanOfflineDate();
            break;
        case OFFLINE_CAN_WRITE:
            writeCanOfflineDate();
            break;
        case OFFLINE_MSG_READ:
            readMsgOfflineDate();
            break;
        case OFFLINE_MSG_WRITE:
            writeMsgOfflineDate();
            break;

        default:
            break;
    }
    g_cache.cmd = OFFLINE_NO_;
}

static void offlineThreadEntry(void)
{
    gOfflineThreadTuning = true;
    bool can_cache       = false;
    bool msg_cache       = false;
    int  read_size       = 0;

    int ret;
    int counter = 0;
    print_log("start Offline thread || || || || || || || || || \n");

    /* clean buff */
    g_cache.ret = checkBuff(g_cache.buf, sizeof(g_cache.buf));
    if(g_cache.ret > 0)
    {
        getRTC(&g_cache.rtc);
        /* want to save msg offline */
        g_cache.cmd = OFFLINE_MSG_WRITE;
        /* wait for write */
        if(!waitForFileThread())
        {
            err_log("file thread not response.\n");
        }
    }

    while(gOfflineThreadTuning)
    {
        /* step uptime */
        gLastOfflineThread = k_uptime_get_32();
        k_sleep(200);
        if(isLoginOk())
        {                       /* online , move cache data to network box from emmc */
            if(isAmpleCache())  //have cache
            {
                /* want to read can offline */
                g_cache.cmd = OFFLINE_CAN_READ;
                /* wait for read */
                if(waitForFileThread())
                {
                    can_cache = (bool)g_cache.ret;
                    if(can_cache)
                    {
                        //insert to send buffer
                        insertToCanSendCache(2, g_cache.buf, SEND_CAN_SIZE);
                        print_log("insert can...............\n");
                    }
                }
                else
                {
                    can_cache = false; /* don't read */
                    err_log("file thread not response.\n");
                }

                /* want to read offline msg */
                g_cache.cmd = OFFLINE_MSG_READ;
                /* wait to read */
                if(waitForFileThread())
                {
                    read_size = g_cache.ret;
                    if(read_size > 0)
                    {
                        msg_cache        = true;
                        uint16_t* pId    = (uint16_t*)&g_cache.buf[6];
                        uint32_t* pmsgId = (uint32_t*)&g_cache.buf[12];
                        uint32_t  tmp    = 0;
                        if(*pId == 0x172 || *pId == 0x123 || *pId == 0x32 || *pId == 0x134
                        || *pId == 0x135 || *pId == 0x103 || *pId == 0x230)
                        {
                            pmsgId = (uint32_t*)&g_cache.buf[12];
                            //print_log("before msgId:[%d], read size :[%d].\n",*pmsgId,read_size);
                        }
                        else
                        {

                            pmsgId = &tmp;
                        }
                        *pmsgId = getUniqueMsgId();
                        insertToSendCache(*pmsgId, g_cache.buf, read_size);
                        print_log("offline thread insert msgId:[%d],size:[%d].................\n",
                                *pmsgId, read_size);
                        print_log("offline thread HEAD:[0x%02x,0x%02x,0x%02x,0x%02x]\n", g_cache.buf[0],
                                g_cache.buf[1], g_cache.buf[2], g_cache.buf[3]);
                    }
                    else
                    {
                        msg_cache = false;
                    }
                }
                else
                {
                    msg_cache = false; /* don't read */
                    err_log("file thread not response.\n");
                }
            }

            if(!can_cache && !msg_cache)
            {
                break; /* no cache stop thread*/
            }
            k_sleep(1000);
        }
        else
        {
            if(0 == counter % 10)
            {
                getRTC(&g_cache.rtc);
            }
            bool rtcUpdateFlag = false;

            ret = 0;
            /*offline, move network data to emmc*/
            memset(&g_cache.buf, 0, sizeof(g_cache.buf));
            if(true == popClearCache(g_cache.buf, sizeof(g_cache.buf), &ret))
            {
                g_cache.ret = ret;
                /* want to save msg offline */
                g_cache.cmd = OFFLINE_MSG_WRITE;
                // print_log("popClear Head:[0x%02x,0x%02x]\n",g_cache.buf[0],g_cache.buf[1]);
                /* wait for write */
                if(waitForFileThread())
                {
                    //uploadRFID_t *rfid = &g_cache.buf[12];
                    //print_log("msgID:%d, cardId:%d, imei:[%s],len:[%d]\n",rfid->msgId, rfid->cardId, rfid->imei,ret);
                    print_log("push message........................\n");
                    rtcUpdateFlag = true;
                }
                else
                {
                    err_log("file thread not response. Delete message...\n");
                }
            }

            ret = 0;
            memset(&g_cache.buf, 0, sizeof(g_cache.buf));
            if(true == popCanCahe(g_cache.buf, sizeof(g_cache.buf), &ret))
            {
                g_cache.ret = ret;
                /* want to save can offline */
                g_cache.cmd = OFFLINE_CAN_WRITE;

                /* wait for write ok */
                if(waitForFileThread())
                {
                    print_log("push can .....................\n");
                    rtcUpdateFlag = true;
                }
                else
                {
                    err_log("file thread not response. Delete message...\n");
                }
            }

            if(rtcUpdateFlag == true)
            {
                counter++;
                k_sleep(500);
                continue;
            }
        }
    }

    gOfflineThreadTuning = false;
    gOfflineThreadId     = 0;
    print_log("Stop Offline thread || || || || || || || || || \n");
}

bool offlineThreadStart(void)
{
    bool ret;

    if(gOfflineThreadId != 0)
    {
        k_sleep(1000);
        if(gOfflineThreadId != 0)
            return true;  //thread run
    }

    gLastOfflineThread = k_uptime_get_32();

    gOfflineThreadId = k_thread_create(
        &gOfflineThread, gOfflineThreadStack, OFFLINE_THREAD_STACK_SIZE,
        (k_thread_entry_t)offlineThreadEntry, NULL, NULL, NULL, K_PRIO_COOP(12), 0, 0);

    if(gOfflineThreadId != 0)
    {
        ret = true;
        print_log("Create Offline THREAD Id:[ %p ]; Stack:[ %p ]; Size:[ %p ]\n", gOfflineThreadId,
                  gOfflineThreadStack, OFFLINE_THREAD_STACK_SIZE);
    }
    else
    {
        ret = false;
        return ret;
    }
    return ret;
}

bool offlineThreadIsRuning(void)
{
    return (gOfflineThreadTuning || (gOfflineThreadId != 0));
}

void offlineThreadSelfStop(void)
{
    gOfflineThreadTuning = false;
}

void offlineThreadSafetyStop(void)
{
    if(gOfflineThreadId != 0)
    {
        k_thread_abort(gOfflineThreadId);
        gOfflineThreadId = 0;
    }
}

void detect_thread_offline_thread(uint32_t ts)
{
    if(offlineThreadIsRuning())
    {
        if((ts > gLastOfflineThread) && (ts - gLastOfflineThread > THREAD_LOOP_TIME_60SEC))
        {
            warning_log("Restart offline thread. ++++++++++\n");
            offlineThreadSafetyStop();
            offlineThreadStart();
        }
    }
}