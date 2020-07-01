#ifndef __BATTER_STAT_ALG_H__
#define __BATTER_STAT_ALG_H__
#include <kernel.h>
uint8_t getBatteryStat(uint32_t bat_vol);
uint8_t getStatefromVolt(uint32_t volt, uint32_t vmin, uint32_t vmax);
void getVotRange(uint32_t volt, uint32_t* vmin, uint32_t* vmax);
#endif