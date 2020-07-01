#ifndef __HW_MSE_H__
#define __HW_MSE_H__
#include "hw_bmi160.h"

#define hwMseInit() hwBmi160Init()
#define hwMseGetGyroscope(x) hwBmi160GetGyroscope(x)
#define hwMseGetAcceleration(x) hwBmi160GetAcceleration(x)


#endif /* __HW_MSE_H__ */