#ifndef __MY_TOOL_H__
#define __MY_TOOL_H__

#include <zephyr.h>

#define check_range(x,min,max) ((((x)<(min) ? (min):(x)) > (max)) ? (max) : ((x)<(min) ? (min):(x)))
#define check_min(x,min) ((x)<(min) ? (min) : (x))
#define check_max(x,min) ((x)>(max) ? (max) : (x))

uint8_t *memstr(const uint8_t *data, const uint8_t *find, int size);
double atod(const uint8_t *str);
void turnEndian(uint8_t *data, int len);
uint32_t toBigEndian(uint32_t val);
void bit32Set(uint32_t *pval, int pos);
void bit32Clear(uint32_t *pval, int pos);

#endif