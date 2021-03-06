#ifndef __GPS_H__
#define __GPS_H__
#include <kernel.h>

int startGpsThread(int* pInterval);
void stopGpsThread(void);
void setFatoryGpsInterval(bool factoryGps);
uint64_t getLongitude(void);
uint64_t getLatitude(void);
#endif /* __GPS_H__ */
