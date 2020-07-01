#include "uartThread.h"
#include "my_misc.h"
#include "hw_uart.h"
#define UART_THREAD_STACK_SIZE 1024
K_THREAD_STACK_DEFINE(g_uartThreadStack, UART_THREAD_STACK_SIZE);
static struct k_thread g_uartThread;
static k_tid_t         g_uartThreadId;

static void uartThreadRun(void* p);
static   void uart6_test(void);


bool startUartThread(void)
{
    bool ret;

    g_uartThreadId =
        k_thread_create(&g_uartThread, g_uartThreadStack, UART_THREAD_STACK_SIZE,
                        (k_thread_entry_t)uartThreadRun, NULL, NULL, NULL, K_PRIO_COOP(5), 0, 0);
    if(g_uartThreadId != 0)
    {
        ret = true;
        print_log("Create uart THREAD Id:[ %p ]; Stack:[ %p ]; Size:[ %p ]\n", g_uartThreadId,
                  g_uartThreadStack, UART_THREAD_STACK_SIZE);
    }
    else
    {
        ret = false;
        err_log("Create RS485 thread Failed.\n\n");
    }

    return ret;
}
void stopUartThread(void)
{
    //hw_rs485_unint();
    if(0 != g_uartThreadId)
    {
        
        k_thread_abort(g_uartThreadId);
        g_uartThreadId = 0;
        print_log("stop uart Thread  success !\n");
    }
}
static void uartThreadRun(void* p)
{
    //1.init rs485
    if(hw_uart_init())
    {
        //printk("[%s,%d] UART6 init ok.\n",__FILE__,__LINE__);
        err_log(" uart init failed \n");
        return false;
    }
    uint8_t preTxRxBuff[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};
    uint8_t rxBuff[20] = {};
    uint16_t tx_buff_size = sizeof(preTxRxBuff);
    char rx_flag = 0; 
    while(1)
    {
    #if 0
        //1.get 485 data num
        rx_flag = 0;
        if(hw_uart_rx_buff_num() >= sizeof(preTxRxBuff))
        {
            memset(rxBuff,0,sizeof(rxBuff));
            hw_rs485_read(rxBuff,sizeof(preTxRxBuff));
            hw_uart_read_bytes(HW_UART6, rxBuff, sizeof(tx_buff_size));
            int buff_size = sizeof(preTxRxBuff)/sizeof(preTxRxBuff[0]);
            for(int i = 0;i < buff_size;i++)
            {
                if(rxBuff[i] !=preTxRxBuff[i])
                {
                    rx_flag = -1;
                    break;
                }
                if(i == buff_size-1)
                {
                    print_buff(rxBuff,buff_size);
                   // hw_rs485_send(preTxRxBuff,tx_buff_size);
                   hw_uart_send_bytes(HW_UART6, preTxRxBuff, tx_buff_size);
                }
            }
        }
        if(rx_flag < 0)
        {
            print_log("uart rx_flag [%d]!!!!!\n",(int)rx_flag);
        }
      #endif
        uart6_test();
        k_sleep(30);
    }
}

static void uart6_test(void)
{
  char uart_buf[128]={0};
  int uart_read_bytes=0;

  uart_read_bytes = hw_uart_read_bytes(HW_UART6, uart_buf, sizeof(uart_buf));

  if(uart_read_bytes){
    printk("UART6 RX [%d]: ", uart_read_bytes);
    for(int i=0;i<uart_read_bytes;i++){
      printk("0x%02x ", uart_buf[i]);
    }
    printk("\n");
    hw_uart_send_bytes(HW_UART6, uart_buf, uart_read_bytes);
  }
  
  return;
}
