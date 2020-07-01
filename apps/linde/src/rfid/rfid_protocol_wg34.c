#include <board.h>
#include <stdio.h>
#include <stdint.h>
#include <stm32f4xx.h>
#include <kernel.h>
#include "my_misc.h"
#include <gpio.h>
#include <device.h>
#include "rfid_protocol_wg34.h"
#define WG34_SIZE 34
static struct device *gpioD0;
static struct gpio_callback gpioD0Callback;
static struct device *gpioD1;
static struct gpio_callback gpioD1Callback;

static unsigned char g_wg34[WG34_SIZE] = {0};
static char g_bitCnt = 0;
static unsigned int g_rfidSeril = 0;
static char g_readSuccessFlag = 0;
static char g_rfidInitSuccessFlag = 0;
char rfidWg34Setup(void);
uint32_t rfidWg34GetSerilId(void);
/*
*can not call this api more than 2 times in one seconds!
*/
static void rfidWeigand(unsigned char* str);
static void wg34ReadD0(struct device *port,struct gpio_callback *cb,u32_t pins);
static void wg34ReadD1(struct device *port,struct gpio_callback *cb,u32_t pins);

static void wg34ReadD0(struct device *port,struct gpio_callback *cb,u32_t pins)
{

    if(g_bitCnt < WG34_SIZE)
    {
        g_wg34[g_bitCnt] = 0x00;
        //print_log("enter D0 CNT = %d\n",g_bitCnt);
        
    }
    g_bitCnt++;
}
static void wg34ReadD1(struct device *port,struct gpio_callback *cb,u32_t pins)
{
    if(g_bitCnt < WG34_SIZE)
    {
        g_wg34[g_bitCnt] = 0x01;
        //print_log("enter D1 CNT = %d\n",g_bitCnt);
        
    }
    g_bitCnt++;
}

char rfidWg34Setup(void)
{
    //step1: init DO
    gpioD0 = device_get_binding(GPIO_RFID_WG34_D0);
    if (!gpioD0)
    {
        print_log("Cannot find %s!\n", GPIO_RFID_WG34_D0);
        return 1;
    }
    // print_log("init success1\n");
    if(gpio_pin_configure(gpioD0, GPIO_RFID_WG34_D0_PIN, GPIO_INT | GPIO_INT_EDGE| GPIO_INT_ACTIVE_LOW ))
    {
        print_log("Configure gpio_nb16_DO failed");
        return 2;
    }
    // print_log("init success2\n");
    gpio_init_callback(&gpioD0Callback,wg34ReadD0,BIT(GPIO_RFID_WG34_D0_PIN));
    //print_log("init success3\n");
    gpio_add_callback(gpioD0, &gpioD0Callback);
    //print_log("init success4\n");
    gpio_pin_enable_callback(gpioD0,GPIO_RFID_WG34_D0_PIN);
    // print_log("init success5\n");

    //step2:init D1
    gpioD1 = device_get_binding(GPIO_RFID_WG34_D1);
    if (!gpioD1)
    {
         print_log("Cannot find %s!\n", GPIO_RFID_WG34_D1);
         return 3;
    }
    if(gpio_pin_configure(gpioD1, GPIO_RFID_WG34_D1_PIN, GPIO_INT | GPIO_INT_EDGE|GPIO_INT_ACTIVE_LOW ))
    {
        print_log("Configure gpio_nb16_D1 failed");
        return 4;
    }
    gpio_init_callback(&gpioD1Callback,wg34ReadD1,BIT(GPIO_RFID_WG34_D1_PIN));
    gpio_add_callback(gpioD1, &gpioD1Callback);
    gpio_pin_enable_callback(gpioD1,GPIO_RFID_WG34_D1_PIN);
    k_sleep(100);
    print_log("cnt = %d,wgstr=%s\n", g_bitCnt,g_wg34);
    memset(g_wg34,0,sizeof(g_wg34));
    print_log("init success\n");
    return 0;
}
char rfidWg34UnInit()
{
    //1.gpio_pin_disable_callback
    gpio_pin_disable_callback(gpioD0,GPIO_RFID_WG34_D0_PIN);
    gpio_pin_disable_callback(gpioD1,GPIO_RFID_WG34_D1_PIN);
    //2.gpio_remove_callback
    gpio_remove_callback(gpioD0, &gpioD0Callback);
    gpio_remove_callback(gpioD1, &gpioD1Callback);
    //3.

}

void static rfidWeigand(unsigned char* str)
{
    // print_log("enter Weigand\n");
    g_readSuccessFlag = 0;
    char odd = 0;
    char even = 0;
    char checkEven = 0;
    char checkOdd = 1;
    char i =0;
    unsigned int rfidSerilTmp = 0;
    even = g_wg34[0];
    odd = g_wg34[33];
    for(i = 1;i <33;i++)
    {
        if(g_wg34[i] == 0)
        {
            rfidSerilTmp|= (0 << (33- i - 1));
        }else if(g_wg34[i] == 1)
        {
            rfidSerilTmp|= (1 << (33- i - 1));
            if(i < 17)
            {
                checkEven = ~checkEven;
            }else if(i>=17)
            {
                checkOdd = ~checkOdd;
            }
            checkEven &= 0x01;
            checkOdd &= 0x01;
       }
    }
    // print_log("111rfid_seril=%x,g_rfidSerilBack=%x~~~even = %d,odd = %d,checkEven = %d,checkEven = %d,\n",\
    //           rfidSerilTmp,rfid_seril_back_tmp,even,odd,checkEven,checkEven);
    if( (even != checkEven) ||  (odd != checkOdd))
    {
        g_bitCnt = 0;
        memset(g_wg34,0,sizeof(g_wg34));
        g_readSuccessFlag = 0;
        // print_log("g_rfidSeril=%d,g_rfidSerilBack=%d~~~even = %d,odd = %d,checkEven = %d,checkEven = %d,\n",\
        //           g_rfidSeril,g_rfidSerilBack,even,odd,checkEven,checkEven);
        return;
    } 
    g_rfidSeril = rfidSerilTmp;
    g_readSuccessFlag = 1;
    // print_log("~~~g_rfidSeril=%u,g_rfidSerilBack=%u\n", g_rfidSeril,g_rfidSerilBack);
}

char rfidWg34RcvBitCnt()
{
    return g_bitCnt;
}

void rfidWg34Reset()
{
    g_readSuccessFlag = 0;
    g_rfidSeril = 0;
    g_bitCnt = 0;
    memset(g_wg34,0,sizeof(g_wg34));
}
unsigned int rfidWg34GetCardId()
{
    rfidWeigand(g_wg34); 
    if(!g_readSuccessFlag)
    {
        return 0;
    }else if(g_readSuccessFlag == 1)
    {
        // uint32_t rfidSerilReturn = g_rfidSeril;
        // rfidSerilReturn = ((rfidSerilReturn & 0XFF) << 24 ) | ((rfidSerilReturn & 0XFF00) << 8) 
        //               |((rfidSerilReturn & 0XFF0000) >> 8)|((rfidSerilReturn & 0XFF000000) >> 24);
        // return rfidSerilReturn;
        return g_rfidSeril;
    }
}



