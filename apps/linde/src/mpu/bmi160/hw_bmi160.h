/********************************************************************************************
 *Copyright:	2016--2025, AIDONG Science & Technology Co., Ltd (AIDONG SUPER AI)
 *Module Name:	BMI160 DRIVE IMPLEMENTATION
 *Module Date:	2019/1/30
 *Module Auth:	liufeng && guowei
 *Module Description: The purpose of this module is to driver the BMI160 sensor.
 *This module provides three function functions. The first one initializing BMI160 sensor;
 *the second one output gyro ternary; the third one output acceleration ternary.				
 *Revision History:	Liufeng maintained this module before 2019. From then on, 
 *guowei has since maintained the module. 
********************************************************************************************/

#ifndef _HW_BMI160_H_
#define _HW_BMI160_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/*The register contains the chip identification code.*/
#define BMI160_REG_WHOAMI     0x00

/*CHIP_ID*/
#define BMI160_REG_WHOAMI_VAL 0xD1

/*X axis Gyr data; resigter address:0x0C; register name: DATA_8; read only*/
#define BMI160_RA_GYRO_XOUT_L 0x0C

/*X axis Acc data; resigter address:0x0x12; register name: DATA_14; read only*/
#define BMI160_RA_ACCEL_XOUT_L 0x12

/*register name CMD; default value:0x00; read & write*/
#define BMI160_CMD_REG 0x7E

/*configure gyroscope to work statu*/
#define BMI160_EN_GYR 0x15

/*configure acceleration to work status*/
#define BMI160_EN_ACC 0x11

//acc output data rate config
#define BMI160_ACC_CONF 0x40
#define BMI160_ACC_OUTPUT 0x2A

//gry output data rate config
#define BMI160_GYR_CONF 0x42
#define BMI160_GYR_OUTPUT 0x2A


/*register name:PMU_STATUS; default value:0x00; read only*/
#define BMI160_GRY_RANGE 0x03

/*gyroscope range register*/
#define BMI160_GRY_RANGE_REG 0x43


/*
 *define struct used for Gyroscope
 *if any one want to get Gyroscope data
 *he must define this struct first
*/
struct bmi160Gyroscope
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
struct bmi160Acceleration
{
    int16_t acceX;
    int16_t acceY;
    int16_t acceZ;
};

typedef struct bmi160Gyroscope mseGyroscope_t;
typedef struct bmi160Acceleration mseAcceleration_t;

/*init the bmi160 config*/
bool hwBmi160Init(void);

/*read bmi160`s Gyroscope data*/
bool hwBmi160GetGyroscope(mseGyroscope_t * gyroData);

/*read bmi160`s Acceleration data*/
bool hwBmi160GetAcceleration(mseAcceleration_t * acceData);

#ifdef __cplusplus
}

#endif

#endif

