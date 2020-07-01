#ifdef __cplusplus
extern "C" {
#endif
#include "hw_rs485.h"

#include <zephyr.h>
#include <string.h>
#include <kernel.h>
#include <uart.h>
#include <gpio.h>
#include "my_misc.h"

static struct device *g_rs485_dev = NULL;
static void hw_uarts_rs485_isr(struct device *x);
void rs485_direction_setDirection(bool is_tx);

#define UART_PORT_RS485 "UART_7"

#define RS485_MAXSIZE 50
#define RS485_RXSIZE 50

static uint8_t g_rx_buffer[RS485_RXSIZE];
static int g_rx_len=0;

#define RS485_DE_PORT "GPIOE"
#define RS485_DE_PIN 9

static struct device *g_rs485_de = NULL;


#define RS485_TX_MODE 1
#define RS485_RX_MODE 0
bool hw_rs485_init(){
	//init rs485 device
  if((g_rs485_dev = device_get_binding(UART_PORT_RS485)) == NULL) {
    err_log("Fail to get RS485 device %s\n", UART_PORT_RS485);
    return false;
  }

	//init rs485 de pin
  g_rs485_de = device_get_binding(RS485_DE_PORT);
	if(g_rs485_de==NULL){
		err_log("Fail to init rs485de pin.\n");
		return false;
	}
	gpio_pin_configure(g_rs485_de, RS485_DE_PIN, GPIO_DIR_OUT);
  gpio_pin_write(g_rs485_de, RS485_DE_PIN, RS485_RX_MODE);

	//init rx work
	g_rx_len = 0;
	
  uart_irq_callback_set(g_rs485_dev, hw_uarts_rs485_isr);
  uart_irq_rx_disable(g_rs485_dev);
  uart_irq_rx_enable(g_rs485_dev);
  return true;
}

void hw_uarts_rs485_isr(struct device *x){
  ARG_UNUSED(x);
	if(g_rx_len<RS485_RXSIZE){
		int len =  uart_fifo_read(g_rs485_dev,
					                    &g_rx_buffer[g_rx_len],
					                    RS485_MAXSIZE-g_rx_len);
		g_rx_len += len;
	}
    else
    {
        uart_irq_rx_disable(g_rs485_dev);
        err_log("RS485 isr over,rs485 rx buff is full.\n");

    }
}

void hw_rs485_send(uint8_t *data,int len){
  if(data==NULL || len<=0)return;

	int i=0;
    uint8_t sent_char;
	gpio_pin_write(g_rs485_de, RS485_DE_PIN, RS485_TX_MODE);
  
  for (i = 0; i < len; i++) {
    sent_char = uart_poll_out(g_rs485_dev, data[i]);

    if (sent_char != data[i]) {
      err_log("expect send %c, actaul send %c\n", data[i], sent_char);
      break;
    }
  }
	k_sleep(5);
	gpio_pin_write(g_rs485_de, RS485_DE_PIN, RS485_RX_MODE);
}

int  hw_rs485_read(uint8_t * out_data,int size){
  int ret=0;
  uart_irq_rx_disable(g_rs485_dev);

  ret = g_rx_len;
  memcpy(out_data, g_rx_buffer,g_rx_len);
  g_rx_len = 0;
  uart_irq_rx_enable(g_rs485_dev);
  return ret;
}

int  hw_get_rs485_rx_buff_num()
{
    return g_rx_len;
}

#ifdef __cplusplus
}
#endif

