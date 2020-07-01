#ifndef __OFFLINE_THREAD_H__
#define __OFFLINE_THREAD_H__

bool offlineThreadIsRuning(void);
bool offlineThreadStart(void);
void offlineThreadSelfStop(void);
void offlineThreadSafetyStop(void);
void detect_thread_offline_thread(uint32_t ts);

#endif