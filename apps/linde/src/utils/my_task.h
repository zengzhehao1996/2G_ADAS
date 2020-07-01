#ifndef MY_TASK_H
#define MY_TASK_H
#ifdef __cplusplus
extern "C" {
#endif

#include "my_misc.h"
#include <kernel.h>

typedef void (*task_setup_callback)(void);
typedef void (*task_timeout_callback)(void);
typedef struct my_task_s{
  s64_t last_trigger;//last trigger time
  int   interval;//second
  task_timeout_callback fptr_timeout;
  task_setup_callback fptr_setup;
}my_task_t;

BOOL task_init(my_task_t* task, int interval,task_setup_callback fptr_setup,task_timeout_callback fptr_timeout);
void task_setup(my_task_t * task);
void task_setInterval(my_task_t * task, int interval, s64_t ts);
void task_trigger(my_task_t * task, s64_t ts);

#ifdef __cplusplus
}
#endif
#endif
