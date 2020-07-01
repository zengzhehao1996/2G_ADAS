#include "hw_can.h"


#include <kernel.h>
#include <string.h>

/////////////////////////////////////
CAN_HandleTypeDef   CAN1_Handler;
CanTxMsgTypeDef     TxMessage;
CanRxMsgTypeDef     RxMessage;
/////////////////////////////////////

#define HW_CAN_PRESCALER 12 //12 is 500kbps, 6 is 1000kbps
#define HW_CAN_RX_LIST_SIZE 128

static CanRxMsgTypeDef g_rxList[HW_CAN_RX_LIST_SIZE];
static int g_rxLen=0;
static s64_t g_last_rx_ts=0;
static BOOL g_isNormal=FALSE;

ISR_DIRECT_DECLARE(can1_rx0_irqhanlder)
{
  HAL_CAN_IRQHandler(&CAN1_Handler);
  __HAL_CAN_ENABLE_IT(&CAN1_Handler,CAN_IT_FMP0);
}
void HAL_CAN_RxCpltCallback(CAN_HandleTypeDef* hcan)
{
#if 0
  printk("id:0x%x, dlc:%d, data:",CAN1_Handler.pRxMsg->StdId,CAN1_Handler.pRxMsg->DLC);
  for(int i=0;i<CAN1_Handler.pRxMsg->DLC;i++){
    printk("%x ",CAN1_Handler.pRxMsg->Data[i]);
  }
  printk("\r\n");
#endif
  g_last_rx_ts = k_uptime_get();
  if(g_rxLen<HW_CAN_RX_LIST_SIZE){
    memcpy((uint8_t*)&(g_rxList[g_rxLen]),(uint8_t*)(CAN1_Handler.pRxMsg),sizeof(CanRxMsgTypeDef));
    ++g_rxLen;
  }
  
}


BOOL hw_can_init(BOOL isNormal)
{
  CAN_FilterConfTypeDef  CAN1_FilerConf;

  g_rxLen = 0;
    
  CAN1_Handler.Instance=CAN1; 
  CAN1_Handler.pTxMsg=&TxMessage;     
  CAN1_Handler.pRxMsg=&RxMessage;  
  CAN1_Handler.Init.Prescaler=HW_CAN_PRESCALER;//hw_baudrate_to_prescaler(buadrate);
  CAN1_Handler.Init.Mode=(isNormal)?CAN_MODE_NORMAL:CAN_MODE_LOOPBACK;
  CAN1_Handler.Init.SJW=CAN_SJW_1TQ;
  CAN1_Handler.Init.BS1=CAN_BS1_3TQ;
  CAN1_Handler.Init.BS2=CAN_BS2_2TQ;
  CAN1_Handler.Init.TTCM=DISABLE;
  CAN1_Handler.Init.ABOM=DISABLE;
  CAN1_Handler.Init.AWUM=ENABLE;
  CAN1_Handler.Init.NART=ENABLE;
  CAN1_Handler.Init.RFLM=DISABLE;
  CAN1_Handler.Init.TXFP=DISABLE;

  if(HAL_CAN_Init(&CAN1_Handler)!=HAL_OK){
    print_log("Fail here.\n");
    return FALSE;
  }

  CAN1_FilerConf.FilterIdHigh=0X0000;
  CAN1_FilerConf.FilterIdLow=0X0000;
  CAN1_FilerConf.FilterMaskIdHigh=0X0000;
  CAN1_FilerConf.FilterMaskIdLow=0X0000;
  CAN1_FilerConf.FilterFIFOAssignment=CAN_FILTER_FIFO0;
  CAN1_FilerConf.FilterNumber=0;
  CAN1_FilerConf.FilterMode=CAN_FILTERMODE_IDMASK;
  CAN1_FilerConf.FilterScale=CAN_FILTERSCALE_32BIT;
  CAN1_FilerConf.FilterActivation=ENABLE;
  CAN1_FilerConf.BankNumber=14;

  if(HAL_CAN_ConfigFilter(&CAN1_Handler,&CAN1_FilerConf)!=HAL_OK){
    print_log("Fail here.\n");
    return FALSE;
  }

  g_isNormal = isNormal;

  return TRUE;
}
BOOL hw_can_isNormal(void){return g_isNormal;}
void HAL_CAN_MspInit(CAN_HandleTypeDef* hcan)
{
  GPIO_InitTypeDef GPIO_Initure;

  __HAL_RCC_CAN1_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

#if defined(CONFIG_BOARD_AIDONG_LINDE429V13) || defined(CONFIG_BOARD_AIDONG_LINDE429V14)
    GPIO_Initure.Pin=GPIO_PIN_0|GPIO_PIN_1;
    GPIO_Initure.Mode=GPIO_MODE_AF_PP;
    GPIO_Initure.Pull=GPIO_NOPULL;    
    GPIO_Initure.Speed=GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_Initure.Alternate=GPIO_AF9_CAN1;
      
    HAL_GPIO_Init(GPIOD,&GPIO_Initure);
#endif

  __HAL_CAN_ENABLE_IT(&CAN1_Handler,CAN_IT_FMP0);
  IRQ_CONNECT(CAN1_RX0_IRQn, 0, can1_rx0_irqhanlder, NULL, 0);
  irq_enable(CAN1_RX0_IRQn);
}


BOOL hw_can_write(uint32_t id,uint8_t* data,int len){
  bool ret=false;
  CAN1_Handler.pTxMsg->StdId=id;
  CAN1_Handler.pTxMsg->IDE=CAN_ID_STD;
  CAN1_Handler.pTxMsg->RTR=CAN_RTR_DATA;
  CAN1_Handler.pTxMsg->DLC=len;                

  memcpy((uint8_t*)&(CAN1_Handler.pTxMsg->Data[0]),data,len);
  ret = (HAL_CAN_Transmit(&CAN1_Handler,300)==HAL_OK);
  //print_log("id=0x%x,len=%d,ret=%d\n",id,len,ret);
  return ret;
}

s64_t hw_can_last_rx_ts(void){ return g_last_rx_ts;}
void hw_can_rx_list_reset(void){
  //irq_disable(CAN1_RX0_IRQn);
  g_rxLen = 0;
  //memset((uint8_t*)(&g_rxList[0]),0,sizeof(CanRxMsgTypeDef)*HW_CAN_RX_LIST_SIZE);
  //irq_enable(CAN1_RX0_IRQn);
}
int hw_can_rx_list_length(void){return g_rxLen;}
CanRxMsgTypeDef * hw_can_rx_list_message(int index)
{
  if(index>=0 && index<g_rxLen){
    return &(g_rxList[index]);
  }
  return NULL;
}

