#include "hw_mpu6050.h"
#include "my_misc.h"

#include <kernel.h>
#include <i2c.h>
#include <zephyr.h>
#include <string.h>

#define MPU6050_I2C_ADDR            0x68
#define MPU6050_REG_WHOAMI          0x75
#define MPU6050_REG_WHOAMI_VAL      0x68
#define MPU6050_RA_GYRO_XOUT_H      0x43
#define MPU6050_RA_ACCEL_XOUT_H     0x3B
#define I2C_DEV_NAME CONFIG_I2C_1_NAME
#define MPU6050_DATA_SIZE 14
static uint8_t g_i2c_rx_buffer[MPU6050_DATA_SIZE];

static union dev_config i2c_cfg = {
    .raw = 0,
    .bits = {
        .use_10_bit_addr = 0,
        .is_master_device = 1,
        //.speed = I2C_SPEED_FAST,
        .speed = I2C_SPEED_STANDARD,
    },
};
static struct device *g_i2c_dev=NULL;

/*init the mpu6050 config*/
bool hwMpu6050Init(void)
{
    uint8_t datas[MPU6050_DATA_SIZE];

    //step1.get dev
    g_i2c_dev = device_get_binding(I2C_DEV_NAME);
    if(g_i2c_dev == NULL)
    {
        err_log("Fail to init i2c device.\n");
        return false;
    }

    /* 1. Verify i2c_configure() */
    if(i2c_configure(g_i2c_dev, i2c_cfg.raw))
    {
        err_log("I2C config failed\n");
        return false;
    }

    /* 2. Verify i2c_write() */
    datas[0] = 0x6B;
    datas[1] = 0x00;
    if(i2c_write(g_i2c_dev, datas, 2, MPU6050_I2C_ADDR))
    {
        err_log("Fail to start sensor MPU6050\n");
        return false;
    }

    k_sleep(10);

    datas[0] = MPU6050_REG_WHOAMI;
    if(i2c_write(g_i2c_dev, datas, 1, MPU6050_I2C_ADDR))
    {
        err_log("Fail to write to sensor MPU6050\n");
        return false;
    }

    /* 3.1 Verify i2c_read() */
    memset(datas, 0, sizeof(datas));
    if(i2c_read(g_i2c_dev, datas, 1, MPU6050_I2C_ADDR))
    {
        err_log("Fail to fetch sample from sensor GY271\n");
        return false;
    }

    print_log("Read MPU6050 Register WHOAMI (0x75): %02x, expteted: %02x\n", datas[0],
              MPU6050_REG_WHOAMI_VAL);
    if(datas[0] != MPU6050_REG_WHOAMI_VAL)
    {
        err_log("Fail to init MPU6050.");
        return false;
    }

    return true;
}

/*read mpu6050`s Gyroscope data*/
bool hwMpu6050GetGyroscope(mseGyroscope_t* gyroData)
{
    memset(g_i2c_rx_buffer,0,6);
    if(i2c_burst_read(g_i2c_dev,MPU6050_I2C_ADDR,MPU6050_RA_GYRO_XOUT_H,g_i2c_rx_buffer,6)){
        err_log("Fail to read data from MPU6050.\n");
        return false;
    }
    gyroData->gyroX = (((int16_t)g_i2c_rx_buffer[0]) << 8) | g_i2c_rx_buffer[1];
    gyroData->gyroY = (((int16_t)g_i2c_rx_buffer[2]) << 8) | g_i2c_rx_buffer[3];
    gyroData->gyroZ = (((int16_t)g_i2c_rx_buffer[4]) << 8) | g_i2c_rx_buffer[5];

    return true;
}

/*read mpu6050`s Acceleration data*/
bool hwMpu6050GetAcceleration(mseAcceleration_t* acceData)
{
    memset(g_i2c_rx_buffer,0,6);
    if(i2c_burst_read(g_i2c_dev,MPU6050_I2C_ADDR,MPU6050_RA_ACCEL_XOUT_H,g_i2c_rx_buffer,14)){
        err_log("Fail to read data from MPU6050.\n");
        return false;
    }
    acceData->acceX = (((int16_t)g_i2c_rx_buffer[0]) << 8) | g_i2c_rx_buffer[1];
    acceData->acceY = (((int16_t)g_i2c_rx_buffer[2]) << 8) | g_i2c_rx_buffer[3];
    acceData->acceZ = (((int16_t)g_i2c_rx_buffer[4]) << 8) | g_i2c_rx_buffer[5];

    return true;
}

#if 0 //usually is 0, run test cast changed to 1
void test_case_mpu6050(void)
{
    mseAcceleration_t tmpAcc;
    mseGyroscope_t    tmpGyr;

    if(!hwMpu6050Init())
    {
        err_log("Init mpu6050 Failed!\n");
    }

    while(1)
    {
        memset(&tmpAcc,0,sizeof(tmpAcc));
        memset(&tmpGyr,0,sizeof(tmpGyr));
        if(hwMpu6050GetGyroscope(&tmpGyr))
        {
            print_log("Gyro:%6d, %6d, %6d\n",tmpGyr.gyroX,tmpGyr.gyroY,tmpGyr.gyroZ);
        }
        else
        {
            warning_log("Read failed Gyroscope.\n");
        }
        if(hwMpu6050GetAcceleration(&tmpAcc))
        {
            print_log("Acce:%6d, %6d, %6d\n",tmpAcc.acceX,tmpAcc.acceY,tmpAcc.acceZ);
        }
        else
        {
            warning_log("Read failed Acceleration.\n");
        }
        
        k_sleep(500);
    }
}
#endif