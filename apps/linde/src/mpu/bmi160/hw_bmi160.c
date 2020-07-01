/*******************************************************************************************
 *Copyright:	2015-2025, AIDONG Science & Technology Co., Ltd (AIDONG SUPER AI)
 *Module Name:	BMI160 DRIVE IMPLEMENTATION
 *Module Date:	2019/1/30
 *Module Auth:	liufeng && guowei
 *Module Description: The purpose of this module is to driver the BMI160 sensor.
 *This module provides three function functions. The first one initializing BMI160 sensor; 
 *the second one output gyro ternary; the third one output acceleration ternary.				
 *Revision History:	Liufeng maintained this module before 2019. From then on, 
 *guowei has since maintained the module. 
********************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include "hw_bmi160.h"
#include "my_misc.h"
#include <kernel.h>
#include <i2c.h>
#include <zephyr.h>
#include <string.h>
#define BMI160_INIT_DELAY_TIME 50
#define I2C_DEV_NAME CONFIG_I2C_1_NAME
#define BMI160_DATA_SIZE 6

static uint8_t BMI160_I2C_ADDR = 0x69; /* More equipment in the future 69 */
static uint8_t i2cRxBuffer[BMI160_DATA_SIZE];

/*used for i2c configure*/
static union dev_config i2cConfig = 
{
    .raw = 0,
    .bits = 
	{
        .use_10_bit_addr = 0,
        .is_master_device = 1,
        //.speed = I2C_SPEED_FAST,
        .speed = I2C_SPEED_STANDARD,
	},
};

static struct device *i2cDevice = NULL;


/*****************************************************************************
 *Function:		hwBmi160Init
 *Description:	Initialize and configuration the BMI160 sensor
 *Calls:		None
 *Calls By:		threadMpuStart
 *Input:		void
 *Output:		void
 *Return:		If configuration success return "true", else return "false".
 *Others:		None
******************************************************************************/
bool hwBmi160Init(void)
{
    uint8_t datas[BMI160_DATA_SIZE];

    /*get the device's name*/
    i2cDevice = device_get_binding(I2C_DEV_NAME);
	
    if(i2cDevice == NULL)
	{
        err_log("Fail to init i2c device.\n");
		
        return false;
	}

    /*verify i2c_configure() is correct*/
    if (i2c_configure(i2cDevice, i2cConfig.raw)) 
    {
        err_log("I2C config failed\n");
		
        return false;
	}

    /*BMI160`s address is 0x69*/
    datas[0] = BMI160_REG_WHOAMI;
    if(i2c_write(i2cDevice,datas,1,BMI160_I2C_ADDR)) 
    {
        BMI160_I2C_ADDR = 0x68;
        if(i2c_write(i2cDevice,datas,1,BMI160_I2C_ADDR))
        {
            err_log("Fail to write to sensor BMI160\n");
            return false;
        }
    }
    print_log("BMI160 Addr is 0x%02x\n",BMI160_I2C_ADDR);
  
    /*verify i2c_read() is correct*/
    memset(datas, 0, sizeof(datas));
    if (i2c_read(i2cDevice, datas, 1, BMI160_I2C_ADDR)) 
    {
        err_log("Fail to fetch sample from sensor GY271\n");
        return false;
    }
    
    print_log("Read BMI160 Register WHOAMI (0xD1): %02x, expteted: %02x\n",\
			datas[0], BMI160_REG_WHOAMI_VAL);
	
    if (datas[0] != BMI160_REG_WHOAMI_VAL)
    {
        err_log("Fail to init BMI160.");
        return false;
    }

    /*configure acceleration output data rate*/
    datas[0] = BMI160_ACC_OUTPUT;
    if(i2c_burst_write(i2cDevice, BMI160_I2C_ADDR, BMI160_ACC_CONF, datas, 1))
    {
        err_log("Fail to config acc output data rate.\n");
        return false;
	}
    k_sleep(BMI160_INIT_DELAY_TIME);

    /*enable acceleration*/
    datas[0] = BMI160_EN_ACC;
    if(i2c_burst_write(i2cDevice, BMI160_I2C_ADDR, BMI160_CMD_REG, datas, 1))
    {
        err_log("Fail to config acc.\n");
        return false;
    }
    k_sleep(BMI160_INIT_DELAY_TIME);
    
    /*configure gyroscope range*/ 
    datas[0] = BMI160_GRY_RANGE;
    if(i2c_burst_write(i2cDevice, BMI160_I2C_ADDR, BMI160_GRY_RANGE_REG, datas, 1))
    {
        err_log("Fail to config acc.\n");
        return false;
    }
    k_sleep(BMI160_INIT_DELAY_TIME);
	
    /*configure gyroscope output data rate*/
    datas[0] = BMI160_GYR_OUTPUT;
    if(i2c_burst_write(i2cDevice, BMI160_I2C_ADDR, BMI160_GYR_CONF, datas, 1))
    {
        err_log("Fail to config acc output data rate.\n");
        return false;
    }
    k_sleep(BMI160_INIT_DELAY_TIME);

    /*enable AGyr*/
    datas[0] = BMI160_EN_GYR;
    if(i2c_burst_write(i2cDevice, BMI160_I2C_ADDR, BMI160_CMD_REG, datas, 1))
    {
        err_log("Fail to config acc.\n");
        return false;
    }
    k_sleep(BMI160_INIT_DELAY_TIME);

    return true;
}

/****************************************************************
 *Function:		hwBmi160GetGyroscope
 *Description:	Read Gyroscope Registers of BMI160
 *Calls:		None
 *Calls By:		thread_mpu_run
 *Input:		void
 *Output:		void
 *Return:		return a struct of bmi160Gyroscope
 *Others:		None
*****************************************************************/
bool hwBmi160GetGyroscope(mseGyroscope_t * gyroData)
{
	//print_log("hwBmi160GetGyroscope\n");
    memset(i2cRxBuffer,0,6);
  
    if(0 != i2c_burst_read(i2cDevice, BMI160_I2C_ADDR, BMI160_RA_GYRO_XOUT_L, i2cRxBuffer, 6))
    {
        err_log("Fail to read data from BMI160_Gyroscope.\n");
        return false;
    }

    gyroData->gyroX = (((int16_t)i2cRxBuffer[1]) << 8) | i2cRxBuffer[0];
    gyroData->gyroY = (((int16_t)i2cRxBuffer[3]) << 8) | i2cRxBuffer[2];
    gyroData->gyroZ = (((int16_t)i2cRxBuffer[5]) << 8) | i2cRxBuffer[4];

    return true;
}


/***************************************************************
 *Function:		hwBmi160GetAcceleration
 *Description:	Read Acceleration Registers of BMI160
 *Calls:		None
 *Calls By:		thread_mpu_run
 *Input:		void
 *Output:		void
 *Return:		return a struct of bmi160Acceleration
 *Others:		None
****************************************************************/
bool hwBmi160GetAcceleration(mseAcceleration_t * acceData)
{
	//print_log("hwBmi160GetAcceleration\n");
    memset(i2cRxBuffer,0,6);
  
    if(0 != i2c_burst_read(i2cDevice, BMI160_I2C_ADDR, BMI160_RA_ACCEL_XOUT_L, i2cRxBuffer, 6))
    {
        err_log("Fail to read data from BMI160_Gyroscope.\n");
        return false;
    }

    acceData->acceX = (((int16_t)i2cRxBuffer[1]) << 8) | i2cRxBuffer[0];
    acceData->acceY = (((int16_t)i2cRxBuffer[3]) << 8) | i2cRxBuffer[2];
    acceData->acceZ = (((int16_t)i2cRxBuffer[5]) << 8) | i2cRxBuffer[4];

    return true;
}


#ifdef __cplusplus

}

#endif

