#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "m_utils.h"
#include "m_queue.h"


static int int_pow(int a){
        return (a * a);
}
static float float_pow(float a){
        return (a * a);
}

float invSqrt(float x)
{
    float xhalf = 0.5f * x;
    int i = *(int*)&x;
    i = 0x5f375a86 - (i>>1);
    x = *(float*)&i;
    x = x * (1.5f - xhalf * x * x);
    x = x * (1.5f - xhalf * x * x);
    x = x * (1.5f - xhalf * x * x);

    return 1/x;
}

//calculate m
unsigned int cal_m(imu_report_t * imu_data)
{
        // printf("m value=%d\n", imu_data->accel_x);
        int m2 = 0;
        unsigned int data_m;
        m2 = int_pow(imu_data->accel_x)+int_pow(imu_data->accel_y)+int_pow(imu_data->accel_z);

        data_m = (int)invSqrt((float)m2);
        return data_m;
}

//calculate mean(junzhi)
float get_mean(int *p_items){
        float mean;
        long sum=0;
        int j=0;
        for (j = 0; j < MAXSIZE; j++) {
                        sum+=p_items[j];
        }
        mean = sum/MAXSIZE;
        return mean;
}


//calculate the difference value
float get_diff(Queue* q){
    float diff = 0.0;
    int qitems[MAXSIZE];
    int *p_items;
    float mean;
    p_items = GetQueueItems(q,qitems);
    mean = get_mean(p_items);
    diff = p_items[MAXSIZE-1] - mean;
    return diff;
}

//calculate ay_means
float get_ay_means(Queue* q){
    float mean;
    int qitems[MAXSIZE];
    int *p_items;
    int use_arr[QUEUE_USESIZE];
    int *use_array;
    p_items = GetQueueItems(q,qitems);
    use_array = get_use_array(p_items,use_arr);
    mean = get_mean(use_array);
    return mean;
}

int * get_use_array(int *full_array,int *use_arr){
    for (int i = 0; i < QUEUE_USESIZE; i++)
    {
        use_arr[i] = full_array[MAXSIZE-QUEUE_USESIZE+i];
    }
    return use_arr;
}

// calculate fangcha ; Just calcuate the QUEUE_USESIZE part.
float get_variance(Queue* q)
{
        int qitems[MAXSIZE];
        int *p_items;
        float mean;
        int use_arr[QUEUE_USESIZE];
        int *use_array;

        p_items = GetQueueItems(q,qitems);
        use_array = get_use_array(p_items,use_arr);
        mean = get_mean(use_array);

        float divisor,sum = 0.0;
        int k;
        for(k=0;k<QUEUE_USESIZE;k++){
                sum += float_pow(use_array[k]-mean);
        }
        // printf("std********%f\n",sum/MAXSIZE);
        return sum/QUEUE_USESIZE;
}

int get_ay_sum(Queue* q,int topn){
    float mean;
    int qitems[MAXSIZE];
    int *p_items;
    int sum = 0;
    p_items = GetQueueItems(q,qitems);
    for(int i=1;i<=topn;i++){
        sum += p_items[MAXSIZE-i];
    }

    return sum;
}

int get_top5_max(Queue* q,int topn){
    float mean;
    int qitems[MAXSIZE];
    int *p_items;
    p_items = GetQueueItems(q,qitems);
    int sum_max = p_items[MAXSIZE-1];
    // int sum_min = p_items[MAXSIZE-i];
    for(int i=2;i<=topn;i++){
        if (sum_max<p_items[MAXSIZE-i])
        {
            sum_max = p_items[MAXSIZE-i];
        }
    }

    return sum_max*topn;
}

int get_top5_min(Queue* q,int topn){
    float mean;
    int qitems[MAXSIZE];
    int *p_items;
    p_items = GetQueueItems(q,qitems);
    int sum_max = p_items[MAXSIZE-1];
    // int sum_min = p_items[MAXSIZE-i];
    // printf("========%d============\n",sum_max);
    for(int i=2;i<=topn;i++){
        if (sum_max>p_items[MAXSIZE-i])
        {
            sum_max = p_items[MAXSIZE-i];
            // printf("%d\n", sum_max);
        }
    }
    return sum_max*topn;
}

