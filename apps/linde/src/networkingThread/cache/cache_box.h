#ifndef __CACHE_BOX_H__
#define __CACHE_BOX_H__
#include <zephyr.h>

bool pushCanToCache(uint8_t *data, int len, uint8_t day, uint8_t week);
bool popCanFromCache(uint8_t *data, int len);
bool pushMessageToCache(uint8_t *data, int len, uint8_t day, uint8_t week);
int  popMessageFromCache(uint8_t *data, int len);
void deleteAllCache(void);

#endif