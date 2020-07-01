#include "move_sensor_speed.h"
#include "hw_gpio.h"
#include "board.h"
#include "my_misc.h"

#define MOVE_SPEED_SENSOR_INTERVAL_MS (2*1000)
static struct device *g_gpio_speed_status;   //EXT_IN_GPIO_PIN2
static sensorData_t movData;
static bool origbuff[10] = {0};

static struct gpio_callback gpio_speed_callback;
static uint16_t speedPulseCnt = 0;
static uint32_t gSpeed = 0;
static uint16_t wheelD = 0;

static void speedPulseCallback(struct device *port,struct gpio_callback *cb,u32_t pins);

bool moveSpeedSetup(gpSensorTpye_t* conf)
{
    print_log("moveSpeedSetup \n ");
    g_gpio_speed_status = hwGpioPinInit(EXT_IN_GPIO_PORT, EXT_IN_GPIO_PIN2, GPIO_INT | GPIO_INT_EDGE| GPIO_INT_ACTIVE_LOW);
	if (!g_gpio_speed_status) 
    {
		print_log("GPIO speed configure failed.\n");
		return false;
	}
    gpio_init_callback(&gpio_speed_callback,speedPulseCallback,BIT(EXT_IN_GPIO_PIN2));
    print_log("speed sensor init success\n");
    gpio_add_callback(g_gpio_speed_status, &gpio_speed_callback);
    gpio_pin_enable_callback(g_gpio_speed_status,EXT_IN_GPIO_PIN2);
    memset(&movData,0,sizeof(sensorData_t));
    speedPulseCnt = 0;
    //TODO: need config !!!!
    wheelD = (*conf).move_threshold;//in "mm",only foar baiwei test
    return true;
}
void moveSpeedRun(void)
{
    static s64_t last_call_time = 0;
    s64_t current_time = k_uptime_get();
    uint32_t diff = current_time -last_call_time;
    print_log("############wheelD[%d]mm,speedPulseCnt = %d,speed = %d m/h\n ",wheelD,speedPulseCnt,gSpeed);
    if(diff >= MOVE_SPEED_SENSOR_INTERVAL_MS)
    {
        last_call_time = current_time;
        gSpeed = speedPulseCnt * 3.14 * wheelD /1000.0/ (diff / 1000.0 / 3600.0);
        print_log("gSpeed[%d]##############\n",gSpeed);
        speedPulseCnt = 0;
    }
    movData.moveLevel = MOVE_LEVEL_DISABLE;
    if(gSpeed){movData.moveStat = MOVE_FORWARD;}
    else{movData.moveStat = MOVE_STOP;}
    movData.speed = gSpeed;

}
void getmoveSpeedData(sensorData_t* data)
{
     *data =  movData;
}

uint32_t getMoveSpeedRunInterval(void)
{
    return MOVE_SPEED_SENSOR_INTERVAL_MS;
}

static void speedPulseCallback(struct device *port,struct gpio_callback *cb,u32_t pins)
{
    //print_log("1111111111111111111111111111111111\n");
    if(speedPulseCnt <= 0xffff)
    {
        speedPulseCnt++;
    }

}

void getMoveSpeedStatus(uint32_t* counter,int32_t* speed,uint32_t* wheel)
{
    *counter = speedPulseCnt;
    *speed = gSpeed;
    *wheel = wheelD;

}

