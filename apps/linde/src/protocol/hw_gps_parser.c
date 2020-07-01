#include "my_misc.h"
#include "my_tool.h"
#include "rtc.h"
#include "rtc_timestamp.h"
#include "hw_gps_parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

hwGPsParserReady_t gGpsCallBack=NULL;
#define GPS_RX_MAX_SIZE 128
#define GPS_START_CODE 	'$'
#define GPS_END_CODE 	'\n'
#define GNGGA_HEAD 		"$GNGGA"
#define GNRMC_HEAD 		"$GNRMC"
#define SEPARATIVE_SIGN ','
#define LAT_PORT 		2
#define LON_PORT 		4
#define FS_PORT 		6
#define NOSV_PORT 		7
#define HDOP_PORT 		8
#define LAT_SIZE 		11
#define LON_SIZE 		12
#define SPEED_PORT 		7
#define DATE_PORT  		9
#define TIME_PORT 		1
#define GN_FLAG_PORT 	2

#undef debug
#define POSI_STAT 1

typedef struct 
{
    uint8_t buffer_[GPS_RX_MAX_SIZE];
    uint16_t index_;
    bool packStartFlag_;
}hwGpsRx_t;

static hwGpsMsg_t gGps;
static hwGpsRx_t  gReceive;

static void resetParserGps(void);
static void resetRxBuffer(void);
static bool isEndbyte(uint8_t byte);
static int  fixedPosition(uint8_t *buffer, int len, int num);
static bool isGNGGA(uint8_t *buf);
static bool isGNRMC(uint8_t *buf);
static void filltoLLNH(uint8_t *buf, int len);
static void hwGpsPushByte(uint8_t byte);


static void resetParserGps(void)
{
    memset(&gGps, 0, sizeof(gGps));
}

static bool isEndbyte(uint8_t byte)
{
    return GPS_END_CODE == byte;
}

static int  fixedPosition(uint8_t *buffer, int len, int num)
{
    int count = 0;
    for(int i = 0; i<len ; ++i)
    {
        if(buffer[i]==SEPARATIVE_SIGN)
        ++count;
        if( count==num )
        return ++i;
    }

    return -1;
}

static bool isGNGGA(uint8_t *buf)
{
    return 0==memcmp(buf,GNGGA_HEAD,strlen(GNGGA_HEAD));
}

static bool isGNRMC(uint8_t *buf)
{
    return 0==memcmp(buf,GNRMC_HEAD,strlen(GNRMC_HEAD));
}

static void filltoLLNH(uint8_t *buf, int len)
{
    int i;
    char flag = '0';
    gGps.flag = false;
    
    //print_log("\n%s\n", buf);

    #ifdef POSI_STAT
    //Positioning state 定位状态
    i = fixedPosition(buf, len, FS_PORT);
    if( i!=-1 ){
        if( buf[i]!=SEPARATIVE_SIGN ){
        flag = buf[i];
        }
    }

    if(flag - '0' == 0){
        return ;
    }
    #endif

    //fatch lat
    i = fixedPosition(buf, len, LAT_PORT);
    if( i!=-1 ){
        if( buf[i]!=SEPARATIVE_SIGN ){
        //memcpy(gps.lat, &buf[i], LAT_SIZE);
        gGps.lat = atod((uint8_t*)&buf[i]);
        }
    }

    //fatch lon
    i = fixedPosition(buf, len, LON_PORT);
    if( i!=-1 ){
        if( buf[i]!=SEPARATIVE_SIGN ){
        //memcpy(gps.lon, &buf[i], LON_SIZE);
        gGps.lon =  atod((char*)&buf[i]);
        }
    }

    //fatch hdop
    i = fixedPosition(buf, len, HDOP_PORT);
    if( i!=-1 ){
        if( buf[i]!=SEPARATIVE_SIGN ){
        gGps.hdop = atod((char *)&buf[i]);
        }
    }

    //fatch nosv
    i = fixedPosition(buf, len, NOSV_PORT);
    if( i!=-1 ){
        if( buf[i]!=SEPARATIVE_SIGN ){
        gGps.nosv = atoi((char *)&buf[i]);
        }
    }

    gGps.flag = true;

}

static void fetchSpeedAndTime(uint8_t* buf, int len)
{
    int i;
    char flag = 0;

    //gps.flag &= ~(0x01<<1);

    #ifdef POSI_STAT
    //Positioning state 定位状态
    i = fixedPosition(buf, len, GN_FLAG_PORT);
    if( i!=-1 ){
        if( buf[i]!=SEPARATIVE_SIGN ){
        flag = buf[i];
        }
    }
    if( flag != 'A' ){
        return ;
    }
    #endif

    // print_log("\n%s", buf);

    //fatch speed
    i = fixedPosition(buf, len, SPEED_PORT);
    if( i!=-1 ){
        if( buf[i]!=SEPARATIVE_SIGN ){
        gGps.speed = atod((char *)&buf[i]);
        }
    }
    #if 1
    bool date_flag=false;
    //fatch year month day
    i = fixedPosition(buf, len, DATE_PORT);
    if( i!=-1 ){
        if( buf[i]!=SEPARATIVE_SIGN ){
            gGps.day = 10*(buf[i]-'0') + (buf[i+1]-'0');
            gGps.month = 10*(buf[i+2]-'0') + (buf[i+3]-'0');
            gGps.year = 10*(buf[i+4]-'0') + (buf[i+5]-'0');
            date_flag = true;
        }
    }

    //fatch hour min sec
    i = fixedPosition(buf, len, TIME_PORT);
    if( i!=-1 ){
        if( buf[i]!=SEPARATIVE_SIGN ){
            gGps.hour = 10*(buf[i]-'0') + (buf[i+1]-'0') + 8;    //beijing time
            gGps.min = 10*(buf[i+2]-'0') + (buf[i+3]-'0');
            gGps.sec = atod((char *)&buf[i+4]);
            if(date_flag){
                gGps.rtc_flag = true;
            }
        }
    }
    if(!timeIsAlreadySet() && gGps.rtc_flag)
    {
        localRTC_t rtc;
        uint32_t ts;

        ts = RTC2TimeStamp(2000+gGps.year, gGps.month, gGps.day, gGps.hour, gGps.min, (uint8_t)gGps.sec);
        setTimeStamp(ts);
        timeStamp2RTC(ts,&rtc);
        setRTC(&rtc);
        print_log("Set gps timestamp and rtc\n");
    }

    // print_log("20%0d-%02d-%02d %02d:%02d:%02d\n",gGps.year,gGps.month,gGps.day,gGps.hour,gGps.min,(int)gGps.sec);
    #endif
}

static void fetchGgps(uint8_t* buf,int len)
{
    if(isGNGGA(buf)){
        //fecth HDOP lat lon NoSV
        filltoLLNH(buf, len);
        //call back gps set
        if(gGps.flag)
        {

            #ifdef debug
                print_log("\n\tdate:%d-%d-%d %d:%d:%d\n",
                        gGps.year, gGps.month, gGps.day, gGps.hour, gGps.min, (int)gGps.sec);
                char buf[128];
                sprintf(buf,"\nHWGPS:\tLAT:[%f]  LON:[%f]  HDOP:[%f] NOSV:[%d]  SPEED:[%f] STAN:[%d]\n"
                            ,gGps.lat,gGps.lon,gGps.hdop,gGps.nosv,gGps.speed,gGps.stan);
                print_log("%s\n",buf);
            #endif
        
            if(gGpsCallBack)
            {
                gGpsCallBack(&gGps);
                // print_log("call back gps,gps.flag :[%d]--------------------------------\n",gGps.flag);
            }
        }

    } else if(isGNRMC(buf)){
        //fecth speed
        fetchSpeedAndTime(buf,len);
    }
}

static void resetRxBuffer(void)
{
    memset(&gReceive, 0, sizeof(gReceive));
}

static void hwGpsPushByte(uint8_t byte)
{
  if(GPS_START_CODE == byte)
  {
    resetRxBuffer();
    gReceive.packStartFlag_ = true;
  }

  //if flag is not set return
  if(!gReceive.packStartFlag_) 
  {
      return;
  }

  //save the data
  if(gReceive.index_ < GPS_RX_MAX_SIZE)
  {
    gReceive.buffer_[gReceive.index_] = byte;
    gReceive.index_++;
  }

  // Exceeded buffer size then Empty buf and return
  if(gReceive.index_ >= GPS_RX_MAX_SIZE && !isEndbyte(byte))
  {
    resetRxBuffer();
	return;
  }

  if (isEndbyte(byte)) 
  {
    fetchGgps((uint8_t*)gReceive.buffer_,strlen((char *)gReceive.buffer_));
    resetRxBuffer();
  }
}

void hwGpsParserInit(hwGPsParserReady_t fptr)
{
    /* step1. reset paser box*/
    resetParserGps();

    /*step2. set callback function*/
    gGpsCallBack = fptr;
}

void hwGpsPushBytes(uint8_t* data,int len)
{
    if(data == NULL || len <= 0)
    {
        return ;
    }

    for(int i=0;i<len;++i)
    {
        hwGpsPushByte(data[i]);
    }
}