#ifndef __FILE_H__
#define __FILE_H__

#include "my_file.h"
bool fileInit();
bool writeRfidList( void *pdata, int len);
int  readRfidList(void *pdata, int len);
bool writeRFIDListMd5(uint8_t *md5);
bool readRFIDListMd5(uint8_t *md5);
void deleteRfidFile(void);
void deleteConfigFile(void);
bool writePowerOffState(void *pdata, int len);
int  readPowerOffState(void *pdata, int len);
void deletePowerOffState(void);
bool writeSpeedLimitState(void *pdata, int len);
int  readSpeedLimitState(void *pdata, int len);
void deleteSpeedLimitState(void);
bool FileUnint();
#endif
