#include "passive_beep.h"
#include "my_misc.h"
#include <gpio.h>
#include <device.h>
#include <board.h>
#include <stdio.h>
#include <stdint.h>

bool passiveBeepSetup()
{
    HAL_Init();
    TIM8_PWM_Init(500-1,72-1);
    TIM_SetTIM8Compare4(0);
    return true;
}
bool passiveBeepOpen()
{
    TIM_SetTIM8Compare4(300);
    return true;
}
bool passiveBeepClose()
{
    TIM_SetTIM8Compare4(0);
    return true;
}

void passiveBeepOnetime()
{
    passiveBeepOpen();
    k_sleep(100);
    passiveBeepClose();

}


