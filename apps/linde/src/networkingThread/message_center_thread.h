#ifndef __MESSAGE_CENTER_THREAD_H__
#define __MESSAGE_CENTER_THREAD_H__

typedef struct
{
    uint16_t gGpsIntrval;
    uint16_t gHbTimeOut;
    uint16_t gHbInterval;
    uint16_t gCarStateInterval;
    uint16_t gCarStateOfflineInterval;
    uint8_t  authType;
}messagePara_t;
bool messageCenterTHreadStart(messagePara_t para);
void messageCenterTHreadSafetyStop(void);
uint32_t getCurrCardId(void);

bool carIsLock(void);

#endif