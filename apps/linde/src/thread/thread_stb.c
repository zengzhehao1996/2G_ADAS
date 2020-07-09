#include <stdio.h>
#include <kernel.h>
#include <string.h>
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
#define SEVEN_T 268435456
#define SIX_T 16777216
#define FIVE_T 1048576
#define FOUR_T 65536
#define THREE_T 4096
#define TWO_T 256
#define ONE_T 16

K_THREAD_STACK_DEFINE(g_thread_stb_stack, THREAD_STB_STACK_SIZE);
struct k_thread g_stb_thread;
k_tid_t g_stb_thread_id = 0;
static int8_t stage_buff[4] = {0};
static int8_t heartbeat_buff[4] = {0};
uint8_t RxBuff[32] = {0};
static uint32_t timestamp_buff[4] = {0};  
//static uint8_t loseHb_buff[4] = {0};

uint8_t stage = 0;
uint8_t lastStage = 0;
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
    static uint64_t longitude = 0;
    static uint64_t latitude = 0;
    static uint8_t *imei = NULL;
    static uint32_t RFID = 0;
    static uint32_t server_start_ts = 0;
    static uint32_t server_end_ts = 0;
    static int server_distance = 0;
    static int server_limit = 0;
    static int server_alert = 0;
    uint8_t tmp_imei[15] = {0};
    int tmp1 = 0;
    int tmp2 = 0;
    int tmp3 = 0;
    int tmp4 = 0;
    int i_loop = 0;
   
    ret = hw_uart_init();
    if(0 != ret)
    {
        print_log("Uart init failed.\n");
    }

    while(1)
    {        
        imei = gSysconfig.devId;
        print_log("imei is %s\n", imei);
        /*while(i_loop <= IMEI_SIZE)
        {
            tmp_imei[i_loop] = *(imei + i_loop);
            i_loop++; 
        }*/
      
        //print_log("tmp_imei is %s\n", tmp_imei);

        longitude = getLongitude();
        latitude = getLatitude();
        print_log("longitude is %ld\n", longitude);
        print_log("latitude is %ld\n", latitude);
        RFID = RFID_get();
        print_log("rfid is %d\n", RFID);
        

        ret = hw_uart_read_bytes(HW_UART6, RxBuff, sizeof(RxBuff));
        k_sleep(500);
        if(ret == 0)
        {
            continue;
        }
        int test_loop = 0;
        //print_log("rcv lenth is %d\n", ret);
        while((test_loop <= 32) && (ret != 0))
        {
            //if(0 != RxBuff[test_loop])
            //{
                print_log("number %d is %x\n",test_loop, RxBuff[test_loop]);
                test_loop += 1;
            //}
        }
        if((RxBuff[0] == 0x44) && (RxBuff[1] == 0xCC))   //send timestamp when LAB asks
        {
            timestamp = getTimeStamp();
            timestamp_buff[0] = TIMESTAMP_START;
            timestamp_buff[1] = timestamp;
            timestamp_buff[2] = END;
            //print_log("%d\n", timestamp_buff[1]);
            hw_uart_send_bytes(HW_UART6, timestamp_buff, sizeof(timestamp_buff));
            print_log("ts sended\n");
        }
        else if(RxBuff[0] == 0x66)
        {
            server_alert = RxBuff[15];
            server_limit = RxBuff[14];
            server_distance = RxBuff[13];
            uploadAlert_t alert = {0};
            bool alert_ret = true;
            
            print_log("alert %d\n", server_alert);
            print_log("limit %d\n", server_limit);
            print_log("distance %d\n", server_distance);

            tmp1 = RxBuff[7];
            tmp2 = RxBuff[6];
            tmp3 = RxBuff[5];
            tmp4 = RxBuff[4];

            server_start_ts = tmp1/16 * SEVEN_T + tmp1%16 * SIX_T + tmp2/16 * FIVE_T + tmp2%16 * FOUR_T + tmp3/16 * THREE_T + tmp3%16 * TWO_T + tmp4/16 * ONE_T + tmp4%16;

            tmp1 = RxBuff[11];
            tmp2 = RxBuff[10];
            tmp3 = RxBuff[9];
            tmp4 = RxBuff[8];

            server_end_ts = tmp1/16 * SEVEN_T + tmp1%16 * SIX_T + tmp2/16 * FIVE_T + tmp2%16 * FOUR_T + tmp3/16 * THREE_T + tmp3%16 * TWO_T + tmp4/16 * ONE_T + tmp4%16;

            print_log("start ts %d\n", server_start_ts);
            print_log("end ts %d\n", server_end_ts);


            alert.start_ts = server_start_ts;
            alert.end_ts = server_end_ts;
            alert.distance = server_distance;
            alert.limit = server_limit;
            alert.alert = server_alert;
            alert.longitude = longitude;
            alert.latitude = latitude;
            alert.RFID = RFID;
            for(i_loop = 0; i_loop < 16; i_loop = i_loop + 1)
            {
                alert.IMEI[i_loop] = *(imei + i_loop);
                //i_loop += 1; 
                print_log("loop is %d\n", i_loop);
                print_log("alert.IMEI is %s\n", alert.IMEI);
                //k_sleep(10);
            }
            
      
            
 
            /*print_log("start ts is %d\n", alert.start_ts);
            print_log("end ts is %d\n", alert.end_ts);
            print_log("distance ts is %d\n", alert.distance);
            print_log("limit ts is %d\n", alert.limit);
            print_log("alert  is %d\n", alert.alert);
            print_log("longitude ts is %ld\n", alert.longitude);
            print_log("latitude ts is %ld\n", alert.latitude);
            print_log("rfid ts is %d\n", alert.RFID);   
            i_loop = 0;
            if(i_loop <= 15)
            {
                print_log("imei is %s\n", alert.IMEI[i_loop]);
                i_loop += 1;
                k_sleep(10);
                //print_log("imei_d is %d\n", alert.IMEI);
            }
            else
            {
                break;
            }*/
 

            alert_ret = serverSendAlertInfo(&alert);
            if(false == alert_ret)
            {
                print_log("send alert info failed\n");
            }
            else
            {
                print_log("send alert info success\n");
            }
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
                if((RxBuff[0] != 0xBB) && (RxBuff[0] != 0x44) && (RxBuff[0] != 0x66)) 
                {
                    hb_count += 1;
                }
            }
            else
            {
                uploadLoseHb_t loseHb = {0};
                bool hb_ret = true;

                loseHb.lose_hb = 1;

                hb_ret = serverSendLoseHBInfo(&loseHb);
                if(false == hb_ret)
                {
                    print_log("send lose hb info failed\n");
                }
                else
                {
                    print_log("send lose hb success\n");
                }
            }   
		}

        speed = getLastSpeed();
		//print_log("speed is %d\n", speed);  //send veichle stage judging by speed
        if(0 == speed)
        {
            stage = 0;
        }
        else if(speed > 0)
        {
            stage = 1;
        }
        else if(speed < 0)
        {
            stage = 2;
        }
		
		//print_log("stage is %d\n", stage);

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
                //print_log("count is %d\n", stage_count);
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
