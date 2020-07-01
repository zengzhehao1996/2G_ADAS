#ifndef _M_STATE_DETECTION_H_
#define _M_STATE_DETECTION_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "m_queue.h"
#include "m_utils.h"
#include "m_state_detection.h"
#include "m_config.h"

// # Detection of static state
void static_detection(imu_report_t *);

// # Detection of move state
void move_detection(imu_report_t *,float m_std);

//Detects burrs in a moving state.
unsigned char fork_state_detection(imu_report_t *);

// get vibration value
unsigned short get_vibration_val(void);

void set_detection_factor(int val);

#endif

