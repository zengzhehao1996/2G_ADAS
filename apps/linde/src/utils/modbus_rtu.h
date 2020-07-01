#ifndef __MODBUS_RTU_H__
#define __MODBUS_RTU_H__
#include "kernel.h"
// MODBUS_MAX_SIZE must >=10
#define MODBUS_MAX_SIZE 17
typedef union modbusParser
{
	uint8_t frame[MODBUS_MAX_SIZE*2 + 5];

	struct __attribute__( ( __packed__ ) )
	{
		uint8_t address;
		uint8_t function;
	} base; //Base shared bytes, common for all frames, which always have the same meaning

	struct __attribute__( ( __packed__ ) )
	{
		uint8_t address;
		uint8_t function;
		uint8_t code;
		uint16_t crc;
	} exception;

	struct __attribute__( ( __packed__ ) )
	{
		uint8_t address;
		uint8_t function;
		uint16_t index;
		uint16_t count;
		uint16_t crc;
	} request0102; //Read multiple coils or discrete inputs

	struct __attribute__( ( __packed__ ) )
	{
		uint8_t address;
		uint8_t function;
		uint8_t length;
		uint8_t values[MODBUS_MAX_SIZE*2];
		uint16_t crc;
	} response0102; //Read multiple coils or discrete inputs - response

	struct __attribute__( ( __packed__ ) )
	{
		uint8_t address;
		uint8_t function;
		uint16_t index;
		uint16_t count;
		uint16_t crc;
	} request0304; //Read multiple holding registers or input registers

	struct __attribute__( ( __packed__ ) )
	{
		uint8_t address;
		uint8_t function;
		uint8_t length;
		uint16_t values[MODBUS_MAX_SIZE];
		uint16_t crc;
	} response0304; //Read multiple holding registers or input registers - response

	struct __attribute__( ( __packed__ ) )
	{
		uint8_t address;
		uint8_t function;
		uint16_t index;
		uint16_t value;
		uint16_t crc;
	} request05; //Write single coil

	struct __attribute__( ( __packed__ ) )
	{
		uint8_t address;
		uint8_t function;
		uint16_t index;
		uint16_t value;
		uint16_t crc;
	} response05; //Write single coil - response

	struct __attribute__( ( __packed__ ) )
	{
		uint8_t address;
		uint8_t function;
		uint16_t index;
		uint16_t value;
		uint16_t crc;
	} request06; //Write single holding register

	struct __attribute__( ( __packed__ ) )
	{
		uint8_t address;
		uint8_t function;
		uint16_t index;
		uint16_t value;
		uint16_t crc;
	} response06; //Write single holding register

	struct __attribute__( ( __packed__ ) )
	{
		uint8_t address;
		uint8_t function;
		uint16_t index;
		uint16_t count;
		uint8_t length;
		uint8_t values[MODBUS_MAX_SIZE -4];
		uint16_t crc;
	} request15; //Write multiple coils

	struct __attribute__( ( __packed__ ) )
	{
		uint8_t address;
		uint8_t function;
		uint16_t index;
		uint16_t count;
		uint16_t crc;
	} response15; //Write multiple coils - response

	struct __attribute__( ( __packed__ ) )
	{
		uint8_t address;
		uint8_t function;
		uint16_t index;
		uint16_t count;
		uint8_t length;
		uint16_t values[(MODBUS_MAX_SIZE -4) / 2];
		uint16_t crc;
	} request16; //Write multiple holding registers

	struct __attribute__( ( __packed__ ) )
	{
		uint8_t address;
		uint8_t function;
		uint16_t index;
		uint16_t count;
		uint16_t crc;
	} response16; //Write multiple holding registers

	struct __attribute__( ( __packed__ ) )
	{
		uint8_t address;
		uint8_t function;
		uint16_t index;
		uint16_t andmask;
		uint16_t ormask;
		uint16_t crc;
	} request22; //Mask write single holding register

	struct __attribute__( ( __packed__ ) )
	{
		uint8_t address;
		uint8_t function;
		uint16_t index;
		uint16_t andmask;
		uint16_t ormask;
		uint16_t crc;
	} response22; //Mask write single holding register
}ModbusParser;



bool modbusMaster0304Req(ModbusParser * m0304Rep,ModbusParser *m0304Req);
bool modbusMaster0304ExchangeReq(ModbusParser * m0304Rep,ModbusParser *m0304Req);

bool modbusSlave0304Req(uint8_t* buff,uint16_t lenth);

bool modbusMaster16Write(ModbusParser * m16Write);//true:write ok;false:write failed



#endif




