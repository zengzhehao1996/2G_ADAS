#ifdef __cplusplus
extern "C"{
#endif
#include "thread_wdt.h"
#include "file.h"
#include <kernel.h>
#include <string.h>

#include <stm32f4xx.h>
#include <stm32f4xx_hal_iwdg.h>

////////////////////////////////////////////////////////////////////
//Thread global settings here////
////////////////////////////////////////////////////////////////////
#define WDT_STACK_SIZE 512
K_THREAD_STACK_DEFINE(g_wdt_stack, WDT_STACK_SIZE);
struct k_thread g_wdt_thread;
k_tid_t g_wdt_thread_id=0;
int g_timeout=0;
//can thread function
static void thread_wdt_run(void * p);
////////////////////////////////////////////////////////////////////


IWDG_HandleTypeDef IWDG_Handler;
void IWDG_Init(uint8_t prer,uint16_t rlr)
{
  IWDG_Handler.Instance=IWDG;
  IWDG_Handler.Init.Prescaler=prer;
  IWDG_Handler.Init.Reload=rlr;  
  HAL_IWDG_Init(&IWDG_Handler);
}
void IWDG_Feed(void)
{
  HAL_IWDG_Refresh(&IWDG_Handler);
}


bool thread_wdt_start(int timeout){
  //step2.create CO thread
  g_timeout = timeout;
  g_wdt_thread_id = k_thread_create(&g_wdt_thread, g_wdt_stack, WDT_STACK_SIZE,
                             (k_thread_entry_t) thread_wdt_run,
                             NULL, NULL, NULL, 0, 0, 0);
  if(g_wdt_thread_id==NULL){
    print_log("Fail to create watch dog timer thread.\n");
    return FALSE;
  }
  print_log("Create WDT THREAD Id:[ %p ]; Stack:[ %p ]; Size:[ %d ]; TimeOut[%d]\n", g_wdt_thread_id,
            g_wdt_stack, WDT_STACK_SIZE,g_timeout);
  return TRUE;
}

#define FREE_WDT_MAX_MS 1000 /* 1 sec */
void thread_wdt_run(void * p){
  int rlr = g_timeout/2;
  int sleepMS = rlr > FREE_WDT_MAX_MS ? FREE_WDT_MAX_MS:rlr;

  print_log("Initializing wdt...\n");
  HAL_Init();
  IWDG_Init(IWDG_PRESCALER_64,rlr);
  print_log("WDT initialized\n");

  while(1){
    IWDG_Feed();
    //print_log("free WDT [%u].\n",k_uptime_get_32());
    k_sleep(sleepMS);
  }
}

void thread_wdt_stop()
{
  if(g_wdt_thread_id!=0){
    unmountFs();
    print_log("restart from wdt.\n");
    k_thread_abort(g_wdt_thread_id);
    g_wdt_thread_id=0;
  }
}
#ifdef __cplusplus
}
#endif
