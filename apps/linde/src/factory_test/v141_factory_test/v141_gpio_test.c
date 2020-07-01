#include "hw_gpio.h"
#include "v141_gpio_test.h"
#include "my_misc.h"
#include <board.h>

//all of 4pin port is EXT_IN_GPIO_PORT
static struct device *g_gpio_seat_status;      //EXT_IN_GPIO_PIN0
static struct device *g_gpio_hydraulic_status; //EXT_IN_GPIO_PIN1
static struct device *g_gpio_forward_status;   //EXT_IN_GPIO_PIN2
static struct device *g_gpio_backward_status;  //EXT_IN_GPIO_PIN3

static struct device *g_gpio_in0;   //GPINT_IN0
static struct device *g_gpio_in1;  //GPINT_IN1
bool linde_gpio_v14_setup()
{
  g_gpio_seat_status = hwGpioPinInit(EXT_IN_GPIO_PORT, EXT_IN_GPIO_PIN0, GPIO_DIR_IN);
	if (!g_gpio_seat_status) {
		print_log("GPIO Seat configure failed.\n");
		return FALSE;
	}

  g_gpio_hydraulic_status = hwGpioPinInit(EXT_IN_GPIO_PORT, EXT_IN_GPIO_PIN1, GPIO_DIR_IN);
	if (!g_gpio_hydraulic_status) {
		print_log("GPIO Hydraulic configure failed.\n");
		return FALSE;
	}

  g_gpio_forward_status = hwGpioPinInit(EXT_IN_GPIO_PORT, EXT_IN_GPIO_PIN2, GPIO_DIR_IN);
	if (!g_gpio_forward_status) {
		print_log("GPIO Forward configure failed.\n");
		return FALSE;
	}

  g_gpio_backward_status = hwGpioPinInit(EXT_IN_GPIO_PORT, EXT_IN_GPIO_PIN3, GPIO_DIR_IN);
	if (!g_gpio_backward_status) {
		print_log("GPIO Backward configure failed.\n");
		return FALSE;
	}
///////////////////////
  g_gpio_in0 = hwGpioPinInit(EXT_INT_GPIO_PORT, EXT_INT_GPIO_PIN0, GPIO_DIR_IN);
	if (!g_gpio_in0) {
		print_log("GPIO GPINT_IN0 configure failed.\n");
		return FALSE;
	}

  g_gpio_in1 = hwGpioPinInit(EXT_INT_GPIO_PORT, EXT_INT_GPIO_PIN1, GPIO_DIR_IN);
	if (!g_gpio_in1) {
		print_log("GPIO GPINT_IN1 configure failed.\n");
		return FALSE;
	}
    

  return TRUE;
}

bool gpio_get_seat_status()
{
  int val = 0;
  gpio_pin_read(g_gpio_seat_status,EXT_IN_GPIO_PIN0, &val);
  if(val == 0){
    return FALSE;
  }else{
    return TRUE;
  }
}
bool gpio_get_hydraulic_status()
{
  int val = 0;
  gpio_pin_read(g_gpio_hydraulic_status,EXT_IN_GPIO_PIN1, &val);
  if(val == 0){
    return FALSE;
  }else{
    return TRUE;
  }
}
bool gpio_get_forward_status()
{
  int val = 0;
  gpio_pin_read(g_gpio_forward_status,EXT_IN_GPIO_PIN2, &val);
  if(val == 0){
    return FALSE;
  }else{
    return TRUE;
  }
}
bool gpio_get_backward_status()
{
  int val = 0;
  gpio_pin_read(g_gpio_backward_status,EXT_IN_GPIO_PIN3, &val);
  if(val == 0){
    return FALSE;
  }else{
    return TRUE;
  }
}
////////////////////////////////
bool gpio_get_gpint_in0_status()
{
  int val = 0;
  gpio_pin_read(g_gpio_in0,EXT_INT_GPIO_PIN0, &val);
  if(val == 0){
    return FALSE;
  }else{
    return TRUE;
  }
}
bool gpio_get_gpint_in1_status()
{
  int val = 0;
  gpio_pin_read(g_gpio_in1,EXT_INT_GPIO_PIN1, &val);
  if(val == 0){
    return FALSE;
  }else{
    return TRUE;
  }
}