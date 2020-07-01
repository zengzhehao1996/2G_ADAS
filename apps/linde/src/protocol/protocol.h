#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include <kernel.h>

typedef void (*cpParserReady_t)(uint16_t cmdid,uint16_t version,uint8_t *data,uint32_t len);

bool cpParserInit(cpParserReady_t fptr);
void cpParserReset(void);
uint32_t cpHeadSize(void);
void cpParserPushBytes(uint8_t *data, int len);
int  cpPaserFillHead(uint16_t cmdid,uint16_t version, uint16_t sendLen, uint8_t *data, int size);
bool isRecvData(void);
void setRecvData(bool val);

#endif