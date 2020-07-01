#ifndef __HW_MPU6050_H__
#define __HW_MPU6050_H__
#include <stdint.h>
#include <stdbool.h>

/*
 *define struct used for Gyroscope
 *if any one want to get Gyroscope data
 *he must define this struct first
*/
struct mpu6050Gyroscope
{
    int16_t gyroX;
    int16_t gyroY;
    int16_t gyroZ;
};

/*
 *define struct used for Acceleration
 *if any one want to get Acceleration data
 *he must define this struct first
*/
struct mpu6050Acceleration
{
    int16_t acceX;
    int16_t acceY;
    int16_t acceZ;
};

typedef struct mpu6050Gyroscope    mseGyroscope_t;
typedef struct mpu6050Acceleration mseAcceleration_t;

/*init the mpu6050 config*/
bool hwMpu6050Init(void);

/*read mpu6050`s Gyroscope data*/
bool hwMpu6050GetGyroscope(mseGyroscope_t* gyroData);

/*read mpu6050`s Acceleration data*/
bool hwMpu6050GetAcceleration(mseAcceleration_t* acceData);

#endif /* __HW_MPU6050_H__ */