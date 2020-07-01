#ifndef _V141_ADC_TEST_H__
#define _V141_ADC_TEST_H__
#include <stdbool.h>

//init adc pin
bool volt_setup(void);
//Acquisition of on-board capacitors or battery voltages
int volt_get_bat(void);
//Get input power supply voltage
int volt_get_power(void);
//Get battery voltage of forklift truck
int volt_get_bat_ext(void);
//Get the key door voltage of the forklift truck
int volt_get_key_ext(void);
//Get the hydraulic sensor voltage of forklift truck
int volt_get_hydraulic_ext(void);
//Get forward throttle sensor voltage of forklift truck
int volt_get_forward_ext(void);
// Get backward throttle sensor voltage of forklift truck
int volt_get_backward_ext(void);
// Reserve, Get adc in3 voltage
int volt_get_adc_seat(void);

#endif //HW_VOLTAGE
