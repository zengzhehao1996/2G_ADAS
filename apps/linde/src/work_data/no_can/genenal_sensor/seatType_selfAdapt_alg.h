#ifndef __SEATTYPE_SELFADAPT_ALG_H__
#define __SEATTYPE_SELFADAPT_ALG_H__
#include <kernel.h>
typedef struct {
  int8_t moveStat;
  int8_t moveLevel;
  int8_t seatAdapStat;//use for self-adapting seat type  
}seatAdapt_t;
void adaptSeatType(seatAdapt_t value);
#endif