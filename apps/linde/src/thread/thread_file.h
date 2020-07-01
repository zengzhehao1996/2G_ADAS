#ifndef __THREAD_FILE_H__
#define __THREAN_FILE_H__

#include <zephyr.h>

bool startThreadFile();
void stopThreadFile();
void detect_thread_file(uint32_t ts);

#endif