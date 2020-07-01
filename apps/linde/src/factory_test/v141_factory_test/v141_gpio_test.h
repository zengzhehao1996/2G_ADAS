#ifndef __V141_GPIO_TEST_H__
#define __V141_GPIO_TEST_H__

#include <kernel.h>

// Initialize the gpio that reads the external state
bool linde_gpio_v14_setup();

/*
 * @return: FALSE,seat off; TRUE,seat on
 */
bool gpio_get_seat_status();
/*
 * @return: FALSE,hydraulic off; TRUE,hydraulic on
 */
bool gpio_get_hydraulic_status();
/*
 * @return: FALSE,fork forward off; TRUE,fork forward on
 */
bool gpio_get_forward_status();
/*
 * @return: FALSE,fork backward off; TRUE,fork backward on
 */
bool gpio_get_backward_status();

////////////////////////////////
bool gpio_get_gpint_in0_status();
bool gpio_get_gpint_in1_status();

#endif