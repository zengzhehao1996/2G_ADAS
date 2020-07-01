#include "thread_mpu.h"
#include <stdio.h>
#include <kernel.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "hw_mse.h"
#include "m_queue.h"  //changed here guowei
#include "m_config.h"
#include "fifo.h"
#include "msg_structure.h"
#include "m_state_detection.h"

#define MPU_STACK_SIZE 1024

extern Queue gImuQueue;

K_THREAD_STACK_DEFINE(mpu_thread_stack, MPU_STACK_SIZE);

#define NO_LIMIT 0
#define HAVE_LIMIT 1

static struct k_thread g_mpu_thread;
static k_tid_t         g_mpu_threadid;
static s64_t           thread_mpu_mainloop_run_alive_last_time = 0;
static imu_report_t    imu_array;  //changed here guowei
static speedLimitConfig_t g_threshold  = { 1500, 1000000, 2000, 4000, 6000, 0 };
static uint8_t mpuCurrState = 0;

void threadMpuRun(void* p);
static void printMseConfig(speedLimitConfig_t* ps);

/*new vibration algorithm*/
mseGyroscope_t    gyroData = { 0, 0, 0 };
mseAcceleration_t acceData = { 0, 0, 0 };

bool threadMpuStart(speedLimitConfig_t* ps)
{
    bool ret;

    setMpuConfig(ps);
    printMseConfig(&g_threshold);

    thread_mpu_mainloop_run_alive_last_time = k_uptime_get_32();

    //step4.start the thread
    g_mpu_threadid = k_thread_create(&g_mpu_thread, mpu_thread_stack, MPU_STACK_SIZE,
                                     (k_thread_entry_t)threadMpuRun, NULL, NULL, NULL,
                                     K_PRIO_COOP(11), 0, 0);
    if(g_mpu_threadid != 0)
    {
        ret = true;
        print_log("Create BMI160 THREAD Id:[ %p ]; Stack:[ %p ]; Size:[ %p ]\n", g_mpu_threadid,
                  mpu_thread_stack, MPU_STACK_SIZE);
    }
    else
    {
        ret = false;
        err_log("Create Thread BMI160 Failed.\n\n");
    }

    return ret;
}

void threadMpuStop()
{
    if(g_mpu_threadid != 0)
    {
        k_thread_abort(g_mpu_threadid);
        g_mpu_threadid = 0;
    }
}

bool getMpuConfig(speedLimitConfig_t* ps)
{
    if(ps)
    {
        memcpy(ps, &g_threshold, sizeof(speedLimitConfig_t));
        return true;
    }
    else
    {
        return false;
    }
}

bool setMpuConfig(speedLimitConfig_t* ps)
{
    #if 0 /* support it in fauture */
    if(ps)
    {
        memcpy(&g_threshold, ps, sizeof(g_threshold));
        PARAM_CRASH_SET_typedef m_config;
        m_config.m_ShakeParam   = g_threshold.vibration_threshold;
        m_config.m_SlightParam  = g_threshold.crash_low_threshold;
        m_config.m_NormalParam  = g_threshold.crash_mid_threshold;
        m_config.m_SeriousParam = g_threshold.crash_hig_threshold;
        set_detection_factor(m_config);
        return true;
    }
    else
    {
        warning_log("mse config is NULL.\n");
        return false;
    }
    #else
    if(ps)
    {
        memcpy(&g_threshold, ps, sizeof(g_threshold));
        return true;
    }
    else
    {
        warning_log("mse config is NULL.\n");
        return false;
    }
    
    #endif
}

void threadMpuRun(void* p1)
{
    uint16_t static_cnt  = 0;
    uint16_t bmi160_flag = 0;
    uint8_t vec_state = 0;
    uint8_t discanlock = 0;
    int32_t alarm_mse=0;
    uint8_t logTimeInterval = 0;

    //step1. init the mpu
    if(!hwMseInit())
    {
        err_log("fail to initialize the bmi160\n");
        return FALSE;
    }

    set_detection_factor(g_threshold.vibration_threshold);

    thread_mpu_mainloop_run_alive_last_time = k_uptime_get_32();
    am_queue = CreateQueue("am");

    while(1)
    {
        thread_mpu_mainloop_run_alive_last_time = k_uptime_get_32();
       // print_log("########mpu_run_begin\n");
        if(hwMseGetGyroscope(&gyroData)
           && hwMseGetAcceleration(&acceData))  //changed here guowei
        {
            imu_array.gyro_x = gyroData.gyroX;  //changed here guowei
            imu_array.gyro_y = gyroData.gyroY;
            imu_array.gyro_z = gyroData.gyroZ;
            imu_array.accel_x = acceData.acceX;
            imu_array.accel_y = acceData.acceY;
            imu_array.accel_z = acceData.acceZ;
        }
        else
        {
            bmi160_flag++;
            print_log("bmi160 read none bmi160_flag[%d]PPPPPPPPPPPPPPPPPPPPPPPPP\n",bmi160_flag);
            if(1000 == bmi160_flag)
            {
                hwMseInit();
                bmi160_flag = 0;
            }
        }
    //    print_log(" call fork_state_detection   before++++++++\n");
       vec_state = fork_state_detection(&imu_array);
        mpuCurrState = vec_state;
    //    print_log(" call fork_state_detection   after++++++++:[%d]\n", vec_state);

       if(vec_state == 0)
       {
           //print_log("vec_state == 0UUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUU\n");
           static_cnt++;
       }
       else if(vec_state == 3)
       {
            print_log("vec_state == 3\n");
            static_cnt = 0;
            alarm_mse = get_vibration_val();
            speedLimitFIFO_t alarm;
            alarm.val = alarm_mse;
            alarm.reason     = TYPE_VIBRATION;
            alarm.limit_flag = NO_LIMIT;
           if(alarm.val != 0 && true == speedLimitFifoSend((char*)&alarm))
           {
               print_log("Send OK mse:[%d]\n", alarm_mse);
           }
           else
           {
               print_log("Send ERR mse:[%d]\n", alarm_mse);
           }
       }
       else
       {    
            //print_log("vec_state == else UUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUU\n");
            static_cnt = 0;
       }

       
       if(static_cnt > 1000)
       {
           static_cnt = 1000;
       }

       if(true == semTakeCanBroken())
       {
           discanlock = 1;
           print_log("<<<<<<<<<<<<<<<   IMu receive can broken semphore ............................\n");
       }

       
       if(true == semTakeCanOnline())
       {
           discanlock = 0;
           print_log("<<<<<<<<<< IMU receive  can connect ......... ............................\n");
       }

       if(discanlock==1 && static_cnt >=100)
       {
           print_log(">>>>>>>>>>>>>>> IMU  start send give can timeout sem lock vehicle  >>>>>>>>>>>>>>\n");
           semGiveCanTimeOut();
           discanlock = 0;
           print_log(">>>>>>>>>>>>>>> IMU send give can timeout sem lock vehicle  >>>>>>>>>>>>>>\n");
       }

      /**************************
       if(semTakeImuReq())
       {

            if(thread_mpu_mainloop_run_alive_last_time < 10000 || static_cnt >=100)
            {
                semGiveImuState();                
                print_log(">>>>>>>>>>>>>>  IMU give static vehicle can be safe lock   >>>>>>>>>>>>>>\n");
            }
            
       }
       *************************/

        k_sleep(50);
        
        logTimeInterval++;
        if(logTimeInterval >= 200)
        {
            logTimeInterval = 0;
            print_log("imu thread is alive static_cnt[%d]\n",static_cnt);
        }
    }
}


s64_t thread_mpu_main_loop_run_alive_get_last_time_ms(void)
{
    return thread_mpu_mainloop_run_alive_last_time;
}


void detect_thread_mpu(uint32_t current_time)
{
    if((current_time > thread_mpu_mainloop_run_alive_last_time)
       && (current_time - thread_mpu_mainloop_run_alive_last_time > THREAD_LOOP_TIME_5SEC))
    {
        warning_log("Restart Mpu thread. ++++++++++++++++++++\n");
        if(0 != g_mpu_threadid)
        {
            k_thread_abort(g_mpu_threadid);
            g_mpu_threadid = 0;
        }
        threadMpuStart(NULL);
    }
}
uint8_t getMpuStat()
{
    return mpuCurrState;
}

static void printMseConfig(speedLimitConfig_t* ps)
{
    print_log("MPU vibration      :[ %d ]\n", ps->vibration_threshold);
    print_log("MPU vibtation limit:[ %d ]\n", ps->vibration_limit_threshold);
    print_log("MPU crash low limit:[ %d ]\n", ps->crash_low_threshold);
    print_log("MPU crash mid limit:[ %d ]\n", ps->crash_mid_threshold);
    print_log("MPU crash hig limit:[ %d ]\n", ps->crash_hig_threshold);
    print_log("MPU crash switch   :[ %d ]\n", ps->carsh_switch);
}

//Detects burrs in a moving state.
/**************
void select_stick(imu_report_t* imuData)
{
    unsigned int dataM;
    dataM = calM(imuData);  //计算m值

    int var = 0;
    if(isFullQueue(&gImuQueue))
    {
        deleteQueue(&gImuQueue);
    }

    addQueue(&gImuQueue, dataM);

    if(isFullQueue(&gImuQueue))
    {

        // printQueue(q);
        var = getDiff(&gImuQueue);
        // std = algo_lib_int_sqrt((uint32_t)var);
        var = var / 5;
        if(var > 10000)
        {
            var = 10000;
        }

        if(var < g_threshold)
        {
            if(getVibraState() == 0)
            {
                printk("========normal======\n");
            }
            setVibraState(getVibraState() + 1);
        }
        else
        {
            if(getVibraState() > 200)
            {
                printk(" -----vibration-------\n");
                mseFIFO_t tmp;
                tmp.mse = var;
                //print_log("MSE: [ %d ] MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM.\n",
                //          var);
                if(true == mseFifoSend((char*)&tmp))
                {
                    print_log("Send OK mse:[%d]\n", tmp.mse);
                }
                else
                {
                    print_log("Send ERR mse:[%d]\n", tmp.mse);
                }

                setVibraState(0);
            }
        }
    }
}
****************/




