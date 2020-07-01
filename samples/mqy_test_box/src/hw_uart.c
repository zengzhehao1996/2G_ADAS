#include <string.h>
#include <stdio.h>
#include <misc/printk.h>
#include <device.h>
#include <gpio.h>
#include <uart.h>
#include <misc/util.h>

#include "hw_uart.h"
#include "my_misc.h"

#define UART_PORT_6		"UART_6"

#define BUF_MAXSIZE		2048
struct uart_rx_buffer {
	u8_t buf[BUF_MAXSIZE];
	int  pos;
};

static struct device *uart_6_dev;
static struct uart_rx_buffer uart_6_buf;

static struct device *gpio_pin_init(const char *port_name, int pin, int dir, int val)
{
	struct device *dev;
	
	dev = device_get_binding(port_name);
	if (!dev) {
		printk("Cannot find %s!\n", port_name);
		return NULL;
	}

	if (gpio_pin_configure(dev, pin, dir)) {
		printk("Configure GPIO %s PIN %d DIRECTION %d failed\n",
			port_name, pin, dir);
		return NULL;
	}
	
	if (dir == GPIO_DIR_IN)
		return dev;

	gpio_pin_write(dev, pin, val);
	return dev;
}

static int uart_send(struct device *uart_dev, const char *buf, int len)
{
	int i;
	unsigned char sent_char;

	for (i = 0; i < len; i++) {
		sent_char = uart_poll_out(uart_dev, buf[i]);

		if (sent_char != buf[i]) {
			printk("expect send %c, actaul send %c\n",
				buf[i], sent_char);
			return -1;
		}
	}

	return i;
}

static void uart_6_isr(struct device *x)
{
	struct uart_rx_buffer *rbuf = &uart_6_buf;
	int pos, len;

	ARG_UNUSED(x);

	pos = rbuf->pos;
	len = uart_fifo_read(uart_6_dev, &rbuf->buf[pos], BUF_MAXSIZE - pos);
	rbuf->pos += len;
	if (rbuf->pos == BUF_MAXSIZE)
		uart_irq_rx_disable(uart_6_dev);
}

static void reset_uart_rx_buf(struct uart_rx_buffer *pbuf)
{
  pbuf->pos = 0;
  memset(pbuf->buf, 0, sizeof(pbuf->buf));
  return;
}

static void data_dump(const char *buf, int len)
{
	int i;
  if(!buf || len <=0){return ;}
  printk("UART6 RX [%d] bytes: ",len);
	for (i = 0; i < len; i++) {
		printk("0x%02X ", buf[i]);
	}
	printk("\n");
  
}

int hw_uart_init()
{
  //init uart 6 device
  if ((uart_6_dev = device_get_binding(UART_PORT_6)) == NULL) {
  printk("Fail to bind U(S)ART device %s\n", UART_PORT_6);
  return -1;
	}
  //set uart6 callback function
  uart_irq_callback_set(uart_6_dev, uart_6_isr);
  //enable uart6 device
  uart_irq_rx_enable(uart_6_dev);

  return 0;
}

int hw_uart_send_bytes(enum hw_uart_num num, const uint8_t *data, int len)
{
  int write_len = 0;
	if(!data || len <= 0){return -1;}  //Invalid parameter return

	switch(num){
  case HW_UART6:
	  if(uart_6_dev){
			write_len = uart_send(uart_6_dev, data, len);
		}
		break;
	default:
		err_log("ERROR: No device");
    break;
	}

	return write_len;
}

int hw_uart_read_bytes(enum hw_uart_num num, uint8_t *data, int size)
{
	int read_len = 0;
	if(!data || size <= 0){return -1;} //Invalid parameter return

	switch(num){
	case HW_UART6:
	  if(uart_6_dev){
			uart_irq_rx_disable(uart_6_dev);
			read_len = size < uart_6_buf.pos ? size : uart_6_buf.pos;
			memcpy(data,uart_6_buf.buf, read_len);
			reset_uart_rx_buf(&uart_6_buf);
			uart_irq_rx_enable(uart_6_dev);
		}
		break;
	default:
	  err_log("ERROR: No device.\n");
	  break;
	}

	return read_len;
}
int hw_uart_rx_buff_num(void)
{
    return uart_6_buf.pos;
}