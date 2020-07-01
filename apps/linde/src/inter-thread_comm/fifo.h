#ifndef __FIFO_H__
#define __FIFO_H__

#include <stdbool.h>

bool fifoInit();

bool canErrFifoSend(char *msgPtr);
bool canErrFifoRcv(char *msgPtr);

bool canWorkHourFifoSend(char *msgPtr);
bool canWorkHourFifoRcv(char *msgPtr);

bool rfid2CtrlFifoSend(char *msgPtr);
bool rfid2CtrlFifoRcv(char *msgPtr);

bool rfid2Sim868FifoSend(char *msgPtr);
bool rfid2Sim868FifoRcv(char *msgPtr);
//drvRcrdFifo_t
bool DrvRecordFifoSend(char *msgPtr);
bool DrvRecordFifoRcv(char *msgPtr);

bool canFifoSend(char *msgPtr);
bool canFifoRcv(char *msgPtr);

bool overspeedFifoSend(char *msgPtr);
bool overspeedFifoRcv(char *msgPtr);

bool gpsFifoSend(char *msgPtr);
bool gpsFifoRcv(char *msgPtr);

bool appendRFIDFifoSend(char *msgPtr);
bool appendRFIDFifoRcv(char *msgPtr);

bool carryDataFifoSend(char *msgPtr);
bool carryDataFifoRcv(char *msgPtr);

bool speedLimitFifoSend(char *msgPtr);
bool speedLimitFifoRcv(char *msgPtr);

bool factryTestV14FifoSend(char *msgPtr);
bool factryTestV14FifoRcv(char *msgPtr);

bool autoCanResultFifoSend(char *msgPtr);
bool autoCanResultFifoRcv(char *msgPtr);

bool unlockCauseFifoSend(char *msgPtr);
bool unlockCauseFifoRcv(char *msgPtr);

bool autoFotaFifoSend(char *msgPtr);
bool autoFotaFifoRcv(char *msgPtr);

#endif
