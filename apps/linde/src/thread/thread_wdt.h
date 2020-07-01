#ifndef THREAD_WDT_H
#define THREAD_WDT_H
#ifdef __cplusplus
extern "C" {
#endif

#include "my_misc.h"

#define THREAD_WDT_DEFAULT_TIMEOUT 6000

bool thread_wdt_start(int timeout);
void thread_wdt_stop();

#ifdef __cplusplus
}
#endif

#endif //THREAD_WDT_H