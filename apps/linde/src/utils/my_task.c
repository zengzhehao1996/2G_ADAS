#ifdef __cplusplus
extern "C" {
#endif

#include "my_task.h"

BOOL task_init(my_task_t* task, int interval,task_setup_callback fptr_setup,task_timeout_callback fptr_timeout)
{
  if(task==NULL || interval<=0)return FALSE;
  task->fptr_timeout = fptr_timeout;
  task->interval = interval;
  task->last_trigger = 0;
  task->fptr_setup = fptr_setup;
  return TRUE;
}
void task_setup(my_task_t * task)
{
  if(task && 
     task->fptr_setup)
  {
    task->fptr_setup();
  }
}

void task_setInterval(my_task_t * task, int interval, s64_t ts)
{
  if(task){
    task->interval = interval;
    task->last_trigger = ts;
  }
}

void task_trigger(my_task_t * task, s64_t ts)
{
  if(task && 
     ts-task->last_trigger>task->interval && 
     task->fptr_timeout)
  {
    task->fptr_timeout();
    task->last_trigger = ts;
  }
}

#ifdef __cplusplus
}
#endif
