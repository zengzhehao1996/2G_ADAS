#ifndef __MY_MISC_H__
#define __MY_MISC_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <misc/printk.h>
#include <stdint.h>

#define print_log(fmt,arg...) do{printk("[%s,%d] ", __FILE__, __LINE__);\
                                 printk(fmt,##arg);\
                              }while(0)
//#define print_log(fmt,arg...)

#define err_log(fmt,arg...) do{printk("[%s,%d] ", __FILE__, __LINE__);\
                               printk(fmt,##arg);\
                              }while(0)

typedef uint8_t  BOOL;
#define TRUE 1
#define FALSE 0


#ifdef __cplusplus
}
#endif

#endif

