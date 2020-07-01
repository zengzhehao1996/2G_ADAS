#ifndef __NETWORK_THREAD_H__
#define __NETWORK_THREAD_H__

typedef struct
{
    uint16_t hb_timeout;
    uint8_t  auto_fota_switch;
}networkPara_t;

enum {
    AUTO_FOTA_OFF = 0,
    AUTO_FOTA_ON = 1,
};


int networkThreadStart(networkPara_t pra);
void safeNetworkTreadStop(void);
void detect_thread_nerwork_thread(uint32_t ts);

enum returnVal
{
    PUSH_OK    = 0,
    PARA_ERR   = -1,
    CACHE_FULL = -2,
    NO_MEM     = -3,
    ERR_MUTEX  = -4
};

void initCache(void);
int pushCache(uint32_t msgId, uint16_t size, uint8_t* pdata, uint16_t headSize, uint8_t* phead);
int pushImCache(uint32_t msgId, uint16_t size, uint8_t* pdata, uint16_t headSize, uint8_t* phead);
int pushCanCache(uint32_t msgId, uint16_t size, uint8_t* pdata, uint16_t headSize, uint8_t* phead);

int insertToSendCache(uint32_t msgId, uint8_t* pdata, uint16_t size);
int insertToCanSendCache(uint32_t msgId, uint8_t* pdata, uint16_t size);
bool popCahe(uint8_t* pout, uint16_t size, int* len);
bool popImCache(uint8_t* pout, uint16_t size, int* len);
bool popClearCache(uint8_t* pout, uint16_t size, int* len);
bool popCanCahe(uint8_t* pout, uint16_t size, int* len);
void deleteElement(uint16_t cmdId);
bool    isFullCache(void);
bool    isEmptyCache(void);
bool    isHaveCache(void);
bool    isEmptyIm(void);
bool    isEmptyCanCache(void);
bool    isAmpleCache(void);
uint8_t isMqttConnect(void);
void setMmqttConnect(uint8_t connect);
#endif