#ifndef __HW_GPIO_H__
#define __HW_GPIO_H__

void hw_gpio_init(void);
struct device *gpio_pin_init(const char *port_name, int pin, int dir);

//int hw_seat_stat(void);

#endif
