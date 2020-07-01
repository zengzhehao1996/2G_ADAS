#include "server_interface.h"
#include "network_thread.h"
#include "protocol.h"
#include "my_misc.h"
#include "smart_link_version.h"
#include "hw_version.h"
#include <string.h>

#include "atc.h"
#include "atcip.h"

#include "qcloud_iot_export.h"
#include "qcloud_iot_import.h"

#include "aidong_custom.h"
#include "thread_gps.h"
#include "thread_led.h"

#define NETWORK_THREAD_STACK_SIZE 4096
K_THREAD_STACK_DEFINE(gNetworkThreadStack, NETWORK_THREAD_STACK_SIZE);
static struct k_thread gNetworkThread;
static k_tid_t         gNetworkThreadId = 0;

static bool firstconnect = true;
static bool subsucessed  = false;
static bool subsucess    = false;

static uint32_t gLastNetworkThread = 0;

K_MUTEX_DEFINE(g_sendBuffer_mutex);
K_MUTEX_DEFINE(g_ImBuffer_mutex);
K_MUTEX_DEFINE(g_canSendBuffer_mutex);

#define SERVER_BUFFER_SIZE 250
#define DERVER_IMBUFFER_SIZE 250
#define CAN_BUFFER_SIZE 128
#define RESEND_TIME_INTERVAL_10SEC (10*1000)
#define RESEND_TIME_INTERVAL (30 * 1000)
#define RESEND_MAX_COUNT 4
static struct
{
    uint8_t  data_[SERVER_BUFFER_SIZE];
    int      index_;
    uint16_t pack_id;
    uint16_t publish_count;
    uint32_t sendMs;
} gTmpBuffer, gTmpBuffer_2, gTmpBuffer_3;

static struct
{
    uint8_t  data_[CAN_BUFFER_SIZE];
    int      index_;
    uint16_t pack_id;
    uint16_t publish_count;
    uint32_t sendMs;
} gTmpBuffer_can;

static struct
{
    uint8_t data_[DERVER_IMBUFFER_SIZE];
    int     index_;
} gImBuffer;

#define RE_UPLOAD_INTERVAL 10000  //10sec
typedef struct
{
    uint32_t msgId_;
    uint32_t sendTime_;
    uint16_t size_;
    uint8_t* data_;
} cache_t;

static networkPara_t g_networkPara;

static uint32_t sg_sub_packet_id = -1;
static uint8_t  mqtt_connect     = 0;

static uint32_t g_last_publish_ms          = 0;
static uint32_t g_publish_timeout_interval = 2 * 60 * 1000;  // 2min
static bool     reconnect(void);
static void     updateLastPublishTm(void);

bool log_handler(const char* message)
{
    //实现日志回调的写方法
    //实现内容后请返回true
    return false;
}

void event_handler(void* pclient, void* handle_context, MQTTEventMsg* msg)
{
    MQTTMessage* mqtt_messge = (MQTTMessage*)msg->msg;
    uintptr_t    packet_id   = (uintptr_t)msg->msg;
    //print_log("EVENT HANDLER..........................................................................\n");
    switch(msg->event_type)
    {
        case MQTT_EVENT_UNDEF:
            print_log("undefined event occur.");
            break;

        case MQTT_EVENT_DISCONNECT:
            print_log("MQTT disconnect.");
            setMmqttConnect(0);
            break;

        case MQTT_EVENT_RECONNECT:
            print_log("MQTT reconnect.");
            break;

        case MQTT_EVENT_PUBLISH_RECVEIVED:
            print_log("topic message arrived but without any related handle: len=%d, "
                      "topic=%s,payload_len=%d",
                      mqtt_messge->topic_len, mqtt_messge->ptopic, mqtt_messge->payload_len);
            break;
        case MQTT_EVENT_SUBCRIBE_SUCCESS:
            print_log("subscribe success, packet-id=%u", (unsigned int)packet_id);
            serverSendLog("subscribe success!!!!!!!!!");
            sg_sub_packet_id = packet_id;
            subsucess        = true;
            subsucessed      = true;
            updateLastPublishTm();
            break;

        case MQTT_EVENT_SUBCRIBE_TIMEOUT:
            print_log("subscribe wait ack timeout, packet-id=%u", (unsigned int)packet_id);
            sg_sub_packet_id = packet_id;
            subsucess        = FALSE;
            break;

        case MQTT_EVENT_SUBCRIBE_NACK:
            print_log("subscribe nack, packet-id=%u", (unsigned int)packet_id);
            sg_sub_packet_id = packet_id;
            subsucess        = FALSE;
            break;

        case MQTT_EVENT_UNSUBCRIBE_SUCCESS:
            print_log("unsubscribe success, packet-id=%u", (unsigned int)packet_id);
            updateLastPublishTm();
            break;

        case MQTT_EVENT_UNSUBCRIBE_TIMEOUT:
            print_log("unsubscribe timeout, packet-id=%u\n", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_UNSUBCRIBE_NACK:
            print_log("unsubscribe nack, packet-id=%u\n", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_PUBLISH_SUCCESS:
            print_log("publish success, packet-id=%u,tmp-id=%u\n", (unsigned int)packet_id,
                      gTmpBuffer.pack_id);

            updateLastPublishTm();

            if(packet_id == gTmpBuffer.pack_id)
            {
                memset(&gTmpBuffer, 0, sizeof(gTmpBuffer));
                print_log("reset Tmp Buffer1.\n");
            }
            else if(packet_id == gTmpBuffer_2.pack_id)
            {
                memset(&gTmpBuffer_2, 0, sizeof(gTmpBuffer_2));
                print_log("reset Tmp Buffer_2.\n");
            }
            else if(packet_id == gTmpBuffer_3.pack_id)
            {
                memset(&gTmpBuffer_3, 0, sizeof(gTmpBuffer_3));
                print_log("reset Tmp Buffer_3.\n");
            }
            else if(packet_id == gTmpBuffer_can.pack_id)
            {
                memset(&gTmpBuffer_can, 0, sizeof(gTmpBuffer_can));
                print_log("reset CAN Tmp CanBuffer.\n");
            }

            break;

        case MQTT_EVENT_PUBLISH_TIMEOUT:
            print_log("publish timeout, packet-id=%u,tmp-id=%u\n", (unsigned int)packet_id,
                      gTmpBuffer.pack_id);
            if(packet_id == gTmpBuffer.pack_id)
            {
                gTmpBuffer.publish_count++;
            }
            else if(packet_id == gTmpBuffer_2.pack_id)
            {
                gTmpBuffer_2.publish_count++;
            }
            else if(packet_id == gTmpBuffer_3.pack_id)
            {
                gTmpBuffer_3.publish_count++;
            }

            if(reconnect())
            {
                setMmqttConnect(0);
                char buf[64];
                sprintf(buf, "WARN: %d publish timeout count=%d", getTimeStamp(),
                        gTmpBuffer.publish_count);
                serverSendErrLog(buf);
                warning_log("Disconnected network !!!!!!!!!!!!!!!!\n");
            }
            break;

        case MQTT_EVENT_PUBLISH_NACK:
            print_log("publish nack, packet-id=%u\n", (unsigned int)packet_id);
            break;
        default:
            print_log("Should NOT arrive here.\n");
            break;
    }
}


static int _setup_connect_init_params(MQTTInitParams* initParams)
{
    initParams->device_name   = DEVICEID;
    initParams->product_id    = SIMCARDID;
    initParams->device_secret = "";

    initParams->command_timeout        = 6 * 1000;  //6sec //QCLOUD_IOT_MQTT_COMMAND_TIMEOUT;
    initParams->keep_alive_interval_ms = QCLOUD_IOT_MQTT_KEEP_ALIVE_INTERNAL;

    initParams->auto_connect_enable  = 1;
    initParams->event_handle.h_fp    = event_handler;
    initParams->event_handle.context = NULL;

    return QCLOUD_ERR_SUCCESS;
}


static void requestConfig2RfidList()
{
    if(false == serverRequestConfig(atcGetIMEI(NULL)))
    {
        k_sleep(1000);
        serverRequestConfig(atcGetIMEI(NULL));
        warning_log("Request config..................................\n");
    }

    /* request rfid */
    uint8_t md5[16] = { 0 };
    serverRequestRFIDList(&md5);
    return;
}


static void networkSendList(void)
{
    /* sendc simCard id */
    serverSendCCID(atcGetCCID(NULL));

    /* send version */
    uploadVersion_t version = { g_hw_major,        g_hw_minor,         g_hw_tiny,
                                g_hw_patch,        SOFT_VERSION_MAJOR, SOFT_VERSION_MINOR,
                                SOFT_VERSION_TINY, SOFT_VERSION_PATCH };
    strncpy(version.text, g_hw_text, sizeof(version.text));
    serverSendVersion(&version);
    print_log("Send version.\n");

    /* tell server power on */
    char buf[64] = { 0 };
    sprintf(buf, "^^^^^^^^ Device Power On. connect ts:%d\n", getTimeStamp());
    serverSendErrLog(buf);
    print_log(buf);

    /* request auto fota bin */
    if(g_networkPara.auto_fota_switch == AUTO_FOTA_ON)
    {
        autoFotaRequest_t rV = { 0 };
        rV.ts                = getTimeStamp();
        rV.hardVersion.major = g_hw_major;
        rV.hardVersion.minor = g_hw_major;
        rV.hardVersion.tiny  = g_hw_tiny;
        rV.hardVersion.least = g_hw_patch;
        serverRequestFotaVersion(&rV);
        print_log("Request auto fota packeg...FFFFFFFFFFFFFFFFFFFFFFFFFFFFFF\n");
    }
}

#define SEND_BUFFER_COUNT 15
#define IM_BUFFER_CONUT 5
#define CAN_BUFFER_COUNT 3
cache_t gSendBuffer[SEND_BUFFER_COUNT];
cache_t ImSendBuffer[IM_BUFFER_CONUT];
cache_t gCanSendBuffer[CAN_BUFFER_COUNT];


void* client = NULL;

static bool first_start = true;

static void networkThreadEntry(void* p1)
{
    networkPara_t* pra         = (networkPara_t*)p1;
    uint16_t       hb_timeout  = pra->hb_timeout;
    g_networkPara              = *pra;
    g_publish_timeout_interval = hb_timeout * 1000;  //para in sec, publish use ms
    print_log("publish time out interval : %d\n", g_publish_timeout_interval);
    uint8_t  offline_con = 0;
    uint32_t interval;
    uint32_t yield_timeout = 500;

    if(first_start)
    {
        cpParserInit(serverCmdParser);

        atc_init(1024);
        first_start = false;
    }
    else
    {
        atc_reset();
        cpParserReset();
    }


    int  rc  = 0;
    bool ret = false;

    while(1)
    {
        /* step uptime */
        gLastNetworkThread = k_uptime_get_32();

        if(isMqttConnect() == 1)
        {
            bool resend_flag = false;
            uint32_t curr_time      = k_uptime_get_32();
            uint32_t diff_send_time = 0;
            yield_timeout           = 500;
            // print_log("in mqtt connect ..................\n");
            thread_led_stat_set(LED_STAT_NO_COMM_SIGNAL, 0);
            offline_con = 0;

            if(true == firstconnect && true == subsucessed && !isEmptyCache())
            {
                print_log("Request List......\n");
                requestConfig2RfidList();
                networkSendList();
                firstconnect = false;
            }

            if(!isEmptyIm())
            {
                memset(&gImBuffer, 0, sizeof(gImBuffer));
                do
                {
                    //print_log("read pop\n");
                    ret = popImCache(&gImBuffer.data_[gImBuffer.index_],
                                     SERVER_BUFFER_SIZE - gImBuffer.index_, &gImBuffer.index_);
                    //print_log("ret=[%d]\n",ret);
                    k_sleep(20);
                } while(ret);

                if(gImBuffer.index_ != 0)
                {
                    // print_log("pubish Im topic \n");
                    if(0 == pubish_imdata(client, gImBuffer.data_, gImBuffer.index_))
                    {
                        yield_timeout = 5000;
                    }
                }
            }

            if(!isEmptyCanCache())
            {
                if(0 == gTmpBuffer_can.pack_id)
                {
                    memset(&gTmpBuffer_can, 0, sizeof(gTmpBuffer_can));
                    int tmp_len = 0;
                    ret         = popCanCahe(&gTmpBuffer_can.data_[gTmpBuffer_can.index_],
                                     SERVER_BUFFER_SIZE - gTmpBuffer_can.index_, &tmp_len);
                    if(ret)
                    {
                        gTmpBuffer_can.index_ += tmp_len;
                    }
                }

                diff_send_time = curr_time - gTmpBuffer_can.sendMs;
                if(gTmpBuffer_can.index_ != 0 && diff_send_time > RESEND_TIME_INTERVAL_10SEC)
                {
                    // print_log("TMP3 pubish data.\n");
                    if(0
                       == pubish_candata_id(client, gTmpBuffer_can.data_, gTmpBuffer_can.index_,
                                            &gTmpBuffer_can.pack_id))
                    {
                        print_log("CAN pubish data id:[%u]\n", gTmpBuffer_can.pack_id);
                        // memset(&gTmpBuffer_can, 0, sizeof(gTmpBuffer_can));
                        yield_timeout = 6000;
                    }
                    gTmpBuffer_can.sendMs = curr_time;
                    if(gTmpBuffer_can.publish_count != 0)
                    {
                        resend_flag = true;
                    }
                }
            }

            if(!isEmptyCache())
            {
                if(0 == gTmpBuffer.pack_id)
                {
                    memset(&gTmpBuffer, 0, sizeof(gTmpBuffer));
                    int tmp_len = 0;
                    do
                    {
                        //print_log("read pop\n");
                        ret = popCahe(&gTmpBuffer.data_[gTmpBuffer.index_],
                                      SERVER_BUFFER_SIZE - gTmpBuffer.index_, &tmp_len);
                        if(ret)
                        {
                            gTmpBuffer.index_ += tmp_len;
                        }
                        k_sleep(20);
                    } while(ret);
                }

                // if(isHaveCache())
                // {
                //     if(0 == gTmpBuffer_2.pack_id)
                //     {
                //         memset(&gTmpBuffer_2, 0, sizeof(gTmpBuffer_2));
                //         int tmp_len = 0;
                //         do
                //         {
                //             //print_log("read pop\n");
                //             ret = popCahe(&gTmpBuffer_2.data_[gTmpBuffer_2.index_],
                //                           SERVER_BUFFER_SIZE - gTmpBuffer_2.index_, &tmp_len);
                //             if(ret)
                //             {
                //                 gTmpBuffer_2.index_ += tmp_len;
                //             }
                //             k_sleep(20);
                //         } while(ret);
                //     }
                // }

                // if(isHaveCache())
                // {
                //     if(0 == gTmpBuffer_3.pack_id)
                //     {
                //         memset(&gTmpBuffer_3, 0, sizeof(gTmpBuffer_3));
                //         int tmp_len = 0;
                //         do
                //         {
                //             //print_log("read pop\n");
                //             ret = popCahe(&gTmpBuffer_3.data_[gTmpBuffer_3.index_],
                //                           SERVER_BUFFER_SIZE - gTmpBuffer_3.index_, &tmp_len);
                //             if(ret)
                //             {
                //                 gTmpBuffer_3.index_ += tmp_len;
                //             }
                //             k_sleep(20);
                //         } while(ret);
                //     }
                // }


                diff_send_time   = curr_time - gTmpBuffer.sendMs;
                
                if(gTmpBuffer.index_ != 0
                   && (0 == gTmpBuffer.sendMs || diff_send_time > RESEND_TIME_INTERVAL))
                {
                    // print_log("TMP1 pubish data.\n");
                    if(0
                       == pubish_data_id(client, gTmpBuffer.data_, gTmpBuffer.index_,
                                         &gTmpBuffer.pack_id))
                    {
                        print_log("TMP1 pubish data id:[%u]\n", gTmpBuffer.pack_id);
                        yield_timeout = 6000;
                    }
                    gTmpBuffer.sendMs = curr_time;
                    if(gTmpBuffer.publish_count != 0)
                    {
                        resend_flag = true;
                    }
                }
                diff_send_time = curr_time - gTmpBuffer_2.sendMs;
                if(gTmpBuffer_2.index_ != 0
                   && (0 == gTmpBuffer_2.sendMs || diff_send_time > RESEND_TIME_INTERVAL))
                {
                    // print_log("TMP2 pubish data.\n");
                    if(0
                       == pubish_data_id(client, gTmpBuffer_2.data_, gTmpBuffer_2.index_,
                                         &gTmpBuffer_2.pack_id))
                    {
                        print_log("TMP2 pubish data id:[%u]\n", gTmpBuffer_2.pack_id);
                        yield_timeout = 6000;
                    }

                    gTmpBuffer_2.sendMs = curr_time;
                    if(gTmpBuffer_2.publish_count != 0)
                    {
                        resend_flag = true;
                    }
                }
                diff_send_time = curr_time - gTmpBuffer_3.sendMs;
                if(gTmpBuffer_3.index_ != 0
                   && (0 == gTmpBuffer_3.sendMs || diff_send_time > RESEND_TIME_INTERVAL))
                {
                    // print_log("TMP3 pubish data.\n");
                    if(0
                       == pubish_data_id(client, gTmpBuffer.data_, gTmpBuffer_3.index_,
                                         &gTmpBuffer_3.pack_id))
                    {
                        print_log("TMP3 pubish data id:[%u]\n", gTmpBuffer_3.pack_id);
                        yield_timeout = 6000;
                    }
                    gTmpBuffer_3.sendMs = curr_time;
                    if(gTmpBuffer_3.publish_count != 0)
                    {
                        resend_flag = true;
                    }
                }

                if(resend_flag)
                {
                    print_log("Resend count TMP1:[%d], TMP2:[%d], TMP3:[%d], CAN:[%d];    ",
                              gTmpBuffer.publish_count, gTmpBuffer_2.publish_count,
                              gTmpBuffer_3.publish_count, gTmpBuffer_can.publish_count);
                    test_log("publish interval:[%d], timeout interval:[%d]\n",
                             curr_time - g_last_publish_ms, g_publish_timeout_interval);
                }
            }

            if(!subsucess)
            {
                subscribe_topic(client, SERVER_EVENT, &sg_sub_packet_id);
                subsucess = TRUE;
                print_log("subscribe topic retry\n");
                k_sleep(100);
            }

            IOT_MQTT_Yield(client, yield_timeout);
        }
        else
        {
            print_log("in mqtt disconnect ..................\n");
            subsucessed = false;
            thread_led_stat_set(LED_STAT_NO_COMM_SIGNAL, 1);
            if(offline_con == 0)
            {
                setLoginOk(false);
                offlineThreadStart();
            }
            if(client != NULL)
            {
                print_log("destroy mqtt client ................\n");
                IOT_MQTT_Destroy(&client);
            }

            readConfig(); /* load server ip and port */
            atc_reset();
            init_gpio();
            // atc_open(10000);
            if(atc_open(30000) != 0)
            {

                warning_log("ATC open timeout!\n");

                /* 为了监控线程,分两次做延时处理 */
                gLastNetworkThread = k_uptime_get_32();
                k_sleep(5000 * offline_con);
                gLastNetworkThread = k_uptime_get_32();
                k_sleep(5000 * offline_con);
                gLastNetworkThread = k_uptime_get_32();

                if(offline_con == 0)
                {
                    offline_con = 1;
                }
                else if(offline_con <= 60)
                {
                    offline_con = 2 * offline_con;
                }
                else
                {
                    offline_con = 60;
                }

                continue;
            }

            if(atcip_init(ATCIP_MODE_TRANSPARENT) != 0)
            {
                warning_log("ATCIP init timeout!");
            }

            custom_init();

            MQTTInitParams init_params = DEFAULT_MQTTINIT_PARAMS;

            rc = _setup_connect_init_params(&init_params);

            if(rc != QCLOUD_ERR_SUCCESS)
            {
                err_log("Setup GSM connect init fail!");
            }
            client    = NULL;
            subsucess = false;
            client    = IOT_MQTT_Construct(&init_params);
            if(client)
            {
                rc = subscribe_topic(client, SERVER_EVENT, &sg_sub_packet_id);
                print_log("subscribe topic return::::::::::%d\n", rc);
                k_sleep(100);
                subsucess = TRUE;
                setMmqttConnect(1);
                uint8_t buf[28] = { 0 };
                serverLoginInMQTT(atcGetIMEI(NULL), buf);
                pubish_imdata(client, buf, 27);
                print_log("logIn imei:%s\n", atcGetIMEI(NULL));
                setLoginOk(true);
            }
            else
            {
                err_log("MQTT client is NULL !!!!!!!!!!\n");
                char buf[64];
                sprintf(buf, "WARN: %d mqtt clinet NULL.", getTimeStamp());
                serverSendErrLog(buf);
            }
        }

        k_sleep(10);
    }
}

int networkThreadStart(networkPara_t pra)
{
    int ret;

    print_log("hb_timeOut:[%d] autoFota:[%d]", pra.hb_timeout, pra.auto_fota_switch);

    gLastNetworkThread = k_uptime_get_32();

    gNetworkThreadId = k_thread_create(
        &gNetworkThread, gNetworkThreadStack, NETWORK_THREAD_STACK_SIZE,
        (k_thread_entry_t)networkThreadEntry, &pra, NULL, NULL, K_PRIO_COOP(9), 0, 0);

    if(gNetworkThreadId != 0)
    {
        ret = 0;
        print_log("Create Network THREAD Id:[ %p ]; Stack:[ %p ]; Size:[ %p ]\n", gNetworkThreadId,
                  gNetworkThreadStack, NETWORK_THREAD_STACK_SIZE);
    }
    else
    {
        ret = -1;
        err_log("Create Thread Network Failed.\n\n");
    }

    return ret;
}

void safeNetworkTreadStop(void)
{
    if(gNetworkThreadId != 0)
    {
        if(client != NULL)
        {
            print_log("destroy mqtt client ................\n");
            IOT_MQTT_Destroy(&client);
            client                   = NULL;
            gTmpBuffer.publish_count = 0;
            setMmqttConnect(0);
        }
        k_thread_abort(gNetworkThreadId);
    }
    gNetworkThreadId = 0;
}

void detect_thread_nerwork_thread(uint32_t ts)
{
    if((ts > gLastNetworkThread) && (ts - gLastNetworkThread > THREAD_LOOP_TIME_10MIN))
    {
        warning_log("Restart network thread. ++++++++++++++++++++\n");
        safeNetworkTreadStop();
        networkPara_t tmp = g_networkPara;
        networkThreadStart(tmp);
        gLastNetworkThread = k_uptime_get_32();
    }
}

void initCache(void)
{
    memset(gSendBuffer, 0, sizeof(gSendBuffer));
    memset(ImSendBuffer, 0, sizeof(ImSendBuffer));
}

static int getSocketFromCache(void)
{
    for(int i = 0; i < SEND_BUFFER_COUNT; ++i)
    {
        if(0 == gSendBuffer[i].msgId_)
        {
            return i;
        }
    }

    return -1;
}

static int getSocketFromCanCache(void)
{
    for(int i = 0; i < CAN_BUFFER_COUNT; ++i)
    {
        if(0 == gCanSendBuffer[i].msgId_)
        {
            return i;
        }
    }

    return -1;
}

static int getSocketFromImCache(void)
{
    for(int i = 0; i < IM_BUFFER_CONUT; ++i)
    {
        if(0 == ImSendBuffer[i].msgId_)
        {
            return i;
        }
    }

    return -1;
}


static int getElementFromCache(void)
{
    static int last     = SEND_BUFFER_COUNT - 1;
    uint32_t   currTime = k_uptime_get_32();
    for(int i = last + 1;; ++i)
    {
        if(i >= SEND_BUFFER_COUNT)
        {
            i = i % SEND_BUFFER_COUNT;
        }

        if(1 == gSendBuffer[i].msgId_)
        {
            last = i;
            return i;
        }
        else if(0 != gSendBuffer[i].msgId_)
        {
            if(currTime - gSendBuffer[i].sendTime_ > RE_UPLOAD_INTERVAL)
            {
                last = i;
                return i;
            }
        }
        //print_log("currtime:[%d] sendTime:[%d] index:[%d] last:[%d]\n",currTime,gSendBuffer[i].sendTime_, i,last);
        if(i == last)
        {
            break;
        }
    }

    return -1;
}

static int getElementFromCanCache(void)
{
    static int last     = CAN_BUFFER_COUNT - 1;
    uint32_t   currTime = k_uptime_get_32();
    for(int i = last + 1;; ++i)
    {
        if(i >= CAN_BUFFER_COUNT)
        {
            i = i % CAN_BUFFER_COUNT;
        }

        if(1 == gCanSendBuffer[i].msgId_)
        {
            last = i;
            return i;
        }
        else if(0 != gCanSendBuffer[i].msgId_)
        {
            if(currTime - gCanSendBuffer[i].sendTime_ > RE_UPLOAD_INTERVAL)
            {
                last = i;
                return i;
            }
        }
        //print_log("currtime:[%d] sendTime:[%d] index:[%d] last:[%d]\n",currTime,gCanSendBuffer[i].sendTime_, i,last);
        if(i == last)
        {
            break;
        }
    }

    return -1;
}

static int getElementFromImCache(void)
{
    static int imlast   = IM_BUFFER_CONUT - 1;
    uint32_t   currTime = k_uptime_get_32();
    for(int i = imlast + 1;; ++i)
    {
        if(i >= IM_BUFFER_CONUT)
        {
            i = i % IM_BUFFER_CONUT;
        }

        if(1 == ImSendBuffer[i].msgId_)
        {
            imlast = i;
            return i;
        }
        else if(0 != ImSendBuffer[i].msgId_)
        {
            if(currTime - ImSendBuffer[i].sendTime_ > RE_UPLOAD_INTERVAL)
            {
                imlast = i;
                return i;
            }
        }
        //print_log("currtime:[%d] sendTime:[%d] index:[%d] last:[%d]\n",currTime,gSendBuffer[i].sendTime_, i,last);
        if(i == imlast)
        {
            break;
        }
    }

    return -1;
}

void cleanAllCache(void)
{
    k_mutex_lock(&g_sendBuffer_mutex, K_FOREVER);
    for(int i = 0; i < SEND_BUFFER_COUNT; ++i)
    {
        gSendBuffer[i].msgId_ = 0;
        gSendBuffer[i].size_  = 0;
        if(gSendBuffer[i].data_)
        {
            k_free(gSendBuffer[i].data_);
            gSendBuffer[i].data_ = NULL;
        }
    }
    k_mutex_unlock(&g_sendBuffer_mutex);
}

int pushCache(uint32_t msgId, uint16_t size, uint8_t* pdata, uint16_t headSize, uint8_t* phead)
{
    int pos;
    int ret;
    //print_log("time:[%u]\n", k_uptime_get_32());

    if((size + headSize) < 12 || phead == NULL)
    {
        return PARA_ERR;
    }

    if(0 != k_mutex_lock(&g_sendBuffer_mutex, 1000))
    {
        warning_log("don't get sendBuffer_mutex.\n");
        return ERR_MUTEX;
    }

    pos = getSocketFromCache();
    if(-1 == pos)
    {
        ret = CACHE_FULL;
        goto END;
    }

    uint8_t* p = (uint8_t*)k_malloc(size + headSize);
    if(p == NULL)
    {
        ret = NO_MEM;
        warning_log("No Memary to Send message.\n");
        goto END;
    }
    //print_log("heap:[%p]\n", p);
    memcpy(p, phead, headSize);
    if(NULL != pdata)
    {
        memcpy(p + headSize, pdata, size);
    }
    k_sched_lock();
    gSendBuffer[pos].data_     = p;
    gSendBuffer[pos].size_     = size + headSize;
    gSendBuffer[pos].sendTime_ = 0;
    gSendBuffer[pos].msgId_    = msgId;
    k_sched_unlock();
    print_log("PUSH TO CACHE Index:[%d]\n", pos);

    ret = PUSH_OK;

END:
    k_mutex_unlock(&g_sendBuffer_mutex);
    //print_log("time:[%u]\n", k_uptime_get_32());
    return ret;
}

int pushCanCache(uint32_t msgId, uint16_t size, uint8_t* pdata, uint16_t headSize, uint8_t* phead)
{
    int pos;
    int ret;
    //print_log("time:[%u]\n", k_uptime_get_32());

    if((size + headSize) < 12 || phead == NULL)
    {
        err_log("Para err in push can cache.\n");
        return PARA_ERR;
    }

    if(0 != k_mutex_lock(&g_canSendBuffer_mutex, 1000))
    {
        warning_log("don't get canSendBuffer_mutex.\n");
        return ERR_MUTEX;
    }

    pos = getSocketFromCanCache();
    if(-1 == pos)
    {
        ret = CACHE_FULL;
        goto END;
    }

    uint8_t* p = (uint8_t*)k_malloc(size + headSize);
    if(p == NULL)
    {
        ret = NO_MEM;
        warning_log("No Memary to Send message.\n");
        goto END;
    }
    //print_log("heap:[%p]\n", p);
    memcpy(p, phead, headSize);
    if(NULL != pdata)
    {
        memcpy(p + headSize, pdata, size);
    }
    k_sched_lock();
    gCanSendBuffer[pos].data_     = p;
    gCanSendBuffer[pos].size_     = size + headSize;
    gCanSendBuffer[pos].sendTime_ = 0;
    gCanSendBuffer[pos].msgId_    = msgId;
    k_sched_unlock();
    print_log("PUSH TO CACHE Index:[%d]\n", pos);

    ret = PUSH_OK;

END:
    k_mutex_unlock(&g_canSendBuffer_mutex);
    //print_log("time:[%u]\n", k_uptime_get_32());
    return ret;
}


int pushImCache(uint32_t msgId, uint16_t size, uint8_t* pdata, uint16_t headSize, uint8_t* phead)
{
    int pos;
    int ret;
    //print_log("time:[%u]\n", k_uptime_get_32());

    if((size + headSize) < 12 || phead == NULL)
    {
        return PARA_ERR;
    }

    if(0 != k_mutex_lock(&g_ImBuffer_mutex, 1000))
    {
        warning_log("don't get ImBuffer_mutex.\n");
        return ERR_MUTEX;
    }

    pos = getSocketFromImCache();
    if(-1 == pos)
    {
        ret = CACHE_FULL;
        goto END;
    }

    uint8_t* p = (uint8_t*)k_malloc(size + headSize);
    if(p == NULL)
    {
        ret = NO_MEM;
        goto END;
    }
    //print_log("heap:[%p]\n", p);
    memcpy(p, phead, headSize);
    if(NULL != pdata)
    {
        memcpy(p + headSize, pdata, size);
    }
    k_sched_lock();
    ImSendBuffer[pos].data_     = p;
    ImSendBuffer[pos].size_     = size + headSize;
    ImSendBuffer[pos].sendTime_ = 0;
    ImSendBuffer[pos].msgId_    = msgId;
    k_sched_unlock();
    print_log("PUSH TO CACHE Index:[%d]\n", pos);

    ret = PUSH_OK;

END:
    k_mutex_unlock(&g_ImBuffer_mutex);
    //print_log("time:[%u]\n", k_uptime_get_32());
    return ret;
}

int insertToSendCache(uint32_t msgId, uint8_t* pdata, uint16_t size)
{
    int pos;
    int ret;

    if(size < 12 || pdata == NULL)
    {
        return PARA_ERR;
    }

    if(0 != k_mutex_lock(&g_sendBuffer_mutex, 1000))
    {
        warning_log("don't get sendBuffer_mutex.\n");
        return ERR_MUTEX;
    }

    pos = getSocketFromCache();
    if(-1 == pos)
    {
        ret = CACHE_FULL;
        goto END;
    }

    uint8_t* p = (uint8_t*)k_malloc(size);
    if(p == NULL)
    {
        ret = NO_MEM;
        warning_log("Don't get malloc memery.\n");
        goto END;
    }

    memcpy(p, pdata, size);
    k_sched_lock();
    gSendBuffer[pos].data_     = p;
    gSendBuffer[pos].size_     = size;
    gSendBuffer[pos].sendTime_ = 0;
    gSendBuffer[pos].msgId_    = msgId;
    k_sched_unlock();
    print_log("Inster In TO CACHE Index:[%d]\n", pos);

    ret = PUSH_OK;

END:
    k_mutex_unlock(&g_sendBuffer_mutex);
    return ret;
}

int insertToCanSendCache(uint32_t msgId, uint8_t* pdata, uint16_t size)
{
    int pos;
    int ret;

    if(size < 12 || pdata == NULL)
    {
        return PARA_ERR;
    }

    if(0 != k_mutex_lock(&g_canSendBuffer_mutex, 1000))
    {
        warning_log("don't get canSendBuffer_mutex.\n");
        return ERR_MUTEX;
    }

    pos = getSocketFromCanCache();
    if(-1 == pos)
    {
        ret = CACHE_FULL;
        goto END;
    }

    uint8_t* p = (uint8_t*)k_malloc(size);
    if(p == NULL)
    {
        ret = NO_MEM;
        warning_log("Don't get malloc memery.\n");
        goto END;
    }

    memcpy(p, pdata, size);
    k_sched_lock();
    gCanSendBuffer[pos].data_     = p;
    gCanSendBuffer[pos].size_     = size;
    gCanSendBuffer[pos].sendTime_ = 0;
    gCanSendBuffer[pos].msgId_    = msgId;
    k_sched_unlock();
    print_log("Insert In TO Can CACHE Index:[%d]\n", pos);

    ret = PUSH_OK;

END:
    k_mutex_unlock(&g_canSendBuffer_mutex);
    return ret;
}

bool popClearCache(uint8_t* pout, uint16_t size, int* len)
{
    int  ret = false;
    int  pos;
    bool loop = true;

    static bool onceflag = false;

    if(!pout || size < 12)
    {
        return false;
    }

    if(0 != k_mutex_lock(&g_sendBuffer_mutex, 1000))
    {
        warning_log("Don't get sendBuffer_mutex in popclearcache\n");
        return false;
    }

    do
    {
        pos = getElementFromCache();

        if(pos == -1)
        {
            ret = false;
            goto END;
        }

        if(gSendBuffer[pos].size_ > size)
        {
            ret = false;
            goto END;
        }

        if(gSendBuffer[pos].msgId_ == UPLOAD_ONCE_RFID)
        {
            if(onceflag)
            {
                ret = false;
                goto END;
            }
            else
            {
                onceflag = true;
                continue;
            }
        }

        k_sched_lock();
        if(gSendBuffer[pos].msgId_ >= 2)
        {
            memcpy(pout, gSendBuffer[pos].data_, gSendBuffer[pos].size_);
            *len += gSendBuffer[pos].size_;
            loop = false;
        }

        if(gSendBuffer[pos].data_)
        {
            k_free(gSendBuffer[pos].data_);
        }
        gSendBuffer[pos].data_     = NULL;
        gSendBuffer[pos].size_     = 0;
        gSendBuffer[pos].sendTime_ = 0;
        gSendBuffer[pos].msgId_    = 0;
        k_sched_unlock();
        print_log("Offline Thread POP  and free FROM CACHE Index:[%d]\n", pos);

        k_sleep(10);
    } while(loop);

    ret = true;

END:
    k_mutex_unlock(&g_sendBuffer_mutex);
    return ret;
}

bool popCahe(uint8_t* pout, uint16_t size, int* len)
{
    int ret = false;
    int pos;
    *len = 0;
    //print_log("p time:[%u]\n", k_uptime_get_32());
    if(!pout || size < 12)
    {
        return false;
    }

    if(0 != k_mutex_lock(&g_sendBuffer_mutex, 1000))
    {
        warning_log("Don't get sendBuffer_mutex.\n");
        return false;
    }

    pos = getElementFromCache();

    if(pos == -1)
    {
        ret = false;
        goto END;
    }

    if(gSendBuffer[pos].size_ > size)
    {
        ret = false;
        goto END;
    }
    k_sched_lock();
    memcpy(pout, gSendBuffer[pos].data_, gSendBuffer[pos].size_);
    *len += gSendBuffer[pos].size_;
#if 0
    if(gSendBuffer[pos].msgId_ <= 2)
    {
        if(gSendBuffer[pos].data_)
        {
            k_free(gSendBuffer[pos].data_);
        }
        gSendBuffer[pos].data_     = NULL;
        gSendBuffer[pos].size_     = 0;
        gSendBuffer[pos].sendTime_ = 0;
        gSendBuffer[pos].msgId_    = 0;
        print_log("POP  and free FROM CACHE Index:[%d]\n", pos);
    }
    else
    {
        gSendBuffer[pos].sendTime_ = k_uptime_get_32();
        print_log("POP FROM CACHE index:[%d]\n", pos);
    }
#else
    if(gSendBuffer[pos].data_)
    {
        k_free(gSendBuffer[pos].data_);
    }
    gSendBuffer[pos].data_     = NULL;
    gSendBuffer[pos].size_     = 0;
    gSendBuffer[pos].sendTime_ = 0;
    gSendBuffer[pos].msgId_    = 0;
    print_log("network thread POP and free FROM CACHE Index:[%d]\n", pos);
#endif
    k_sched_unlock();
    ret = true;

END:
    k_mutex_unlock(&g_sendBuffer_mutex);
    // print_log("p time:[%u]\n", k_uptime_get_32());
    return ret;
}

bool popCanCahe(uint8_t* pout, uint16_t size, int* len)
{
    int ret = false;
    int pos;
    *len = 0;
    //print_log("p time:[%u]\n", k_uptime_get_32());
    if(!pout || size < 12)
    {
        return false;
    }

    if(0 != k_mutex_lock(&g_canSendBuffer_mutex, 1000))
    {
        warning_log("Don't get canSendBuffer_mutex.\n");
        return false;
    }

    pos = getElementFromCanCache();

    if(pos == -1)
    {
        ret = false;
        goto END;
    }

    if(gCanSendBuffer[pos].size_ > size)
    {
        ret = false;
        goto END;
    }
    k_sched_lock();
    memcpy(pout, gCanSendBuffer[pos].data_, gCanSendBuffer[pos].size_);
    *len += gCanSendBuffer[pos].size_;

    if(gCanSendBuffer[pos].data_)
    {
        k_free(gCanSendBuffer[pos].data_);
    }
    gCanSendBuffer[pos].data_     = NULL;
    gCanSendBuffer[pos].size_     = 0;
    gCanSendBuffer[pos].sendTime_ = 0;
    gCanSendBuffer[pos].msgId_    = 0;
    print_log("network thread POP and free FROM CAN CACHE Index:[%d]\n", pos);

    k_sched_unlock();
    ret = true;

END:
    k_mutex_unlock(&g_canSendBuffer_mutex);
    // print_log("p time:[%u]\n", k_uptime_get_32());
    return ret;
}


bool popImCache(uint8_t* pout, uint16_t size, int* len)
{
    int ret = false;
    int pos;
    //print_log("p time:[%u]\n", k_uptime_get_32());
    if(!pout || size < 12)
    {
        return false;
    }

    if(0 != k_mutex_lock(&g_ImBuffer_mutex, 1000))
    {
        warning_log("Don't get ImBuffer_mutex.\n");
        return false;
    }

    pos = getElementFromImCache();

    if(pos == -1)
    {
        ret = false;
        goto END;
    }

    if(ImSendBuffer[pos].size_ > size)
    {
        ret = false;
        goto END;
    }
    k_sched_lock();
    memcpy(pout, ImSendBuffer[pos].data_, ImSendBuffer[pos].size_);
    *len += ImSendBuffer[pos].size_;

    if(ImSendBuffer[pos].data_)
    {
        k_free(ImSendBuffer[pos].data_);
    }
    ImSendBuffer[pos].data_     = NULL;
    ImSendBuffer[pos].size_     = 0;
    ImSendBuffer[pos].sendTime_ = 0;
    ImSendBuffer[pos].msgId_    = 0;
    print_log("POP  and free FROM IM CACHE Index:[%d]\n", pos);

    k_sched_unlock();
    ret = true;

END:
    k_mutex_unlock(&g_ImBuffer_mutex);
    //print_log("p time:[%u]\n", k_uptime_get_32());
    return ret;
}

void deleteElement(uint16_t msgId)
{
    if(0 != k_mutex_lock(&g_sendBuffer_mutex, 500))
    {
        warning_log("Don't get sendBuffer_mutex in delete.\n");
        return;
    }
    for(int i = 0; i < SEND_BUFFER_COUNT; ++i)
    {
        if(gSendBuffer[i].msgId_ == msgId)
        {
            if(gSendBuffer[i].data_)
            {
                k_free(gSendBuffer[i].data_);
                print_log("free [%p]\n", gSendBuffer[i].data_);
                gSendBuffer[i].data_ = NULL;
            }
            gSendBuffer[i].msgId_ = 0;
            gSendBuffer[i].size_  = 0;
            print_log("DELETE ELEMENT msgId[%d] index:[%d]\n", msgId, i);
            k_mutex_unlock(&g_sendBuffer_mutex);
            return;
        }
    }
    k_mutex_unlock(&g_sendBuffer_mutex);
    print_log("Don't find msgId:[%d]\n", msgId);
}

bool isFullCache(void)
{
    for(int i = 0; i < SEND_BUFFER_COUNT; ++i)
    {
        if(gSendBuffer[i].msgId_ == 0)
        {
            return false;
        }
    }

    return true;
}

bool isEmptyCache(void)
{
    for(int i = 0; i < SEND_BUFFER_COUNT; ++i)
    {
        if(gSendBuffer[i].msgId_ != 0)
        {
            return false;
        }
    }
    return true;
}

bool isHaveCache(void)
{
    uint8_t count = 0;
    for(int i = 0; i < SEND_BUFFER_COUNT; ++i)
    {
        if(gSendBuffer[i].msgId_ != 0)
        {
            count++;
        }
    }
    if(count >= 6)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool isEmptyIm(void)
{
    for(int i = 0; i < IM_BUFFER_CONUT; ++i)
    {
        if(ImSendBuffer[i].msgId_ != 0)
        {
            return false;
        }
    }
    return true;
}

bool isEmptyCanCache(void)
{
    for(int i = 0; i < CAN_BUFFER_COUNT; ++i)
    {
        if(gCanSendBuffer[i].msgId_ != 0)
        {
            return false;
        }
    }
    return true;
}


bool isAmpleCache(void)
{
    int counter = 0;
    for(int i = 0; i < SEND_BUFFER_COUNT; ++i)
    {
        if(gSendBuffer[i].msgId_ == 0)
        {
            counter++;
        }

        if(5 <= counter)
        {
            return true;
        }
    }
    return false;
}

uint8_t isMqttConnect(void)
{
    return mqtt_connect;
}

void setMmqttConnect(uint8_t connect)
{
    mqtt_connect = connect;
    updateLastPublishTm();
    return;
}

int checkBuff(uint8_t* buf, int len)
{
    int ret = 0;
    if(!buf || len < gTmpBuffer.index_)
    {
        err_log("Para ERROR.\n");
        return ret;
    }

    if(gTmpBuffer.index_ == 0)
    {
        return 0;
    }

    memcpy(buf, gTmpBuffer.data_, gTmpBuffer.index_);
    ret = gTmpBuffer.index_;
    memset(&gTmpBuffer, 0, sizeof(gTmpBuffer));
    return ret;
}

static bool reconnect(void)
{
    uint32_t curr_ms = k_uptime_get_32();
    uint32_t diff    = curr_ms - g_last_publish_ms;
    if(curr_ms > g_last_publish_ms && diff > g_publish_timeout_interval)
    {
        warning_log("Publish time OUT: reconnect..\n");
        return true;
    }
    else
    {
        return false;
    }
}

static void updateLastPublishTm(void)
{
    g_last_publish_ms = k_uptime_get_32();
}