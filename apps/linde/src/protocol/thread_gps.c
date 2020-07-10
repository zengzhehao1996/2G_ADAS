#include "thread_gps.h"
#include "gpd.h"
#include "my_misc.h"
#include "hw_gps_parser.h"
#include "msg_structure.h"
#include "server_interface.h"
#include <zephyr.h>
#include <string.h>
#include "factory_test.h"
#define THREAD_GPS_SIZE 1024
K_THREAD_STACK_DEFINE(g_thread_gps_stack, THREAD_GPS_SIZE);
static struct k_thread g_thread_gps;
static k_tid_t         g_thread_gps_id;
static bool factoryFlag = false;

static hwGpsMsg_t g_gps;
static uint64_t stb_lon = 0;
static uint64_t stb_lat = 0;

static void gpsCallBack(hwGpsMsg_t* gps)
{
    memcpy(&g_gps, gps, sizeof(g_gps));
    // print_log("set gps flag :[%d],src_gps.flag:[%d]\n", g_gps.flag, gps->flag);
}

bool isGetGps(void)
{
    return g_gps.flag;
}

#define GPS_READ_BUF_SIZE 512
static uint8_t read_buf[GPS_READ_BUF_SIZE];

static void threadGpsEntry(void* p1)
{
    uint32_t intervalRun    = (*(uint16_t*)p1); //p1 is s
    intervalRun *= 1000;                        //intervalRun is ms 
    uint32_t interval  = intervalRun;           // interval is ms
    int      len         = 0;
    bool     uploadFirst = true;
    print_log("GPS interval :[%u]\n",interval);

    gpsFIFO_t gpsFifo;

    uint32_t curr_time = 0;
    uint32_t last_time = 0;

    memset(&g_gps, 0, sizeof(g_gps));
    hwGpsParserInit((hwGPsParserReady_t)gpsCallBack);

    while(1)
    {
        curr_time = k_uptime_get_32();
        len       = gpd_read(read_buf, sizeof(read_buf));
        if(len > 0)
        {
            hwGpsPushBytes(read_buf, len);
            // print_log("read buf[%s]\n",read_buf);
        }
        else
        {
            k_sleep(1000);
        }
        if(factoryFlag)
        {
            interval = FACTORY_GPS_INTERVAL;
        }
        else
        {
            interval = intervalRun;
        }
        // print_log("gps flag:[%d] interval:[%d],triger interval:[%d] factoryFlag:[%d],\n",g_gps.flag, curr_time - last_time,interval,factoryFlag);
        if(true == g_gps.flag && (curr_time - last_time > interval || uploadFirst ))
        {
            uploadGPS_t tmp = { 0 };

            tmp.timestamp = getTimeStamp();
            tmp.latitude  = (uint64_t)(g_gps.lat * 1000000);
            tmp.longitude = (uint64_t)(g_gps.lon * 1000000);
            tmp.speed     = (uint32_t)(g_gps.speed * 1.852 * 1000);
            stb_lon = tmp.longitude;
            print_log("stb_lon is %ld\n", stb_lon);
            stb_lat = tmp.latitude;
            print_log("stb_lat is %ld\n", stb_lat);

            if(factoryFlag)
            {
                print_log("gps hdop = %d....................\n",tmp.hdop);
                tmp.hdop = 1000;
            }
            else
            {
                tmp.hdop      = (uint32_t)(g_gps.hdop * 1000);
            }
            
            serverSendGpsInfo(&tmp);
            memset(&g_gps, 0, sizeof(g_gps));
            print_log("Send gps to server hdop:[%d]\n", tmp.hdop);
            print_log("GPS get ##############################\n");
            
            last_time   = curr_time;
            uploadFirst = false;
        }

        k_sleep(50);
    }

    g_thread_gps_id = 0;
}

int startGpsThread(int* pInterval)
{
    if(g_thread_gps_id != 0)
    {
        warning_log("GPS thread already start!\n");
        return;
    }

    if(0 != gpd_init())
    {
        err_log("GPD init FAILED!\n");
        return -1;
    }

    g_thread_gps_id = k_thread_create(&g_thread_gps, g_thread_gps_stack, THREAD_GPS_SIZE,
                                      (k_thread_entry_t)threadGpsEntry, pInterval, NULL, NULL,
                                      K_PRIO_COOP(12), 0, 0);

    if(g_thread_gps_id)
    {
        print_log("Create GPS THREAD Id:[ %p ]; Stack:[ %p ]; Size:[ %d ]\n", g_thread_gps_id,
                  g_thread_gps_stack, THREAD_GPS_SIZE);
    }
    else
    {
        print_log("create gps thread faild!\n");
        return -1;
    }

    return 0;
}

void stopGpsThread(void)
{
    if(0 != g_thread_gps_id)
    {
        k_thread_abort(g_thread_gps_id);
    }
    g_thread_gps_id = 0;
}

void setFatoryGpsInterval(bool factoryGps)
{
    factoryFlag = factoryGps;
}
hwGpsMsg_t getDFTtailGps()
{
    return g_gps;
}

uint64_t getLongitude()
{
    uint64_t longi = 0;
    longi = stb_lon;
    //print_log("get longitude is %ld\n", longi);
    return longi;
}

uint64_t getLatitude()
{
    uint64_t lati = 0;
    lati = stb_lat;
    //print_log("get latitude is %ld\n", lati);
    return lati;
}
