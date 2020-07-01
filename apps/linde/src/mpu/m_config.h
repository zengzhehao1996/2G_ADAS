#ifndef _M_CONFIG_H_    
#define _M_CONFIG_H_ 

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#define STD_THRESHOLD  100  // The m value standard deviation threshold for entering the standard state;
#define STD_MOVE_THRESHOLD 150 // The m value standard deviation threshold for entering the forward and backward state;

#define STATIC_THRESHOLD  10  // # Delay threshold at rest
#define MOVE_THRESHOLD  10 // # The threshold of vehicle motion

// #define AM_QUEUE_USESIZE  200 // # The threshold of vehicle motion
// #define AY_QUEUE_USESIZE  50 // # The threshold of vehicle motion
#define QUEUE_USESIZE  50 // # The threshold of vehicle motion

#define AXIAL_DIRECTION imu_data->accel_y

void set_static_delayed(int delayed);

int get_static_delayed();

void set_move_delayed(int delayed);

int get_move_delayed();

void set_fork_state(int fork_s);

int get_fork_state();

void set_queue_state();

bool get_queue_state();

void set_vibra_state(int aa);

int get_vibra_state();

#endif