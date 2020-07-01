#ifndef __AIDONG_CUSTOM_H__
#define __AIDONG_CUSTOM_H__
#include "config.h"

#define DEFAULT_USERNAME                ""
#define DEFAULT_PASSWORD                ""

//#define DEFAULT_HOST                    "123.206.181.122"   //test server
//#define DEFAULT_PORT                    1883               //test server port

#define DEFAULT_HOST                    gSysconfig.serverAddr
#define DEFAULT_PORT                    gSysconfig.serverPort

//#define DEFAULT_LOCATIONFREQUENCY       10
#define DEFAULT_MQTT_COMMAND_TIMEOUT    100
#define DEFAULT_MORETIME_FORSUBMSG      6000

//#define MAX_LOCATIONFREQUENCY_NUM       7
//#define FOTA_BUF_SIZE                   ATC_RESPBUFSIZE

#define USERNAME_LEN                    12
#define PASSWORD_LEN                    12
#define SERVERNAME_LEN                  32
//#define FOTAFILENAME_LEN                12


typedef struct _config_t{
    char host[SERVERNAME_LEN];
    uint32_t port;
   // uint32_t locationFrequency[MAX_LOCATIONFREQUENCY_NUM];
    char username[USERNAME_LEN];
    char password[PASSWORD_LEN];
#if 0
    uint32_t mqtt_command_timeout;
    uint32_t mqtt_moretime_forsubmsg;
#endif
    uint8_t reserved[2];
    uint16_t checksum;
} config_t;

/********
typedef struct _fota_ftp_t {
    char username[USERNAME_LEN];
    char password[PASSWORD_LEN];
    char host[SERVERNAME_LEN];
    char filename[FOTAFILENAME_LEN];
} fota_ftp_t;
****/


typedef struct _rmc_t {
    char timestamp[24];
    char latitude[16];
    char longitude[16];
    uint32_t  valid;
} rmc_t;

typedef struct _realtimedata_t {
    rmc_t rmc;
    uint32_t   battery;
} realtimedata_t;

#define DEVICEID    atc_gsn
#define SIMCARDID   atc_ccid

#define MAX_SUBSCRIBE_NUM 1 
#define MAX_SIZE_OF_TOPIC_CONTENT 250

#define sleep k_sleep


enum subscribetopic_name {
    SERVER_EVENT =0,
    
};

void custom_init(void);
int subscribe_topic(void *client, uint32_t topictype, uint32_t *packetid);
int pubish_message(void *client, uint8_t *custom_msg);
int pubish_data_id(void* client, uint8_t* data, int len, uint16_t *id);
int pubish_candata_id(void* client, uint8_t* data, int len, uint16_t *id);
int pubish_data(void *client, uint8_t *data, int len);
int pubish_imdata(void* client, uint8_t* data, int len);
void parse_rmc(char *sentence);

extern config_t config;

extern unsigned int QCLOUD_IOT_MQTT_COMMAND_TIMEOUT;

#endif

