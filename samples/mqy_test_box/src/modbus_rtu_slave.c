#include "modbus_rtu_slave.h"
#include "crc16.h"
#include "hw_rs485.h"
#include "my_misc.h"
#include "kernel.h"


#define MODBUS_DATA_BUFF (5)
static uint16_t modbusBuff[MODBUS_DATA_BUFF] = {0,0,0,0,0};
static bool startFlag = false;
static  struct k_mutex modbusBufMux;

static inline uint16_t modbusSwapEndian( uint16_t data ) { return ( data << 8 ) | ( data >> 8 ); }

static void modbus03Pars(uint8_t* data,uint16_t lenth);
static void modbus16Pars(uint8_t* data,uint16_t lenth);

bool modbusPars(uint8_t* data,uint16_t lenth)
{
    if(lenth < 6 || !data)
    {
        print_log("lenth [%d] < 6 or data is NULL\n",lenth);
        return false;        
    }
    #if 0
    print_log("data:\n");
    for(int i = 0;i < lenth;i++)
    {
        printk("%02x ",data[i]);
    }
    printk("\n");
    #endif
    if(data[0] != SLAVE_ADDR)
    {
        print_log("slave addr is err[%d]\n",data[0]);
    }
    uint8_t function = data[1];
    //ModbusParser dataPars;
    //print_log("slave addr [%d],fun[%d]..\n",data[0],function);
    switch(function)
    {
        case MODBUS_CMD_03:
        {
            modbus03Pars(data,lenth);
            break;

        }
        case  MODBUS_CMD_16:
        {
            modbus16Pars(data,lenth);
            break;
        }
        default:
            print_log("cmd = %d\n",function);
        break;
    }
    
}

static void modbus03Pars(uint8_t* data,uint16_t lenth)
{
    ModbusParser dataPars;
    memcpy(&dataPars,data,lenth);
    //check crc
    uint16_t crcRcv = dataPars.request0304.crc;
   // print_log(" rcv crc = %04x\n",crcRcv);
    uint16_t dataLen = 6;
    uint16_t crc = modbus_crc16(data,dataLen);
   // print_log("crc = %04x\n",crc);
    if(crcRcv != crc)
    {
        print_log("crc err:crcRcv[%04x],crc[%4x]\n",crcRcv,crc);
        return;
    }
    //2.
    dataPars.request0304.index = modbusSwapEndian(dataPars.request0304.index);
    dataPars.request0304.count = modbusSwapEndian(dataPars.request0304.count);
    //print_log("requst reg_start is %04x\n",dataPars.request0304.index);
    //print_log("requst count is %04x\n",dataPars.request0304.count);
    //  pack payload
    uint8_t buf[40] = {0};
    buf[0] = SLAVE_ADDR;
    buf[1] = MODBUS_CMD_03;
    buf[2] = dataPars.request0304.count * 2;// byte num

    uint16_t modbusBuffTmp[MODBUS_DATA_BUFF] = {0};
    if(modbusLock())
    {
        for(int i = 0;i< MODBUS_DATA_BUFF;i++)
        {
            modbusBuffTmp[i] = modbusSwapEndian(modbusBuff[i]);
        }
        modbusUnlock();

    }
    else
    {
        print_log("get lock failed\n");
        return false;

    }

    memcpy(&buf[3],&modbusBuffTmp[dataPars.request0304.index],dataPars.request0304.count * 2);
    crc = modbus_crc16(buf,1+1+1+dataPars.request0304.count * 2);
    memcpy(&buf[3+dataPars.request0304.count * 2],&crc,sizeof(crc));
    #if 0
    print_log("modbus 03 response:\n");
    for(int i = 0;i < 1+1+1+dataPars.request0304.count * 2 +2;i++)
    {
        printk("%02x ",buf[i]);
    }
    printk("\n");
    #endif
    k_sleep(5);
    hw_rs485_send(buf,1+1+1+dataPars.request0304.count * 2 +2);
    
    
    
    
}
static void modbus16Pars(uint8_t* data,uint16_t lenth)
{
    ModbusParser dataPars;
    uint16_t indexTmp = 0;
    memcpy(&dataPars,data,lenth);
    //check crc
    dataPars.request16.crc = (data[7+data[6]] << 8 | data[8+data[6]]);
    uint16_t crcRcv = dataPars.request16.crc;
    //print_log(" modbus16 rcv crc = %04x\n",crcRcv);
    //uint16_t dataLen = 7+ data[6] ;
    uint16_t dataLen = 7+ dataPars.request16.length ;
   // print_log("modbus16 num =%d\n",dataLen);
    uint16_t crc = modbusSwapEndian(modbus_crc16(data,dataLen));
    //print_log("crc = %04x\n",crc);
    if(crcRcv != crc)
    {
        print_log("crc err:crcRcv[%04x],crc[%4x]\n",crcRcv,crc);
        return;
    }
    //2.
    //memcpy(&dataPars,data,dataLen);
    dataPars.request16.index = modbusSwapEndian(dataPars.request16.index);
    dataPars.request16.count = modbusSwapEndian(dataPars.request16.count);
    //dataPars.request16.length = data[6];
    //print_log("requst reg_start is %04x,count is %04x,lenth is %02x\n,",
     //          dataPars.request16.index,dataPars.request16.count,dataPars.request16.length);

        //  pack ack payload
        uint8_t buf[8] = {0};
        buf[0] = SLAVE_ADDR;
        buf[1] = MODBUS_CMD_16;
        indexTmp  = modbusSwapEndian(dataPars.request16.index);
        memcpy(&buf[2],&indexTmp,sizeof(indexTmp));//buff[2],buff[3]
        indexTmp  = modbusSwapEndian(dataPars.request16.count);
        memcpy(&buf[4],&indexTmp,sizeof(indexTmp));//buff[4],buff[5]
        //crc
        crc = modbus_crc16(buf,6);
        memcpy(&buf[6],&crc,sizeof(crc));//buff[6],buff[7]
#if 0
        print_log("modbus 16 response:\n");
        for(int i = 0;i < 8;i++)
        {
            printk("%02x ",buf[i]);
        }
        printk("\n");
#endif
        k_sleep(5);
        hw_rs485_send(buf,8);


    //unpack
    #if 0
    print_log("rcv modbus 16 data:\n");
    for(int i = 0;i < dataPars.request16.count;i++)
    {
        uint16_t data = modbusSwapEndian(dataPars.request16.values[0+i]);
         printk("D%d[%d] \n",
                dataPars.request16.index+i,data);
         //memcpy();
    }
    #endif
    uint16_t modbusBuffTmp[MODBUS_DATA_BUFF] = {0};
    memcpy(&modbusBuffTmp[dataPars.request16.index],&dataPars.request16.values[0],dataPars.request16.length);
    for(int i = 0;i< MODBUS_DATA_BUFF;i++)
    {
        modbusBuffTmp[i] = modbusSwapEndian(modbusBuffTmp[i]);
        //print_log("modbusBuffTmp[%d]=%d\n",i,modbusBuffTmp[i]);
    }
    if(modbusLock())
    {
        memcpy(&modbusBuff[dataPars.request16.index],&modbusBuffTmp[dataPars.request16.index],dataPars.request16.length);
        if(modbusBuff[2] == 3)
        {
            startFlag = true;
            print_log("rcv start cmd\n");
        }
        
        modbusUnlock();
    }
    else
    {
        print_log("get lock failed\n");
        return false;
    }
    #if 0
    for(int i = 0;i< MODBUS_DATA_BUFF;i++)
    {
        //modbusBuffTmp[i] = modbusSwapEndian(modbusBuffTmp[i]);
        print_log("modbusBuff[%d]=%d\n",i,modbusBuff[i]);
    }

    
    #endif


}

bool modbusSetData(uint16_t* data,uint16_t start,uint16_t lenth)

{
    if(start + lenth > MODBUS_DATA_BUFF)
    {
        print_log("buff is out of range\n");
        return false;
    }
    //print_log("222222222\n");
    if(modbusLock())
    {
        memcpy(&modbusBuff[start],data,lenth*2);
        modbusUnlock();

    }
    else
    {
        print_log("get lock failed\n");
        return false;
    }
    //print_log("222222222\n");
    return true;
}

bool ModbusGetData(uint16_t* data,uint16_t addr,uint16_t num)
{
    if(addr < 0 || (addr + num >MODBUS_DATA_BUFF))
    {
        print_log("get modbus err ,addr is out range!\n");
        return false;
    }
    //print_log("222222222\n");
    if(modbusLock())
    {
        memcpy(data,&modbusBuff[addr],num*sizeof(modbusBuff[0]));
        modbusUnlock();
    }
    else
    {
        print_log("get lock failed\n");
        return false;
    }
    
    //print_log("222222222\n");
    return true;

}
void modbufMuxInit()
{
    k_mutex_init(&modbusBufMux);
}
bool modbusLock()
{
    return true;
    if(k_mutex_lock(&modbusBufMux,20) != 0)
    {
        return false;
    }
    else
    {
        return true;
    }
}
bool modbusUnlock()
{
    k_mutex_unlock(&modbusBufMux);
    return true;
}

bool getStartAck()
{
    return startFlag;

}
bool resetStartAck()
{
    startFlag = false;

}



