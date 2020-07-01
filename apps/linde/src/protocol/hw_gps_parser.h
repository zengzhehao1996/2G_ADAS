#ifndef __HW_GPS_PARSER_H__
#define __HW_GPS_PARSER_H__

#include <stdint.h>
#include "my_misc.h"

typedef struct
{
  double lat;
  double lon;
  double hdop;     //horizontal dilution of precision
  float speed;
  unsigned int nosv;
  unsigned int ts;
  char stan;    //star count
  char flag;
  char rtc_flag;
  unsigned char year;
  unsigned char month;
  unsigned char day;
  unsigned char hour;
  unsigned char min;
  double sec;
}hwGpsMsg_t;

typedef void (*hwGPsParserReady_t)(hwGpsMsg_t *gps);
void hwGpsParserInit(hwGPsParserReady_t fptr);
void hwGpsPushBytes(uint8_t* data,int len);

#endif