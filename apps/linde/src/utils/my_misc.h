#ifndef __MY_MISC_H__
#define __MY_MISC_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <misc/printk.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "smart_link_version.h"

typedef uint8_t  BOOL;
#define TRUE  1
#define FALSE 0
#define THREAD_LOOP_TIME_5SEC   (5*1000)
#define THREAD_LOOP_TIME_10SEC  (10*1000)
#define THREAD_LOOP_TIME_60SEC  (60*1000)
#define THREAD_LOOP_TIME_10MIN  (10*60*1000)
#define THREAD_LOOP_TIME_15MIN  (15*60*1000)
#define THREAD_LOOP_TIME_20MIN  (20*60*1000)

extern bool g_log_flag;

#define my_abs(a) ((a)<0?(-(a)):(a))

#undef LOG_ON
#undef DEBUG_THREAD_FILE // if debug thread file define it

//////////////////////////////////////////////////////////////////////
#if LOG_SWITCH
#define print_log(fmt, arg...) do{\
                                    char* p=strrchr(__FILE__, '/') + 1;\
                                    printf("[%s,%d] ",p, __LINE__);\
                                    printf(fmt, ##arg);\
                                }while(0)
#else
#define print_log(fmt, arg...)
#endif
///////////////////////////////////////////////////////////////////////

#define err_log(fmt,arg...) do{\
                                char* p=strrchr(__FILE__, '/') + 1;\
                                 printf("[%s,%d] ERROR:",p, __LINE__);\
                                 printf(fmt, ##arg);\
                            }while(0)

#define warning_log(fmt,arg...) do{\
                                    char* p=strrchr(__FILE__, '/') + 1;\
                                 printf("[%s,%d] WARNING:",p, __LINE__);\
                                 printf(fmt, ##arg);\
                                }while(0)

#define test_log(fmt,arg...) do{  printf(fmt,##arg);  }while(0);


#ifdef __cplusplus
}
#endif

#endif

