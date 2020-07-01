#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "m_state_detection.h"
#include "my_misc.h"

static  int g_vibration_val=0;
static  int g_detection_factor=0;

#define  MSE_RATIO 10

// # Detection of static state
void static_detection(imu_report_t * imu_data)
{
        set_static_delayed(get_static_delayed()+1);
        set_move_delayed(0);
        if(get_static_delayed()>=STATIC_THRESHOLD){
            set_fork_state(0);
        }
}


// # Detection of move state
void move_detection(imu_report_t * imu_data,float m_std){
    set_static_delayed(0);
    set_move_delayed(get_move_delayed()+1);
    if(get_move_delayed()>MOVE_THRESHOLD)
    {
    	if (m_std>=STD_MOVE_THRESHOLD)
        {
            set_fork_state(1);
    	}

    }

}

//Detects burrs in a moving state.
// u_int8_t fork_state_detection(imu_report_t * imu_data){
unsigned char fork_state_detection(imu_report_t * imu_data)
{
    unsigned int data_m;
    // printk("==%d--",imu_data->accel_x);
    data_m = cal_m(imu_data);//计算m值
    float m_var = 0.0, m_std = 0.0,var_diff = 0.0;
    if(IsFullQ(am_queue))
    {
        DeleteQ(am_queue);
    }

    AddQ(am_queue,data_m);

    if(IsFullQ(am_queue))
    {
        // PrintQueue(am_queue);
        m_var = get_variance(am_queue);
        m_std = (float)invSqrt(m_var);
        var_diff = get_diff(am_queue);
        //printk("std********%f\n",m_std);

        if(var_diff<=g_detection_factor)
       {
            if(m_std<STD_THRESHOLD)
            {
                static_detection(imu_data);    
            }
            else
            {
                move_detection(imu_data,m_std);
            }
            if(get_vibra_state()==0)
            {
               //printk("=========normal======\n");  
            }
            set_vibra_state(get_vibra_state()+1);                                              

        }
        else
       {
           if(get_vibra_state()>200)
           {
                 set_fork_state(3);
                 print_log("vibrate alarm:[%d],g_vib:[%d]\n",(int)var_diff,g_vibration_val);
                 set_vibra_state(0);
                 // 保留振动期间最大值，而不是最后一个值.用于修复MSE误报问题
                 if((int)var_diff>g_vibration_val)
                 {
                     g_vibration_val = var_diff;
                     // 不能有短整型溢出
                     if((g_vibration_val/MSE_RATIO) > 0xffff)
                     {
                         g_vibration_val = 0xffff * MSE_RATIO;
                     }
                 }
           }    

        }

    }

    return get_fork_state();
}


unsigned short get_vibration_val(void)
{
    unsigned short vibration = 0;
    vibration = (unsigned short)(g_vibration_val/MSE_RATIO);
    if(vibration > 10000)
    {
        vibration = 10000;
    }
    g_vibration_val = 0;
    return vibration;
}

void set_detection_factor(int val)
{
    g_detection_factor = MSE_RATIO*val;
}

