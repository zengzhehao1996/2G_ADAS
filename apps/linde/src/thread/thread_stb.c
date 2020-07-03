#include <stdio.h>
#include <kernel.h>
#include "thread_stb.h"
#include "handler_caninfo.h"
#include "hw_can.h"
#include "proto_adapt.h"
#include "hw_uart.h"
#include "rtc.h"
#include "rfid_thread.h"
#include "thread_gps.h"
#include "message_center_thread.h"
#include "server_interface.h"
#include "config.h"

#define THREAD_STB_STACK_SIZE 2048
#define FORWARD 0x01
#define BACKWARD 0x02
#define STILL 0x03
#define STATE_START 0xAA
#define END 0xFF
#define HEART_BEAT 0x55
#define HEART_BEAT_START 0xBB
#define HEART_BEAT_INTERVEL 30000
#define TIMESTAMP_START 0x33

K_THREAD_STACK_DEFINE(g_thread_stb_stack, THREAD_STB_STACK_SIZE);
struct k_thread g_stb_thread;
k_tid_t g_stb_thread_id = 0;
static int8_t stage_buff[4] = {0};
static int8_t heartbeat_buff[4] = {0};
int8_t RxBuff[12] = {0};
static uint32_t timestamp_buff[4] = {0};  

uint8_t stage = '0';
uint8_t lastStage = '0';
int32_t speed = 0;

static void threadSTBEntry();

bool startThreadSTB()
{
    print_log("ready");
    g_stb_thread_id = k_thread_create(&g_stb_thread, g_thread_stb_stack, THREAD_STB_STACK_SIZE,
         (k_thread_entry_t)threadSTBEntry, 
         NULL, NULL, NULL, K_PRIO_COOP(5), 0, 0);  
    if(g_stb_thread_id == 0)
    {
        print_log("Fail to create STB thread.\n");
        return false;
    }
    print_log("Create STB thread ID:[ %p ]; Stack:[ %p ]; Size:[ %d ]\n", g_stb_thread_id, 
              g_thread_stb_stack, THREAD_STB_STACK_SIZE);
    return true;
}


void stopThreadSTB()
{
    if(g_stb_thread_id != 0)
    {
        k_thread_abort(g_stb_thread_id);
        g_stb_thread_id = 0;
    }
}


static void threadSTBEntry()
{ 
    int32_t ret = 0;
    static int last_time = 0;
    static int timestamp = 0;
    static int stage_count = 0;
    static int hb_count = 0;
    static uint32_t rfid = 0;
    static uint64_t longitude = 0;
    static uint64_t latitude = 0;
    static uint8_t *imei = NULL;
   
    ret = hw_uart_init();
    if(0 != ret)
    {
        print_log("Uart init failed.\n");
    }

    while(1)
    {
        
        /*
        imei = gSysconfig.devId;
        //print_log("imei is %s\n", imei);
        longitude = getLongitude();
        latitude = getLatitude();
        print_log("longitude is %ld\n", longitude);
        print_log("latitude is %ld\n", latitude);
        */


        hw_uart_read_bytes(HW_UART6, RxBuff, sizeof(RxBuff));
        if((RxBuff[0] == 0x44) && (RxBuff[1] == 0xCC))   //send timestamp when LAB asks
        {
            timestamp = getTimeStamp();
            timestamp_buff[0] = TIMESTAMP_START;
            timestamp_buff[1] = timestamp;
            timestamp_buff[2] = END;
            //print_log("%d\n", timestamp_buff[1]);
            hw_uart_send_bytes(HW_UART6, timestamp_buff, sizeof(timestamp_buff));
        }

        int current_time = k_uptime_get();
        if((current_time - last_time >= HEART_BEAT_INTERVEL) || last_time == 0)  //send heartbeat every 30s, get response from LAB, tell server if there is no response for 4 times
        {
            heartbeat_buff[0] = HEART_BEAT_START;
            heartbeat_buff[1] = HEART_BEAT;
            heartbeat_buff[2] = END;
            //strcpy(heartbeat_buff, "uart_test");
			hw_uart_send_bytes(HW_UART6, heartbeat_buff, sizeof(heartbeat_buff));
            print_log("heart beat sended\n");
			last_time = current_time;
            
            hw_uart_read_bytes(HW_UART6, RxBuff, sizeof(RxBuff));
            if(hb_count < 4)
            {
                if(RxBuff[0] != 0xBB)
                {
                    hb_count += 1;
                }
            }
            else
            {
                //tell server
            }   
		}

        speed = getLastSpeed();
		print_log("speed is %d\n", speed);  //send veichle stage judging by speed
        if(0 == speed)
        {
            stage = '0';
        }
        else if(speed > 0)
        {
            stage = '1';
        }
        else if(speed < 0)
        {
            stage = '2';
        }
		
		print_log("stage is %s\n", stage);

        if(stage_count <= 30)
        {
            if(stage != lastStage)
            {    
                stage_count = 1;
                lastStage = stage;
            }
            else
            {
                stage_count += 1;
                print_log("count is %d\n", count);
            }
        }
        else
        {
            switch(stage)
            {
                case '0':
                    stage_buff[0] = STATE_START;
                    stage_buff[1] = STILL;
                    stage_buff[2] = END;
                    break;

                case '1':
                    stage_buff[0] = STATE_START;
                    stage_buff[1] = FORWARD;
                    stage_buff[2] = END;
                    break;

                case '2':
                    stage_buff[0] = STATE_START;
                    stage_buff[1] = BACKWARD;
                    stage_buff[2] = END;
                    break;
            }
            hw_uart_send_bytes(HW_UART6, stage_buff, sizeof(stage_buff));
            //接下来要从串口读回复
            print_log("stage change msg sended");
            lastStage = stage;
            stage_count = 0;
        }
            
        
    k_sleep(20);
    }
}
