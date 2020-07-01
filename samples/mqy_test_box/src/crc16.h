#ifndef __CRC_H__
#define __CRC_H__

#include "kernel.h"

unsigned short modbus_crc16(unsigned char *buf, unsigned int bsize);

#endif /*__CRC_H__ */
