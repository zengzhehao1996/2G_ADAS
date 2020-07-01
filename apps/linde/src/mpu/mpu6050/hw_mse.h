#ifndef __HW_MSE_H__
#define __HW_MSE_H__
#include "hw_mpu6050.h"

#define hwMseInit() hwMpu6050Init()
#define hwMseGetGyroscope(x) hwMpu6050GetGyroscope(x)
#define hwMseGetAcceleration(x) hwMpu6050GetAcceleration(x)


#endif /* __HW_MSE_H__ */