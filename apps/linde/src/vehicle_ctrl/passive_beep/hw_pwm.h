#ifndef __HW_PWM_H__
#define __HW_PWM_H__
#include <stdint.h>
void TIM8_PWM_Init(uint16_t arr,uint16_t psc);
void TIM_SetTIM8Compare4(uint32_t compare); 
#endif //endif __HW_PWM_H__