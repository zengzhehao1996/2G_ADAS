#ifndef __CACHE_OPT_H__
#define __CACHE_OPT_H__

#include <kernel.h>

bool pushToCacheFixed(uint8_t *filename, uint8_t *data, int len);
bool popFromCacheFixed(uint8_t *filename, uint8_t *data, int len);
bool pushToCacheUnfixed(uint8_t *filename, uint8_t *data, int len);
int  popFromCacheUnfixed(uint8_t *filename, uint8_t *data, int len);
int  getFileLastDate(const uint8_t *filename);
bool updateFileLastDate(uint8_t *filename, int day);
int  getFileSize(uint8_t *filename);
void deleteCacheFile(uint8_t *filename);
bool fileExist(uint8_t *filename);

#endif