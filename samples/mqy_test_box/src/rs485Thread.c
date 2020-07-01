#include "rs485Thread.h"
#include "my_misc.h"
#include "hw_rs485.h"
#include "modbus_rtu_slave.h"
#define RS485_THREAD_STACK_SIZE 1024
K_THREAD_STACK_DEFINE(g_rs485ThreadStack, RS485_THREAD_STACK_SIZE);
static struct k_thread g_rs485Thread;
static k_tid_t         g_rs485ThreadId;
static bool isRs485ThreradStartFlag = false;
static uint8_t rxBuff[60] = {0};
static uint16_t rcvDataNum = 0;

/*该参数设置接受区大小*/
#define RECERIVRSIZE  200//接受区大小
typedef struct
{
    int Pread;//读指针
    int Pwrite;//写指针
    int Count;//缓冲区计数
    uint8_t  Recerivrbuffer[RECERIVRSIZE];//接受缓冲区
}Usart_ReceriveType;
static Usart_ReceriveType rs485rcvFifo;
static RS485_FIFO_RET rs485_fifo_write(uint8_t * buf,uint8_t length);
static RS485_FIFO_RET rs485_fifo_Read(uint8_t * buf,uint8_t length);
static void rs485_fifo_Clear();
static RS485_FIFO_RET rs485_fifo_cat(uint8_t * buf,uint8_t length);
static RS485_FIFO_RET rs485_fifo_init();
static RS485_FIFO_RET rs485_fifo_num_get();
static void rs485dataParse();
static void rs485ThreadRun(void* p);
bool startRs485Thread(void)
{
    bool ret;

    g_rs485ThreadId =
        k_thread_create(&g_rs485Thread, g_rs485ThreadStack, RS485_THREAD_STACK_SIZE,
                        (k_thread_entry_t)rs485ThreadRun, NULL, NULL, NULL, K_PRIO_COOP(5), 0, 0);
    if(g_rs485ThreadId != 0)
    {
        ret = true;
        print_log("Create RS485 THREAD Id:[ %p ]; Stack:[ %p ]; Size:[ %p ]\n", g_rs485ThreadId,
                  g_rs485ThreadStack, RS485_THREAD_STACK_SIZE);
    }
    else
    {
        ret = false;
        err_log("Create RS485 thread Failed.\n\n");
    }

    return ret;
}
void stopRs485Thread(void)
{
    hw_rs485_unint();
    if(0 != g_rs485ThreadId)
    {
        
        k_thread_abort(g_rs485ThreadId);
        g_rs485ThreadId = 0;
        isRs485ThreradStartFlag = false;
        print_log("stopRs485 Thread  success !\n");
    }
}
static void rs485ThreadRun(void* p)
{
    //1.init rcv fifo
    rs485_fifo_init();
    //2.init rs485
    if(!hw_rs485_init())
    {
        err_log(" rs485 init failed \n");
        return false;
    }
    modbufMuxInit();
    isRs485ThreradStartFlag = true;
    //uint8_t preRxBuff[] = {0x01,0x03,0x00,0x00,0x00,0x01,0x84,0x0A};
    //uint8_t txBuff[] = {0x01,0x03,0x02,0x01,0x2C,0xB8,0x09};

    //uint16_t tx_buff_size = sizeof(txBuff);
    char rx_flag = 0; 
    
    while(1)
    {

        rcvDataNum = 0;
        rcvDataNum = hw_rs485_read(rxBuff,sizeof(rxBuff));
#if 0

        print_log("read data:\n");
        for(int i = 0;i < rcvDataNum;i++)
        {
            printk("%02x ",rxBuff[i]);
        }
        printk("\n");
#endif
        if(rcvDataNum > 0)
        {
            rs485_fifo_write(rxBuff,rcvDataNum);

        }
        if(rs485_fifo_num_get() < 6)
        {
            k_sleep(5);
            continue;
        }
        rs485dataParse();
        //modbusPars(rxBuff,rcvDataNum);
    
        k_sleep(5);
    }
}
void print_buff(uint8_t* buff,int len)
{
    print_log("buff :");
    for(int i  = 0;i < len;i++)
    {
        printk("%02x ",buff[i]);
    }
    printk("\n");
    
}

bool isRs485ThreradStart()
{
    return isRs485ThreradStartFlag;
}


///////////////////////////////////////////////////////////
RS485_FIFO_RET rs485_fifo_Read(uint8_t * buf,uint8_t length)
{
    if (rs485rcvFifo.Count - length  < 0)//缓冲区没有足够的数据
    {
        return USARTREADOVER;//读数据越界
    }
    while (length--)
    {
        *buf = rs485rcvFifo.Recerivrbuffer[rs485rcvFifo.Pread];
        buf++;    
        rs485rcvFifo.Count --;
        rs485rcvFifo.Pread++;//读取指针自加
        if(rs485rcvFifo.Pread == RECERIVRSIZE)
        {
            rs485rcvFifo.Pread =0;
        }
            
    }
    return USARTOK;//数据读取成功
}

/*向缓冲区中写入length个数据*/
RS485_FIFO_RET rs485_fifo_write(uint8_t * buf,uint8_t length)
{
    if(buf == NULL || length == 0)
    {
        return UART_ENTER_ERR;
    }
    if (rs485rcvFifo.Count + length  > RECERIVRSIZE)//写入的数据超过缓冲区
    {
        return USARTWRITEOVER;//写数据越界
    }
    while(length--)
    {
        rs485rcvFifo.Recerivrbuffer[rs485rcvFifo.Pwrite] = *buf;//赋值给缓冲区
        buf++;//缓冲区地址加一        
        rs485rcvFifo.Count ++;
        rs485rcvFifo.Pwrite++;//
        if(rs485rcvFifo.Pwrite == RECERIVRSIZE)
        {
            rs485rcvFifo.Pwrite =0;
        }
            
    }
    return USARTOK;//数据读取成功
    
}
 
/*清空缓冲区*/
void rs485_fifo_Clear()
{
    rs485rcvFifo.Count = 0;
    rs485rcvFifo.Pread =0;//读指针为0
    rs485rcvFifo.Pwrite = 0;//写指针为0
}


RS485_FIFO_RET rs485_fifo_init()
{
    memset(&rs485rcvFifo,0,sizeof(rs485rcvFifo));
    return USARTOK;
}

RS485_FIFO_RET rs485_fifo_cat(uint8_t * buf,uint8_t length)
{
    if(rs485rcvFifo.Count +length > RECERIVRSIZE)
    {
        return USARTCATOVER; //预览越界
    }
    memcpy(buf,rs485rcvFifo.Recerivrbuffer,length);
    return USARTOK;
}

RS485_FIFO_RET rs485_fifo_num_get()
{
    return rs485rcvFifo.Count;
}

static void rs485dataParse()
{
    uint8_t buff[30] = {0};
    if(rs485_fifo_Read(buff,1) == USARTREADOVER)
    {
        print_log("485 fifo num is enought!\n");
        return;
    }
    if(buff[0] != SLAVE_ADDR)
    {
        print_log("addr is err[%d]\n",buff[0]);
        return;
    }
    
    if(rs485_fifo_Read(&buff[1],1) == USARTREADOVER)
    {
        print_log("485 fifo num is enought!\n");
        return;
    }

    
    if(buff[1] == MODBUS_CMD_03)
    {
        for(int i = 0;i < 2;i++)
        {
            if(rs485_fifo_Read(&buff[2],6) == USARTREADOVER)
            {
                //print_log("cmd =03,read 485 fifo num is enought!\n");
                k_sleep(20);
                rcvDataNum = 0;
                rcvDataNum = hw_rs485_read(rxBuff,sizeof(rxBuff));
                if(rcvDataNum > 0)
                {
                    rs485_fifo_write(rxBuff,rcvDataNum);
            
                }
                else
                {
                    if(i == 1)
                    {
                        print_log(" read 2times,cmd =03,read 485 fifo num is enought!\n");
                        return;
                    }
                }
                //return;
                
            }
            else
            {
                break;
            }
            

        }

        modbusPars(buff,8);
    }
    else if(buff[1] == MODBUS_CMD_16)
    {
        uint16_t readnum = 0;
        
        for(int i = 0;i < 2;i++)
        {
            if(rs485_fifo_Read(&buff[2],5) == USARTREADOVER)
            {
                print_log("cmd =16,read 485 fifo num is enought!\n");
                k_sleep(50);
                rcvDataNum = 0;
                rcvDataNum = hw_rs485_read(rxBuff,sizeof(rxBuff));
                if(rcvDataNum > 0)
                {
                    rs485_fifo_write(rxBuff,rcvDataNum);
            
                }
                else
                {
                    if(i == 1)
                    {
                        print_log(" read 2times,cmd =16,read 485 fifo num is enought!\n");
                        return;
                    }
                }

            }
            else
            {
                break;

            }
        }
        readnum = 2* ((buff[4] << 8)| buff[5]) +2;
        print_log("cmd = 16 read num = %d\n",readnum);
        /////////////////////////
        for(int i = 0;i < 2;i++)
        {
            if(rs485_fifo_Read(&buff[7],readnum) == USARTREADOVER)
            {
                print_log("cmd =16,read 485 fifo num is enought!\n");
                k_sleep(50);
                rcvDataNum = 0;
                rcvDataNum = hw_rs485_read(rxBuff,sizeof(rxBuff));
                if(rcvDataNum > 0)
                {
                    rs485_fifo_write(rxBuff,rcvDataNum);
            
                }
                else
                {
                    if(i == 1)
                    {
                        print_log(" read 2times,cmd =16,read 485 fifo num is enought!\n");
                        return;
                    }
                }

            }
            else
            {
                break;

            }
        }        
        /////////////////////////
#if 0
        print_log("data:\n");
        for(int i = 0;i < readnum + 7;i++)
        {
            printk("%02x ",buff[i]);
        }
        printk("\n");
#endif

        modbusPars(buff,readnum + 9);
    }
    else
    {
        print_log("pars is err! buff[0] = %d,buff[1] = %d\n",buff[0],buff[1]);
    }
    
    
}

