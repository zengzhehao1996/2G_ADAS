#include "canThread.h"
#include "my_misc.h"
#include "hw_can.h"
#define CAN_THREAD_STACK_SIZE 1024

#define CAN_SPEED_ID 0x181
#define CAN_FORK_ID 0x282
#define CAN_BATTERY_SEAT_BRAKE_ID 0x281
#define CAN_WORKHOUR_ID 0x794
#define CAN_WORKHOUR_REQ_ID 0x784

K_THREAD_STACK_DEFINE(g_canThreadStack, CAN_THREAD_STACK_SIZE);
static struct k_thread g_canThread;
static k_tid_t         g_canThreadId;

static void canThreadRun(void* p);
static void can_workhour_test();

static uint8_t canErrCounter = 0;

static char can_speed[8]={0xDC,0x0A, 0x03, 0x04, 0x05, 0x06, 0x40, 0x20}; //10Km/h
static char can_battery[8]={0x40, 0x08, 0x19, 0x3C, 0xF6, 0x09, 0x00, 0x05};//battery:60%
static char can_fork[8] =  {0x40, 0x80, 0x19, 0x20, 0x5D, 0x02, 0x00, 0x05}; //fork move
static char can_workhour[8] = {0x06,0x61,0x14,0x57,0x8E,0x89,0x01,0x00}; //WORK_HOUR=7164h


bool startCanThread(void)
{
    bool ret;

    g_canThreadId =
        k_thread_create(&g_canThread, g_canThreadStack, CAN_THREAD_STACK_SIZE,
                        (k_thread_entry_t)canThreadRun, NULL, NULL, NULL, K_PRIO_COOP(5), 0, 0);
    if(g_canThreadId != 0)
    {
        ret = true;
        print_log("Create can THREAD Id:[ %p ]; Stack:[ %p ]; Size:[ %p ]\n", g_canThreadId,
                  g_canThreadStack, CAN_THREAD_STACK_SIZE);
    }
    else
    {
        ret = false;
        err_log("Create can thread Failed.\n\n");
    }

    return ret;
}
void stopCanThread(void)
{
    //hw_rs485_unint();
    if(0 != g_canThreadId)
    {
        
        k_thread_abort(g_canThreadId);
        g_canThreadId = 0;
        print_log("stop can Thread  success !\n");
    }
}
static void canThreadRun(void* p)
{
    //1.init can
    if(!hw_can_init(true))
    {
        err_log(" can init failed \n");
        return false;
    }
    
    while(1)
    {

        if(!hw_can_write(CAN_SPEED_ID,can_speed,8))
        {
            //print_log("can err!!!!!!!!!!!!!!!!\n");
            canErrCounter++;
        }
        if(!hw_can_write(CAN_FORK_ID,can_fork,8))
        {
            canErrCounter++;
        }
        if(!hw_can_write(CAN_BATTERY_SEAT_BRAKE_ID,can_battery,8))
        {
            canErrCounter++;
        }
    
        //1.work hour rx tx
        can_workhour_test();






        
        if(canErrCounter >=30)
        {
            print_log("can err counter = %d\n",canErrCounter);
            canErrCounter = 0;
            hw_can_init(true);
            k_sleep(100);
        }
        k_sleep(30);
    }
}

void can_workhour_test()
{
  CanRxMsgTypeDef * current_mesg;
  if(hw_can_rx_list_length() <= 0){
    //print_log("list length is 0\n");
    return;
  }
  for(int i=0;i<hw_can_rx_list_length();++i)
  {
    current_mesg = hw_can_rx_list_message(i);
   // 
    if(current_mesg)
    {//if message list
        if(current_mesg->StdId == CAN_WORKHOUR_REQ_ID)
        {
            //print_log("current_mesg->StdId == CAN_WORKHOUR_REQ_ID,data0[%02x],data1[%02x],data2[%02x]\n",current_mesg->Data[0],current_mesg->Data[1],current_mesg->Data[2]);
            if(current_mesg->Data[0] != 0x02 || current_mesg->Data[1] != 0x21 || current_mesg->Data[2] != 0x14)
            {
               // print_log("\nAAAAAAAAAAAAAAAArx work hour can msg data is err!!\n");
                hw_can_rx_list_reset();
                return;

            }

        }
        else
        {
            //print_log("\nAAAAAAAAAAAAAstd id err!!\n");
        }
        hw_can_rx_list_reset();
        if(!hw_can_write(CAN_WORKHOUR_ID,can_workhour,8))
        {
            canErrCounter++;
            print_log("can_workhour send failed\n");
        }
        else
        {
            //print_log("send ok \n");
            hw_can_rx_list_reset();
            break;
        }
    }

  }

}

