#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <zephyr.h>
#include "atd.h"
#include <device.h>

#include "qcloud_iot_export.h"
#include "qcloud_iot_import.h"

#include "atc.h"

#include "aidong_custom.h"


#include "my_misc.h"
#include "protocol.h"

const char publish_topic_report[] = "device/report/";
const char publish_topic_im[] = "device/im/";
const char publish_topic_can[] = "device/can/";

config_t   config;
//fota_ftp_t fotaurl;

realtimedata_t  realtimedata;
uint32_t        QCLOUD_IOT_MQTT_COMMAND_TIMEOUT;
//static uint8_t  configUpdate;
//static uint8_t* fota_buf     = NULL;
//static uint32_t fota_buflen  = 0;
//static uint8_t  fota_ftpdone = 0;





struct subscribetopic_desc
{
    enum subscribetopic_name idx;
    char                     topic[64];
    OnMessageHandler         handler;
};


static void dumpSubscribeMsg(struct configmsg_t* pConfigMsg)
{
#if DUMPSUBSCRIBEMSG
    printk("deviceid: %s\n", pConfigMsg->deviceId);
    printk("LocationFrequency.value: [");
    for(int i = 0; i < pConfigMsg->locationFrequency.value_len; i++)
    {
        if(i != (pConfigMsg->locationFrequency.value_len - 1))
            printk("%d,", pConfigMsg->locationFrequency.value[i]);
        else
        {
            printk("%d]\n", pConfigMsg->locationFrequency.value[i]);
        }
    }
    printk("LocationFrequency.isUpdate: %d\n", pConfigMsg->locationFrequency.isUpdate);
    printk("host.ip: %s\n", pConfigMsg->host.ip);
    printk("host.ip: %d\n", pConfigMsg->host.port);
    printk("host.isUpdate: %d\n", pConfigMsg->host.isUpdate);
    printk("fota.updateVersion: %s\n", pConfigMsg->fota.updateVersion);
    printk("fota.resUrl: %s\n", pConfigMsg->fota.resUrl);
    printk("fota.isUpdate: %d\n", pConfigMsg->fota.isUpdate);
    printk("updateTimestamp: %s\n", pConfigMsg->updateTimestamp);
#endif
}

static char* _findnextstr(char* ptr)
{
    while(*ptr != '\0')
        ptr++;
    ptr++;

    return ptr;
}


void readConfig(void)
{
    memset(&config, 0, sizeof(config));
    strcpy(config.username,DEFAULT_USERNAME);
    strcpy(config.password,DEFAULT_PASSWORD);
    strcpy(config.host,DEFAULT_HOST);
    config.port = DEFAULT_PORT;
    QCLOUD_IOT_MQTT_COMMAND_TIMEOUT = DEFAULT_MQTT_COMMAND_TIMEOUT;
}


/**
 * MQTT消息接收event消息处理函数
 *
 * @param topicName         topic主题
 * @param topicNameLen      topic长度
 * @param message           已订阅消息的结构
 * @param userData         消息负载
 */
static void on_message_event_callback(void* pClient, MQTTMessage* message, void* userData)
{
    if(message == NULL)
    {
        return;
    }
    print_log("Receive Message topic length: %d   packet-id:%d\n", (int)message->topic_len,message->id);
    // print_log("Receive Message topic length: %d\n", (int)message->topic_len);
    // print_log("Receive Message topicName   : %s\n", message->ptopic);
    // ((char*)message->payload)[message->payload_len] = '\0';
    /************88
    print_log("Message payload:\n\t");
    for(int i=0;i<message->payload_len;++i ){
        printk("0x%02x ",((char*)message->payload)[i]);
    }
    printk("\n");
    *************/
    cpParserPushBytes(message->payload,message->payload_len);

    //print_log("Receive Message topiccontent : %s,len=%d\n", message->payload,message->payload_len);
}


struct subscribetopic_desc subscribe_table[] = {
    { SERVER_EVENT, "server/event/", on_message_event_callback }
    //{ SUB_CONFIG, "config/base/", on_message_config_callback },
    //{ RFID_LIST, "config/rfid/", on_message_rfid_callback },
};

/**
 * 订阅关注topic和注册相应回调处理
 *
 */
static int _register_subscribe_topics(void* client, uint32_t topictype)
{
    SubscribeParams sub_params    = DEFAULT_SUB_PARAMS;
    sub_params.on_message_handler = subscribe_table[topictype].handler;
    sub_params.qos                = QOS1;
    //Log_i("Subscribe topic=%s", subscribe_table[topictype].topic);
    //printk("Subscribe topic=%s\n", subscribe_table[topictype].topic);
    return IOT_MQTT_Subscribe(client, subscribe_table[topictype].topic, &sub_params);
}

int subscribe_topic(void* client, uint32_t topictype, uint32_t* packetid)
{
    int rc;

    *packetid = -1;
    
    rc = _register_subscribe_topics(client, topictype);
    
    if(rc < 0)
    {
        print_log("Client Subscribe Topic Failed:........... %d\n", rc);
        Log_e("Client Subscribe Topic Failed1111111111: %d", rc);
        return rc;
    }
    rc = IOT_MQTT_Yield(client, DEFAULT_MQTT_COMMAND_TIMEOUT);
    //rc = IOT_MQTT_Yield(client, 10000);
    if((*packetid) <= 0)
    {
        Log_e("Client Subscribe Topic Failed: %d", rc);
        print_log("Client Subscribe Topic Failed2222222: %d\n", rc);
        return rc;
    }

    return 0;
}

/**
 * 发送topic消息
 *
 */
int pubish_message(void* client, uint8_t* custom_msg)
{
    char* topicName;
    char* topic_content;
    int   rc;

    topicName = k_malloc(64);
    if(topicName == NULL)
    {
        printk("Not enough memory to publish message.\n");
    }
    topic_content = k_malloc(MAX_SIZE_OF_TOPIC_CONTENT);
    if(topic_content == NULL)
    {
        printk("Not enough memory to publish message.\n");
    }
    HAL_Snprintf(topicName, 63, "%s%s", publish_topic_report, DEVICEID);

    PublishParams pub_params = DEFAULT_PUB_PARAMS;
    pub_params.qos           = QOS1;
    pub_params.id            = get_next_packet_id(client);

    uint32_t size = 0;

    if(size < 0 || size > MAX_SIZE_OF_TOPIC_CONTENT - 1)
    {
        Log_e("payload content length not enough! content size:%d  buf size:%d", size,
              (int)MAX_SIZE_OF_TOPIC_CONTENT);
        k_free(topicName);
        k_free(topic_content);
        return -3;
    }
    strcpy(topic_content, custom_msg);
    pub_params.payload     = topic_content;
    pub_params.payload_len = strlen(topic_content);
    /***
    printk("pub topic:%s\n", topicName);
    printk("pub content:%s\n", topic_content);
    ****/
    rc = IOT_MQTT_Publish(client, topicName, &pub_params);
    k_free(topicName);
    k_free(topic_content);

    if(rc < 0)
    {
        Log_e("client publish topic failed :%d.", rc);
    }

    rc = IOT_MQTT_Yield(client, DEFAULT_MQTT_COMMAND_TIMEOUT);

    return rc;
}

/*
 * Send data
*/
int pubish_data(void* client, uint8_t* data, int len)
{
    char* topicName;
    char* topic_content;
    int   rc;

    if(!client || !data || len <= 0 || len > MAX_SIZE_OF_TOPIC_CONTENT)
    {
        warning_log("Para ERROR. client:%p,data:%p,len=[%d]\n", client,data,len);
        return -1;
    }

    topicName = k_malloc(64);
    if(topicName == NULL)
    {
        err_log("Not enough memory to publish message.\n");
    }
    topic_content = k_malloc(MAX_SIZE_OF_TOPIC_CONTENT);
    if(topic_content == NULL)
    {
        err_log("Not enough memory to publish message.\n");
    }
    HAL_Snprintf(topicName, 63, "%s%s", publish_topic_report, DEVICEID);

    PublishParams pub_params = DEFAULT_PUB_PARAMS;
    pub_params.qos           = QOS1;
    pub_params.id            = get_next_packet_id(client);

    uint32_t size = 0;

    if(size < 0 || size > MAX_SIZE_OF_TOPIC_CONTENT - 1)
    {
        warning_log("payload content length not enough! content size:%d  buf size:%d", size,
              (int)MAX_SIZE_OF_TOPIC_CONTENT);
        k_free(topicName);
        k_free(topic_content);
        return -3;
    }
    memcpy(topic_content, data, len);
    pub_params.payload     = topic_content;
    pub_params.payload_len = len;
    print_log("pubish topic:[%s]\n", topicName);
    print_log("pubish content:[\n", topic_content);
    /****
    for(int i=0;i<len;++i){
        printk("0x%02x ",topic_content[i]);
    }
    printk("]len=%d\n",len);
    ****/
    rc = IOT_MQTT_Publish(client, topicName, &pub_params);
    k_free(topicName);
    k_free(topic_content);

    if(rc < 0)
    {
        err_log("client publish topic failed :%d.", rc);
    }

    rc = IOT_MQTT_Yield(client, DEFAULT_MQTT_COMMAND_TIMEOUT);

    return rc;
}

/*
 * Send data
*/
int pubish_data_id(void* client, uint8_t* data, int len, uint16_t *id)
{
    char* topicName;
    char* topic_content;
    int   rc;

    if(!client || !data || len <= 0 || len > MAX_SIZE_OF_TOPIC_CONTENT)
    {
        warning_log("Para ERROR. client:%p,data:%p,len=[%d]\n", client,data,len);
        return -1;
    }

    topicName = k_malloc(64);
    if(topicName == NULL)
    {
        err_log("Not enough memory to publish message.\n");
    }
    topic_content = k_malloc(MAX_SIZE_OF_TOPIC_CONTENT);
    if(topic_content == NULL)
    {
        err_log("Not enough memory to publish message.\n");
    }
    HAL_Snprintf(topicName, 63, "%s%s", publish_topic_report, DEVICEID);

    PublishParams pub_params = DEFAULT_PUB_PARAMS;
    pub_params.qos           = QOS1;
    if(0==*id)
    {
        pub_params.id = get_next_packet_id(client);
        *id = pub_params.id;
    }
    else
    {
        pub_params.id = *id;
    }

    uint32_t size = 0;

    if(size < 0 || size > MAX_SIZE_OF_TOPIC_CONTENT - 1)
    {
        warning_log("payload content length not enough! content size:%d  buf size:%d", size,
              (int)MAX_SIZE_OF_TOPIC_CONTENT);
        k_free(topicName);
        k_free(topic_content);
        return -3;
    }
    memcpy(topic_content, data, len);
    pub_params.payload     = topic_content;
    pub_params.payload_len = len;
    print_log("pubish topic:[%s]\n", topicName);
    print_log("pubish content: len = %d\n[",len);
    /***
    for(int i=0;i<len;++i){
        printk("0x%02x ",topic_content[i]);
    }
    printk("]\n");
    ***/
    rc = IOT_MQTT_Publish(client, topicName, &pub_params);
    k_free(topicName);
    k_free(topic_content);

    if(rc < 0)
    {
        err_log("client publish topic failed :%d\n", rc);
        return rc;
    }

    rc = IOT_MQTT_Yield(client, DEFAULT_MQTT_COMMAND_TIMEOUT);

    return rc;
}



int pubish_imdata(void* client, uint8_t* data, int len)
{
    char* topicName;
    char* topic_content;
    int   rc;

    if(!client || !data || len <= 0 || len > MAX_SIZE_OF_TOPIC_CONTENT)
    {
        warning_log("Para ERROR. client:%p,data:%p,len=[%d]\n", client,data,len);
        return -1;
    }

    topicName = k_malloc(64);
    if(topicName == NULL)
    {
        err_log("Not enough memory to publish message.\n");
    }
    topic_content = k_malloc(MAX_SIZE_OF_TOPIC_CONTENT);
    if(topic_content == NULL)
    {
        err_log("Not enough memory to publish message.\n");
    }
    HAL_Snprintf(topicName, 63, "%s%s", publish_topic_im, DEVICEID);

    PublishParams pub_params = DEFAULT_PUB_PARAMS;
    pub_params.qos           = QOS1;
    pub_params.id            = get_next_packet_id(client);

    uint32_t size = 0;

    if(size < 0 || size > MAX_SIZE_OF_TOPIC_CONTENT - 1)
    {
        warning_log("payload content length not enough! content size:%d  buf size:%d", size,
              (int)MAX_SIZE_OF_TOPIC_CONTENT);
        k_free(topicName);
        k_free(topic_content);
        return -3;
    }
    memcpy(topic_content, data, len);
    pub_params.payload     = topic_content;
    pub_params.payload_len = len;
    print_log("pubish topic:[%s]\n", topicName);
    print_log("pubish content: len = %d \n[", len);
    /**
    for(int i=0;i<len;++i){
        printk("0x%02x ",topic_content[i]);
    }
    printk("]\n",len);
    **/
    rc = IOT_MQTT_Publish(client, topicName, &pub_params);
    k_free(topicName);
    k_free(topic_content);

    if(rc < 0)
    {
        err_log("client publish topic failed :%d.", rc);
    }

    rc = IOT_MQTT_Yield(client, DEFAULT_MQTT_COMMAND_TIMEOUT);

    return rc;
}

int pubish_candata_id(void* client, uint8_t* data, int len, uint16_t *id)
{
    char* topicName;
    char* topic_content;
    int   rc;

    if(!client || !data || len <= 0 || len > MAX_SIZE_OF_TOPIC_CONTENT)
    {
        warning_log("Para ERROR. client:%p,data:%p,len=[%d]\n", client,data,len);
        return -1;
    }

    topicName = k_malloc(64);
    if(topicName == NULL)
    {
        err_log("Not enough memory to publish message.\n");
    }
    topic_content = k_malloc(MAX_SIZE_OF_TOPIC_CONTENT);
    if(topic_content == NULL)
    {
        err_log("Not enough memory to publish message.\n");
    }
    HAL_Snprintf(topicName, 63, "%s%s", publish_topic_can, DEVICEID);

    PublishParams pub_params = DEFAULT_PUB_PARAMS;
    pub_params.qos           = QOS1;
    if(0==*id)
    {
        pub_params.id = get_next_packet_id(client);
        *id = pub_params.id;
    }
    else
    {
        pub_params.id = *id;
    }

    uint32_t size = 0;

    if(size < 0 || size > MAX_SIZE_OF_TOPIC_CONTENT - 1)
    {
        warning_log("payload content length not enough! content size:%d  buf size:%d", size,
              (int)MAX_SIZE_OF_TOPIC_CONTENT);
        k_free(topicName);
        k_free(topic_content);
        return -3;
    }
    memcpy(topic_content, data, len);
    pub_params.payload     = topic_content;
    pub_params.payload_len = len;
    print_log("pubish topic:[%s]\n", topicName);
    print_log("pubish content: len = %d\n[",len);
    /***
    for(int i=0;i<len;++i){
        printk("0x%02x ",topic_content[i]);
    }
    printk("]\n");
    ***/
    rc = IOT_MQTT_Publish(client, topicName, &pub_params);
    k_free(topicName);
    k_free(topic_content);

    if(rc < 0)
    {
        err_log("client publish topic failed :%d\n", rc);
        return rc;
    }

    rc = IOT_MQTT_Yield(client, DEFAULT_MQTT_COMMAND_TIMEOUT);

    return rc;
}



void custom_init(void)
{
    HAL_Snprintf(subscribe_table[SERVER_EVENT].topic, 100, "%s%s",
                 "server/event/", DEVICEID);  // sovle multiple deviceid in inint topic error 
}

void parse_rmc(char* sentence)
{
    char*    ptr;
    uint32_t len;
    char*    pTime;

    realtimedata.rmc.valid = 0;
    len                    = strlen(sentence);
    for(uint32_t i = 0; i < len; i++)
    {
        if(sentence[i] == ',')
            sentence[i] = '\0';
    }
    ptr   = sentence;           //$GNRMC
    ptr   = _findnextstr(ptr);  //UTC
    pTime = ptr;
    ptr   = _findnextstr(ptr);  //Valid
    if(*ptr == 'A')
    {
        realtimedata.rmc.valid = 1;
    }
    ptr = _findnextstr(ptr);  //Latitude
    strncpy(realtimedata.rmc.latitude, ptr, sizeof(realtimedata.rmc.latitude) - 1);
    ptr = _findnextstr(ptr);  //N/S Indicator
    ptr = _findnextstr(ptr);  //Longitude
    strncpy(realtimedata.rmc.longitude, ptr, sizeof(realtimedata.rmc.longitude) - 1);

    ptr = _findnextstr(ptr);  //E/W Indicator
    ptr = _findnextstr(ptr);  //Speed Over Ground
    ptr = _findnextstr(ptr);  //Course Over Ground
    ptr = _findnextstr(ptr);  //Date
    snprintf(realtimedata.rmc.timestamp, sizeof(realtimedata.rmc.timestamp), "%s%s", ptr, pTime);
    if(strlen(realtimedata.rmc.timestamp) > 12)
    {
        realtimedata.rmc.timestamp[12] = '\0';
    }
    if(strlen(realtimedata.rmc.timestamp) == 0)
    {
        strcpy(realtimedata.rmc.timestamp, "000000000000");
    }
    Log_i("Latitude: %s, Longitude: %s, Timestamp: %s", realtimedata.rmc.latitude,
          realtimedata.rmc.longitude, realtimedata.rmc.timestamp);

    return;
}

void mprint(const char* file, const char* func, const int line, const char* fmt, ...)
{
    char  ch = '/';
    char* q  = strrchr(file, ch) + 1;

    HAL_Printf("INF|%s|%s|%s(%d): ", HAL_Timer_current(), q, func, line);

    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);

    HAL_Printf("\r\n");

    return;
}
