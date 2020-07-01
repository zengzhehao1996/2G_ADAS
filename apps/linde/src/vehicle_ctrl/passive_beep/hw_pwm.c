#include "hw_pwm.h"
#include <stm32f4xx.h>
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_tim.h"

#include "my_misc.h"
#include <kernel.h>
#include <stdio.h>

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim);
static TIM_HandleTypeDef g_TIM8_Handler; //定时器 8PWM 句柄
static TIM_OC_InitTypeDef g_TIM8_CH4Handler; //定时器 8 通道 4 句柄

void TIM8_PWM_Init(uint16_t arr,uint16_t psc){
  
  //定时器基本配置
  g_TIM8_Handler.Instance=TIM8; //定时器 8
  g_TIM8_Handler.Init.Prescaler=psc; //定时器分频
  g_TIM8_Handler.Init.CounterMode=TIM_COUNTERMODE_UP;//向上计数模式
  g_TIM8_Handler.Init.Period=arr; //自动重装载值
  g_TIM8_Handler.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1;
  HAL_TIM_PWM_Init(&g_TIM8_Handler); //初始化 PWM
 
  //MOE 主输出使能
  TIM8->BDTR |= 1<<15;
  
  //输出配置
  g_TIM8_CH4Handler.OCMode=TIM_OCMODE_PWM1; //模式选择 PWM1
  g_TIM8_CH4Handler.Pulse=arr/2; //设置比较值,此值用来确定占空比
  g_TIM8_CH4Handler.OCPolarity=TIM_OCPOLARITY_LOW; //输出比较极性为低
  HAL_TIM_PWM_ConfigChannel(&g_TIM8_Handler,&g_TIM8_CH4Handler,TIM_CHANNEL_4); //配置 TIM8 通道 4
  HAL_TIM_PWM_Start(&g_TIM8_Handler,TIM_CHANNEL_4);//开启 PWM 通道 4

}

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim){
  GPIO_InitTypeDef GPIO_Initure;
  __HAL_RCC_TIM8_CLK_ENABLE();  //使能定时器 8
  __HAL_RCC_GPIOI_CLK_ENABLE();  //开启 GPIOI 时钟
  GPIO_Initure.Pin=GPIO_PIN_2; //PI2
  GPIO_Initure.Mode=GPIO_MODE_AF_PP; //复用推挽输出
  GPIO_Initure.Pull=GPIO_NOPULL; //外部已下拉
  GPIO_Initure.Speed=GPIO_SPEED_HIGH; //高速
  GPIO_Initure.Alternate= GPIO_AF3_TIM8;  //PI2 复用为 TIM8
  HAL_GPIO_Init(GPIOI,&GPIO_Initure);
}

void TIM_SetTIM8Compare4(uint32_t compare){
  TIM8->CCR4=compare;
}





