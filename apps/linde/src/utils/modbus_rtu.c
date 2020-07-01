#include "modbus_rtu.h"
#include "crc16.h"
#include "hw_rs485.h"
#include "my_misc.h"
#include "kernel.h"
#define SLAVE_ADDR 1
#define FUNCTION_03 3
#define MODBUS_SEND_BUFF 40
static uint8_t modbusSendBuff[MODBUS_SEND_BUFF] = {0};
static inline uint16_t modbusSwapEndian( uint16_t data ) { return ( data << 8 ) | ( data >> 8 ); }
static void print_modbus(ModbusParser* ptr)
{
    print_log("response0304 modbus \n,address=%d,function=%d,length=%d,crc=%d\n",
                            ptr->response0304.address,
                            ptr->response0304.function,
                            ptr->response0304.length,
                            ptr->response0304.crc);
    for(int i = 0;i<ptr->response0304.length/2;i++)
    {
        printk("values[%d] = %d ",i,(int) ptr->response0304.values[i]);
    }
    printk("\n");

}
bool modbusMaster0304Req(ModbusParser * m0304Rep,ModbusParser *m0304Req)
{
    ModbusParser rcvData;
    memset(&rcvData,0,sizeof(ModbusParser));
    //1.check validity
    if(m0304Req == NULL)
    {
        return false;
    }
    if(m0304Req->request0304.function != 3)
    {
        return false;
    }
    //2.pack data
    uint8_t buf[MODBUS_MAX_SIZE] = {0};
    buf[0] = m0304Req->request0304.address;
    buf[1] = m0304Req->request0304.function;
    uint16_t index = modbusSwapEndian(m0304Req->request0304.index);
    //uint16_t index = (m0304Req->request0304.index);
    memcpy(&buf[2],&index,sizeof(m0304Req->request0304.index));
    uint16_t count1 = modbusSwapEndian(m0304Req->request0304.count);
    memcpy(&buf[4],&count1,sizeof(m0304Req->request0304.count));
    //buf[4] = m0304Req->request0304.count;
    uint16_t crc =modbus_crc16(&buf,6);
    memcpy(&buf[6],&crc,2);
    #if 0
    print_log("modbus send buf:\n");
    for(int i = 0;i<8;i++)
    {
        printk("%02x ",buf[i]);
    }
    printk("\n");
    #endif
    //3.send data to com
    hw_rs485_send(buf,8);
    k_sleep(200);
    
    //4.rcv data
    //ModbusParser rcv;
    memset(buf,0,MODBUS_MAX_SIZE);
    int predRcvNum = 5 + 2 * m0304Req->request0304.count;
    uint16_t rcvNum = hw_rs485_read(buf,predRcvNum);
    #if 0
    print_log("modbus rcv num is %d:\n",(int)rcvNum);
    //
    print_log("modbus rcv buf:\n");
    for(int i = 0;i<rcvNum;i++)
    {
        printk("%02x ",buf[i]);
    }
    printk("\n");
    #endif
    //5. check data
    if(rcvNum != predRcvNum)
    {
        err_log("data num is err rcvNum = %d,predRcvNum=%d\n",rcvNum,predRcvNum);
        return false;
    }
    if(buf[0] != m0304Req->request0304.address)
    {
        err_log("addr err! buf[0] is %02x,m0304Req addr = %d\n",buf[0],m0304Req->request0304.address);
        return false;
    }
    crc = modbus_crc16(buf,rcvNum -2);
    uint16_t crcRcv = (buf[rcvNum-2] << 8)| buf[rcvNum -1];
    if(crc != modbusSwapEndian(crcRcv))
    {
        err_log("crc err rcv crc =%04x, calcu crc is %04x\n ",crc,crcRcv);
        return false;
    }
    //6.parsa data
    m0304Rep->response0304.length = m0304Req->request0304.count *2;
    for(int i = 0;i <m0304Req->request0304.count;i++ )
    {
        uint16_t data = 0;
        data = (buf[3 +2*i] << 8) |(buf[3 +2*i +1]) ;
        //data = modbusSwapEndian(data);
        m0304Rep->response0304.values[i] = data;
        //err_log("data is %04x \n",data);
    }
    //print_modbus(m0304Rep);
    return true;
}

bool modbusMaster0304ExchangeReq(ModbusParser * m0304Rep,ModbusParser *m0304Req)
{
      ModbusParser rcvData;
      memset(&rcvData,0,sizeof(ModbusParser));
      //1.check validity
      if(m0304Req == NULL)
      {
          return false;
      }
      if(m0304Req->request0304.function != 3)
      {
          return false;
      }
      //2.pack data
      uint8_t buf[MODBUS_MAX_SIZE] = {0};
      uint8_t buf_rcv[MODBUS_MAX_SIZE] = {0};
      buf[0] = m0304Req->request0304.address;
      buf[1] = m0304Req->request0304.function;
      uint16_t index = modbusSwapEndian(m0304Req->request0304.index);
      //uint16_t index = (m0304Req->request0304.index);
      memcpy(&buf[2],&index,sizeof(m0304Req->request0304.index));
      uint16_t count1 = modbusSwapEndian(m0304Req->request0304.count);
      memcpy(&buf[4],&count1,sizeof(m0304Req->request0304.count));
      //buf[4] = m0304Req->request0304.count;
      uint16_t crc =modbus_crc16(&buf,6);
      memcpy(&buf[6],&crc,2);
  #if 0
      print_log("modbus send buf:\n");
      for(int i = 0;i<8;i++)
      {
          printk("%02x ",buf[i]);
      }
      printk("\n");
  #endif

      
      //3.rcv data
      //ModbusParser rcv;
      memset(buf_rcv,0,MODBUS_MAX_SIZE);
      int predRcvNum = 5 + 2 * m0304Req->request0304.count;
      uint16_t rcvNum = hw_rs485_read(buf_rcv,predRcvNum);
      //3.send data to com
      hw_rs485_send(buf,8);
      //print_log("modbus rcv num is %d:\n",(int)rcvNum);
      //
      #if 0
      print_log("modbus rcv buf:\n");
      for(int i = 0;i<rcvNum;i++)
      {
          printk("%02x ",buf_rcv[i]);
      }
      printk("\n");
      #endif
      
      //5. check data
      if(rcvNum != predRcvNum)
      {
          err_log("\t\ndata num is err rcvNum = %d,predRcvNum=%d\n",rcvNum,predRcvNum);
          return false;
      }
      if(buf_rcv[0] != m0304Req->request0304.address)
      {
          err_log("addr err! buf[0] is %02x,m0304Req addr = %d\n",buf_rcv[0],m0304Req->request0304.address);
          return false;
      }
      crc = modbus_crc16(buf_rcv,rcvNum -2);
      uint16_t crcRcv = (buf_rcv[rcvNum-2] << 8)| buf_rcv[rcvNum -1];
      if(crc != modbusSwapEndian(crcRcv))
      {
          err_log("crc err rcv crc =%04x, calcu crc is %04x\n ",crc,crcRcv);
          return false;
      }
      //6.parsa data
      m0304Rep->response0304.length = m0304Req->request0304.count *2;
      for(int i = 0;i <m0304Req->request0304.count;i++ )
      {
          uint16_t data = 0;
          data = (buf_rcv[3 +2*i] << 8) |(buf_rcv[3 +2*i +1]) ;
          //data = modbusSwapEndian(data);
          m0304Rep->response0304.values[i] = data;
          //err_log("data is %04x \n",data);
      }
      //print_modbus(m0304Rep);

      return true;

}

bool modbusSlave0304Req(uint8_t* buff,uint16_t lenth)
{
    ModbusParser truckChainReq;
    ModbusParser truckChainRep;
    
    uint16_t crc = 0;
    //uint16_t crcRcv = 0;
    int predRcvNum = 8;

    //step1.wait requst
    while(hw_rs485_availabe_num() <8)
    {
        k_sleep(50);
        print_log("485 rcv num is %d\n",hw_rs485_availabe_num());
        //continue;
        return false;
    }
    memset(&truckChainReq,0,sizeof(ModbusParser));
    predRcvNum = 8;
    hw_rs485_read(&truckChainReq.frame,predRcvNum);
    print_log("truckChainReq data is :\n");
    for(int i = 0;i < predRcvNum;i++)
    {
        printk("buff[%d] = %02x ",i,truckChainReq.frame[i]);
    }
    printk("\n");
    //1.check slaver addr and function
    if(truckChainReq.request0304.address != SLAVE_ADDR || 
       truckChainReq.request0304.function != FUNCTION_03)
    {
        err_log("addr or function is err! addr = %d,funciton = %d\n",\
                 truckChainReq.request0304.address,truckChainReq.request0304.function);
        memset(&truckChainReq,0,sizeof(ModbusParser));
        return false;
    }
    print_log("------------------------address and function is right\n");
    crc = modbus_crc16(&truckChainReq.frame,predRcvNum -2);
    //crcRcv = (buf[rcvNum-2] << 8)| buf[rcvNum -1];
    uint16_t reqNum = modbusSwapEndian(truckChainReq.request0304.count);
    if(reqNum >17)
    {
        err_log("request data is too large,reqNum  = %d\n",reqNum);
        return false;
    }
    if(crc != truckChainReq.request0304.crc)
    {
        err_log("crc err rcv crc =%04x, calcu crc is %04x\n ",truckChainReq.request0304.crc,crc);
        memset(&truckChainReq,0,sizeof(ModbusParser));
        return false;
    }
    print_log("------------------------crc is rignt\n");
    //step2.pack response master frame
    memset(&truckChainRep,0,sizeof(truckChainRep));
    truckChainRep.response0304.address = SLAVE_ADDR;
    truckChainRep.response0304.function = FUNCTION_03;
    truckChainRep.response0304.length =  2* modbusSwapEndian(truckChainReq.request0304.count);
    uint8_t buff_temp = 0;
    print_log("------------------------buf change endian\n");
    print_log("truckChainRep.response0304.length = %d\n",truckChainRep.response0304.length);
    for(int i = 0; i <truckChainRep.response0304.length -1;i+=2)
    {
        buff_temp = buff[i];
        buff[i] = buff[i+1];
        buff[i+1] = buff_temp;
        print_log("------------i = %d\n",i);
    }
    print_log("------------------------buf change endian finished\n");
    memcpy(&truckChainRep.response0304.values,buff,truckChainRep.response0304.length);
    memcpy(modbusSendBuff,&truckChainRep.response0304,truckChainRep.response0304.length+ 3);
    truckChainRep.response0304.crc  = (modbus_crc16((uint8_t*)&truckChainRep,truckChainRep.response0304.length+3));
    print_log("before crcTmp = %d\n",truckChainRep.response0304.crc);

    print_log("truck response data:\n");
   // uint8_t buff1[20] = {0};
    memcpy(&modbusSendBuff[truckChainRep.response0304.length+ 3],&truckChainRep.response0304.crc,sizeof(truckChainRep.response0304.crc));
#if 1
    for(int i = 0;i <truckChainRep.response0304.length+5;i++)
    {
        printk("frame[%d] = %02x ",i,modbusSendBuff[i]);
    }
    printk("\n");
#endif
    hw_rs485_send(&modbusSendBuff,truckChainRep.response0304.length+5);
    
}

bool modbusMaster16Write(ModbusParser * m16Write)
{
    ModbusParser rcvData;
    memset(&rcvData,0,sizeof(ModbusParser));
    //1.check validity
    if(m16Write == NULL)
    {
        print_log("m16Write is NULL\n");
        return false;
    }
    if(m16Write->request16.function != 0x10)
    {
        print_log("function is err!funciton[%d]\n",m16Write->request16.function);
        return false;
    }
    if(MODBUS_MAX_SIZE < 9 + m16Write->request16.count * 2)
    {
        uint16_t num = 9 + m16Write->request16.count * 2;
        uint16_t max = MODBUS_MAX_SIZE;
        err_log("modbus 16 cmd buff is not enough,num[%d],max[%d]!!!\n",num,max);
        return false;
    }
    //2.pack data
    uint8_t buf[MODBUS_MAX_SIZE] = {0};
    buf[0] = m16Write->request16.address;
    buf[1] = m16Write->request16.function;
    uint16_t index = modbusSwapEndian(m16Write->request16.index);
    //uint16_t index = (m0304Req->request0304.index);
    memcpy(&buf[2],&index,sizeof(m16Write->request16.index));
    uint16_t count1 = modbusSwapEndian(m16Write->request16.count);
    memcpy(&buf[4],&count1,sizeof(m16Write->request16.count));
    //lenth
    uint8_t writeByteNum = m16Write->request16.count * 2;
    //m16Write->request16.length = m16Write->request16.count * 2;
    memcpy(&buf[6],&writeByteNum,sizeof(m16Write->request16.length));
    //data
    //uint16_t data[5] = {0};
    for(int i = 0;i < m16Write->request16.count;i++)
    {
        uint16_t data_ = modbusSwapEndian(m16Write->request16.values[i]);
        print_log("data_ = %d..\n",data_);
        memcpy(&buf[7 + i],&data_,sizeof(uint16_t));
    }
    
    uint16_t crc =modbus_crc16(&buf,7+writeByteNum);
    memcpy(&buf[7+writeByteNum],&crc,2);
#if 0
    print_log("send num = %d,modbus send buf:\n",7+writeByteNum + 2);
    for(int i = 0;i<7+writeByteNum + 2;i++)
    {
        printk("%02x ",buf[i]);
    }
    printk("\n");
#endif
    //3.send data to com
//    print_log();
    hw_rs485_send(buf,7+writeByteNum + 2);
    k_sleep(200);
    
    //4.rcv data
    //ModbusParser rcv;
    memset(buf,0,MODBUS_MAX_SIZE);
    int predRcvNum = 8;
    uint16_t rcvNum = hw_rs485_read(buf,predRcvNum);
#if 0
    print_log("modbus rcv num is %d:\n",(int)rcvNum);
    
    print_log("modbus rcv buf:\n");
    for(int i = 0;i<rcvNum;i++)
    {
        printk("%02x ",buf[i]);
    }
    printk("\n");
#endif
    //5. check data
    if(rcvNum != predRcvNum)
    {
        err_log("data num is err rcvNum = %d,predRcvNum=%d\n",rcvNum,predRcvNum);
        return false;
    }
    if(buf[0] != m16Write->request16.address)
    {
        err_log("addr err! buf[0] is %02x,m16Req addr = %d\n",buf[0],m16Write->request16.address);
        return false;
    }
    crc = modbus_crc16(buf,rcvNum -2);
    uint16_t crcRcv = (buf[rcvNum-2] << 8)| buf[rcvNum -1];
    if(crc != modbusSwapEndian(crcRcv))
    {
        err_log("crc err rcv crc =%04x, calcu crc is %04x\n ",crc,crcRcv);
        return false;
    }
    //6.check reg addr
    uint16_t regAddr = (buf[rcvNum-6] << 8)| buf[rcvNum -5];
    if(regAddr != m16Write->request16.index)
    {
        err_log("reg Add is err!rcv_regAddr[%d],request16.index[%d]\n",regAddr,m16Write->request16.index);
        return false;
    }
    //7.check data lenth
    uint16_t rcvRegLenth = (buf[rcvNum-4] << 8)| buf[rcvNum -3];
    if(rcvRegLenth != m16Write->request16.count)
    {
        err_log("reg lenth is err!rcvRegLenth[%d],request16.count[%d]\n",rcvRegLenth,m16Write->request16.count);
        return false;
    }

    //print_log("!!!!!!!!!!!!!!!!!modbus 10 rcv OK@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
    return true;

}


